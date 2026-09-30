#include "device_management.hpp"
#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_rpr_model_api.h"

#include <cstddef>
#include "esp_log.h"
const char* TAG = "DEVICE MNG";
static bool is_unbindable_sig_model(uint16_t model_id) {
    return model_id == 0x0000 || model_id == 0x0001 || model_id == 0x0003 || model_id == 0x0004 || model_id == 0x0005;
}

void DeviceManager::set_prov_key(uint16_t net_idx, uint16_t app_idx, const uint8_t* app_key) {
    std::lock_guard<std::mutex> lock(dev_mutex);
    prov_key.net_idx = net_idx;
    prov_key.app_idx = app_idx;
    if (app_key != nullptr) {
        memcpy(prov_key.app_key, app_key, 16);
    }
}

ProvKeyInfo DeviceManager::get_prov_key() {
    std::lock_guard<std::mutex> lock(dev_mutex);
    return prov_key;
}

DeviceInfo* DeviceManager::alloc_node(const uint8_t uuid[16], uint16_t unicast, uint8_t elem_num) {
    std::lock_guard<std::mutex> lock(dev_mutex);
    for (auto& pair : devices) {
        if (memcmp(pair.second.uuid, uuid, 16) == 0) {
            pair.second.unicast = unicast;
            pair.second.elem_num = elem_num;
            pair.second.state = ConfigState::IDLE;
            pair.second.elements.clear();
            pair.second.current_bind_element_idx = 0;
            pair.second.current_bind_model_idx = 0;
            pair.second.binding_vendor = false;
            return &pair.second;
        }
    }
    DeviceInfo new_dev;
    memcpy(new_dev.uuid, uuid, 16);
    new_dev.unicast = unicast;
    new_dev.elem_num = elem_num;
    new_dev.state = ConfigState::IDLE;
    devices[unicast] = new_dev;
    return &devices[unicast];
}

DeviceInfo* DeviceManager::find_node(uint16_t addr) {
    std::lock_guard<std::mutex> lock(dev_mutex);
    for (auto& pair : devices) {
        uint16_t primary_addr = pair.second.unicast;
        uint8_t elem_num = pair.second.elem_num;
        if (primary_addr != 0 && addr >= primary_addr && addr < (primary_addr + elem_num)) {
            return &pair.second;
        }
    }
    return nullptr;
}

int DeviceManager::delete_device(uint16_t unicast) {
    std::lock_guard<std::mutex> lock(dev_mutex);
    return static_cast<int>(devices.erase(unicast));
}

int DeviceManager::parse_composition_data(uint16_t unicast, const uint8_t *data, uint16_t length) {
    std::lock_guard<std::mutex> lock(dev_mutex);
    ESP_LOGI(TAG, "COMPOSITION DATA of NODE 0x%04x (len: %d bytes) ---", unicast, length);
    ESP_LOG_BUFFER_HEX(TAG, data, length);
    auto it = devices.find(unicast);
    if (it == devices.end()) {
        return -1;
    }
    DeviceInfo& node = it->second;
    node.elements.clear();
    node.current_bind_element_idx = 0;
    node.current_bind_model_idx = 0;
    node.binding_vendor = false;

    if (length < 10) {
        return -1;
    }
    node.features = data[8] | (data[9] << 8);
    node.rpr_server = (node.features & (1 << 4)) != 0;
    uint16_t offset = 10;
    uint16_t current_element_addr = unicast;

    while (offset < length) {
        if (offset + 4 > length) {
            break;
        }
        ElementInfo elem;
        elem.element_addr = current_element_addr;
        uint8_t num_s = data[offset + 2];
        uint8_t num_v = data[offset + 3];
        offset += 4;
        for (int i = 0; i < num_s; i++) {
            if (offset + 2 > length) {
                break;
            }
            uint16_t model_id = data[offset] | (data[offset + 1] << 8);
            ESP_LOGW(TAG, "parse SIG Model %d: 0x%04x", i, model_id);
            
            if (model_id == 0x0004) { 
                ESP_LOGW(TAG, "found (0x0004), rpr_server = true");
                node.rpr_server = true; 
            }
            elem.sig_models.push_back(model_id);
            offset += 2;
        }
        for (int i = 0; i < num_v; i++) {
            if (offset + 4 > length) {
                break;
            }
            uint32_t cid = data[offset] | (data[offset + 1] << 8);
            uint32_t mod = data[offset + 2] | (data[offset + 3] << 8);
            elem.vendor_models.push_back((cid << 16) | mod);
            offset += 4;
        }
        node.elements.push_back(elem);
        current_element_addr++;
    }
    return 0;
}

void DeviceManager::set_state(uint16_t unicast, ConfigState st) {
    std::lock_guard<std::mutex> lock(dev_mutex);
    auto it = devices.find(unicast);
    if (it != devices.end()) {
        it->second.state = st;
    }
}

ConfigState DeviceManager::get_state(uint16_t unicast) {
    std::lock_guard<std::mutex> lock(dev_mutex);
    auto it = devices.find(unicast);
    if (it != devices.end()) {
        return it->second.state;
    }
    return ConfigState::IDLE;
}

bool DeviceManager::exists(uint16_t unicast) {
    std::lock_guard<std::mutex> lock(dev_mutex);
    return devices.find(unicast) != devices.end();
}

bool DeviceManager::get_next_bind_target(uint16_t unicast, uint16_t &element_addr, uint16_t &model_id, uint16_t &company_id, bool &is_sig)
{
    DeviceInfo *node = find_node(unicast);
    if (!node) return false;

    for (size_t e_idx = node->current_bind_element_idx; e_idx < node->elements.size(); ++e_idx) {
        auto &elem = node->elements[e_idx];

        if (!node->binding_vendor) {
            while (node->current_bind_model_idx < elem.sig_models.size()) {
                model_id = elem.sig_models[node->current_bind_model_idx++];
                company_id = ESP_BLE_MESH_CID_NVAL;
                is_sig = true;
                element_addr = elem.element_addr;
                node->current_bind_element_idx = e_idx;
                return true; 
            }
            node->current_bind_model_idx = 0;
            node->binding_vendor = true;
        }

        if (node->binding_vendor) {
            while (node->current_bind_model_idx < elem.vendor_models.size()) {
                uint32_t packed = elem.vendor_models[node->current_bind_model_idx++];
                company_id = static_cast<uint16_t>((packed >> 16) & 0xFFFF);
                model_id   = static_cast<uint16_t>(packed & 0xFFFF);
                is_sig = false;
                element_addr = elem.element_addr;
                node->current_bind_element_idx = e_idx;
                return true; 
            }
            node->current_bind_model_idx = 0;
            node->binding_vendor = false;
        }
    }
    return false; 
}

std::vector<uint16_t> DeviceManager::get_rpr_capable_ready_nodes() {
    std::lock_guard<std::mutex> lock(dev_mutex);
    std::vector<uint16_t> result;
    for (auto& pair : devices) {
        if (pair.second.rpr_server && pair.second.state == ConfigState::READY) {
            result.push_back(pair.second.unicast);
        }
    }
    return result;
}