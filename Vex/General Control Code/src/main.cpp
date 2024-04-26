/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Module:       main.cpp                                                  */
/*    Author:       VEX                                                       */
/*    Created:      Wed Sep 25 2019                                           */
/*    Description:  Neuromorphic Pendulum                                     */
/*                                                                            */
/*    Name:                                                                   */
/*    Date                                                                    */
/*    Class:                                                                  */
/*                                                                            */
/*----------------------------------------------------------------------------*/

// ---- START VEXCODE CONFIGURED DEVICES ----
// Robot Configuration:
// [Name]               [Type]        [Port(s)]
// PendulumMotor        motor         1
// ---- END VEXCODE CONFIGURED DEVICES ----

// This include changes which test is done
#include "hco.h"


#include "vex.h"
#include "math.h"

using namespace vex;


#define SYSTEM_PERIOD ((WRITING_PERIOD * SENSORS_MILLI_STEP * 1000 / (SIM_MICRO_STEP * SYSTEM_RECORD_PERIOD)) + 1)

#define SYSTEM_DATA_LEN (3 * SYSTEM_PERIOD * NB_CHANNEL_SYSTEM)/2
#define SYSTEM_FILENAME BASE_FILENAME "_system.bin"

#define SENSORS_DATA_LEN (3 * WRITING_PERIOD * NB_CHANNEL_SENSORS)/2
#define SENSORS_FILENAME BASE_FILENAME "_sensors.bin"


int sim_thread();
int save_thread();



//double com;
static bool inserted = false;

static long long int misses;
static long long int done;

static int sensors_record_count;
static int system_record_count;

//int update_thread(); // Task that takes care of communication

int main()
{
  // Data vectors

  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("Initialization...");

  // Initializing Robot Configuration. DO NOT REMOVE!
  vexcodeInit();
  // Initialize anything related to current simulation
  init_module();

#if NB_CHANNEL_SENSORS > 0 || NB_CHANNEL_SYSTEM > 0
  // Create a text file detailing experiment
  create_header_file();

  if (Brain.SDcard.isInserted()) {
    inserted = true;
  #if NB_CHANNEL_SYSTEM > 0
    system_data = (double *)malloc(SYSTEM_DATA_LEN*sizeof(double));
    //system_data_backup = (double*) malloc(SYSTEM_DATA_LEN*sizeof(double));
  #endif
  #if NB_CHANNEL_SENSORS > 0
    sensors_data = (double*) malloc(SENSORS_DATA_LEN*sizeof(double));
    //sensors_data_backup = (double*) malloc(SENSORS_DATA_LEN*sizeof(double));
  #endif

    uint8_t bytes[3];

    // Check if little or big endian and write the result as the first byte 
    unsigned int i = 1;
    char *c = (char *)&i;
    if (*c) // Little endian
    {
      bytes[0] = 0;
    }
    else // Big endian
    {
      bytes[0] = 1;
    }
    bytes[1] = sizeof(double); // Write the size of a double on this machine
#if NB_CHANNEL_SYSTEM > 0
    bytes[2] = NB_CHANNEL_SYSTEM; // Number of values getting saved
    Brain.SDcard.savefile(SYSTEM_FILENAME, bytes, 3);
#endif
  #if NB_CHANNEL_SENSORS > 0
    bytes[2] = NB_CHANNEL_SENSORS; // Number of values getting saved
    Brain.SDcard.savefile(SENSORS_FILENAME, bytes, 3);
#endif
  } else {
    inserted = false;
  }
#endif

  misses = 0;
  done = 0;

  sensors_record_count = 0;
  system_record_count = 0;

  this_thread::setPriority(thread::threadPriorityNormal);

  Brain.Screen.clearLine(1);
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("Initialized");

  PendulumMotor.spin(fwd, 20000.0, voltageUnits::mV);
  wait(100.0, msec);

  thread simulation = thread(&sim_thread);
  simulation.setPriority(thread::threadPriorityHigh);

  // thread communication = thread(&update_thread);
  // communication.setPriority(thread::threadPriorityNormal);
  PendulumMotor.spin(fwd, 0.0, voltageUnits::mV);


  uint64_t end;
  uint64_t wait_time;
  uint64_t step = SENSORS_MILLI_STEP*1000;
  uint64_t start = Brain.Timer.systemHighResolution();

  int loop_count = 0;
  while (true)
  {
    start = Brain.Timer.systemHighResolution();
    end = start + step;

    double motor_voltage = system_update();

    PendulumMotor.spin(fwd, motor_voltage, voltageUnits::mV); // NEGATIVE VOLTAGES WORK

#if NB_CHANNEL_SENSORS > 0
    if (inserted) {
      system_record(sensors_record_count++);
    }
#endif

#if NB_CHANNEL_SENSORS > 0 || NB_CHANNEL_SYSTEM > 0
    loop_count++; 
    if (inserted && loop_count % WRITING_PERIOD == 0) {
      //thread(&save_thread).setPriority(thread::threadPrioritylow);
      loop_count = 0;
      simulation.interrupt();
      PendulumMotor.spin(fwd, 0.0, voltageUnits::mV); // NEGATIVE VOLTAGES WORK
      wait(20.0, msec);
      save_thread();
      wait(20.0, msec);
      Brain.Screen.clearLine(1);
      Brain.Screen.setCursor(1, 1);
      Brain.Screen.print("Saved !!!");
      Brain.Screen.setCursor(2, 1);
      Brain.Screen.print("gsm = %lf", GSM);
      Brain.Screen.setCursor(3, 1);
      Brain.Screen.print("gup = %lf", GUP);
      Brain.Screen.setCursor(4, 1);
      Brain.Screen.print("Iapp = %lf", IAPP);
      return 0;
    }
#endif

    uint64_t now = Brain.Timer.systemHighResolution();
    if (end > now){ // Sleep is necessary
      wait_time = ((double) (end - Brain.Timer.systemHighResolution())) / 1000.0;
      wait(wait_time, msec);
    }
    start = end;
  } 

  return 0;
}


int save_thread(){
  //Brain.Timer.event(&update_task, SENSORS_MILLI_STEP);

  double *tmp;
  lock.lock(); // Switch the data buffer to allow conccurent execution (if possible)
#if NB_CHANNEL_SYSTEM > 0
  tmp = system_data;
  system_data = system_data_backup;
  system_data_backup = tmp;
  int system_count_tmp = system_record_count;
  system_record_count = 0;
#endif

#if NB_CHANNEL_SENSORS > 0
  tmp = sensors_data;
  sensors_data = sensors_data_backup;
  sensors_data_backup = tmp;
  int sensors_count_tmp = sensors_record_count;
  sensors_record_count = 0;
#endif
  lock.unlock();

#if NB_CHANNEL_SYSTEM > 0
  Brain.SDcard.appendfile(SYSTEM_FILENAME, (uint8_t *)system_data_backup, system_count_tmp * sizeof(double) * NB_CHANNEL_SYSTEM);
#endif
#if NB_CHANNEL_SENSORS > 0
  Brain.SDcard.appendfile(SENSORS_FILENAME, (uint8_t *)sensors_data_backup, sensors_count_tmp * sizeof(double) * NB_CHANNEL_SENSORS);
#endif

  return 0;
}


int sim_thread()
{
  // Setup time related stuff
  double step = SIM_MICRO_STEP * 0.000001;

  double *input, *state, *state2, *dstate, *dstate2, *output;
  int nb_state, nb_input, nb_output;

  sim_setup(nb_state, nb_input, nb_output, input, state, state2, dstate, dstate2, output);

  uint64_t start = Brain.Timer.systemHighResolution();
  uint64_t end;
  int i = 0;
  while (true)
  {
    end = start + SIM_MICRO_STEP; // Time in micro seconds

    sim_com(input, output);

    dyn_system(input, state, dstate, output);

#if NB_CHANNEL_SYSTEM > 0
    if (inserted) {
      i++;
      if (i % SYSTEM_RECORD_PERIOD == 0) {

        sim_record(input, state, dstate, output, system_record_count++);

        i = 0;
      }
    }
#endif

    for (int i = 0 ; i < nb_state ; i++) {
      state2[i] = state[i] + step*dstate[i];
    }

    dyn_system(input, state2, dstate2, output);

    for (int i = 0 ; i < nb_state ; i++) {
      state[i] = state[i] + step*(dstate[i] + dstate2[i])/2;
    }

    if (Brain.Timer.systemHighResolution() < end) {
      while (Brain.Timer.systemHighResolution() < end) {}
    } //else {
      //end = Brain.Timer.systemHighResolution();
      //misses++;
    //}
    start = end;
    //done++;
  }

  free(input);
  free(state);
  free(state2);
  free(dstate);
  free(dstate2);
  free(output);

  return 0;
}