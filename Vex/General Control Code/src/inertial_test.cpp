#include <string.h>

#include "fast_tanh.h"
#include "inertial_test.h"
#include "vex.h"

static double accx_read;

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

void dyn_system_inertial_test(double *input, double *state, double *dstate, double *output)
{
	double val = 0;
	for (int i = 0; i < 10; i++) {
		val = fast_tanh(val +  ((val < 0) ? -1 : 1) * input[0]);
	}
}


void inertial_test_init() {
	return;
}

void inertial_test_create_header_file()
{
	char text[10000];

	int nb_char = sprintf(text,
						  "Param :"
						  "None"
						  "\nSystem channels : %d"
						  "\nSensor channels : %d",
						  NB_CHANNEL_SYSTEM,
						  NB_CHANNEL_SENSORS);

	Brain.SDcard.savefile(HEADER_FILENAME,
						  (uint8_t *)text,
						  nb_char * sizeof(char));
}

double inertial_test_system_update()
{
	double accx_read_tmp = CollisionSensor.acceleration(xaxis);

	lock.lock();
	accx_read = accx_read_tmp;
	lock.unlock();

	return 0.0;
}

void inertial_test_system_record(int count)
{
	double accx = CollisionSensor.acceleration(xaxis);
	double accy = CollisionSensor.acceleration(yaxis);
	double accz = CollisionSensor.acceleration(zaxis);
	double time = ((double)Brain.Timer.systemHighResolution()) * 1e-6;

	int offset = NB_CHANNEL_SENSORS * count;

	lock.lock();
	sensors_data[offset + 0] = time;
	sensors_data[offset + 1] = accx;
	sensors_data[offset + 2] = accy;
	sensors_data[offset + 3] = accz;
	lock.unlock();
}

void inertial_test_setup(int *nb_state, int *nb_input, int *nb_output,
					  double **input, double **state, double **state2, double **dstate, double **dstate2, double **output) 
{
	*nb_state = 0;
	*nb_input = 1;
	*nb_output = 0;

	(*input) = (double*) malloc(sizeof(double));
}

void inertial_test_sim_com(double *input, double *output) 
{
	lock.lock();
	input[0] = accx_read;
	lock.unlock();
	return;
}

void inertial_test_sim_record(double *input, double *state, double *dstate, double *output, int count)
{
	lock.lock();
	lock.unlock();
	return;
}
