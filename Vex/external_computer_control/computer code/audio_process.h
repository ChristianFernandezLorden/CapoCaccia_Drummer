#ifndef AUDIO_PROCESS_H
#define AUDIO_PROCESS_H

#include <pthread.h>
#include "portaudio.h"
#include "structs.h"

void startPortAudio();

void listInputDevices();

void startAudioStream(PaStream *stream, com_data_t *threadData);

void stopAudioStream(PaStream *stream);

int processAudio(const void *inputBuffer, void *outputBuffer,
				 unsigned long framesPerBuffer,
				 const PaStreamCallbackTimeInfo *timeInfo,
				 PaStreamCallbackFlags statusFlags,
				 void *userData);


#endif // AUDIO_PROCESS_H