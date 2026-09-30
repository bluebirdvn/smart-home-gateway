#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"
#include "driver/i2c_master.h"

#define BH1750_ERR -1
#define BH1750_OK 0x00

/**
 * @brief Add the BH1750 as an I2C device on the given bus and return its dev_handle.
 */
int16_t bh1750_i2c_hal_init(i2c_master_bus_handle_t bus_handle, uint8_t i2c_addr,
                             uint32_t i2c_freq_hz, i2c_master_dev_handle_t *out_dev_handle);

int16_t bh1750_i2c_hal_read(i2c_master_dev_handle_t dev_handle, uint8_t *data, uint16_t count);
int16_t bh1750_i2c_hal_write(i2c_master_dev_handle_t dev_handle, uint8_t *data, uint16_t count);
void bh1750_i2c_hal_ms_delay(uint32_t ms);

#ifdef __cplusplus
}
#endif