#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define BUFFER_LEN 10000

#define FLOAT_SIZE sizeof(float)
#define DOUBLE_SIZE sizeof(double)

void readFloat(void *buffer, double *output_arr, FILE *ptr, size_t read_size, size_t file_size);
void readFloatInv(void *buffer, double *output_arr, FILE *ptr, size_t read_size, size_t file_size);
void readDouble(void *buffer, double *output_arr, FILE *ptr, size_t read_size, size_t file_size);
void readDoubleInv(void *buffer, double *output_arr, FILE *ptr, size_t read_size, size_t file_size);

double *binaryToVector(const char *inputFile, unsigned long long *size, unsigned long long *nb_col)
{
    uint8_t byte_buffer[BUFFER_LEN];
    void *buffer = (void*) (byte_buffer);


    FILE *ptr;
    ptr = fopen(inputFile, "rb"); // r for read, b for binary

    fseek(ptr, 0, SEEK_END); // seek to end of file
    long file_size = ftell(ptr);       // get current file pointer aka size
    fseek(ptr, 0, SEEK_SET); // seek back to beginning of file

    if (file_size == 0)
    {
        fprintf(stderr, "File does not exist\n");
        return NULL;
    }
    if (file_size < 3)
    {
        fprintf(stderr, "File too small\n");
        return NULL;
    }
    
    size_t read = fread(byte_buffer, 1, 3, ptr);

    size_t endianness = (size_t) (byte_buffer[0]);
    size_t number_size = (size_t) (byte_buffer[1]);
    size_t nb_channel = (size_t)(byte_buffer[2]);
    int inverse = 0;
    {
        unsigned int i = 1;
        char *c = (char *)(&i);
        if (*c)
        { // This computer is little endian
            if (endianness == 1)
            { // The other computer is big endian
                inverse = 1;
            }
        }
        else
        { // This computer is big endian
            if (endianness == 0)
            { // The other computer is little endian
                inverse = 1;
            }
        }
    }

    size_t read_size = BUFFER_LEN / (number_size); // Number of doubles that can be read at once
    //size_t size = number_size * (nb_channel);
    if (read_size * number_size > BUFFER_LEN)
    {
        fprintf(stderr, "Size computation failed");
        return NULL;
    }

    if (number_size != FLOAT_SIZE && number_size != DOUBLE_SIZE)
    { // Do no treat unsupported sizes
        fprintf(stderr, "Double size of %d bytes is not supported.\n", number_size);
        return NULL;
    }

    if ((file_size - 3) % (number_size * nb_channel) != 0)
    {
        fprintf(stderr, "Corrected file size (%d B) indicates an non integrer number of doubles (%d B).\n", file_size - 3, number_size);
        return NULL;
    }

    size_t nb_elem = (file_size - 3)/number_size;
    double *output_arr = malloc(nb_elem * sizeof(double));
    if (output_arr == NULL)
    {
        fprintf(stderr, "Allocation of vector failed");
        return NULL;
    }

    if (number_size == FLOAT_SIZE)
    {
        if (inverse) 
            readFloatInv(buffer, output_arr, ptr, read_size, nb_elem);
        else
            readFloat(buffer, output_arr, ptr, read_size, nb_elem);
    }
    if (number_size == DOUBLE_SIZE)
    {
        if (inverse)
            readDoubleInv(buffer, output_arr, ptr, read_size, nb_elem);
        else
            readDouble(buffer, output_arr, ptr, read_size, nb_elem);
    }

    fclose(ptr);

    size[0] = nb_elem;
    nb_col[0] = nb_channel;

    return output_arr;
}

void readFloat(void *buffer, double *output_arr, FILE *ptr, size_t read_size, size_t file_size)
{
    size_t read = fread(buffer, FLOAT_SIZE, read_size, ptr);
    size_t count = 0;
    while (read > 0)
    {
        if (count + read >= file_size)
        {
            fprintf(stderr, "Underestimated size of file. Error from SEEK_END.");
            return NULL;
        }
        for (int i = 0; i < read; i++)
        {                                            // Convert to different pointer then read
            //uint32_t byte_val = ((uint32_t *)buffer)[i]; // Cast then read
            output_arr[count] = (double) (((float *)buffer)[i]); // Dark magic pointer conversion
            count++;
        }
        read = fread(buffer, FLOAT_SIZE, read_size, ptr);
    }
}

void readFloatInv(void *buffer, double *output_arr, FILE *ptr, size_t read_size, size_t file_size)
{
    size_t read = fread(buffer, FLOAT_SIZE, read_size, ptr);
    size_t count = 0;
    while (read > 0)
    {
        if (count + read >= file_size)
        {
            fprintf(stderr, "Underestimated size of file. Error from SEEK_END.");
            return NULL;
        }
        for (int i = 0; i < read; i++)
        {                                                // Convert to different pointer then read
            uint32_t byte_val = ((uint32_t *)buffer)[i]; // Cast then read
            byte_val = __builtin_bswap32(byte_val); // Swap bytes
            output_arr[count] = (double)(*(float *)(&byte_val)); // Dark magic pointer conversion

            count++;
        }
        read = fread(buffer, FLOAT_SIZE, read_size, ptr);
    }
}

void readDouble(void *buffer, double *output_arr, FILE *ptr, size_t read_size, size_t file_size)
{
    size_t read = fread(buffer, DOUBLE_SIZE, read_size, ptr);
    size_t count = 0;
    while (read > 0)
    {
        if (count + read >= file_size)
        {
            fprintf(stderr, "Underestimated size of file. Error from SEEK_END.");
            return NULL;
        }
        for (int i = 0; i < read; i++)
        { // Convert to different pointer then read
            output_arr[count] = ((double *)buffer)[i]; // Dark magic pointer conversion
            count++;
        }
        read = fread(buffer, DOUBLE_SIZE, read_size, ptr);
    }
}

void readDoubleInv(void *buffer, double *output_arr, FILE *ptr, size_t read_size, size_t file_size)
{
    size_t read = fread(buffer, DOUBLE_SIZE, read_size, ptr);
    size_t count = 0;
    while (read > 0)
    {
        if (count + read >= file_size)
        {
            fprintf(stderr, "Underestimated size of file. Error from SEEK_END.");
            return NULL;
        }
        for (int i = 0; i < read; i++)
        {                                              // Convert to different pointer then read
            uint64_t byte_val = ((uint64_t *)buffer)[i]; // Cast then read
            byte_val = __builtin_bswap64(byte_val);      // Swap bytes
            output_arr[count] = *((double *)&byte_val);   // Dark magic pointer conversion
            count++;
        }
        read = fread(buffer, DOUBLE_SIZE, read_size, ptr);
    }
}

void freeVector(double* vector) {
    free(vector);
}

/* Old readind code
 read = fread(buffer, number_size, read_size, ptr);
    size_t count = 0;
    while (read > 0)
    {
        if (count+read >= nb_double_file) {
            fprintf(stderr, "Underestimated size of file. Error from SEEK_END.");
            return NULL;
        }
        for (int i = 0; i < read; i++)
        {
            if (number_size == 4)
            {                                                                // Convert to different pointer then read
                uint32_t byte_val = ((uint32_t *) buffer)[i]; // Cast then read
                if (inverse)
                {
                    byte_val = __builtin_bswap32(byte_val);
                }
                output_arr[count] = (double) (*(float *)(&byte_val)); // Dark magic pointer conversion
            }
            else if (number_size == 8)
            {                                                                // Convert to different pointer then read
                uint64_t byte_val = ((uint64_t *) buffer)[i]; // Cast then read
                if (inverse)
                {
                    byte_val = __builtin_bswap64(byte_val);
                }
                output_arr[count] = *(double *)(&byte_val); // Dark magic pointer conversion
            }

            count++;
        }
        read = fread(buffer, number_size, read_size, ptr);
    }
*/