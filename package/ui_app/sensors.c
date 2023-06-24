#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "sensors.h"

#define RAW_DATA_BUFFER_SIZE 10

#define DHT11_TEMPERATURE "/sys/bus/iio/devices/iio:device0/in_temp_input"
#define DHT11_HUMIDITY "/sys/bus/iio/devices/iio:device0/in_humidityrelative_input"
#define LPS25HB_PRESSURE "/dev/barometer0"

static int dht11_read_temperature(int *read_temperature);
static int dht11_read_humidity(int *read_humidity);
static int lps25hb_read_pressure(int *read_pressure);
static int driver_read(const char *file, int *reading_out);

void *sensors_thread_entry(void *data)
{
    int current_temperature, current_humidity, current_pressure;
    struct sensors_handle *sensors = data;

    current_temperature = 0;
    current_humidity = 0;
    current_pressure = 0;

    while (1)
    {
        dht11_read_temperature(&current_temperature);
        dht11_read_humidity(&current_humidity);
        lps25hb_read_pressure(&current_pressure);

        if (pthread_mutex_lock(&sensors->lock) == 0)
        {
            sensors->humidity = current_humidity;
            sensors->pressure = current_pressure;
            sensors->temperature = current_temperature;
            pthread_mutex_unlock(&sensors->lock);
        }

        usleep(1000 * 1000);
    }
}

int sensors_get_data(struct sensors_handle *sensors, int *pressure, int *temperature, int *humidity)
{
    int status;

    status = pthread_mutex_lock(&sensors->lock);
    if (status)
        return status;

    if (pressure != NULL)
        *pressure = sensors->pressure;

    if (temperature != NULL)
        *temperature = sensors->temperature;

    if (humidity != NULL)
        *humidity = sensors->humidity;

    pthread_mutex_unlock(&sensors->lock);
    return 0;
}

static int dht11_read_temperature(int *read_temperature)
{
    int status, tmp_temperature;
    status = driver_read(DHT11_TEMPERATURE, &tmp_temperature);
    if (status)
        return status;

    /* discard the decimal part */
    *read_temperature = tmp_temperature / 1000;
    return 0;
}

static int dht11_read_humidity(int *read_humidity)
{
    int status, tmp_humidity;
    status = driver_read(DHT11_HUMIDITY, &tmp_humidity);
    if (status)
        return status;

    *read_humidity = tmp_humidity / 1000;
    return 0;
}

static int lps25hb_read_pressure(int *read_pressure)
{
    return driver_read(LPS25HB_PRESSURE, read_pressure);
}

static int driver_read(const char *file, int *reading_out)
{
    int fd;
    ssize_t read_count;
    char raw_data[RAW_DATA_BUFFER_SIZE];
    int tmp_reading;

    memset(raw_data, 0, RAW_DATA_BUFFER_SIZE);
    fd = open(file, O_RDONLY);
    if (fd < 0)
        return -EIO;

    read_count = read(fd, raw_data, RAW_DATA_BUFFER_SIZE - 1);
    close(fd);
    if (read_count < 0)
        return -EIO;

    errno = 0;
    tmp_reading = (int)strtol(raw_data, NULL, 10);
    if (errno)
        return -EINVAL;

    *reading_out = tmp_reading;
    return 0;
}
