#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "graphics.h"
#include "font.h"

static void display_swap(char *a, char *b);

int display_init(struct display_handle *handle, const char *fb_file)
{
    if (handle == NULL)
        return -EINVAL;

    handle->fb_fd = open(fb_file, O_RDWR);
    if (handle->fb_fd < 0)
        return -ENODEV;

    ioctl(handle->fb_fd, FBIOGET_VSCREENINFO, &handle->fb_info);

    if ((handle->fb_info.xres != DISPLAY_WIDTH) ||
        (handle->fb_info.yres != DISPLAY_HEIGHT) ||
        (handle->fb_info.bits_per_pixel != 1))
    {
        close(handle->fb_fd);
        return -EINVAL;
    }

    /* display driver does not support mmap */
    handle->fb_buff_size = (handle->fb_info.xres * handle->fb_info.yres) / 8;
    handle->fb_buff = malloc(handle->fb_buff_size);
    if (handle->fb_buff == NULL)
    {
        close(handle->fb_fd);
        return -ENOMEM;
    }

    return 0;
}

void display_deinit(struct display_handle *handle)
{
    if (handle == NULL)
        return;

    free(handle->fb_buff);
    close(handle->fb_fd);
}

void display_set_pixel(struct display_handle *handle, int16_t x, int16_t y, uint8_t value)
{
    int index, x_byte_index, byte_offset;

    if ((x >= DISPLAY_WIDTH) || (y >= DISPLAY_HEIGHT))
        return;

    x_byte_index = x / 8;
    byte_offset = x % 8;
    index = (y * (DISPLAY_WIDTH / 8)) + x_byte_index;

    if (value)
        handle->fb_buff[index] |= (1 << byte_offset);
    else
        handle->fb_buff[index] &= ~(1 << byte_offset);
}

void display_draw_line(struct display_handle *handle, int16_t start_x, int16_t start_y, int16_t end_x, int16_t end_y, uint8_t value)
{
    int16_t y;
    int dx, dy;
    float a, b;

    if (start_x > end_x)
    {
        display_swap((char *)&start_x, (char *)&end_x);
        display_swap((char *)&start_y, (char *)&end_y);
    }

    // horizontal line
    if (start_x == end_x)
    {
        if (start_y > end_y)
        {
            display_swap((char *)&start_y, (char *)&end_y);
        }
        while (start_y < end_y)
        {
            display_set_pixel(handle, start_x, start_y, value);
            start_y++;
        }
    }

    // vertical line
    if (start_y == end_y)
    {
        while (start_x <= end_x)
        {
            display_set_pixel(handle, start_x, start_y, value);
            start_x++;
        }
    }

    dx = end_x - start_x;
    dy = end_y - start_y;

    a = dy / (dx * 1.0);
    b = (-1.0 * start_x) + start_y;

    while (start_x < end_x)
    {
        y = (a * start_x) + b;
        display_set_pixel(handle, start_x, y, value);
        start_x++;
    }
}

void display_draw_char(struct display_handle *handle, int16_t start_x, int16_t start_y, char c)
{
    int x, y;
    int set;
    char *bitmap;
    int16_t current_x;

    if (c > 127)
        return;

    bitmap = font8x8_basic[(int)c];
    current_x = start_x;

    for (x = 0; x < 8; x++)
    {
        for (y = 0; y < 8; y++)
        {
            set = bitmap[x] & 1 << y;
            display_set_pixel(handle, current_x, start_y, set ? 1 : 0);
            current_x++;
        }
        current_x = start_x;
        start_y++;
    }
}

void display_draw_string(struct display_handle *handle, int16_t x, int16_t y, char *str)
{
    int index = 0;

    if (str == NULL)
        return;

    while (str[index] != '\0')
    {
        display_draw_char(handle, x, y, str[index]);
        index++;
        x += 8;
    }
}

void display_draw(struct display_handle *handle)
{
    write(handle->fb_fd, handle->fb_buff, handle->fb_buff_size);
}

void display_clear_buffer(struct display_handle *handle)
{
    memset(handle->fb_buff, 0x00, handle->fb_buff_size);
}

static void display_swap(char *a, char *b)
{
    char tmp = *a;
    *a = *b;
    *b = tmp;
}