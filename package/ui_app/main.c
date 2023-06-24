#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "display.h"
#include "sensors.h"

int main(void)
{
    int status;
    pthread_t sensors_thread, display_thread;
    struct sensors_handle sensors;

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

    pthread_join(sensors_thread, NULL);
    pthread_join(display_thread, NULL);

    return 0;
}