#include "../models.h"
#include "../sim.h"
#include "../common.h"
#include "../structs.h"

#include "tanh_approx.h"

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
    sim_param->nb_in = 5;
    sim_param->nb_states = 12;
    sim_param->init = &init_hco;
    sim_param->system = &system_hco;
    sim_param->communicate = &communicate_hco_sim_side;
}

void init_hco(double *input, double *state, double *output)
{
    input[0] = 0;
    input[1] = 0;
    input[2] = -1.1; // Iapp Base
    input[3] = 6000.0; // gsyn
    input[4] = 0.0; // dsyn

    state[0] = 0;
    state[1] = 0;
    state[2] = 0;
    state[3] = 0;
    state[4] = -0.5; // Assymetric initialisation of HCO
    state[5] = -0.5;
    state[6] = -0.5;
    state[7] = -0.5;
    state[8] = 0;
    state[9] = 0;
    state[10] = 0;
    state[11] = 0;

    output[0] = 0;
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

void system_hco(double *input, double *state, double *dstate, double *output)
{
    // Named Constants
    double gfm = -2.0;
    double gsp = 6.0;
    double gsm = -5.0;
    double gup = 5.0;
    double dfm = 0.0;
    double dsp = 0.5;
    double dsm = -0.5;
    double dup = -0.5;
    double V0 = -0.85;
    double tau_V_inv = 1000.0;
    double tau_vf_inv = 1000.0;
    double tau_vs_inv = 25.0;
    double tau_vu_inv = 2.5;
    double tau_vsyn_out_inv = 25.0;
    double gsyn = -1.0;
    double dsyn = 0.0;
    double tau_vsyn_inv = 25.0;

    // Input reading
    double feed1 = input[0];
    double feed2 = input[1];
    double Iapp = input[2];
    double gsyn_out = input[3];
    double dsyn_out = input[4];

    // State reading
    double synapse_out_1_s_1 = state[10];
    double synapse_out_2_s_1 = state[11];
    double synapse1_s_1 = state[8];
    double neuron2_s_1 = state[4];
    double neuron2_s_2 = state[5];
    double neuron2_s_3 = state[6];
    double neuron2_s_4 = state[7];
    double neuron1_s_1 = state[0];
    double neuron1_s_2 = state[1];
    double neuron1_s_3 = state[2];
    double neuron1_s_4 = state[3];
    double synapse2_s_1 = state[9];

    // Output Computing
    double synapse_out_1_i_1 = (gsyn_out * ((fast_tanh(((synapse_out_1_s_1 - dsyn_out) * 2.0)) + 1.0) * 0.5));
    double synapse_out_2_i_1 = (gsyn_out * ((fast_tanh(((synapse_out_2_s_1 - dsyn_out) * 2.0)) + 1.0) * 0.5));
    double synapse1_i_1 = (gsyn * ((fast_tanh(((synapse1_s_1 - dsyn) * 2.0)) + 1.0) * 0.5));
    double neuron2_i_1 = (gfm * (fast_tanh((neuron2_s_2 - dfm)) - fast_tanh((V0 - dfm))));
    double neuron2_i_2 = (gsp * (fast_tanh((neuron2_s_3 - dsp)) - fast_tanh((V0 - dsp))));
    double neuron2_i_3 = (gsm * (fast_tanh((neuron2_s_3 - dsm)) - fast_tanh((V0 - dsm))));
    double neuron2_i_4 = (gup * (fast_tanh((neuron2_s_4 - dup)) - fast_tanh((V0 - dup))));
    double neuron1_i_1 = (gfm * (fast_tanh((neuron1_s_2 - dfm)) - fast_tanh((V0 - dfm))));
    double neuron1_i_2 = (gsp * (fast_tanh((neuron1_s_3 - dsp)) - fast_tanh((V0 - dsp))));
    double neuron1_i_3 = (gsm * (fast_tanh((neuron1_s_3 - dsm)) - fast_tanh((V0 - dsm))));
    double neuron1_i_4 = (gup * (fast_tanh((neuron1_s_4 - dup)) - fast_tanh((V0 - dup))));
    double adder_i_1 = (Iapp + (synapse1_i_1 + feed1));
    double adder_i_3 = gsyn_out * (((fast_tanh(((neuron1_s_1 - dsyn_out) * 2.0)) + 1.0) * 0.5) - (((fast_tanh(((neuron2_s_1 - dsyn_out) * 2.0)) + 1.0) * 0.5)));
    double synapse2_i_1 = (gsyn * ((fast_tanh(((synapse2_s_1 - dsyn) * 2.0)) + 1.0) * 0.5));
    double adder_i_2 = (Iapp + (synapse2_i_1 + feed2));

    // Derivative Computing
    dstate[10] = (tau_vsyn_out_inv * (neuron1_s_1 - synapse_out_1_s_1));
    dstate[11] = (tau_vsyn_out_inv * (neuron2_s_1 - synapse_out_2_s_1));
    dstate[8] = (tau_vsyn_inv * (neuron1_s_1 - synapse1_s_1));
    dstate[4] = (tau_V_inv * ((((((V0 + adder_i_1) - neuron2_i_1) - neuron2_i_2) - neuron2_i_3) - neuron2_i_4) - neuron2_s_1));
    dstate[5] = (tau_vf_inv * (neuron2_s_1 - neuron2_s_2));
    dstate[7] = (tau_vu_inv * (neuron2_s_1 - neuron2_s_4));
    dstate[6] = (tau_vs_inv * (neuron2_s_1 - neuron2_s_3));
    dstate[1] = (tau_vf_inv * (neuron1_s_1 - neuron1_s_2));
    dstate[0] = (tau_V_inv * ((((((V0 + adder_i_2) - neuron1_i_1) - neuron1_i_2) - neuron1_i_3) - neuron1_i_4) - neuron1_s_1));
    dstate[2] = (tau_vs_inv * (neuron1_s_1 - neuron1_s_3));
    dstate[3] = (tau_vu_inv * (neuron1_s_1 - neuron1_s_4));
    dstate[9] = (tau_vsyn_inv * (neuron2_s_1 - synapse2_s_1));

    // Output Setting
    output[0] = adder_i_3;
}
