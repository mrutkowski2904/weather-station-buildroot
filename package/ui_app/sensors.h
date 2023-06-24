#ifndef _SENSORS_H
#define _SENSORS_H

#include <pthread.h>

struct sensors_handle
{
    int pressure;
    int temperature;
    int humidity;
    pthread_mutex_t lock;
};

void *sensors_thread_entry(void *data);
int sensors_get_data(struct sensors_handle *handle, int *pressure, int *temperature, int *humidity);

#endif /* _SENSORS_H */