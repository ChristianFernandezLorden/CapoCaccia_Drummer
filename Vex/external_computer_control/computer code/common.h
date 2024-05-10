#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
// define something for Windows (32-bit and 64-bit, this part is common)
#ifdef _WIN64
// define something for Windows (64-bit only)
#else
// define something for Windows (32-bit only)
#endif
#define WINDOWS_OS
#else
// define something for Linux
#define LINUX_MAC_OS
#endif

#include <pthread.h>

#include "structs.h"

extern pthread_mutex_t mutex;
extern pthread_barrier_t init_barrier;
extern com_data_t sim_com_data;
