#pragma once

#include <cstdint>
#include <stdint.h>
#include "device.hpp"

class IActuator : public Device
{
public:
    IActuator(uint8_t actuator_id, uint8_t device_type) : actuator_id(actuator_id), device_type(device_type) {}
    virtual bool set_state(float setpoint, uint8_t status, uint64_t time) = 0;
    virtual uint8_t get_state(uint8_t state_id) const = 0;
    virtual float get_setpoint(void) const = 0;
    uint8_t get_id() const { 
        return actuator_id; 
    }
    uint8_t get_type() const { 
        return device_type; 
    }
private:
    uint8_t actuator_id;
    uint8_t device_type;
};