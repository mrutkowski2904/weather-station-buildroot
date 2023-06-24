#include <stdio.h>

#include "graphics.h"

int main(void)
{
    int status;
    struct display_handle handle;
    status = display_init(&handle, "/dev/fb0");
    if (status)
        return status;

    // display_set_pixel(&handle, DISPLAY_WIDTH / 2, 0, 1);
    // display_set_pixel(&handle, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, 1);
    // display_draw_line(&handle, 0, 0, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, 1);

    // display_draw_line(&handle, 0, 0, 0, DISPLAY_HEIGHT, 1);
    // display_draw_line(&handle, DISPLAY_WIDTH - 1, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT, 1);

    // display_draw_char(&handle, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, 'A');
    display_draw_string(&handle, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, "hello");

    display_draw(&handle);

    display_deinit(&handle);
    return 0;
}