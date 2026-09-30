#pragma once

#include "sensor.hpp"
#include "bh1750_i2c.h"

/**
 * @brief BH1750 ambient light sensor, exposed through the ISensor interface.
 */
class BH1750Sensor : public ISensor
{
public:
    explicit BH1750Sensor(i2c_master_bus_handle_t i2c_bus, uint8_t addr = I2C_ADDRESS_BH1750);
    ~BH1750Sensor() override;

    bool init() override;
    float readSensor() const override;
    void deinit() override;

private:
    i2c_master_bus_handle_t i2c_bus;
    uint8_t addr;
    bh1750_dev_t dev;
};