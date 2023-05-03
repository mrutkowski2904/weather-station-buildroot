#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sys.h>
#include <linux/cdev.h>
#include <linux/i2c.h>

#define LPS25HB_MAX_DEVICES 15

struct driver_data
{
    struct class *sysfs_class;
    dev_t device_num_base;
    int number_of_devices;
};

static int device_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int device_remove(struct i2c_client *client);

/* dt match */
struct of_device_id lps25hb_dt_match[] = {
    {.compatible = "ws2,customlps25hb"},
    {/* NULL */},
};
MODULE_DEVICE_TABLE(of, lps25hb_dt_match);

struct i2c_device_id lps25hb_ids[] = {
    {"customlps25hb", 0},
    {/* NULL */},
};
MODULE_DEVICE_TABLE(i2c, lps25hb_ids);

struct i2c_driver lps25hb_platform_driver = {
    .probe = device_probe,
    .remove = device_remove,
    .id_table = lps25hb_ids,
    .driver = {
        .name = "customlps25hb",
        .of_match_table = of_match_ptr(lps25hb_dt_match),
    },
};

static struct driver_data global_driver_data;

static int device_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    dev_info(&client->dev, "probe called\n");
    return 0;
}

static int device_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "remove called\n");
    return 0;
}

static int __init lps25hb_init(void)
{
    int status;

    status = alloc_chrdev_region(&global_driver_data.device_num_base, 0, LPS25HB_MAX_DEVICES, "lps25hb");
    if (status < 0)
    {
        pr_err("registering char device number for lps25hb failed\n");
        return status;
    }
    global_driver_data.number_of_devices = 0;
    global_driver_data.sysfs_class = class_create(THIS_MODULE, "lps25hb");
    if (IS_ERR_OR_NULL(global_driver_data.sysfs_class))
    {
        pr_err("error while creating lps25hb class\n");
        unregister_chrdev_region(global_driver_data.device_num_base, LPS25HB_MAX_DEVICES);
        return PTR_ERR(global_driver_data.sysfs_class);
    }

    /* register this driver as i2c driver - probe will be called after that */
    status = i2c_add_driver(&lps25hb_platform_driver);
    if (status < 0)
    {
        class_destroy(global_driver_data.sysfs_class);
        unregister_chrdev_region(global_driver_data.device_num_base, LPS25HB_MAX_DEVICES);
        pr_err("error while inserting lps25hb i2c platform driver\n");
        return status;
    }

    pr_info("driver for lps25hb initialised\n");
    return 0;
}

static void __exit lps25hb_exit(void)
{
    i2c_del_driver(&lps25hb_platform_driver);
    class_destroy(global_driver_data.sysfs_class);
    unregister_chrdev_region(global_driver_data.device_num_base, LPS25HB_MAX_DEVICES);
    pr_info("lps25hb driver unregistered\n");
}

module_init(lps25hb_init);
module_exit(lps25hb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maciej Rutkowski");
MODULE_DESCRIPTION("Driver for LPS25HB");