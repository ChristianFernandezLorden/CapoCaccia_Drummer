#pragma once

#include <pthread.h>

typedef struct com_data_t{
    int nb_out, nb_in;
    double *out, *in;
    char *has_new_data;
    pthread_mutex_t *com_mutex;
} com_data_t;

typedef struct sim_param_t{
    com_data_t* sim_com_data;
    int nb_out, nb_in, nb_states;
    void (*init)(double *, double *, double *);
    void (*system)(double *, double *, double *, double *);
    void (*communicate)(double *, double *, com_data_t *);
} sim_param_t;

typedef struct sys_specific_functions_t {
    void (*init_sim_com)(com_data_t *);
    void (*init_sim_param)(sim_param_t *);
    double (*communicate)(com_data_t*, double);
} sys_specific_functions_t;