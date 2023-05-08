#ifndef _LPS25HB_H
#define _LPS25HB_H

#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/wait.h>

#define LPS25HB_MAX_DEVICES 15
#define PRESSURE_STRING_BUFFER_SIZE 8
#define LPS25HB_LAST_READ_VALID_TIME_MS 500
#define LPS25HB_LAST_READ_VALID_TIME_MIN_MS 75

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
    struct work_struct device_read_work;
    bool hw_operational;

    bool fresh_data;
    wait_queue_head_t read_event;

    int last_read_pressure;
    unsigned long last_read_jiffies;
    u32 last_read_valid_time_ms;
};

/* global variable from lps25hb.c */
extern struct driver_data global_driver_data;

#endif