#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <gpiod.h>

#include "led.h"
#include "sensors.h"
#include "main.h"

#define LED_GPIOCHIP "gpiochip4"
#define LED_OFFSET_BLUE 0
#define LED_OFFSET_GREEN 1
#define LED_OFFSET_RED 2

#define TEMPERATURE_OPTIMAL_MAX 25
#define TEMPERATURE_OPTIMAL_MIN 20

void *led_thread_entry(void *data)
{
    int temperature;
    struct sensors_handle *sensors = data;
    struct gpiod_chip *chip;
    struct gpiod_line *line_blue, *line_green, *line_red;

    chip = gpiod_chip_open_by_name(LED_GPIOCHIP);
    if (!chip)
        goto chip_get_fail;

    line_blue = gpiod_chip_get_line(chip, LED_OFFSET_BLUE);
    if (!line_blue)
        goto line_blue_fail;

    line_green = gpiod_chip_get_line(chip, LED_OFFSET_GREEN);
    if (!line_green)
        goto line_green_fail;

    line_red = gpiod_chip_get_line(chip, LED_OFFSET_RED);
    if (!line_red)
        goto line_red_fail;

    gpiod_line_request_output(line_blue, "blue", 0);
    gpiod_line_request_output(line_green, "green", 0);
    gpiod_line_request_output(line_red, "red", 0);

    while (keep_running)
    {
        sensors_get_data(sensors, NULL, &temperature, NULL);

        if (temperature < TEMPERATURE_OPTIMAL_MIN)
        {
            gpiod_line_set_value(line_blue, 1);
            gpiod_line_set_value(line_green, 0);
            gpiod_line_set_value(line_red, 0);
        }
        else if (temperature > TEMPERATURE_OPTIMAL_MAX)
        {
            gpiod_line_set_value(line_blue, 0);
            gpiod_line_set_value(line_green, 0);
            gpiod_line_set_value(line_red, 1);
        }
        else
        {
            gpiod_line_set_value(line_blue, 0);
            gpiod_line_set_value(line_green, 1);
            gpiod_line_set_value(line_red, 0);
        }
        usleep(750 * 1000);
    }

    gpiod_line_release(line_red);
line_red_fail:
    gpiod_line_release(line_green);
line_green_fail:
    gpiod_line_release(line_blue);
line_blue_fail:
    gpiod_chip_close(chip);
chip_get_fail:
}