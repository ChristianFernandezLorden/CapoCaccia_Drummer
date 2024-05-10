#include "common.h"

#define _POSIX_C_SOURCE 199309L

#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "sim.h"

/// Convert seconds to milliseconds
#define SEC_TO_MS(sec) ((sec) * 1000)
/// Convert seconds to microseconds
#define SEC_TO_US(sec) ((sec) * 1000000)
/// Convert seconds to nanoseconds
#define SEC_TO_NS(sec) ((sec) * 1000000000)

/// Convert nanoseconds to seconds
#define NS_TO_SEC(ns) ((ns) / 1000000000)
/// Convert nanoseconds to milliseconds
#define NS_TO_MS(ns) ((ns) / 1000000)
/// Convert nanoseconds to microseconds
#define NS_TO_US(ns) ((ns) / 1000)

#define MICRO_STEP 50
#define TRANSMIT_RATE 5

void *simulate(void *param)
{
    //sim_param_t *sim_param = (sim_param_t *) param;

    double peak = 0;
    double voltage = 0;
    int transmit_counter = 0;

    // pthread_barrier_wait(&init_barrier);

    struct timespec ts;
#ifdef WINDOWS_OS
    clock_gettime(CLOCK_MONOTONIC, &ts);
#else
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#endif
    time_t start_us = SEC_TO_US(ts.tv_sec) + NS_TO_US(ts.tv_nsec);
    time_t end_us, us_left, current_us;
    end_us = start_us;

    while (1)
    {
        end_us = end_us + MICRO_STEP;
        transmit_counter = (transmit_counter + 1) % TRANSMIT_RATE;
        if (transmit_counter == 0)
        {
            pthread_mutex_lock(&mutex);
            if (sim_param.has_new_data[0] == 1)
            {
                sim_param.has_new_data[0] = 0;
                peak = sim_param.in[0];
            }
            //voltage = peak * 0.5;
            sim_param.out[0] = voltage;
            sim_param.has_new_data[1] = 1;
            pthread_mutex_unlock(&mutex);
        }
        

        //start_us = end_us;
        struct timespec ts;

    
#ifdef WINDOWS_OS
        clock_gettime(CLOCK_MONOTONIC, &ts);
#else
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#endif
        current_us = SEC_TO_US(ts.tv_sec) + NS_TO_US(ts.tv_nsec);
        us_left = end_us - current_us;
        voltage = us_left;
        if (us_left > 1000)
        {
            usleep(us_left);
        }
    }
    pthread_exit(NULL); 
    return NULL;
}