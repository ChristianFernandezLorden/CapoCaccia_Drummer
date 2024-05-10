#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "portaudio.h"
#include "audio_process.h"
#include "structs.h"

#define SAMPLE_RATE 192000 // 192 kHz for the amplifier used
#define NUM_CHANNELS 1 // 1 for standard microphone, 2 in the case of the amplifier
#define FRAME_SIZE 512 // Number of samples per frame
#define PEAK_THRESHOLD 0.1 // Threshold for peak detection, experimentally determined
#define AUDIO_DEVICE 1 // Device number for the audio interface

// Function to process audio samples
int processAudio(const void *inputBuffer, void *outputBuffer,
                 unsigned long framesPerBuffer,
                 const PaStreamCallbackTimeInfo *timeInfo,
                 PaStreamCallbackFlags statusFlags,
                 void *userData) {
    com_data_t *threadData = (com_data_t *)userData;
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
        pthread_mutex_lock(threadData->com_mutex);
        threadData->has_new_data[0] = 1;
        threadData->in[0] = maxSample;
        pthread_mutex_unlock(threadData->com_mutex);
    }

    return paContinue;
}

void startPortAudio()
{
	PaError err;
	err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        return;
    }
}

void listInputDevices()
{
	const PaDeviceInfo *deviceInfo;
	int numDevices = Pa_GetDeviceCount();
    if (numDevices < 0) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(numDevices));
        Pa_Terminate();
        return;
    }

    // List available input devices
    printf("Available input devices:\n");
    for (int i = 0; i < numDevices; i++) {
        deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo->maxInputChannels > 0) {
            printf("%d: %s\n", i, deviceInfo->name);
        }
    }
}

void startAudioStream(PaStream *stream, com_data_t *threadData)
{
	PaError err;
	PaStreamParameters inputParameters;
	const PaDeviceInfo *deviceInfo;

    threadData->nb_in = 0;
    threadData->in = malloc(threadData->nb_in * sizeof(double));
    threadData->nb_out = 1;
    threadData->out = malloc(threadData->nb_out * sizeof(double));
    threadData->has_new_data = malloc(2*sizeof(char));
    threadData->has_new_data[0] = 0;

	// Set up input parameters
	inputParameters.device = AUDIO_DEVICE;
	inputParameters.channelCount = NUM_CHANNELS;
	inputParameters.sampleFormat = paFloat32;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	// Open the audio stream
	err = Pa_OpenStream(&stream, &inputParameters, NULL, SAMPLE_RATE, FRAME_SIZE, paClipOff, processAudio, threadData);
	if (err != paNoError)
	{
		fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        return;
	}

	// Start the audio stream
	err = Pa_StartStream(stream);
	if (err != paNoError)
	{
		fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        Pa_Terminate();
        return;
	}
}

void stopAudioStream(PaStream *stream)
{
	PaError err;
	err = Pa_StopStream(stream);
	if (err != paNoError)
	{
		 fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
		return;
	}
	Pa_CloseStream(stream);
    Pa_Terminate();
}