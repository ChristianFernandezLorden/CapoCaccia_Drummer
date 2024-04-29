#pragma once

// Required defines (for main)

#define BASE_FILENAME "inertial_test"

#define NB_CHANNEL_SYSTEM 0
#define NB_CHANNEL_SENSORS 4

#define SIM_MICRO_STEP 100
#define SENSORS_MILLI_STEP 5

#define SYSTEM_RECORD_PERIOD 20
#define WRITING_PERIOD 2000

// Optional defines (for inertial_test.c)

#define HEADER_FILENAME BASE_FILENAME ".txt"

// Function declarations

void dyn_system_inertial_test(double *input, double *state, double *dstate, double *output);
void inertial_test_init();
void inertial_test_create_header_file();
double inertial_test_system_update();
void inertial_test_system_record(int count);
void inertial_test_setup(int *nb_state, int *nb_input, int *nb_output,
                             double **input, double **state, double **state2, double **dstate, double **dstate2, double **output);
void inertial_test_sim_com(double *input, double *output);
void inertial_test_sim_record(double *input, double *state, double *dstate, double *output, int count);

// Rename functions for use in main

#define init_module() inertial_test_init()
#define create_header_file() inertial_test_create_header_file()
#define system_update() inertial_test_system_update()
#define system_record(count) inertial_test_system_record(count)

#define sim_setup(nb_state, nb_input, nb_output, input, state, state2, dstate, dstate2, output) \
        inertial_test_setup(&nb_state, &nb_input, &nb_output, &input, &state, &state2, &dstate, &dstate2, &output)
#define sim_com(input, output) inertial_test_sim_com(input, output)
#define sim_record(input, state, dstate, output, count) inertial_test_sim_record(input, state, dstate, output, count)
#define dyn_system(input, state, dstate, output) dyn_system_inertial_test(input, state, dstate, output)