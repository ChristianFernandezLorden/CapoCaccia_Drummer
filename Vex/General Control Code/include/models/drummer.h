#pragma once

// Required defines (for main)

#define BASE_FILENAME "drummer_test"

#define NB_CHANNEL_SYSTEM 5
#define NB_CHANNEL_SENSORS 9

#define SIM_MICRO_STEP 100
#define SENSORS_MILLI_STEP 5

#define SYSTEM_RECORD_PERIOD 20
#define WRITING_PERIOD 2000

// Optional defines (for drummer.c)

#define HEADER_FILENAME BASE_FILENAME ".txt"

#define FREQ 1.0

// Function declarations

void dyn_system_drummer(double *input, double *state, double *dstate, double *output);
void drummer_init();
void drummer_create_header_file();
double drummer_system_update();
void drummer_system_record(int count);
void drummer_setup(int *nb_state, int *nb_input, int *nb_output,
                             double **input, double **state, double **state2, double **dstate, double **dstate2, double **output);
void drummer_sim_com(double *input, double *output);
void drummer_sim_record(double *input, double *state, double *dstate, double *output, int count);

// Rename functions for use in main

#define init_module() drummer_init()
#define create_header_file() drummer_create_header_file()
#define system_update() drummer_system_update()
#define system_record(count) drummer_system_record(count)

#define sim_setup(nb_state, nb_input, nb_output, input, state, state2, dstate, dstate2, output) \
        drummer_setup(&nb_state, &nb_input, &nb_output, &input, &state, &state2, &dstate, &dstate2, &output)
#define sim_com(input, output) drummer_sim_com(input, output)
#define sim_record(input, state, dstate, output, count) drummer_sim_record(input, state, dstate, output, count)
#define dyn_system(input, state, dstate, output) dyn_system_drummer(input, state, dstate, output)