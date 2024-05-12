#include "common.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#ifdef WINDOWS_OS
#else
#include <termios.h>
#endif

// Imported library
#include "portaudio.h"

#include "sim.h"
#include "models.h"
#include "structs.h"
#include "audio_process.h"

#define PORT_NAME "/dev/tty.usbmodem11303"
#define BUFFER_SIZE 8192


int main()
{
    pthread_mutex_t sim_mutex;
    pthread_mutex_t audio_mutex;
    com_data_t sim_com_data;
    com_data_t audio_com_data;

    fprintf(stderr, "Starting Initialization\n");

    sys_specific_functions_t sys_specific_functions = unconnected_hco();

    pthread_t sim_thread;
    //pthread_t audio_process_thread , Auto created
    sim_param_t sim_param;

    if (pthread_mutex_init(&sim_mutex, NULL) != 0 ||
        pthread_mutex_init(&audio_mutex, NULL) != 0)
    {
        fprintf(stderr, "Mutex init failed\n");
        return -1;
    }
    sim_com_data.com_mutex = &sim_mutex;
    audio_com_data.com_mutex = &audio_mutex;

    sys_specific_functions.init_sim_com(&sim_com_data);
    sys_specific_functions.init_sim_param(&sim_param);
    sim_param.sim_com_data = &sim_com_data;

    // Thread Creation
    PaStream *stream;

    startPortAudio();
    listInputDevices();
    startAudioStream(stream, &audio_com_data); // Launch audio thread

    pthread_create(&sim_thread, NULL, simulate, &sim_param); // Launch sim thread

    int fd = open(PORT_NAME, O_RDWR | O_NOCTTY | O_SYNC | O_EXCL);
    if (fd < 0)
    {
        fprintf(stderr, "error %d opening %s: %s", errno, PORT_NAME, strerror(errno));
        return -1;
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0)
    {
        fprintf(stderr, "error %d from tcgetattr", errno);
        return -1;
    }
    cfsetospeed(&tty, B230400);
    cfsetispeed(&tty, B230400);
    /*
    cfmakeraw(&tty);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;                 // disable break processing
    tty.c_lflag = 0;                        // no signaling chars, no echo,
                                            // no canonical processing
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);   // ignore modem controls,
                                       // enable reading
    tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
    tty.c_cflag |= 0;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cc[VTIME] = 1; // 0.5 seconds read timeout

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        fprintf(stderr, "error %d from tcsetattr", errno);
        return -1;
    }
    */

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

    out_buffer[0] = 0x01;
    out_buffer[1] = 0x20;
    out_buffer[2] = 0x03;
    out_buffer[3] = 0x40;
    out_buffer[4] = sizeof(double);
    out_buffer[5] = endianness;

    char_written = write(fd, out_buffer, 6);
    if (char_written != 6)
    {
        fprintf(stderr, "error writing handshake\n");
        return -1;
    }


    char_read = read(fd, in_buffer, 5);
    if (char_read < 5)
    {
        fprintf(stderr, "error in reading\n");
    }

    if (in_buffer[4] != endianness)
    {
        fprintf(stderr, "Oposite endianness (not supported yet).\n");
        return -1;
    }

    fprintf(stderr, "Initialized\n");

    double voltage;
    int i = 0;
    while (1)
    {   
        if (i == 0)
        {
            fprintf(stderr, "Running\n");
        }
        i = (i + 1) % 200;
        pthread_mutex_lock(sim_com_data.com_mutex);
        voltage = sys_specific_functions.communicate(&sim_com_data, voltage);
        voltage = 0.0;
        pthread_mutex_unlock(sim_com_data.com_mutex);
        pthread_mutex_lock(audio_com_data.com_mutex);
        int peak = 0;
        if (audio_com_data.has_new_data[0] == 1)
        {
            audio_com_data.has_new_data[0] = 0;
            peak = 1;
        }
        pthread_mutex_unlock(audio_com_data.com_mutex);
        if (peak)
        {
            printf("Voltage: %f\n", voltage);
        }

        out_buffer[0] = 0x01;
        out_buffer[1] = 0x20;
        out_buffer[2] = 0x03;
        out_buffer[3] = 0x40;
        *((double*)&(out_buffer[4])) = 200*voltage;

        //fprintf(stderr, "Byte sent: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n", *((unsigned char*)&out_buffer[0]), *((unsigned char*)&out_buffer[1]), *((unsigned char*)&out_buffer[2]), *((unsigned char*)&out_buffer[3]), *((unsigned char*)&out_buffer[4]), *((unsigned char*)&out_buffer[5]), *((unsigned char*)&out_buffer[6]), *((unsigned char*)&out_buffer[7]), *((unsigned char*)&out_buffer[8]), *((unsigned char*)&out_buffer[9]), *((unsigned char*)&out_buffer[10]), *((unsigned char*)&out_buffer[11]));

        size_t char_to_write = sizeof(double) + 4*sizeof(char);

        char_written = write(fd, out_buffer, char_to_write);

        if (char_written < char_to_write)
        {
            //fprintf(stderr, "error in writing\n");
        }

        while (char_written < char_to_write)
        {
            char_written += write(fd, &(out_buffer[char_written]), char_to_write - char_written);
        }
        

        size_t char_to_read = 4*sizeof(char);
        char_read = read(fd, in_buffer, char_to_read);

        if (char_read < char_to_read)
        {
            fprintf(stderr, "error in reading\n");
        }

        while (char_read < char_to_read)
        {
            char_read += read(fd, &(in_buffer[char_read]), char_to_read - char_read);
        }

        if (in_buffer[0] != 0x10 || in_buffer[1] != 0x02 || in_buffer[2] != 0x30 || in_buffer[3] != 0x04)
        {
            fprintf(stderr, "error in header in read\n");
        }
    }
}

/*
double voltage;
    for (int i = 0; i < 10000; i++)
    {
        pthread_mutex_lock(sim_com_data.com_mutex);
        voltage = sys_specific_functions.communicate(&sim_com_data, voltage);
        pthread_mutex_unlock(sim_com_data.com_mutex);
        pthread_mutex_lock(audio_com_data.com_mutex);
        int peak = 0;
        if (audio_com_data.has_new_data[0] == 1)
        {
            audio_com_data.has_new_data[0] = 0;
            peak = 1;
        }
        pthread_mutex_unlock(audio_com_data.com_mutex);
        if (peak)
        {
            printf("Voltage: %f\n", voltage);
        }
        usleep(100000);
    }
*/

/*
int set_interface_attribs(int fd, int speed, int parity)
{
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0)
    {
        fprintf(stderr, "error %d from tcgetattr", errno);
        return -1;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK; // disable break processing
    tty.c_lflag = 0;        // no signaling chars, no echo,
                            // no canonical processing
    tty.c_oflag = 0;        // no remapping, no delays
    tty.c_cc[VMIN] = 0;     // read doesn't block
    tty.c_cc[VTIME] = 5;    // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);   // ignore modem controls,
                                       // enable reading
    tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        fprintf(stderr, "error %d from tcsetattr", errno);
        return -1;
    }
    return 0;
}

void set_blocking(int fd, int should_block)
{
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0)
    {
        fprintf(stderr, "error %d from tggetattr", errno);
        return;
    }

    tty.c_cc[VMIN] = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5; // 0.5 seconds read timeout

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
        fprintf(stderr, "error %d setting term attributes", errno);
}
*/