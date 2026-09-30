#ifndef _RELAY_ACTUATOR_HPP
#define _RELAY_ACTUATOR_HPP
#include "actuator.hpp"
#include "driver/gpio.h"
#include <stdint.h>
class RelayActuator : public IActuator {
    private:
    uint8_t gpio_num;
    uint8_t status;
    uint8_t setpoint;
    public:

    explicit RelayActuator(gpio_num_t gpio,  uint8_t actuator_id, uint8_t device_type) : IActuator(actuator_id, device_type), gpio_num(gpio), status(0), setpoint(0) {}
    bool init() override;

    void deinit() override;

    bool set_state(float setpoint, uint8_t status, uint64_t time) override;

    uint8_t get_state(uint8_t state_id) const override;

    float get_setpoint() const override;
};


#endif