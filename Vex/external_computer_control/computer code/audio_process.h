#ifndef AUDIO_PROCESS_H
#define AUDIO_PROCESS_H

#include <pthread.h>

typedef struct audio_data_t {
	pthread_mutex_t* com_mutex;
	int peakDetected;
} audio_data_t;

void startPortAudio();

void listInputDevices();

void startAudioStream(PaStream *stream, struct audio_data_t *threadData);

void stopAudioStream(PaStream *stream);

int processAudio(const void *inputBuffer, void *outputBuffer,
				 unsigned long framesPerBuffer,
				 const PaStreamCallbackTimeInfo *timeInfo,
				 PaStreamCallbackFlags statusFlags,
				 void *userData);

void waitForPeak(struct audio_data_t *threadData);

#endif // AUDIO_PROCESS_H