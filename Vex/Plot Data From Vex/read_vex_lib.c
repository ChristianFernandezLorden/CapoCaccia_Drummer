#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Relative to file format
#define HEADER_SIZE 3 

// Arbitrary number determining the number of bytes in the read buffer.
#define BUFFER_LEN 10000

// Shortcuts
#define FLOAT_SIZE sizeof(float)
#define DOUBLE_SIZE sizeof(double)

static inline int readFloat(void *buffer, double *output_arr, FILE *ptr, size_t read_size, size_t file_size);
static inline int readFloatInv(void *buffer, double *output_arr, FILE *ptr, size_t read_size, size_t file_size);
static inline int readDouble(void *buffer, double *output_arr, FILE *ptr, size_t read_size, size_t file_size);
static inline int readDoubleInv(void *buffer, double *output_arr, FILE *ptr, size_t read_size, size_t file_size);

double *binaryfileToVector(const char *inputFile, unsigned long long *size, unsigned long long *nb_col)
{
    uint8_t byte_buffer[BUFFER_LEN];        // Create a read buffer
    void *buffer = (void*) (byte_buffer);   // Declare the void* 

    // Set error values as default
    size[0] = 0;
    nb_col[0] = 0;

    FILE *ptr;
    ptr = fopen(inputFile, "rb");   // r for read, b for binary

    fseek(ptr, 0, SEEK_END);        // Put file ptr at the end of the file
    long file_size = ftell(ptr);    // get current file pointer position aka size in bytes
    fseek(ptr, 0, SEEK_SET);        // seek back to beginning of file for reading data

    if (file_size == 0)
    {
        fprintf(stderr, "File \"%s\" does not exist.\n", inputFile);
        return NULL;
    }
    if (file_size < HEADER_SIZE)
    {
        fprintf(stderr, "File is too small to contain the required header.\n");
        return NULL;
    }
    
    size_t read = fread(byte_buffer, 1, 3, ptr);

    // Read the three byte header containing the info on the file
    size_t endianness = (size_t) (byte_buffer[0]);  // Endianness of the device that has written the file (1 = big endian, 0 = little endian)
    size_t number_size = (size_t) (byte_buffer[1]); // Size of the float number representation in bytes (only 4 and 8 bytes representation are currently supported)
    size_t nb_channel = (size_t) (byte_buffer[2]);  // Number of columns in the file 
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

    if (read_size * number_size > BUFFER_LEN) // By properties of integer division, this should not happen
    {
        fprintf(stderr, "Max read size computation failed, reaching a read size greater than buffer size.\n");
        return NULL;
    }

    if (number_size != FLOAT_SIZE && number_size != DOUBLE_SIZE)
    { // Do not treat unsupported sizes
        fprintf(stderr, "Double size of %d bytes is not supported.\n", (int) number_size);
        return NULL;
    }

    if ((file_size - HEADER_SIZE) % (number_size * nb_channel) != 0)
    {
        fprintf(stderr, "Corrected file size (%d B) indicates a non integrer number of rows (%d B per row).\n", (int)(file_size - HEADER_SIZE), (int)(number_size * nb_channel));
        return NULL;
    }

    size_t nb_elem = (file_size - HEADER_SIZE) / number_size;
    double *output_arr = malloc(nb_elem * DOUBLE_SIZE);
    if (output_arr == NULL)
    {
        fprintf(stderr, "Allocation of vector with a size of %.2lf MB failed.\n", (nb_elem * DOUBLE_SIZE)/1048576.0);
        return NULL;
    }

    if (number_size == FLOAT_SIZE)
    {
        if (inverse) {
            if(!readFloatInv(buffer, output_arr, ptr, read_size, nb_elem)) {
                return NULL;
            }
        } else { 
            if (!readFloat(buffer, output_arr, ptr, read_size, nb_elem)){
                return NULL;
            }
        }
    }
    if (number_size == DOUBLE_SIZE)
    {
        if (inverse) {
            if(!readDoubleInv(buffer, output_arr, ptr, read_size, nb_elem)) {
                return NULL;
            }
        } else { 
            if (!readDouble(buffer, output_arr, ptr, read_size, nb_elem)){
                return NULL;
            }
        }
    }

    fclose(ptr);

    // Set correct values
    size[0] = nb_elem;
    nb_col[0] = nb_channel;

    return output_arr;
}

void freeVector(double *vector)
{
    free(vector);
}

/* Static functions */

int readFloat(void *buffer, double *output_arr, FILE *ptr, size_t read_size, size_t file_size)
{
    size_t read = fread(buffer, FLOAT_SIZE, read_size, ptr);
    size_t count = 0;
    while (read > 0)
    {
        if (count + read > file_size)
        {
            fprintf(stderr, "Underestimated size of file (file_size = %ld, read = %ld, total = %ld). Error from SEEK_END.\n", 
                            file_size, read, count+read);
            return 0;
        }
        for (size_t i = 0; i < read; i++)
        {                                            
            output_arr[count] = (double)(((float *)buffer)[i]); // Cast then read
            count++;
        }
        read = fread(buffer, FLOAT_SIZE, read_size, ptr);
    }
    return 1;
}

int readFloatInv(void *buffer, double *output_arr, FILE *ptr, size_t read_size, size_t file_size)
{
    size_t read = fread(buffer, FLOAT_SIZE, read_size, ptr);
    size_t count = 0;
    while (read > 0)
    {
        if (count + read > file_size)
        {
            fprintf(stderr, "Underestimated size of file (file_size = %ld, read = %ld, total = %ld). Error from SEEK_END.\n",
                    file_size, read, count + read);
            return 0;
        }
        for (size_t i = 0; i < read; i++)
        {                                                
            uint32_t byte_val = ((uint32_t *)buffer)[i];            // Cast then read
            byte_val = __builtin_bswap32(byte_val);                 // Swap bytes
            output_arr[count] = (double)(*(float *)(&byte_val));    // Dark magic pointer conversion

            count++;
        }
        read = fread(buffer, FLOAT_SIZE, read_size, ptr);
    }
    return 1;
}

int readDouble(void *buffer, double *output_arr, FILE *ptr, size_t read_size, size_t file_size)
{
    size_t read = fread(buffer, DOUBLE_SIZE, read_size, ptr);
    size_t count = 0;
    while (read > 0)
    {
        if (count + read > file_size)
        {
            fprintf(stderr, "Underestimated size of file (file_size = %ld, read = %ld, total = %ld). Error from SEEK_END.\n",
                    file_size, read, count + read);
            return 0;
        }
        for (size_t i = 0; i < read; i++)
        { 
            output_arr[count] = ((double *)buffer)[i]; // Cast then read
            count++;
        }
        read = fread(buffer, DOUBLE_SIZE, read_size, ptr);
    }
    return 1;
}

int readDoubleInv(void *buffer, double *output_arr, FILE *ptr, size_t read_size, size_t file_size)
{
    size_t read = fread(buffer, DOUBLE_SIZE, read_size, ptr);
    size_t count = 0;
    while (read > 0)
    {
        if (count + read > file_size)
        {
            fprintf(stderr, "Underestimated size of file (file_size = %ld, read = %ld, total = %ld). Error from SEEK_END.\n",
                    file_size, read, count + read);
            return 0;
        }
        for (size_t i = 0; i < read; i++)
        {                                           
            uint64_t byte_val = ((uint64_t *)buffer)[i];    // Cast then read
            byte_val = __builtin_bswap64(byte_val);         // Swap bytes
            output_arr[count] = *((double *)&byte_val);     // Dark magic pointer conversion
            count++;
        }
        read = fread(buffer, DOUBLE_SIZE, read_size, ptr);
    }
    return 1;
}

