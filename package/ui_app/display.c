#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "display.h"
#include "graphics.h"
#include "sensors.h"

#define DISPLAY_X_PADDING 5
#define DISPLAY_END_OFFSET 5

#define DISPLAY_TITLE "Weather"
#define DISPLAY_TITLE_X_POS 36
#define DISPLAY_TITLE_Y_POS 4

#define DISPLAY_READINGS_Y_OFFSET (DISPLAY_TITLE_Y_POS + 7)
#define SENSOR_DATA_STR_LEN 20

static void draw_interface(struct display_handle *display, struct sensors_handle *sensors);

void *display_thread_entry(void *data)
{
    int status = 0;
    struct display_handle display;
    struct sensors_handle *sensors = data;

    status = display_init(&display, "/dev/fb0");
    if (status)
        return (void *)status;

    while (1)
    {
        display_clear_buffer(&display);

        draw_interface(&display, sensors);

        display_draw(&display);
        usleep(500 * 1000);
    }

    display_deinit(&display);
    return (void *)status;
}

static void draw_interface(struct display_handle *display, struct sensors_handle *sensors)
{
    int pressure, temperature, humidity;
    char sensor_data_str[SENSOR_DATA_STR_LEN];

    memset(sensor_data_str, 0, SENSOR_DATA_STR_LEN);
    sensors_get_data(sensors, &pressure, &temperature, &humidity);
    
    /* draw borders */
    display_draw_line(display, 0, 0, 0, DISPLAY_HEIGHT - DISPLAY_END_OFFSET, 1);
    display_draw_line(display, DISPLAY_WIDTH - 1, 0, DISPLAY_WIDTH - 1,
                      (DISPLAY_HEIGHT - DISPLAY_END_OFFSET), 1);
    display_draw_line(display, 0, 0, DISPLAY_WIDTH - 1, 0, 1);
    display_draw_line(display, 0, (DISPLAY_HEIGHT - DISPLAY_END_OFFSET), DISPLAY_WIDTH - 1,
                      (DISPLAY_HEIGHT - DISPLAY_END_OFFSET), 1);

    /* draw title */
    display_draw_string(display, DISPLAY_TITLE_X_POS, DISPLAY_TITLE_Y_POS, DISPLAY_TITLE);
    display_draw_line(display, DISPLAY_TITLE_X_POS - 5,
                      DISPLAY_TITLE_Y_POS + 9,
                      DISPLAY_TITLE_X_POS + ((sizeof(DISPLAY_TITLE) - 1) * CHARACTER_DIMENSION) + 5,
                      DISPLAY_TITLE_Y_POS + 9,
                      1);

    snprintf(sensor_data_str, SENSOR_DATA_STR_LEN, "Temp: %d C", temperature);
    display_draw_string(display, DISPLAY_X_PADDING, DISPLAY_READINGS_Y_OFFSET + 10, sensor_data_str);

    snprintf(sensor_data_str, SENSOR_DATA_STR_LEN, "Pres: %d hPa", pressure);
    display_draw_string(display, DISPLAY_X_PADDING, DISPLAY_READINGS_Y_OFFSET + 21, sensor_data_str);

    snprintf(sensor_data_str, SENSOR_DATA_STR_LEN, "Hum: %d %%", humidity);
    display_draw_string(display, DISPLAY_X_PADDING, DISPLAY_READINGS_Y_OFFSET + 32, sensor_data_str);
}