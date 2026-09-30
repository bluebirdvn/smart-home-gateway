#pragma once

#include "device.hpp"

class ISensor : public Device
{
public:
    virtual float readSensor() const = 0;
};