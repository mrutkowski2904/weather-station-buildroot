#include <stdint.h>
#include <unistd.h>

#include "display.h"
#include "graphics.h"

#define MS (1000U)

void *display_thread_entry(void *data)
{
    int status = 0;
    struct display_handle handle;

    (void)data;
    status = display_init(&handle, "/dev/fb0");
    if (status)
        return (void *)status;

    while (1)
    {
        display_clear_buffer(&handle);
        display_draw_string(&handle, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, "test");
        display_draw(&handle);
        usleep(500 * MS);
    }

    display_deinit(&handle);
    return (void *)status;
}