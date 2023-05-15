#ifndef _SSD1306_H
#define _SSD1306_H

#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/kthread.h>
#include <linux/fb.h>

struct device_data
{
    struct i2c_client *i2c_client;
    struct task_struct *display_thread;
    bool operational;

    u8 * display_hw_buffer;
    u8 * display_fb;
    struct fb_info *info;
};

#endif /* _SSD1306_H */