#ifndef _LPS25HB_IO_H
#define _LPS25HB_IO_H

#include <linux/i2c.h>

int lps25hb_check_communication(struct i2c_client *client);
int lps25hb_configure_device(struct i2c_client *client);
int lps25hb_read_pressure_hpa(struct i2c_client *client);

#endif