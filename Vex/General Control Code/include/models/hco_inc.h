#pragma once

// Required defines (for main)

#define BASE_FILENAME "module_test/wta"

#define NB_CHANNEL_SYSTEM 5
#define NB_CHANNEL_SENSORS 8

#define SIM_MICRO_STEP 100
#define SENSORS_MILLI_STEP 5

#define SYSTEM_RECORD_PERIOD 20
#define WRITING_PERIOD 2000

// Optional defines (for hco.c)

#define HEADER_FILENAME BASE_FILENAME ".txt"

#define IAPP -2.0
#define GSM -5.0
#define GUP 5.0

#define SMOOTHING_PERIOD 9

#include "hco.inc.h"

// Rename functions for use in main

#define init_module() hco_init()
#define create_header_file() hco_create_header_file()
#define system_update() hco_system_update()
#define system_record(count) hco_system_record(count)

#define sim_setup(nb_state, nb_input, nb_output, input, state, state2, dstate, dstate2, output) \
        hco_setup(nb_state, nb_input, nb_output, input, state, state2, dstate, dstate2, output)
#define sim_com(input, output) hco_sim_com(input, output)
#define sim_record(input, state, dstate, output, count) hco_sim_record(input, state, dstate, output, count)
#define dyn_system(input, state, dstate, output) dyn_system_hco(input, state, dstate, output)