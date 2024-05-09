#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

#define PORT_NAME "/dev/tty.usbmodem11303"
#define BUFFER_SIZE 1024

pthread_mutex_t mutex;

int main()
{
    pthread_t sim_thread;
    pthread_t audio_process_thread;
    pthread_mutex_init(&mutex, NULL);

    int fd = open(PORT_NAME, O_RDWR | O_NOCTTY | O_SYNC);
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

    // Dont know how useful this is
    //set_interface_attribs(fd, B115200, 0); // set speed to 115,200 bps, 8n1 (no parity)
    //set_blocking(fd, 1);                   // set no blocking

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
    //char_written = fwrite(out_buffer, sizeof(char), 6, fd);

    if (char_written != 6)
    {
        fprintf(stderr, "error writing handshake\n");
        return -1;
    }

    //usleep((6 + 25) * 100); // sleep enough to transmit the 7 plus
                            // receive 25:  approx 100 uS per char transmit (only useful for async com)

    char_read = read(fd, in_buffer, 5);
    //char_read = fread(in_buffer, sizeof(char), 5, fd);

    if (char_read < 5)
    {
        fprintf(stderr, "error in reading\n");
    }

    if (in_buffer[4] != endianness)
    {
        fprintf(stderr, "Oposite endianness (not supported yet).\n");
        return -1;
    }

    //usleep((20 + 25) * 100);
    // 13 = 1101
    // 10 = 1010

    int i = 0;
    char i_char = 0;
    while (i < 10000)
    {
        i++;
        i_char = ++i_char%200;
        out_buffer[0] = i_char;
        out_buffer[1] = 0x02;
        out_buffer[2] = 0x03;
        out_buffer[3] = 0x04;
        out_buffer[4] = 0x05;
        out_buffer[5] = 0x06;
        out_buffer[6] = 0x07;
        out_buffer[7] = 0x08;

        char_written = write(fd, out_buffer, 8);
        //char_written = fwrite(out_buffer, sizeof(char), 8, fd);

        if (char_written < 8)
        {
            fprintf(stderr, "error in writing\n");
        }

        while (char_written < 8)
        {
            char_written += write(fd, &(out_buffer[char_written]), 8 - char_written);
            //char_written += fwrite(&(out_buffer[char_written]), sizeof(char), 8 - char_written, fd);
        }
        

        //usleep(2000);

        char_read = read(fd, in_buffer, 8);
        //char_read = fread(in_buffer, sizeof(char), 8, fd);        

        if (char_read < 8)
        {
            fprintf(stderr, "error in reading\n");
        }

        while (char_read < 8)
        {
            char_read += read(fd, &(in_buffer[char_read]), 8 - char_read);
            //char_read += fread(&(in_buffer[char_read]), sizeof(char), 8 - char_read, fd);
        }

        //usleep((5 + 25) * 100);

        for (int k = 0; k < 8; k++)
        {
            if (in_buffer[k] != out_buffer[k])
            {
                fprintf(stderr, "error in received data: in: %d != %d :out\n", in_buffer[k], out_buffer[k]);
            }
        }
        if (out_buffer[0] == 1)
        {
            printf("Wrapping %d\n", i);
        }
    }
}

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