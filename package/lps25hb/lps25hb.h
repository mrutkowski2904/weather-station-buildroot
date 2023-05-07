#ifndef _LPS25HB_H
#define _LPS25HB_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/sys.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>

#define LPS25HB_MAX_DEVICES 15

struct driver_data
{
    struct class *sysfs_class;
    dev_t device_num_base;
    int number_of_devices;
};

struct device_data
{
    dev_t dev_num;
    struct device *device;
    struct cdev cdev;
    struct mutex read_mutex;
    int last_read_pressure;
    unsigned long last_read_jiffies;
};

/* global variable from lps25hb.c */
extern struct driver_data global_driver_data;

#endif