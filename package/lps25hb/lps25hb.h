#ifndef _LPS25HB_H
#define _LPS25HB_H

#include <linux/cdev.h>
#include <linux/i2c.h>

#define LPS25HB_MAX_DEVICES 15

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
    struct mutex read_mutex;
    int last_read_pressure;
    unsigned long last_read_jiffies;
};

/* global variable from lps25hb.c */
extern struct driver_data global_driver_data;

#endif