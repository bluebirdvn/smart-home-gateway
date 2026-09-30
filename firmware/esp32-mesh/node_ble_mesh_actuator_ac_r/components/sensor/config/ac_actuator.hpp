#ifndef _AC_ACTUATOR_HPP
#define _AC_ACTUATOR_HPP

#include "actuator.hpp"
#include <cstdint>
#include <stdint.h>
#include "driver/rmt_tx.h"
#include <string>
#include <unordered_map>
#include <vector>
class ACActuator : public IActuator
{
private:
    std::string path_to_config_file;
    uint8_t power = 0;
    uint8_t mode = 0;
    uint8_t temp = 0;
    uint8_t fan = 0;

    uint8_t max_temp = 0;
    uint8_t min_temp = 0;
    
    uint8_t gpio_num;
    rmt_channel_handle_t tx_channel = nullptr;
    rmt_encoder_handle_t encoder = nullptr;
    rmt_tx_channel_config_t tx_config;

    std::string generate_key() const;
    std::unordered_map<std::string, std::string> ir_cmds;
    void send_ir_code(const std::string& ir_base64) const;
    bool load_ir_mapping(const std::string& json_str);

public:
    explicit ACActuator(const std::string& path,const uint8_t gpio_num, uint8_t actuator_id, uint8_t device_type);
    bool init() override;
    void deinit() override;
    bool set_state(float setpoint, uint8_t status, uint64_t time) override;
    uint8_t get_state(uint8_t state_id) const override;
    float get_setpoint(void) const override;
};


#endif