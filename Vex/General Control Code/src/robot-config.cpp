#include "vex.h"

using namespace vex;

// A global instance of brain used for printing to the V5 Brain screen

brain Brain;

// VEXcode device constructors
motor PendulumMotor = motor(PORT1, ratio18_1, false);
rotation SpeedSensor = rotation(PORT2, true);
inertial CollisionSensor = inertial(PORT3);

// Other shared variables
semaphore lock = semaphore();
// Pointer for datalog communication
double *system_data;
double *system_data_backup;
double *sensors_data;
double *sensors_data_backup;

// VEXcode generated functions

/**
 * Used to initialize code/tasks/devices added using tools in VEXcode Pro.
 *
 * This should be called at the start of your int main function.
 */
void vexcodeInit(void) {
  PendulumMotor.setBrake(brakeType::coast);
  PendulumMotor.spinTo(0.0, rotationUnits::deg);
  SpeedSensor.resetPosition();
}