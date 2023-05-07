#ifndef _LPS25HB_IO_H
#define _LPS25HB_IO_H

#include <linux/i2c.h>

int lps25hb_check_communication(struct i2c_client *client);

#endif