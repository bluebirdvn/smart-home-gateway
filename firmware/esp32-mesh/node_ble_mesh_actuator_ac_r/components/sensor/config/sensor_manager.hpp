#pragma once

#include <memory>
#include "aht30_sensor.hpp"
#include "bh1750.hpp"
#include "pir_sensor.hpp"
#include "vendor_model.h"

/**
 * @brief Reads every physical sensor on the node and builds the
 *        sensor_data_t payload that gets sent over BLE Mesh.
 */
class SensorManager
{
public:
    SensorManager(i2c_master_bus_handle_t i2c_bus, uint8_t sensor_id, uint8_t gpio_num_pir);
    /**
     * @brief Initialize every sensor. Uses the common ISensor interface so
     *        AHT30 (temperature + humidity) and BH1750 (lux) are all
     *        initialized the exact same way.
     * @return true if all sensors initialized successfully.
     */
    bool init();

    /**
     * @brief Read every sensor and fill the payload to publish.
     * @param out Filled with the latest readings.
     * @return true if every value was read successfully.
     */
    bool readAll(sensor_data_t &out);

private:

    uint8_t m_sensor_id;

    std::shared_ptr<AHT30Sensor> aht30;
    AHT30TemperatureSensor temp_sensor;
    AHT30HumiditySensor humi_sensor;
    BH1750Sensor lux_sensor;
    PIRSensor pir_sensor;
    ISensor *sensors[4];
};