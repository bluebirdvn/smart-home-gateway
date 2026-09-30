#include "bh1750.hpp"
#include "esp_log.h"

static const char *TAG = "BH1750_CPP";

BH1750Sensor::BH1750Sensor(i2c_master_bus_handle_t i2c_bus, uint8_t addr)
    : i2c_bus(i2c_bus), addr(addr)
{
    dev.dev_handle = nullptr;
    dev.mtreg_val = DEFAULT_MEAS_TIME_REG_VAL;
    dev.meas_time = H_RES_MODE_MEASUREMENT_TIME_MS;
    dev.meas_time_mul = 1;
}

BH1750Sensor::~BH1750Sensor()
{
    deinit();
}

bool BH1750Sensor::init()
{
    if (dev.dev_handle == nullptr)
    {
        if (bh1750_i2c_hal_init(i2c_bus, addr, 400000, &dev.dev_handle) != BH1750_OK)
        {
            ESP_LOGE(TAG, "Failed to add BH1750 on I2C bus");
            return false;
        }
    }

    if (bh1750_i2c_set_power_mode(dev, BH1750_POWER_ON) != BH1750_OK)
    {
        ESP_LOGE(TAG, "Power ON failed");
        return false;
    }

    if (bh1750_i2c_set_resolution_mode(&dev, BH1750_CONT_H_RES_MODE) != BH1750_OK)
    {
        ESP_LOGE(TAG, "Set resolution mode failed");
        return false;
    }

    return true;
}

void BH1750Sensor::deinit()
{
    if (dev.dev_handle != nullptr)
    {
        bh1750_i2c_set_power_mode(dev, BH1750_POWER_DOWN);
        i2c_master_bus_rm_device(dev.dev_handle);
        dev.dev_handle = nullptr;
    }
}

float BH1750Sensor::readSensor() const
{
    uint16_t lux_val = 0;

    if (bh1750_i2c_read_data(dev, &lux_val) != BH1750_OK)
    {
        ESP_LOGE(TAG, "Read lux failed");
        return -1.0f;
    }

    return static_cast<float>(lux_val);
}