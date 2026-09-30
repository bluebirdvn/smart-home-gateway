#pragma once

#include <memory>
#include "sensor.hpp"
#include "aht30_sensor.h"


class AHT30Sensor
{
public:
    AHT30Sensor(i2c_master_bus_handle_t i2c_bus, uint8_t addr = 0x38);
    ~AHT30Sensor();

    bool init();
    bool readTemperature(float &temperature);
    bool readHumidity(float &humidity);

private:
    bool read(float &temperature, float &humidity);

    aht_init_config_t config;
    aht_handle_t *handle{nullptr};
};


class AHT30TemperatureSensor : public ISensor
{
public:
    explicit AHT30TemperatureSensor(std::shared_ptr<AHT30Sensor> sensor);

    bool init() override;
    void deinit() override;
    float readSensor() const override;

private:
    std::shared_ptr<AHT30Sensor> sensor;
};



class AHT30HumiditySensor : public ISensor
{
public:
    explicit AHT30HumiditySensor(std::shared_ptr<AHT30Sensor> sensor);

    bool init() override;
    void deinit() override;
    float readSensor() const override;

private:
    std::shared_ptr<AHT30Sensor> sensor;
};