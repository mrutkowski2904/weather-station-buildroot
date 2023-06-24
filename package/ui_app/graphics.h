#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <stdint.h>
#include <linux/fb.h>

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define CHARACTER_DIMENSION 8

struct display_handle
{
    int fb_fd;
    struct fb_var_screeninfo fb_info;
    uint8_t *fb_buff;
    size_t fb_buff_size;
};

int display_init(struct display_handle *handle, const char *fb_file);
void display_deinit(struct display_handle *handle);

void display_set_pixel(struct display_handle *handle, int16_t x, int16_t y, uint8_t value);
void display_draw_line(struct display_handle *handle, int16_t start_x, int16_t start_y, int16_t end_x, int16_t end_y, uint8_t value);
void display_draw_char(struct display_handle *handle, int16_t start_x, int16_t start_y, char c);
void display_draw_string(struct display_handle *handle, int16_t x, int16_t y, char *str);

void display_draw(struct display_handle *handle);
void display_clear_buffer(struct display_handle *handle);

#endif /* _GRAPHICS_H */