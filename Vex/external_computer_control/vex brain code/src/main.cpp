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

  char in_buffer[BUFFER_SIZE];
  char out_buffer[BUFFER_SIZE];

  
  char endianness;
  int is_float;
  size_t char_read;
  size_t char_written;

  {
    unsigned int i = 1;
    char *c = (char *)&i;
    if (*c) // Little endian
    {
      endianness = 0;
    }
    else // Big endian
    {
      endianness = 1;
    }
  }

  //fprintf(stdout, "ready");

  /*serial_link computer = serial_link(, "computer", linkType::raw);
  int nb = sprintf((char*)buffer, "ready2");
  computer.send(buffer, nb);
  computer.receive(buffer, 100, 10000000);*/

  
  wait(100, msec);

  char_read = fread(in_buffer, sizeof(char), BUFFER_SIZE, stdin);
  if (char_read == 0)
  {
    Brain.Screen.clearLine(1);
    Brain.Screen.setCursor(1, 1);
    Brain.Screen.print("Error establishing connection.");
    return -1;
  }

  if (char_read != 6 || in_buffer[0] != 0x01 || in_buffer[1] != 0x20 || in_buffer[2] != 0x03 || in_buffer[3] != 0x40)
  {
    Brain.Screen.clearLine(1);
    Brain.Screen.setCursor(1, 1);
    Brain.Screen.print("Handshake with computer returned wrong handshake value.");
    return -1;
  }

  size_t double_size_computer = in_buffer[4];
  size_t endianness_computer = in_buffer[5];

  if (double_size_computer == sizeof(float)) {
    is_float = 1;
  } else if (double_size_computer == sizeof(double)) {
    is_float = 0;
  } else {
    Brain.Screen.clearLine(1);
    Brain.Screen.setCursor(1, 1);
    Brain.Screen.print("Error: computer use wrong float size (%d B) for brain.", double_size_computer);
    return -1;
  }

  out_buffer[0] = 0x10;
  out_buffer[1] = 0x02;
  out_buffer[2] = 0x30;
  out_buffer[3] = 0x04;
  out_buffer[4] = endianness;
  fwrite(out_buffer, sizeof(char), 5, stdout);

  Brain.Screen.clearLine(1);
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("Handshake done. Connection established.");

  while (true)
  {
    wait(100, msec);
    char_read = fread(in_buffer, sizeof(char), BUFFER_SIZE, stdin);

    if (char_read == 0)
    {
      Brain.Screen.clearLine(2);
      Brain.Screen.setCursor(2, 1);
      Brain.Screen.print("No data read.");
      continue;
    }

    for (int k = 0; k < char_read; k++)
    {
      out_buffer[k] = in_buffer[k];	
    }

    char_written = fwrite(out_buffer, sizeof(char), char_read, stdout);

    if (char_written != char_read)
    {
      Brain.Screen.clearLine(2);
      Brain.Screen.setCursor(2, 1);
      Brain.Screen.print("Error writing data.");
    } else {
      Brain.Screen.clearLine(2);
      Brain.Screen.setCursor(2, 1);
      Brain.Screen.print("Data written (%d). Starts with %d.", char_written, in_buffer[0]);
    }
  }



  //int fd = open(stdin, O_RDWR | O_NOCTTY | O_SYNC);
  //write()
  return 0;
}
