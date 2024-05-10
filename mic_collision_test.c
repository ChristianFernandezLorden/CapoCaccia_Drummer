#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "portaudio.h"

// Compilation command:
// gcc -framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework CoreServices -framework CoreFoundation -o audio audio.c /usr/local/lib/libportaudio.a -lm -pthread

#define SAMPLE_RATE 192000 // 192 kHz for the amplifier used
#define NUM_CHANNELS 2 // 1 for standard microphone, 2 in the case of the amplifier
#define FRAME_SIZE 512 // Number of samples per frame
#define PEAK_THRESHOLD 0.1 // Threshold for peak detection, experimentally determined

float sample = 0.0;

// Struct to pass data between threads
struct ThreadData {
    pthread_mutex_t mutex;
    int peakDetected;
};

// Function to process audio samples
int processAudio(const void *inputBuffer, void *outputBuffer,
                 unsigned long framesPerBuffer,
                 const PaStreamCallbackTimeInfo *timeInfo,
                 PaStreamCallbackFlags statusFlags,
                 void *userData) {
    struct ThreadData *threadData = (struct ThreadData *)userData;
    float *input = (float *)inputBuffer;
    (void)outputBuffer; // Prevent unused variable warning

    // Process audio data to detect peaks
    float maxSample = 0.0;
    for (int i = 0; i < framesPerBuffer; i++) {
        if (input[i] > maxSample) {
            maxSample = input[i];
        }
    }
    if (maxSample > PEAK_THRESHOLD) {
        // Peak detected, signal the other thread
        pthread_mutex_lock(&(threadData->mutex));
        sample = maxSample;
        threadData->peakDetected = 1;
        pthread_mutex_unlock(&(threadData->mutex));
    }

    return paContinue;
}

// Function for the thread to wait for peak signal
void waitForPeak(struct ThreadData *threadData) {
    while (1) {
        pthread_mutex_lock(&(threadData->mutex));
        if (threadData->peakDetected) {
            printf("%f!\n",sample);
            threadData->peakDetected = 0;
        }
        pthread_mutex_unlock(&(threadData->mutex));
    }
}

int main() {
    PaError err;
    PaStream *stream;
    struct ThreadData threadData;
    PaStreamParameters inputParameters;
    const PaDeviceInfo *deviceInfo;
    int numDevices, i;

    // Initialize PortAudio
    err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    // Get number of available devices
    numDevices = Pa_GetDeviceCount();
    if (numDevices < 0) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(numDevices));
        Pa_Terminate();
        return 1;
    }

    // List available input devices
    printf("Available input devices:\n");
    for (i = 0; i < numDevices; i++) {
        deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo->maxInputChannels > 0) {
            printf("%d: %s\n", i, deviceInfo->name);
        }
    }

    // Select an input device
    printf("Select an input device: ");
    scanf("%d", &i);

    if (i < 0 || i >= numDevices) {
        fprintf(stderr, "Invalid device number\n");
        Pa_Terminate();
        return 1;
    }

    // Set input parameters
    inputParameters.device = i;
    inputParameters.channelCount = NUM_CHANNELS;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    // Open an audio stream
    err = Pa_OpenStream(&stream, &inputParameters, NULL,
                                SAMPLE_RATE, FRAME_SIZE, paClipOff, processAudio,
                                &threadData);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        return 1;
    }

    // Start the audio stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    // Initialize thread data
    pthread_mutex_init(&(threadData.mutex), NULL);
    threadData.peakDetected = 0;

    // Create thread for peak detection
    pthread_t peak_thread;
    pthread_create(&peak_thread, NULL, (void *)waitForPeak, (void *)&threadData);

    // Wait for user input to stop
    printf("Press Enter to stop...\n");
    getchar();
    getchar(); // Need two getchar() calls to wait for Enter (the first one consumes the previous key press of scanf)

    // Stop and close the audio stream
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
    }
    Pa_CloseStream(stream);
    Pa_Terminate();
    pthread_mutex_destroy(&(threadData.mutex));

    return 0;
}
