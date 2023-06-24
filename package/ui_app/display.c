#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "display.h"
#include "graphics.h"

#define MS (1000U)
#define DISPLAY_X_PADDING 5
#define DISPLAY_END_OFFSET 5

#define DISPLAY_TITLE "Weather"
#define DISPLAY_TITLE_X_POS 36
#define DISPLAY_TITLE_Y_POS 4

#define DISPLAY_READINGS_Y_OFFSET (DISPLAY_TITLE_Y_POS + 7)

static void draw_interface(struct display_handle *handle);

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

        draw_interface(&handle);

        display_draw(&handle);
        usleep(500 * MS);
    }

    display_deinit(&handle);
    return (void *)status;
}

static void draw_interface(struct display_handle *handle)
{
    /* draw borders */
    display_draw_line(handle, 0, 0, 0, DISPLAY_HEIGHT - DISPLAY_END_OFFSET, 1);
    display_draw_line(handle, DISPLAY_WIDTH - 1, 0, DISPLAY_WIDTH - 1,
                      (DISPLAY_HEIGHT - DISPLAY_END_OFFSET), 1);
    display_draw_line(handle, 0, 0, DISPLAY_WIDTH - 1, 0, 1);
    display_draw_line(handle, 0, (DISPLAY_HEIGHT - DISPLAY_END_OFFSET), DISPLAY_WIDTH - 1,
                      (DISPLAY_HEIGHT - DISPLAY_END_OFFSET), 1);

    /* draw title */
    display_draw_string(handle, DISPLAY_TITLE_X_POS, DISPLAY_TITLE_Y_POS, DISPLAY_TITLE);
    display_draw_line(handle, DISPLAY_TITLE_X_POS - 5,
                      DISPLAY_TITLE_Y_POS + 9,
                      DISPLAY_TITLE_X_POS + ((sizeof(DISPLAY_TITLE) - 1) * CHARACTER_DIMENSION) + 5,
                      DISPLAY_TITLE_Y_POS + 9,
                      1);

    /* dummy readings */
    display_draw_string(handle, DISPLAY_X_PADDING, DISPLAY_READINGS_Y_OFFSET + 10, "Temp: 21 C");
    display_draw_string(handle, DISPLAY_X_PADDING, DISPLAY_READINGS_Y_OFFSET + 21, "Pres: 1015 hPa");
    display_draw_string(handle, DISPLAY_X_PADDING, DISPLAY_READINGS_Y_OFFSET + 32, "Hum: 74 %");
}