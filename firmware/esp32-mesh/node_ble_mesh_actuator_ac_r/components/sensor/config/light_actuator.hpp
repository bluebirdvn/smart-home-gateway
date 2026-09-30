#ifndef _LIGHT_ACTUATOR_HPP
#define _LIGHT_ACTUATOR_HPP
#include "actuator.hpp"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include <stdint.h>
class LightActuator : public IActuator {
    private:
    uint8_t gpio_num;
    uint8_t status;
    uint8_t setpoint;

    ledc_timer_config_t timer_config;
    ledc_channel_config_t channel_config;
     
    public:

    explicit LightActuator(gpio_num_t gpio, ledc_timer_config_t timer_config, ledc_channel_config_t channel_config,  uint8_t actuator_id, uint8_t device_type) : IActuator(actuator_id, device_type), gpio_num(gpio), status(0), setpoint(0), timer_config(timer_config), channel_config(channel_config) {}
    bool init() override;

    void deinit() override;

    bool set_state(float setpoint, uint8_t status, uint64_t time) override;

    uint8_t get_state(uint8_t state_id) const override;

    float get_setpoint() const override;
};


#endif