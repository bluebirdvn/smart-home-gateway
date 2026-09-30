#include "light_actuator.hpp"

#include "actuator.hpp"
#include "driver/gpio.h"
#include <stdint.h>
#include "driver/ledc.h"
#include "esp_log.h"
#define LEDC_DUTY_RES LEDC_TIMER_8_BIT 
#define MAX_DUTY 255

bool LightActuator::init()
{
    ledc_timer_config(&timer_config);
    ledc_channel_config(&channel_config);
    this->status = 0;
    this->setpoint = 0.0f;
    return true;
}


void LightActuator::deinit()
{
    ledc_stop(timer_config.speed_mode, channel_config.channel, 0);
}

bool LightActuator::set_state(float setpoint, uint8_t status, uint64_t time)
{
    this->status = (status > 0) ? 1 : 0;
    
    if (setpoint < 0.0f) setpoint = 0.0f;
    if (setpoint > 100.0f) setpoint = 100.0f;
    this->setpoint = setpoint;

    uint32_t duty = 0;
    if (this->status == 1) {
        duty = (uint32_t)((this->setpoint / 100.0f) * MAX_DUTY);
    } 
    ledc_set_duty(timer_config.speed_mode, channel_config.channel, duty);
    ledc_update_duty(timer_config.speed_mode, channel_config.channel);

    return true;

}

uint8_t LightActuator::get_state(uint8_t state_id) const
{
    return status;
}

float LightActuator::get_setpoint() const 
{   
    return setpoint;
}
