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
        return -EINVAL;

    return 0;
}