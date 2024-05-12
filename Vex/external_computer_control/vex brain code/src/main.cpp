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
#include <unistd.h>

using namespace vex;
using namespace std;

//vex::PORT22;

#define BUFFER_SIZE 8192
#define COM_WAIT_MILLI 2

int main() {
  // Initializing Robot Configuration. DO NOT REMOVE!
  vexcodeInit();

  
  Brain.Screen.print("Waiting for connection");

  uint8_t in_buffer[BUFFER_SIZE];
  uint8_t out_buffer[BUFFER_SIZE];

  int num_in = fileno(stdin);
  int num_out = fileno(stdout);

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
  
  serial_link computer = serial_link(num_in, "computer", linkType::raw);

  wait(100, msec);

  vexSerialWriteBuffer(num_out, (uint8_t *)"ready", 5);
  vexSerialPeekChar(num_in);


  //char_read = fread(in_buffer, sizeof(char), 6, stdin);
  char_read = read(num_in, in_buffer, 6);
  //char_read = computer.receive(in_buffer, 6, 1000000);
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
    Brain.Screen.clearLine(1);
    Brain.Screen.setCursor(1, 1);
    Brain.Screen.print("Error: single precicion float size (%d B) not supported.", double_size_computer);
    return -1;
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
  //char_written = fwrite(out_buffer, sizeof(char), 5, stdout);
  char_written = write(num_out, out_buffer, 5);
  //char_written = computer.send(out_buffer, 5);

  if (char_written != 5) {
    Brain.Screen.clearLine(1);
    Brain.Screen.setCursor(1, 1);
    Brain.Screen.print("Handshake with computer returned wrong handshake value.");
    return -1;
  }

  Brain.Screen.clearLine(1);
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("Handshake done. %s | %s.", num_in, num_out);

  uint64_t end;
  uint64_t wait_time;
  uint64_t step = COM_WAIT_MILLI * 1000;
  uint64_t start = Brain.Timer.systemHighResolution();
  double voltage = 0.0;
  int i = 0;
  while (true)
  {
    end = start + step;
    //wait(1, msec);
    //char_read = fread(in_buffer, sizeof(char), 8, stdin);
    size_t char_to_read = sizeof(double) + 4*sizeof(char);
    char_read = read(num_in, in_buffer, char_to_read);
    //char_read = computer.receive(in_buffer, char_to_read, step);

    /*
    if (char_read == 0)
    {
      Brain.Screen.clearLine(2);
      Brain.Screen.setCursor(2, 1);
      Brain.Screen.print("No data read.");
      continue;
    }*/

    while (char_read < char_to_read)
    {
      // char_read += fread(&(in_buffer[char_read]), sizeof(char), 8-char_read, stdin);
      Brain.Screen.clearLine(3);
      Brain.Screen.setCursor(3, 1);
      Brain.Screen.print("Waiting for reading more data.");
      return -1;
      //char_read += read(num_in, &(in_buffer[char_read]), char_to_read - char_read);
    }

    if (in_buffer[0] != 0x01 || in_buffer[1] != 0x20 || in_buffer[2] != 0x03 || in_buffer[3] != 0x40)
    {
      Brain.Screen.clearLine(2);
      Brain.Screen.setCursor(2, 1);
      Brain.Screen.print("Wrong header reading data.");
      return -1;
    }
    voltage = *((double *)&(in_buffer[4]));

    size_t char_to_write = 4 * sizeof(char);
    out_buffer[0] = 0x10;
    out_buffer[1] = 0x02;
    out_buffer[2] = 0x30;
    out_buffer[3] = 0x04;

    //char_written = fwrite(out_buffer, sizeof(char), char_read, stdout);
    char_written = write(num_out, out_buffer, char_to_write);
    //char_written = computer.send(out_buffer, char_to_write);

    while (char_written < char_to_write)
    {
      // char_written += fwrite(&(out_buffer[char_written]), sizeof(char), char_read - char_written, stdout);
      Brain.Screen.clearLine(3);
      Brain.Screen.setCursor(3, 1);
      Brain.Screen.print("Waiting for writing more data.");
      return -1;
      //char_written += write(num_out, &(out_buffer[char_written]), char_to_write - char_written);
    }

    /*
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
    */
    PendulumMotor.spin(fwd, voltage, voltageUnits::mV);

    
    i = (i+1) % 199;
    if (i == 0)
    {
      Brain.Screen.clearLine(2);
      Brain.Screen.setCursor(2, 1);
      Brain.Screen.print("Voltage %lf mV.", voltage);
    }
    

    uint64_t now = Brain.Timer.systemHighResolution();
    if (end > now+1000)
    { // Sleep is necessary
      wait_time = ((double)(end - Brain.Timer.systemHighResolution())) / 1000.0;
      wait(wait_time, msec);
    }
    start = end;
  }

  //int fd = open(stdin, O_RDWR | O_NOCTTY | O_SYNC);
  //write()
  return 0;
}
