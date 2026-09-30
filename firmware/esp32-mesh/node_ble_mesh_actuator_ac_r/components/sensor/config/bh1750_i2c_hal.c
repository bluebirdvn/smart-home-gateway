#include "bh1750_i2c_hal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

int16_t bh1750_i2c_hal_init(i2c_master_bus_handle_t bus_handle, uint8_t i2c_addr,
                             uint32_t i2c_freq_hz, i2c_master_dev_handle_t *out_dev_handle)
{
    if (bus_handle == NULL || out_dev_handle == NULL) {
        return BH1750_ERR;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = i2c_addr,
        .scl_speed_hz = i2c_freq_hz,
    };

    esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_config, out_dev_handle);
    return (err == ESP_OK) ? BH1750_OK : BH1750_ERR;
}

int16_t bh1750_i2c_hal_read(i2c_master_dev_handle_t dev_handle, uint8_t *data, uint16_t count)
{
    if (dev_handle == NULL) {
        return BH1750_ERR;
    }
    esp_err_t err = i2c_master_receive(dev_handle, data, count, pdMS_TO_TICKS(1000));
    return (err == ESP_OK) ? BH1750_OK : BH1750_ERR;
}

int16_t bh1750_i2c_hal_write(i2c_master_dev_handle_t dev_handle, uint8_t *data, uint16_t count)
{
    if (dev_handle == NULL) {
        return BH1750_ERR;
    }
    esp_err_t err = i2c_master_transmit(dev_handle, data, count, pdMS_TO_TICKS(1000));
    return (err == ESP_OK) ? BH1750_OK : BH1750_ERR;
}

void bh1750_i2c_hal_ms_delay(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}