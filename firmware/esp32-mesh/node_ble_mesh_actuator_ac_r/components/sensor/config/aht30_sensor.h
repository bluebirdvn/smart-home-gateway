#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Opaque handle to an initialized AHT sensor instance.
 */
typedef struct aht_handle_t aht_handle_t;

/**
 * @brief Configuration used to initialize an AHT sensor.
 */
typedef struct
{
    i2c_master_bus_handle_t i2c_handle; /*!< I2C bus the sensor is connected to (must be set by the caller) */
    uint8_t i2c_address;                /*!< I2C address: 0x38, 0x39 or 0x44 */
    uint32_t i2c_frequency;             /*!< I2C clock speed in Hz, max 400000 */
} aht_init_config_t;

#define AHT_INIT_CONFIG_DEFAULT()  \
    {                              \
        .i2c_handle = NULL,        \
        .i2c_address = 0x38,       \
        .i2c_frequency = 400000,   \
    }

/**
 * @brief Error statistics collected across all AHT operations.
 */
typedef struct
{
    uint32_t i2c_driver_error; /*!< Number of low level I2C transaction failures */
} aht_stats_t;

/**
 * @brief Initialize the AHT sensor and add it to the given I2C bus.
 *
 * @param config Initialization configuration, see ::aht_init_config_t.
 * @param handle Output handle. Must point to a NULL pointer before the call.
 *
 * @return ESP_OK on success.
 * @return ESP_ERR_INVALID_ARG on invalid argument or configuration.
 * @return ESP_ERR_INVALID_STATE if the handle is already initialized.
 * @return ESP_ERR_NO_MEM if the handle could not be allocated.
 * @return ESP_ERR_NOT_FOUND if the sensor did not respond on the bus.
 * @return ESP_FAIL on I2C driver error.
 */
esp_err_t aht_init(const aht_init_config_t *config, aht_handle_t **handle);

/**
 * @brief Remove the sensor from the I2C bus and free its handle.
 */
esp_err_t aht_deinit(aht_handle_t **handle);

/**
 * @brief Trigger a measurement and read temperature and humidity.
 *
 * @param handle Initialized sensor handle.
 * @param humidity Output relative humidity in %RH.
 * @param temperature Output temperature in degrees Celsius.
 *
 * @return ESP_OK on success.
 * @return ESP_ERR_INVALID_ARG on invalid argument.
 * @return ESP_ERR_TIMEOUT if the sensor did not finish the measurement in time.
 * @return ESP_ERR_INVALID_CRC if the CRC check of the received data failed.
 * @return ESP_FAIL on I2C driver error.
 */
esp_err_t aht_read(aht_handle_t **handle, float *humidity, float *temperature);

/**
 * @brief Soft reset the sensor.
 */
esp_err_t aht_reset(aht_handle_t **handle);

/**
 * @brief Get a pointer to the internal error statistics.
 */
const aht_stats_t *aht_get_stats(void);

/**
 * @brief Reset the internal error statistics counters to zero.
 */
void aht_reset_stats(void);

#ifdef __cplusplus
}
#endif