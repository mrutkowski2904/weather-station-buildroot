#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/sys.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/wait.h>

#include "ssd1306.h"

static int display_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int display_remove(struct i2c_client *client);

/* open firmware dt match */
struct of_device_id ssd1306_dt_match[] = {
    {.compatible = "ws2,customssd1306"},
    {/* NULL */},
};
MODULE_DEVICE_TABLE(of, ssd1306_dt_match);

struct i2c_device_id ssd1306_ids[] = {
    {"customssd1306", 0},
    {/* NULL */},
};
MODULE_DEVICE_TABLE(i2c, ssd1306_ids);

/* driver wide data */
struct driver_data driver_data;

struct i2c_driver ssd1306_platform_driver = {
    .probe = display_probe,
    .remove = display_remove,
    .id_table = ssd1306_ids,
    .driver = {
        .name = "customssd1306",
        .of_match_table = of_match_ptr(ssd1306_dt_match),
    },
};

static int display_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    dev_info(&client->dev, "in ssd1306 probe\n");
    return 0;
}

static int display_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "in ssd1306 remove\n");
    return 0;
}

static int __init ssd1306_init(void)
{
    int status;

    status = alloc_chrdev_region(&driver_data.device_num_base, 0, SSD1306_MAX_DEVICES, "ssd1306");
    if (status < 0)
    {
        pr_err("registering char device number for ssd1306 failed\n");
        return status;
    }
    driver_data.number_of_devices = 0;
    mutex_init(&driver_data.driver_data_mutex);
    driver_data.sysfs_class = class_create(THIS_MODULE, "ssd1306");
    if (IS_ERR_OR_NULL(driver_data.sysfs_class))
    {
        pr_err("error while creating ssd1306 class\n");
        unregister_chrdev_region(driver_data.device_num_base, SSD1306_MAX_DEVICES);
        return PTR_ERR(driver_data.sysfs_class);
    }

    /* register this driver as i2c driver */
    status = i2c_add_driver(&ssd1306_platform_driver);
    if (status < 0)
    {
        class_destroy(driver_data.sysfs_class);
        unregister_chrdev_region(driver_data.device_num_base, SSD1306_MAX_DEVICES);
        pr_err("error while inserting ssd1306 i2c platform driver\n");
        return status;
    }

    pr_info("driver for ssd1306 initialised\n");
    return 0;
}

static void __exit ssd1306_exit(void)
{
    i2c_del_driver(&ssd1306_platform_driver);
    class_destroy(driver_data.sysfs_class);
    unregister_chrdev_region(driver_data.device_num_base, SSD1306_MAX_DEVICES);
    pr_info("ssd1306 driver unregistered\n");
}

module_init(ssd1306_init);
module_exit(ssd1306_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maciej Rutkowski");
MODULE_DESCRIPTION("Driver for SSD1306 display");