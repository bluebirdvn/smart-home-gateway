#include "bh1750_i2c.h"
#include "math.h"

int16_t bh1750_i2c_dev_reset(bh1750_dev_t dev)
{
    uint8_t data = OPECODE_RESET;
    return bh1750_i2c_hal_write(dev.dev_handle, &data, 1);
}

int16_t bh1750_i2c_set_power_mode(bh1750_dev_t dev, bh1750_power_mode_t mode)
{
    uint8_t data = (uint8_t)mode;
    return bh1750_i2c_hal_write(dev.dev_handle, &data, 1);
}

int16_t bh1750_i2c_set_resolution_mode(bh1750_dev_t *dev, bh1750_res_mode_t mode)
{
    uint8_t data = (uint8_t)mode;
    int16_t err = bh1750_i2c_hal_write(dev->dev_handle, &data, 1);

    if (err != BH1750_OK)
    {
        return err;
    }

    if (mode == BH1750_CONT_L_RES_MODE || mode == BH1750_ONETIME_L_RES_MODE)
    {
        dev->meas_time = L_RES_MODE_MEASUREMENT_TIME_MS;
    }
    else
    {
        dev->meas_time = H_RES_MODE_MEASUREMENT_TIME_MS;
    }

    bh1750_i2c_hal_ms_delay(RES_MODE_SET_DELAY_TIME_MS);
    return err;
}

int16_t bh1750_i2c_set_mtreg_val(bh1750_dev_t *dev, float sens)
{
    sens /= 100;

    if (sens > 100 || ((dev->mtreg_val * (uint8_t)(1 / sens)) > 0xFF))
    {
        return BH1750_ERR;
    }

    uint8_t mtreg = dev->mtreg_val * (uint8_t)(1 / sens);
    uint8_t data = OPECODE_MEAS_TIME_HIGH_BIT | (mtreg >> 5);
    int16_t err = bh1750_i2c_hal_write(dev->dev_handle, &data, 1);

    if (err != BH1750_OK)
    {
        return err;
    }

    data = OPECODE_MEAS_TIME_LOW_BIT | (mtreg & 0x1F);
    err = bh1750_i2c_hal_write(dev->dev_handle, &data, 1);

    if (err != BH1750_OK)
    {
        return err;
    }

    dev->meas_time_mul = (uint8_t)(1 / sens);
    dev->mtreg_val = mtreg;
    return err;
}

int16_t bh1750_i2c_read_data(bh1750_dev_t dev, uint16_t *dt)
{
    uint8_t data[2];
    int16_t err = bh1750_i2c_hal_read(dev.dev_handle, data, 2);

    *dt = (data[0] << 8) | data[1];
    *dt = (uint16_t)round(*dt / 1.2f);

    bh1750_i2c_hal_ms_delay(dev.meas_time * dev.meas_time_mul);
    return err;
}