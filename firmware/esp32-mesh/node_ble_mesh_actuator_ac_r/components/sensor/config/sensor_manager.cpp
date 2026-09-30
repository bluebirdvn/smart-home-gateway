#include "sensor_manager.hpp"
#include "esp_log.h"
#include "pir_sensor.hpp"
#include <cstdint>

static const char *TAG = "SENSOR_MANAGER";

SensorManager::SensorManager(i2c_master_bus_handle_t i2c_bus, uint8_t sensor_id, uint8_t gpio_num_pir)
    : m_sensor_id(sensor_id),
      aht30(std::make_shared<AHT30Sensor>(i2c_bus, 0x38)),
      temp_sensor(aht30),
      humi_sensor(aht30),
      lux_sensor(i2c_bus),
      pir_sensor(gpio_num_pir),
      sensors{&temp_sensor, &humi_sensor, &lux_sensor, &pir_sensor}
{

}

bool SensorManager::init()
{
    for (ISensor *sensor : sensors)
    {
        if (!sensor->init())
        {
            ESP_LOGE(TAG, "A sensor failed to initialize");
            return false;
        }
    }

    return true;
}


bool SensorManager::readAll(sensor_data_t &out)
{
    float temperature = 0.0f;
    float humidity = 0.0f;

    bool aht_ok = aht30->readTemperature(temperature) && aht30->readHumidity(humidity);
    if (!aht_ok)
    {
        ESP_LOGE(TAG, "Failed to read AHT30");
        return false;
    }

    float lux = lux_sensor.readSensor();
    if (lux < 0.0f)
    {
        ESP_LOGE(TAG, "Failed to read BH1750");
        return false;
    }

    out.soil_moisture = 0; 
    out.temperature = (int8_t)temperature;
    out.humidity = (uint8_t)humidity;
    out.lux = (lux > 0xFFFF) ? 0xFFFF : (uint16_t)lux;
    out.motion = (uint8_t)pir_sensor.readSensor();
    out.battery = 100; 
    
    return true;
}