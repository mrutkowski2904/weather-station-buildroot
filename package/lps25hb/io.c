#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/errno.h>

#include "io.h"
#include "registers.h"

int lps25hb_check_communication(struct i2c_client *client)
{
    s32 read_result;
    read_result = i2c_smbus_read_byte_data(client, LPS25HB_REG_WHO_AM_I_ADDR);
    if (read_result < 0)
        return read_result;

    if (read_result != LPS25HB_REG_WHO_AM_I_EXPECTED_VAL)
        return -EIO;

    return 0;
}

int lps25hb_configure_device(struct i2c_client *client)
{
    return i2c_smbus_write_byte_data(client, LPS25HB_REG_CTRL1_ADDR, LPS25HB_REG_CTRL1_VALUE);
}

int lps25hb_read_pressure_hpa(struct i2c_client *client)
{
    s32 read_result;
    u8 pressure_reg_h, pressure_reg_l, pressure_reg_xl;
    int pressure;

    read_result = i2c_smbus_read_byte_data(client, LPS25HB_REG_PRESS_OUT_XL_ADDR);
    if (read_result < 0)
        return -EIO;
    pressure_reg_xl = read_result & 0xff;

    read_result = i2c_smbus_read_byte_data(client, LPS25HB_REG_PRESS_OUT_L_ADDR);
    if (read_result < 0)
        return -EIO;
    pressure_reg_l = read_result & 0xff;

    read_result = i2c_smbus_read_byte_data(client, LPS25HB_REG_PRESS_OUT_H_ADDR);
    if (read_result < 0)
        return -EIO;
    pressure_reg_h = read_result & 0xff;

    pressure = ((pressure_reg_h << 16) | (pressure_reg_l << 8) | (pressure_reg_xl < 0)) / 4096;
    return pressure;
}