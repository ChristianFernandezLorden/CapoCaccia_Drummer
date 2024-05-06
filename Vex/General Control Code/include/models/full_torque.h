#pragma once

// Required defines (for main)

#define BASE_FILENAME "module_test/wta"

#define NB_CHANNEL_SYSTEM 5
#define NB_CHANNEL_SENSORS 8

#define SIM_MICRO_STEP 100
#define SENSORS_MILLI_STEP 5

#define SYSTEM_RECORD_PERIOD 20
#define WRITING_PERIOD 2000

// Optional defines (for full_torque.c)

#define HEADER_FILENAME BASE_FILENAME ".txt"

#define IAPP -2.0
#define GSM -5.0
#define GUP 5.0

#define SMOOTHING_PERIOD 9

// Function declarations

void dyn_system_full_torque(double *input, double *state, double *dstate, double *output);
void full_torque_init();
void full_torque_create_header_file();
double full_torque_system_update();
void full_torque_system_record(int count);
void full_torque_setup(int *nb_state, int *nb_input, int *nb_output,
                             double **input, double **state, double **state2, double **dstate, double **dstate2, double **output);
void full_torque_sim_com(double *input, double *output);
void full_torque_sim_record(double *input, double *state, double *dstate, double *output, int count);

// Rename functions for use in main

#define init_module() full_torque_init()
#define create_header_file() full_torque_create_header_file()
#define system_update() full_torque_system_update()
#define system_record(count) full_torque_system_record(count)

#define sim_setup(nb_state, nb_input, nb_output, input, state, state2, dstate, dstate2, output) \
        full_torque_setup(&nb_state, &nb_input, &nb_output, &input, &state, &state2, &dstate, &dstate2, &output)
#define sim_com(input, output) full_torque_sim_com(input, output)
#define sim_record(input, state, dstate, output, count) full_torque_sim_record(input, state, dstate, output, count)
#define dyn_system(input, state, dstate, output) dyn_system_full_torque(input, state, dstate, output)