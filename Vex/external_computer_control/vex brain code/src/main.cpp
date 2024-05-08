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
#include <stdio.h>

using namespace vex;
using namespace std;

//vex::PORT22;

#define BUFFER_SIZE 128

int main() {
  // Initializing Robot Configuration. DO NOT REMOVE!
  vexcodeInit();

  
  Brain.Screen.print("Waiting for connection");

  char buffer[BUFFER_SIZE];

  //fprintf(stdout, "ready");

  /*serial_link computer = serial_link(, "computer", linkType::raw);
  int nb = sprintf((char*)buffer, "ready2");
  computer.send(buffer, nb);
  computer.receive(buffer, 100, 10000000);*/

  
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
  Brain.Screen.print("Size of a double = %d", buffer[0]);

  fprintf(stdout, "Thanks");

  wait(1000, msec);

  //int fd = open(stdin, O_RDWR | O_NOCTTY | O_SYNC);
  //write()
  exit(0);
}
