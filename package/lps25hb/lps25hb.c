#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/sys.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/wait.h>

#include "lps25hb.h"
#include "io.h"

/* device file io */
static int device_fd_open(struct inode *inode, struct file *file);
static int device_fd_release(struct inode *inode, struct file *file);
static ssize_t device_fd_read(struct file *file, char __user *buffer, size_t count, loff_t *f_pos);

static void workqueue_read_callback(struct work_struct *work);

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

static struct file_operations file_ops = {
    .owner = THIS_MODULE,
    .open = device_fd_open,
    .release = device_fd_release,
    .read = device_fd_read,
};

struct driver_data driver_data;

static int device_fd_open(struct inode *inode, struct file *file)
{
    struct device_data *dev_data;

    /* /dev/barometerX is read only */
    if (file->f_mode & FMODE_WRITE)
        return -EPERM;

    dev_data = container_of(inode->i_cdev, struct device_data, cdev);
    file->private_data = dev_data;

    return 0;
}

static int device_fd_release(struct inode *inode, struct file *file)
{
    /* nothing to do */
    return 0;
}

static ssize_t device_fd_read(struct file *file, char __user *buffer, size_t count, loff_t *f_pos)
{
    struct device_data *dev_data;
    char pressure_str[PRESSURE_STRING_BUFFER_SIZE];
    int pressure_str_len;

    dev_data = file->private_data;

    /* hardware problem */
    if (!dev_data->hw_operational)
        return -EIO;

    /* userspace program (cat) does second read
     * because output string is shorter than requested amount
     * there is no more data to read
     */
    if (*f_pos != 0)
        return 0;

    /* handle non blocking read */
    if (file->f_flags & O_NONBLOCK)
    {
        /* if stored data is not old, return it, else schedule read */
        return -EAGAIN;
    }

    /* check if data update is needed */
    if (mutex_lock_interruptible(&dev_data->data_mutex))
        return -ERESTARTSYS;
    dev_data->fresh_data = !time_is_before_jiffies(dev_data->last_read_jiffies + msecs_to_jiffies(dev_data->last_read_valid_time_ms));
    mutex_unlock(&dev_data->data_mutex);

    if (!dev_data->fresh_data)
    {
        schedule_work(&dev_data->device_read_work);
        if (wait_event_interruptible(dev_data->read_event, dev_data->fresh_data == true))
            return -ERESTARTSYS;

        /* error might have occured during scheduled read */
        if (!dev_data->hw_operational)
            return -EIO;
    }

    if (mutex_lock_interruptible(&dev_data->data_mutex))
        return -ERESTARTSYS;
    pressure_str_len = snprintf(pressure_str, PRESSURE_STRING_BUFFER_SIZE, "%d", dev_data->last_read_pressure);
    mutex_unlock(&dev_data->data_mutex);

    if (pressure_str_len > count)
        pressure_str_len = count;

    if (copy_to_user(buffer, pressure_str, pressure_str_len))
        return -EFAULT;

    *f_pos += pressure_str_len;
    return pressure_str_len;
}

static void workqueue_read_callback(struct work_struct *work)
{
    int pressure;
    struct device_data *dev_data;
    dev_data = container_of(work, struct device_data, device_read_work);

    mutex_lock(&dev_data->data_mutex);
    pressure = lps25hb_read_pressure_hpa(dev_data->i2c_client);
    if (pressure < 0)
        dev_data->hw_operational = false;
    dev_data->last_read_pressure = pressure;
    dev_data->last_read_jiffies = jiffies;
    dev_data->fresh_data = true;
    mutex_unlock(&dev_data->data_mutex);
    wake_up_interruptible_all(&dev_data->read_event);
}

static int device_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int status;
    struct device_data *dev_data;

    /* check if this is our device and communication works */
    status = lps25hb_check_communication(client);
    if (status < 0)
    {
        dev_err(&client->dev, "error while reading data from the device\n");
        return status;
    }

    dev_data = devm_kzalloc(&client->dev, sizeof(struct device_data), GFP_KERNEL);
    if (dev_data == NULL)
    {
        dev_err(&client->dev, "error while allocating memory for driver data\n");
        return -ENOMEM;
    }
    i2c_set_clientdata(client, dev_data);

    mutex_init(&dev_data->data_mutex);
    dev_data->i2c_client = client;
    dev_data->fresh_data = false;
    dev_data->hw_operational = false;
    dev_data->last_read_valid_time_ms = LPS25HB_LAST_READ_VALID_TIME_MS;
    dev_data->cdev.owner = THIS_MODULE;
    init_waitqueue_head(&dev_data->read_event);
    INIT_WORK(&dev_data->device_read_work, workqueue_read_callback);

    /* create /dev/barometerX */
    mutex_lock(&driver_data.driver_data_mutex);
    dev_data->dev_num = driver_data.device_num_base + driver_data.number_of_devices;
    cdev_init(&dev_data->cdev, &file_ops);
    status = cdev_add(&dev_data->cdev, dev_data->dev_num, 1);
    if (status < 0)
    {
        mutex_unlock(&driver_data.driver_data_mutex);
        dev_err(&client->dev, "LPS25HB cdev add failed\n");
        return status;
    }

    /* this might sleep - mutex needed */
    dev_data->device = device_create(driver_data.sysfs_class, &client->dev, dev_data->dev_num, dev_data, "barometer%d", driver_data.number_of_devices);
    if (IS_ERR(dev_data->device))
    {
        mutex_unlock(&driver_data.driver_data_mutex);
        status = PTR_ERR(dev_data->device);
        dev_err(&client->dev, "LPS25HB error while creating device\n");
        return status;
    }
    driver_data.number_of_devices++;
    mutex_unlock(&driver_data.driver_data_mutex);

    status = lps25hb_configure_device(client);
    if (status)
    {
        device_destroy(driver_data.sysfs_class, dev_data->dev_num);
        dev_err(&client->dev, "LPS25HB error while configuring the device\n");
        return status;
    }
    dev_data->hw_operational = true;

    dev_info(&client->dev, "LPS25HB probe successful, established communication with the device\n");
    return 0;
}

static int device_remove(struct i2c_client *client)
{
    struct device_data *dev_data;
    dev_data = i2c_get_clientdata(client);
    device_destroy(driver_data.sysfs_class, dev_data->dev_num);
    dev_info(&client->dev, "LPS25HB removed\n");
    return 0;
}

static int __init lps25hb_init(void)
{
    int status;

    status = alloc_chrdev_region(&driver_data.device_num_base, 0, LPS25HB_MAX_DEVICES, "lps25hb");
    if (status < 0)
    {
        pr_err("registering char device number for lps25hb failed\n");
        return status;
    }
    driver_data.number_of_devices = 0;
    mutex_init(&driver_data.driver_data_mutex);
    driver_data.sysfs_class = class_create(THIS_MODULE, "lps25hb");
    if (IS_ERR_OR_NULL(driver_data.sysfs_class))
    {
        pr_err("error while creating lps25hb class\n");
        unregister_chrdev_region(driver_data.device_num_base, LPS25HB_MAX_DEVICES);
        return PTR_ERR(driver_data.sysfs_class);
    }

    /* register this driver as i2c driver - probe will be called after that */
    status = i2c_add_driver(&lps25hb_platform_driver);
    if (status < 0)
    {
        class_destroy(driver_data.sysfs_class);
        unregister_chrdev_region(driver_data.device_num_base, LPS25HB_MAX_DEVICES);
        pr_err("error while inserting lps25hb i2c platform driver\n");
        return status;
    }

    pr_info("driver for lps25hb initialised\n");
    return 0;
}

static void __exit lps25hb_exit(void)
{
    i2c_del_driver(&lps25hb_platform_driver);
    class_destroy(driver_data.sysfs_class);
    unregister_chrdev_region(driver_data.device_num_base, LPS25HB_MAX_DEVICES);
    pr_info("lps25hb driver unregistered\n");
}

module_init(lps25hb_init);
module_exit(lps25hb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maciej Rutkowski");
MODULE_DESCRIPTION("Driver for LPS25HB");