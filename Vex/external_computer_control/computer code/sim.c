#include "common.h"

#define _POSIX_C_SOURCE 199309L

#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "sim.h"
#include "structs.h"

#include <stdio.h>

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
#define MIN_MICRO_SLEEP_TIME 1000
#define TRANSMIT_RATE 10

void *simulate(void *param)
{
    sim_param_t *sim_param = (sim_param_t *) param;

    double *input = malloc(sim_param->nb_in * sizeof(double));
    double **states = malloc(4 * sizeof(double *));
    double **dstates = malloc(4 * sizeof(double *));
    for (int i = 0; i < 4; i++) {
        states[i] = malloc(sim_param->nb_states * sizeof(double));
        dstates[i] = malloc(sim_param->nb_states * sizeof(double));
    }
    double *output = malloc(sim_param->nb_out * sizeof(double));

    sim_param->init(input, states[0], output);

    double sim_step = (double)MICRO_STEP / 1000000.0;
    double half_sim_step = sim_step * 0.5;
    double sixth_sim_step = sim_step / 6.0;
    double third_sim_step = sim_step / 3.0;
    double eigth_sim_step = sim_step / 8.0;

    int transmit_counter = 0;

    //pthread_barrier_wait(&init_barrier);

    struct timespec ts;
#ifdef WINDOWS_OS
    clock_gettime(CLOCK_MONOTONIC, &ts);
#else
    clock_gettime(_CLOCK_MONOTONIC, &ts);
#endif
    time_t end_us = SEC_TO_US(ts.tv_sec) + NS_TO_US(ts.tv_nsec);
    time_t us_left, current_us;

    int i = 0;
    while (1)
    {
        end_us = end_us + MICRO_STEP;
        transmit_counter = (transmit_counter + 1) % TRANSMIT_RATE;
        if (transmit_counter == 0)
        {
            pthread_mutex_lock(sim_param->sim_com_data->com_mutex);
            sim_param->communicate(input, output, sim_param->sim_com_data);
            /*
            if (sim_com_data.has_new_data[0] == 1)
            {
                sim_com_data.has_new_data[0] = 0;
                peak = sim_com_data.in[0];
            }
            //voltage = peak * 0.5;
            sim_com_data.out[0] = voltage;
            sim_com_data.has_new_data[1] = 1;
            */
            pthread_mutex_unlock(sim_param->sim_com_data->com_mutex);
        }

        // Implement the 3/8 rule of the Runge-Kutta 4th order method

        sim_param->system(input, states[0], dstates[0], output);

        for (int i = 0; i < sim_param->nb_states; i++)
        {
            states[1][i] = states[0][i] + dstates[0][i] * third_sim_step;
        }

        sim_param->system(input, states[1], dstates[1], output);

        for (int i = 0; i < sim_param->nb_states; i++)
        {
            states[2][i] = states[0][i] + dstates[1][i] * sim_step - dstates[0][i] * third_sim_step;
        }

        sim_param->system(input, states[2], dstates[2], output);

        for (int i = 0; i < sim_param->nb_states; i++)
        {
            states[3][i] = states[0][i] + (dstates[0][i] - dstates[1][i] + dstates[2][i]) * sim_step;
        }

        sim_param->system(input, states[3], dstates[3], output);

        for (int i = 0; i < sim_param->nb_states; i++)
        {
            states[0][i] = states[0][i] + (dstates[0][i] + 3 * dstates[1][i] + 3 * dstates[2][i] + dstates[3][i]) * eigth_sim_step;
        }
        
        
        struct timespec ts; 
        do { // Busywait
#ifdef WINDOWS_OS
            clock_gettime(CLOCK_MONOTONIC, &ts);
#else
            clock_gettime(_CLOCK_MONOTONIC, &ts);
#endif
            current_us = SEC_TO_US(ts.tv_sec) + NS_TO_US(ts.tv_nsec);
            us_left = end_us - current_us;
        }while (us_left > MIN_MICRO_SLEEP_TIME);
        
    }

    free(input);
    for (int i = 0; i < 4; i++) {
        free(states[i]);
        free(dstates[i]);
    }
    free(states);   
    free(dstates);
    free(output);

    pthread_exit(NULL); 
    return NULL;
}