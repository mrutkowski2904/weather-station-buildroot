#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/fb.h>

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

static struct fb_ops ssd1306_fb_ops = {
    .owner = THIS_MODULE,
    .fb_fillrect = cfb_fillrect,
    .fb_imageblit = cfb_imageblit,
    .fb_copyarea = cfb_copyarea,
};

struct fb_fix_screeninfo ssd1306_fix_screninfo = {
    .id = "SSD1306",
    .type = FB_TYPE_PACKED_PIXELS,
    .visual = FB_VISUAL_MONO10,
    .line_length = SSD1306_LINE_LENGTH,
    .accel = FB_ACCEL_NONE,
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
        ssd1306_map_fb_to_display_buffer(dev_data->display_hw_buffer, dev_data->display_fb);
        ssd1306_write_data(dev_data->i2c_client, dev_data->display_hw_buffer, SSD1306_DISPLAY_BUFFER_SIZE);
        msleep(200);
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

    dev_data->display_fb = devm_kzalloc(&client->dev, SSD1306_DISPLAY_BUFFER_SIZE, GFP_KERNEL);
    if (dev_data->display_fb == NULL)
    {
        dev_err(&client->dev, "error while allocating memory for ssd1306 frame buffer\n");
        return -ENOMEM;
    }

    dev_data->display_hw_buffer = devm_kzalloc(&client->dev, SSD1306_DISPLAY_BUFFER_SIZE, GFP_KERNEL);
    if (dev_data->display_hw_buffer == NULL)
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

    dev_data->info = framebuffer_alloc(0, &client->dev);
    if (dev_data->info == NULL)
    {
        dev_err(&client->dev, "error while allocating memory for ssd1306 frame buffer structure\n");
        return -ENOMEM;
    }
    dev_data->info->screen_base = dev_data->display_fb;
    dev_data->info->screen_size = SSD1306_DISPLAY_BUFFER_SIZE;
    dev_data->info->fbops = &ssd1306_fb_ops;
    memcpy(&dev_data->info->fix, &ssd1306_fix_screninfo, sizeof(ssd1306_fix_screninfo));

    dev_data->info->var.yres = SSD1306_HEIGHT;
    dev_data->info->var.xres = SSD1306_WIDTH;
    dev_data->info->var.yres_virtual = SSD1306_HEIGHT;
    dev_data->info->var.xres_virtual = SSD1306_WIDTH;
    dev_data->info->var.red.length = 1;
    dev_data->info->var.green.length = 1;
    dev_data->info->var.blue.length = 1;
    dev_data->info->var.bits_per_pixel = 1;
    dev_data->info->var.grayscale = 1;
    dev_data->info->var.activate = FB_ACTIVATE_NOW;

    if (register_framebuffer(dev_data->info))
    {
        framebuffer_release(dev_data->info);
        dev_err(&client->dev, "error while registering framebuffer\n");
        return -EINVAL;
    }

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
    unregister_framebuffer(dev_data->info);
    framebuffer_release(dev_data->info);
    dev_info(&client->dev, "ssd1306 display removed\n");
    return 0;
}

module_i2c_driver(ssd1306_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maciej Rutkowski");
MODULE_DESCRIPTION("Driver for SSD1306 display");