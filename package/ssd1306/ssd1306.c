#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include "ssd1306.h"
#include "display_io.h"

static int display_thread(void *data);
static int display_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int display_remove(struct i2c_client *client);

/* open firmware dt match */
static struct of_device_id ssd1306_dt_match[] = {
    {.compatible = "ws2,customssd1306"},
    {/* NULL */},
};
MODULE_DEVICE_TABLE(of, ssd1306_dt_match);

static struct i2c_device_id ssd1306_ids[] = {
    {"customssd1306", 0},
    {/* NULL */},
};
MODULE_DEVICE_TABLE(i2c, ssd1306_ids);

static struct i2c_driver ssd1306_platform_driver = {
    .probe = display_probe,
    .remove = display_remove,
    .id_table = ssd1306_ids,
    .driver = {
        .name = "customssd1306",
        .of_match_table = of_match_ptr(ssd1306_dt_match),
    },
};

static int display_thread(void *data)
{
    int status;
    struct device_data *dev_data;
    dev_data = data;

    status = ssd1306_configure_hardware(dev_data->i2c_client);
    if (status)
    {
        dev_err(&dev_data->i2c_client->dev, "error while configuring display\n");
        dev_data->operational = false;
        return status;
    }

    while (!kthread_should_stop())
    {
        /* TMP DISPLAY TEST */
        ssd1306_set_pixel(dev_data->display_buffer, 10, 10, 1);
        ssd1306_write_data(dev_data->i2c_client, dev_data->display_buffer, SSD1306_DISPLAY_BUFFER_SIZE);

        msleep(1000);
    }
    return 0;
}

static int display_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct device_data *dev_data;

    dev_data = devm_kzalloc(&client->dev, sizeof(struct device_data), GFP_KERNEL);
    if (dev_data == NULL)
    {
        dev_err(&client->dev, "error while allocating memory for ssd1306 driver\n");
        return -ENOMEM;
    }
    i2c_set_clientdata(client, dev_data);

    dev_data->display_buffer = devm_kzalloc(&client->dev, SSD1306_DISPLAY_BUFFER_SIZE, GFP_KERNEL);
    if (dev_data->display_buffer == NULL)
    {
        dev_err(&client->dev, "error while allocating memory for ssd1306 display buffer\n");
        return -ENOMEM;
    }

    dev_data->display_thread = kthread_create(display_thread, dev_data, "ssd1306_kthread");
    if (!dev_data->display_thread)
    {
        dev_err(&client->dev, "error while creating display thread\n");
        return -ECHILD;
    }
    dev_data->i2c_client = client;
    dev_data->operational = true;

    dev_info(&client->dev, "ssd1306 probe finished\n");
    wake_up_process(dev_data->display_thread);

    return 0;
}

static int display_remove(struct i2c_client *client)
{
    struct device_data *dev_data;
    dev_data = i2c_get_clientdata(client);
    if (dev_data->operational)
        kthread_stop(dev_data->display_thread);
    dev_info(&client->dev, "ssd1306 display removed\n");
    return 0;
}

module_i2c_driver(ssd1306_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maciej Rutkowski");
MODULE_DESCRIPTION("Driver for SSD1306 display");