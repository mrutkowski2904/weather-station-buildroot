#ifndef _SSD1306_H
#define _SSD1306_H

#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/i2c.h>

#define SSD1306_MAX_DEVICES 10

struct driver_data
{
    struct class *sysfs_class;
    dev_t device_num_base;
    int number_of_devices;
    struct mutex driver_data_mutex;
};

struct device_data
{
    dev_t dev_num;
    struct device *device;
    struct cdev cdev;
    struct i2c_client *i2c_client;
    struct mutex data_mutex;
};

#endif /* _SSD1306_H */