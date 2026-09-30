#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "bh1750_i2c_hal.h"

#define I2C_ADDRESS_BH1750 0x23
/* #define I2C_ADDRESS_BH1750 0x5C */ /* ADDR pin high */

#define OPECODE_RESET 0x07
#define OPECODE_MEAS_TIME_HIGH_BIT 0x40
#define OPECODE_MEAS_TIME_LOW_BIT 0x60

#define RES_MODE_SET_DELAY_TIME_MS 180
#define H_RES_MODE_MEASUREMENT_TIME_MS 120
#define L_RES_MODE_MEASUREMENT_TIME_MS 16
#define DEFAULT_MEAS_TIME_REG_VAL 0x45

typedef struct
{
    i2c_master_dev_handle_t dev_handle;
    uint16_t meas_time;
    uint16_t meas_time_mul;
    uint8_t mtreg_val;
} bh1750_dev_t;

typedef enum
{
    BH1750_CONT_H_RES_MODE = 0x10,
    BH1750_CONT_H_RES_MODE2 = 0x11,
    BH1750_CONT_L_RES_MODE = 0x13,
    BH1750_ONETIME_H_RES_MODE = 0x20,
    BH1750_ONETIME_H_RES_MODE2 = 0x21,
    BH1750_ONETIME_L_RES_MODE = 0x23,
} bh1750_res_mode_t;

typedef enum
{
    BH1750_POWER_DOWN = 0x00,
    BH1750_POWER_ON = 0x01,
} bh1750_power_mode_t;

int16_t bh1750_i2c_dev_reset(bh1750_dev_t dev);
int16_t bh1750_i2c_set_power_mode(bh1750_dev_t dev, bh1750_power_mode_t mode);
int16_t bh1750_i2c_set_resolution_mode(bh1750_dev_t *dev, bh1750_res_mode_t mode);
int16_t bh1750_i2c_set_mtreg_val(bh1750_dev_t *dev, float sens);
int16_t bh1750_i2c_read_data(bh1750_dev_t dev, uint16_t *dt);

#ifdef __cplusplus
}
#endif