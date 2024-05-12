#include "vex.h"

using namespace vex;

// A global instance of brain used for printing to the V5 brain screen
brain Brain;

// VEXcode device constructors
motor PendulumMotor = motor(PORT1, ratio18_1, false);

/**
 * Used to initialize code/tasks/devices added using tools in VEXcode Pro.
 *
 * This should be called at the start of your int main function.
 */
void vexcodeInit(void){
  PendulumMotor.setBrake(brakeType::coast);
  PendulumMotor.spinTo(0.0, rotationUnits::deg);
}