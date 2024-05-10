#include "../models.h"
#include "../sim.h"
#include "../common.h"
#include "../structs.h"

#include <stdlib.h>

void init_sim_com_hco(com_data_t *com_data);
void init_sim_param_hco(sim_param_t *sim_param);
double communicate_hco_vex_side(com_data_t *com_data, double voltage);
void init_hco(double *input, double *states, double *output);
void system_hco(double *input, double *states, double *dstates, double *output);
void communicate_hco_sim_side(double *input, double *output, com_data_t *com_data);

sys_specific_functions_t unconnected_hco()
{
    sys_specific_functions_t sys_specific_functions;
    sys_specific_functions.init_sim_com = &init_sim_com_hco;
    sys_specific_functions.init_sim_param = &init_sim_param_hco;
    sys_specific_functions.communicate = &communicate_hco_vex_side;
    return sys_specific_functions;
}

void init_sim_com_hco(com_data_t *com_data)
{
    com_data->nb_out = 1;
    com_data->nb_in = 1;
    com_data->out = (double*) malloc(com_data->nb_out * sizeof(double));
    com_data->in = (double*) malloc(com_data->nb_in * sizeof(double));
    com_data->has_new_data = (char*) malloc(2* sizeof(char));
    for (int i = 0; i < com_data->nb_out; i++) {
        com_data->out[i] = 0;
    }
    com_data->has_new_data[0] = 0;
    com_data->has_new_data[1] = 0;
}

void init_sim_param_hco(sim_param_t *sim_param)
{
    sim_param->nb_out = 1;
    sim_param->nb_in = 1;
    sim_param->nb_states = 1;
    sim_param->init = &init_hco;
    sim_param->system = &system_hco;
    sim_param->communicate = &communicate_hco_sim_side;
}

void init_hco(double *input, double *states, double *output)
{
    input[0] = 0;
    states[0] = 100;
    output[0] = 0;
}

void system_hco(double *input, double *states, double *dstates, double *output)
{
    dstates[0] = 1*(-states[0] + input[0]);
    output[0] = states[0];
}

void communicate_hco_sim_side(double *input, double *output, com_data_t *com_data)
{
    com_data->in[0] = output[0];
    com_data->has_new_data[0] = 1;
    if (com_data->has_new_data[1] == 1) {
        input[0] = com_data->out[0];
        com_data->has_new_data[1] = 0;
    }
}

double communicate_hco_vex_side(com_data_t *com_data, double voltage)
{
    if (com_data->has_new_data[0] == 1) {
        voltage = com_data->in[0];
        com_data->has_new_data[0] = 0;
        com_data->has_new_data[1] = 1;
        if (voltage < 20) {
            com_data->out[0] = 50;
        } else {
            com_data->out[0] = 0;
        }
    }
    return voltage;
}

