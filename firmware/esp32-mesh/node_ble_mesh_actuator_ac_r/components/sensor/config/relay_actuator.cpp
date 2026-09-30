#include "relay_actuator.hpp"

#include "actuator.hpp"
#include "driver/gpio.h"
#include <stdint.h>
#include "esp_log.h"

bool RelayActuator::init() {
    gpio_reset_pin(static_cast<gpio_num_t>(gpio_num));
    gpio_set_direction(static_cast<gpio_num_t>(gpio_num), GPIO_MODE_OUTPUT);
    gpio_set_level(static_cast<gpio_num_t>(gpio_num), 0);
    return true;
}


void RelayActuator::deinit() {
    gpio_set_level(static_cast<gpio_num_t>(gpio_num), 0);
}

bool RelayActuator::set_state(float setpoint, uint8_t status, uint64_t time)
{
    (void)time;
    this->setpoint = static_cast<uint8_t>(setpoint);
    status = (status > 0) ? 1 : 0;
    gpio_set_level(static_cast<gpio_num_t>(gpio_num), 1);
    return true;

}

uint8_t RelayActuator::get_state(uint8_t state_id) const
{
    return status;
}

float RelayActuator::get_setpoint() const 
{   
    return setpoint;
}
