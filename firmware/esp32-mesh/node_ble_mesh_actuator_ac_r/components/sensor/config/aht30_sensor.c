#include "aht30_sensor.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "aht_sensor";

#define AHT_LOGI(msg, ...) ESP_LOGI(TAG, msg, ##__VA_ARGS__)
#define AHT_LOGE(msg, err, ...) ESP_LOGE(TAG, "[%s:%d:%s] " msg, __FILE__, __LINE__, esp_err_to_name(err), ##__VA_ARGS__)

#define AHT_CHECK(cond, err, cleanup, msg, ...) \
    if (!(cond))                                \
    {                                            \
        AHT_LOGE(msg, err, ##__VA_ARGS__);      \
        cleanup;                                \
        return err;                             \
    }

/**
 * @brief Internal structure managing the I2C device connection.
 */
struct aht_handle_t
{
    i2c_master_dev_handle_t dev_handle;
};

static const uint8_t s_aht_reset_command = 0xBA;
static const uint8_t s_aht_read_command[] = {0xAC, 0x33, 0x00};
static const uint8_t s_aht_init_command[] = {0xBE, 0x08, 0x00};
static aht_stats_t s_stats = {0};

static esp_err_t aht_validate_config(const aht_init_config_t *config);
static esp_err_t aht_i2c_init(const aht_init_config_t *config, aht_handle_t *handle);
static uint8_t aht_calc_crc(const uint8_t *buf, size_t len);

esp_err_t aht_init(const aht_init_config_t *config, aht_handle_t **handle)
{
    AHT_LOGI("AHT initialization begin.");
    AHT_CHECK(config != NULL && handle != NULL, ESP_ERR_INVALID_ARG, NULL, "AHT initialization failed. Invalid argument.");
    AHT_CHECK(aht_validate_config(config) == ESP_OK, ESP_ERR_INVALID_ARG, NULL, "AHT initialization failed. Invalid config.");
    AHT_CHECK(*handle == NULL, ESP_ERR_INVALID_STATE, NULL, "AHT initialization failed. AHT is already initialized.");

    *handle = heap_caps_calloc(1, sizeof(aht_handle_t), MALLOC_CAP_8BIT);
    AHT_CHECK(*handle != NULL, ESP_ERR_NO_MEM, NULL, "AHT initialization failed. Failed to allocate AHT handle.");

    esp_err_t err = aht_i2c_init(config, *handle);
    if (err != ESP_OK)
    {
        heap_caps_free(*handle);
        *handle = NULL;
        AHT_LOGE("AHT initialization failed. I2C init failed.", err);
        return err;
    }

    AHT_LOGI("AHT initialization completed successfully.");
    return ESP_OK;
}

esp_err_t aht_deinit(aht_handle_t **handle)
{
    AHT_LOGI("AHT deinitialization started.");
    AHT_CHECK(handle != NULL && *handle != NULL, ESP_ERR_INVALID_ARG, NULL, "AHT deinitialization failed. Invalid argument.");
    AHT_CHECK(i2c_master_bus_rm_device((*handle)->dev_handle) == ESP_OK, ESP_FAIL, NULL, "AHT deinitialization failed. I2C remove device failed.");

    heap_caps_free(*handle);
    *handle = NULL;
    AHT_LOGI("AHT deinitialization completed successfully.");
    return ESP_OK;
}

esp_err_t aht_read(aht_handle_t **handle, float *humidity, float *temperature)
{
    AHT_LOGI("AHT read begin.");
    AHT_CHECK(handle != NULL && *handle != NULL && humidity != NULL && temperature != NULL,
              ESP_ERR_INVALID_ARG, NULL, "AHT read fail. Invalid argument.");

    uint8_t sensor_data[7] = {0};

    esp_err_t err = i2c_master_transmit((*handle)->dev_handle, s_aht_read_command, sizeof(s_aht_read_command), 1000 / portTICK_PERIOD_MS);
    if (err != ESP_OK)
    {
        ++s_stats.i2c_driver_error;
        AHT_LOGE("AHT read fail. I2C driver error.", err);
        return ESP_FAIL;
    }

    vTaskDelay(80 / portTICK_PERIOD_MS);

    err = i2c_master_receive((*handle)->dev_handle, sensor_data, sizeof(sensor_data), 1000 / portTICK_PERIOD_MS);
    if (err != ESP_OK)
    {
        ++s_stats.i2c_driver_error;
        AHT_LOGE("AHT read fail. I2C driver error.", err);
        return ESP_FAIL;
    }

    AHT_CHECK((sensor_data[0] & 0x80) == 0, ESP_ERR_TIMEOUT, NULL, "AHT read fail. Timeout exceeded.");
    AHT_CHECK(aht_calc_crc(sensor_data, 6) == sensor_data[6], ESP_ERR_INVALID_CRC, NULL, "AHT read fail. Invalid CRC.");

    uint32_t raw_humidity = (((uint32_t)sensor_data[1] << 16) | ((uint32_t)sensor_data[2] << 8) | sensor_data[3]) >> 4;
    uint32_t raw_temperature = (((uint32_t)sensor_data[3] << 16) | ((uint32_t)sensor_data[4] << 8) | sensor_data[5]) & 0xFFFFF;

    *humidity = raw_humidity / 1048576.0f * 100.0f;
    *temperature = raw_temperature / 1048576.0f * 200.0f - 50.0f;

    AHT_LOGI("AHT read completed successfully.");
    return ESP_OK;
}

esp_err_t aht_reset(aht_handle_t **handle)
{
    AHT_LOGI("AHT reset begin.");
    AHT_CHECK(handle != NULL && *handle != NULL, ESP_ERR_INVALID_ARG, NULL, "AHT reset fail. Invalid argument.");

    esp_err_t err = i2c_master_transmit((*handle)->dev_handle, &s_aht_reset_command, sizeof(s_aht_reset_command), 1000 / portTICK_PERIOD_MS);
    if (err != ESP_OK)
    {
        ++s_stats.i2c_driver_error;
        AHT_LOGE("AHT reset fail. I2C driver error.", err);
        return ESP_FAIL;
    }

    vTaskDelay(20 / portTICK_PERIOD_MS);
    AHT_LOGI("AHT reset completed successfully.");
    return ESP_OK;
}

const aht_stats_t *aht_get_stats(void)
{
    return &s_stats;
}

void aht_reset_stats(void)
{
    AHT_LOGI("Error statistic reset started.");
    s_stats.i2c_driver_error = 0;
    AHT_LOGI("Error statistic reset successfully.");
}

static esp_err_t aht_validate_config(const aht_init_config_t *config)
{
    AHT_CHECK(config->i2c_address == 0x38 || config->i2c_address == 0x39 || config->i2c_address == 0x44,
              ESP_ERR_INVALID_ARG, NULL, "Invalid I2C address.");
    AHT_CHECK(config->i2c_frequency <= 400000, ESP_ERR_INVALID_ARG, NULL, "Invalid I2C frequency.");
    AHT_CHECK(config->i2c_handle != NULL, ESP_ERR_INVALID_ARG, NULL, "Invalid I2C handle.");
    return ESP_OK;
}

static esp_err_t aht_i2c_init(const aht_init_config_t *config, aht_handle_t *handle)
{
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = config->i2c_address,
        .scl_speed_hz = config->i2c_frequency,
    };

    i2c_master_dev_handle_t dev_handle = NULL;
    AHT_CHECK(i2c_master_bus_add_device(config->i2c_handle, &dev_config, &dev_handle) == ESP_OK,
              ESP_FAIL, NULL, "Add I2C device failed.");

    vTaskDelay(100 / portTICK_PERIOD_MS);

    esp_err_t err = i2c_master_probe(config->i2c_handle, config->i2c_address, 1000 / portTICK_PERIOD_MS);
    if (err != ESP_OK)
    {
        i2c_master_bus_rm_device(dev_handle);
        AHT_LOGE("Sensor not connected or not responded.", err);
        return ESP_ERR_NOT_FOUND;
    }

    uint8_t status_byte = 0;
    err = i2c_master_receive(dev_handle, &status_byte, sizeof(status_byte), 1000 / portTICK_PERIOD_MS);
    if (err != ESP_OK)
    {
        i2c_master_bus_rm_device(dev_handle);
        AHT_LOGE("I2C driver error.", err);
        return ESP_FAIL;
    }

    if ((status_byte & 0x08) == 0)
    {
        err = i2c_master_transmit(dev_handle, s_aht_init_command, sizeof(s_aht_init_command), 1000 / portTICK_PERIOD_MS);
        if (err != ESP_OK)
        {
            i2c_master_bus_rm_device(dev_handle);
            AHT_LOGE("I2C driver error.", err);
            return ESP_FAIL;
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }

    handle->dev_handle = dev_handle;
    return ESP_OK;
}

static uint8_t aht_calc_crc(const uint8_t *buf, size_t len)
{
    uint8_t crc = 0xFF;

    for (size_t i = 0; i < len; i++)
    {
        crc ^= buf[i];

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if ((crc & 0x80) != 0)
            {
                crc = (uint8_t)((crc << 1) ^ 0x31);
            }
            else
            {
                crc = (uint8_t)(crc << 1);
            }
        }
    }

    return crc;
}