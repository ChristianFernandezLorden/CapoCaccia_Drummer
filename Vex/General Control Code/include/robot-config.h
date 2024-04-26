#pragma once

using namespace vex;

extern brain Brain;

// VEXcode devices
extern motor PendulumMotor;
extern rotation SpeedSensor;

// Other shared variables 
extern semaphore lock;

extern double *system_data;
extern double *system_data_backup;
extern double *sensors_data;
extern double *sensors_data_backup;

/**
 * Used to initialize code/tasks/devices added using tools in VEXcode Pro.
 *
 * This should be called at the start of your int main function.
 */
void vexcodeInit(void);