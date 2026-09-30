#include "ac_actuator.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include "cJSON.h"
#include "mbedtls/base64.h"
#include "esp_log.h"
#include <vector>

static const char* TAG = "AC_ACTUATOR";

extern "C" {
    extern const char _binary_1064_json_start[];
    extern const char _binary_1064_json_end[];
}


ACActuator::ACActuator(const std::string& path, const uint8_t gpio_num, uint8_t actuator_id, uint8_t device_type) :
    IActuator(actuator_id, device_type), 
    path_to_config_file(path), 
    power(0), 
    mode(0), 
    temp(24), 
    fan(0), 
    max_temp(30), 
    min_temp(16), 
    gpio_num(gpio_num),
    tx_channel(nullptr),
    encoder(nullptr)
{}

bool ACActuator::load_ir_mapping(const std::string& json_str)
{
    cJSON* root = cJSON_Parse(json_str.c_str());
    if (root == nullptr) {
        ESP_LOGE(TAG, "Failed to parse JSON string");
        return false;
    }

    cJSON* min_temp_str = cJSON_GetObjectItem(root, "minTemperature");
    if (min_temp_str != nullptr) min_temp = min_temp_str->valueint;

    cJSON* max_temp_str = cJSON_GetObjectItem(root, "maxTemperature");
    if (max_temp_str != nullptr) max_temp = max_temp_str->valueint;

    cJSON* cmds = cJSON_GetObjectItem(root, "commands");
    if (cmds == nullptr) {
        cJSON_Delete(root);
        return false;
    }

    cJSON* off_cmd = cJSON_GetObjectItem(cmds, "off");
    if (off_cmd != nullptr) {
        ir_cmds["off"] = off_cmd->valuestring;
    }

    const char* modes[] = {"cool", "dry"};
    const char* fans[] = {"low", "medium", "high"};

    for (const char* m : modes) {
        cJSON* mode_obj = cJSON_GetObjectItem(cmds, m);
        if (mode_obj == nullptr) continue;

        for (const char* level : fans) {
            cJSON* fan_obj = cJSON_GetObjectItem(mode_obj, level);
            if (fan_obj == nullptr) continue;

            for (uint8_t t = min_temp; t <= max_temp; t++) {
                std::string temp_str = std::to_string(t);
                cJSON* temp_obj = cJSON_GetObjectItem(fan_obj, temp_str.c_str());
                if (temp_obj != nullptr) {
                    std::string cmd_key = std::string(m) + "_" + std::string(level) + "_" + temp_str;
                    ir_cmds[cmd_key] = temp_obj->valuestring;
                }
            }
        }
    }

    cJSON_Delete(root);
    return true;
}

std::string ACActuator::generate_key() const {
    std::string mode_str = (mode == 0) ? "cool" : "dry";
    std::string fan_str  = (fan == 0) ? "low" : (fan == 1 ? "medium" : "high");
    return mode_str + "_" + fan_str + "_" + std::to_string(static_cast<int>(temp));
}

void ACActuator::send_ir_code(const std::string& ir_base64) const 
{
    ESP_LOGI(TAG, "TX IR (Base64 length: %zu)", ir_base64.length());
    size_t len = ir_base64.length();
    size_t out_len;
    
    std::vector<uint8_t> data(len);

    int ret = mbedtls_base64_decode(data.data(), data.size(), &out_len, (const unsigned char*)ir_base64.c_str(), len);
    if (ret != 0 || out_len < 4) {
        ESP_LOGE(TAG, "Base64 decode failed (ret=%d, len=%zu)", ret, out_len);
        return;
    }

    if (data[0] != 0x26) {
        ESP_LOGE(TAG, "Invalid Broadlink IR format (Header = 0x%02X)", data[0]);
        return;
    }

    std::vector<rmt_symbol_word_t> rmt_symbols;
    bool is_mark = true; 
    uint32_t mark_us = 0;
    const float TICK_US = 26.9f; 

    for (size_t i = 4; i < out_len; ) {
        uint32_t duration_ticks = data[i++];
        
        if (duration_ticks == 0) {
            if (i + 1 >= out_len) break;
            duration_ticks = (data[i + 1] << 8) | data[i]; 
            i += 2;
        }

        uint32_t duration_us = static_cast<uint32_t>(duration_ticks * TICK_US);
        if (duration_us == 0) break;

        if (is_mark) {
            mark_us = duration_us;
            is_mark = false;
        } else {
            uint32_t space_us = duration_us;
            rmt_symbol_word_t sym;
            
            if (space_us > 32767) space_us = 32767;
            if (mark_us > 32767) mark_us = 32767;

            sym.duration0 = mark_us;   
            sym.level0 = 1; 
            sym.duration1 = space_us;  
            sym.level1 = 0; 
            rmt_symbols.push_back(sym);

            is_mark = true; 
            mark_us = 0;
        }
    }

    if (!is_mark) {
        rmt_symbol_word_t sym;
        sym.duration0 = mark_us; 
        sym.level0 = 1;
        sym.duration1 = 0;       
        sym.level1 = 0;
        rmt_symbols.push_back(sym);
    }

    if (rmt_symbols.empty() || !tx_channel || !encoder) {
        ESP_LOGE(TAG, "Empty IR data or RMT not initialized");
        return;
    }

    rmt_transmit_config_t tx_cfg = {};
    tx_cfg.loop_count = 0; 

    ESP_ERROR_CHECK(rmt_transmit(tx_channel, encoder, rmt_symbols.data(), rmt_symbols.size() * sizeof(rmt_symbol_word_t), &tx_cfg));
    rmt_tx_wait_all_done(tx_channel, -1);
    
    ESP_LOGI(TAG, "IR signal transmitted successfully! Symbols: %zu", rmt_symbols.size());
}

bool ACActuator::init()
{
    std::string json_str(_binary_1064_json_start, _binary_1064_json_end - _binary_1064_json_start);

    if (!load_ir_mapping(json_str)) {
        ESP_LOGE(TAG, "Failed to parse embedded JSON mapping");
        return false;
    }
    ESP_LOGI(TAG, "AC IR commands loaded successfully from embedded storage");

    rmt_tx_channel_config_t tx_config = {};
    tx_config.clk_src = RMT_CLK_SRC_DEFAULT;
    tx_config.gpio_num = static_cast<gpio_num_t>(gpio_num);
    tx_config.mem_block_symbols = 64;
    tx_config.resolution_hz = 1000000; 
    tx_config.trans_queue_depth = 4;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_config, &tx_channel));

    rmt_carrier_config_t carrier_cfg = {};
    carrier_cfg.frequency_hz = 38000;
    carrier_cfg.duty_cycle = 0.33;    
    carrier_cfg.flags.polarity_active_low = false;
    carrier_cfg.flags.always_on = false;
    ESP_ERROR_CHECK(rmt_apply_carrier(tx_channel, &carrier_cfg));

    rmt_copy_encoder_config_t encoder_config = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&encoder_config, &encoder));

    ESP_ERROR_CHECK(rmt_enable(tx_channel));
    return true;
}

void ACActuator::deinit() 
{
    set_state(temp, 0, 0);
    if (tx_channel) {
        rmt_disable(tx_channel);
        rmt_del_channel(tx_channel);
        tx_channel = nullptr;
    }
    if (encoder) {
        rmt_del_encoder(encoder);
        encoder = nullptr;
    }
}

bool ACActuator::set_state(float setpoint, uint8_t status, uint64_t time)
{
    (void)time;
    
    power = status & 0x01;                
    mode  = (status >> 1) & 0x03;         
    fan   = (status >> 3) & 0x03;

    int target_temp  = static_cast<int>(setpoint);
    if (target_temp < min_temp) target_temp = min_temp;
    if (target_temp > max_temp) target_temp = max_temp;
    temp = static_cast<uint8_t>(target_temp);

    if (power == 0) {
        if (ir_cmds.count("off")) {
            send_ir_code(ir_cmds.at("off"));
            return true;
        }
    } else {
        std::string key = generate_key();
        if (ir_cmds.count(key)) {
            send_ir_code(ir_cmds.at(key));
            return true;
        } else {
            ESP_LOGW(TAG, "No IR cmd matched for key: %s", key.c_str());
        }
    }
    return false;
}

uint8_t ACActuator::get_state(uint8_t state_id) const 
{
    (void)state_id;
    uint8_t packed_status = 0;
    packed_status |= (power & 0x01);
    packed_status |= ((mode & 0x03) << 1);
    packed_status |= ((fan & 0x03) << 3);
    return packed_status;
}

float ACActuator::get_setpoint(void) const 
{
    return static_cast<float>(temp);
}