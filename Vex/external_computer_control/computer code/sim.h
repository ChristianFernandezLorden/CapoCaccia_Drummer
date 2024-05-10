#pragma once
#include <pthread.h>

typedef struct sim_param_t {
    pthread_mutex_t* com_mutex;
    // pthread_barrier_t* init_barrier;
    int nb_out, nb_in;
    double* out, *in;
    char* has_new_data;
} sim_param_t;


void *simulate(void* param);