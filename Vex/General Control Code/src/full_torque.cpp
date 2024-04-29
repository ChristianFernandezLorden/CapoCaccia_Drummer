#include <string.h>

#include "fast_tanh.h"
#include "full_torque.h"
#include "vex.h"

static double voltage = 0;
static double angle = 0;
static double speed = 0;
static double feed1_com = 0;
static double feed2_com = 0;

static double smoothing_filter[SMOOTHING_PERIOD];
static double filter_sum = 0;
static int filter_ind = 0;

using namespace vex;

/* 
 * INPUTS
 * ======
 * input : double vector of length 5 
 * state : double vector of length 12
 * 
 * OUTPUTS
 * =======
 * dstate : double vector of length 12
 * output : double vector of length 1
 */

void dyn_system_full_torque(double *input, double *state, double *dstate, double *output)
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


void full_torque_init() {
	for (int i = 0; i < SMOOTHING_PERIOD; i++)
	{
		smoothing_filter[i] = 0;
	}
}

void full_torque_create_header_file()
{
	char text[10000];

	int nb_char = sprintf(text,
						  "Param :"
						  "\nIapp = %lf\ngsm = %lf\ngup=%lf"
						  "\nSystem channels : %d"
						  "\nSensor channels : %d",
						  IAPP, GSM, GUP,
						  NB_CHANNEL_SYSTEM,
						  NB_CHANNEL_SENSORS);

	Brain.SDcard.savefile(HEADER_FILENAME,
						  (uint8_t *)text,
						  nb_char * sizeof(char));
}

double full_torque_system_update()
{
	angle = (SpeedSensor.position(deg) * M_PI / 180.0);
	double tmp_speed = SpeedSensor.velocity(rpm) * M_PI / 30.0;

	filter_sum -= smoothing_filter[filter_ind];
	smoothing_filter[filter_ind] = tmp_speed;
	filter_sum += tmp_speed;

	filter_ind = (filter_ind + 1) % SMOOTHING_PERIOD;

	speed = filter_sum / SMOOTHING_PERIOD;

	double tmp_feed1_com = 0;
	double tmp_feed2_com = 0;
	if (speed < 0.1 && speed > -0.1)
	{
		if (angle > 0.1) {
			tmp_feed1_com = 1;
		} else if (angle < 0.1) {
			tmp_feed2_com = 1;
		}
	}

	lock.lock();
	double tmp_voltage = voltage;
	feed1_com = tmp_feed1_com;
	feed2_com = tmp_feed2_com;
	lock.unlock();

	return 24000;
}

void full_torque_system_record(int count)
{
	double torque = PendulumMotor.torque(Nm);
	double voltage_mot = PendulumMotor.voltage(volt);
	double current = PendulumMotor.current(amp);
	double power = PendulumMotor.power(watt);
	double time = ((double)Brain.Timer.systemHighResolution()) * 1e-6;

	int offset = NB_CHANNEL_SENSORS * count;

	lock.lock();
	sensors_data[offset + 0] = time;
	sensors_data[offset + 1] = angle;
	sensors_data[offset + 2] = speed;
	sensors_data[offset + 3] = torque;
	sensors_data[offset + 4] = voltage_mot;
	sensors_data[offset + 5] = current;
	sensors_data[offset + 6] = power;
	sensors_data[offset + 7] = voltage;
	lock.unlock();
}

void full_torque_setup(int *nb_state, int *nb_input, int *nb_output,
					  double **input, double **state, double **state2, double **dstate, double **dstate2, double **output) 
{
	*nb_state = 12;
	*nb_input = 5;
	*nb_output = 1;

	*input = (double *)malloc(*nb_input * sizeof(double));
	*state = (double *)malloc(*nb_state * sizeof(double));
	*state2 = (double *)malloc(*nb_state * sizeof(double));
	*dstate = (double *)malloc(*nb_state * sizeof(double));
	*dstate2 = (double *)malloc(*nb_state * sizeof(double));
	*output = (double *)malloc(*nb_output * sizeof(double));

	(*state)[0] = 0;
	(*state)[1] = 0;
	(*state)[2] = 0;
	(*state)[3] = 0;
	(*state)[4] = -0.5;
	(*state)[5] = -0.5;
	(*state)[6] = -0.5;
	(*state)[7] = -0.5;
	(*state)[8] = 0;
	(*state)[9] = 0;
	(*state)[10] = 0;
	(*state)[11] = 0;

	(*input)[0] = 0; // Iapp
	(*input)[1] = 0; // Iapp

	(*input)[2] = -2; // Iapp

	(*input)[3] = 6000.0; // gsyn
	(*input)[4] = 0.0;	  // dsyn
}

void full_torque_sim_com(double *input, double *output) 
{
	lock.lock();

	input[0] = feed2_com;
	input[1] = feed1_com;

	voltage = output[0];

	lock.unlock();
}

void full_torque_sim_record(double *input, double *state, double *dstate, double *output, int count)
{
	int offset = NB_CHANNEL_SYSTEM * count;

	lock.lock();
	system_data[offset] = ((double)Brain.Timer.systemHighResolution()) * 1e-6; // Time
	system_data[offset + 1] = output[0];									   // out 1
	system_data[offset + 2] = output[1];									   // out 2
	system_data[offset + 3] = input[0];										   // in 1
	system_data[offset + 4] = input[1];	
	lock.unlock();
}
