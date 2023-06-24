#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#include "display.h"
#include "sensors.h"
#include "led.h"

volatile sig_atomic_t keep_running = true;

static void int_handler(int sig);

int main(void)
{
    int status;
    pthread_t sensors_thread, display_thread, led_thread;
    struct sensors_handle sensors;

    signal(SIGINT, int_handler);
    memset(&sensors, 0, sizeof(struct sensors_handle));
    status = pthread_mutex_init(&sensors.lock, NULL);
    if (status)
        return status;

    status = pthread_create(&sensors_thread, NULL, sensors_thread_entry, &sensors);
    if (status)
        return status;

    status = pthread_create(&display_thread, NULL, display_thread_entry, &sensors);
    if (status)
        return status;
    
    status = pthread_create(&led_thread, NULL, led_thread_entry, &sensors);
    if (status)
        return status;

    pthread_join(sensors_thread, NULL);
    pthread_join(display_thread, NULL);
    pthread_join(led_thread, NULL);

    return 0;
}

static void int_handler(int sig)
{
    (void)sig;
    keep_running = false;
}