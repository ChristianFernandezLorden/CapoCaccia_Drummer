/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Module:       main.cpp                                                  */
/*    Author:       VEX                                                       */
/*    Created:      Sun Oct 06 2019                                           */
/*    Description:  This program will run a thread parallel (at the same time */
/*                  to main.                                                  */
/*                                                                            */
/*----------------------------------------------------------------------------*/

// ---- START VEXCODE CONFIGURED DEVICES ----
// ---- END VEXCODE CONFIGURED DEVICES ----

#include "vex.h"

using namespace vex;
using namespace std;

#define BUFFER_SIZE 128

int main() {
  // Initializing Robot Configuration. DO NOT REMOVE!
  vexcodeInit();

  Brain.Screen.print("Waiting for connection");

  char buffer[BUFFER_SIZE];

  fprintf(stdout, "ready");

  wait(1000, msec);

  if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) 
  {
    Brain.Screen.clearLine(1);
    Brain.Screen.setCursor(1, 1);
    Brain.Screen.print("No connection established");
  }
  Brain.Screen.clearLine(1);
  Brain.Screen.clearLine(2);
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("Data read");
  Brain.Screen.setCursor(2, 1);
  Brain.Screen.print(buffer);
}
