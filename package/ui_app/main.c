#include <stdio.h>
#include <pthread.h>

#include "display.h"

int main(void)
{
    int status;
    pthread_t display_thread;

    status = pthread_create(&display_thread, NULL, display_thread_entry, NULL);
    if (status)
        return status;

    pthread_join(display_thread, NULL);

    return 0;
}