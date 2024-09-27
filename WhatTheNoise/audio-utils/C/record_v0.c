/**
 * ************************
 * ****** record_v0.c ********
 * ************************
 *
 * Records audio into two microphones simultaneously when detected above a threshold.
 * Stops recording after a period of silence.
 * Made with PortAudio.
 * This version does not use threads.
 *
 * ~ Author: rubennmg
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "portaudio.h"

/**
 * @brief Constants
 * 
 * Parameters for the audio recording. Chosen according to the best 
 * combination for the USB microphones used in testing.
 */
#define SAMPLE_RATE (44100)
#define SAMPLE_FORMAT (paInt16)
#define FRAMES_PER_BUFFER (128)
#define NUM_CHANNELS (1)
#define THRESHOLD (0.04)         // % of max amplitude value
#define MAX_SILENCE_SECONDS (1)  // seconds
#define INPUT_LATENCY (0.008685) // low input latency
#define MIC1_INDEX (3)           // USB PnP Sound Device: Audio (hw:2,0) --> Configure your own devices
#define MIC2_INDEX (4)           // USB PnP Sound Device: Audio (hw:3,0) --> Configure your own devices

/**
 * @brief Struct to hold data
 * 
 * Stores data for each microphone recording.
 */
typedef struct
{
    int recording;              /**< Mic is recording or not */ 
    time_t lastRecordedTime; /**< Last time audio was recorded */
    int fileIndex;              /**< Index of the current audio file */
    char fileName[100];       /**< Name of the current audio file */
    char micName[20];        /**< Name of the microphone */
} paData;

/**
 * @brief Function to open and initialize one file for recording
 * 
 * @param data struct with data for the microphone
 * @return FILE pointer to the opened file
 */
FILE *openFileForRecording(paData *data)
{
    data->fileIndex++;
    sprintf(data->fileName, "samples_%s/samples_%s_%d.raw", data->micName, data->micName, data->fileIndex);
    FILE *file = fopen(data->fileName, "wb");
    if (file == NULL)
    {
        fprintf(stderr, "Could not open file %s.\n", data->fileName);
        return NULL;
    }
    printf("Starting new recording: %s\n", data->fileName);
    return file;
}

/**
 * @brief Function to handle the recording logic for each microphone
 * 
 * This function is called by PortAudio when audio is detected.
 * 
 * @param inputBuffer audio buffer when audio is captured
 * @param outputBuffer audio buffer when audio is played
 * @param framesPerBuffer number of frames per buffer
 * @param timeInfo struct with time information
 * @param statusFlags flags for the callback
 * @param userData 
 * @return int status of the callback
 */
static int recordCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    paData *data = (paData *)userData;
    const short *in = (const short *)inputBuffer;
    FILE *file = NULL;
    int aboveThreshold = 0;

    // Avoid unusued warnings
    (void)outputBuffer;
    (void)timeInfo;
    (void)statusFlags;

    for (int i = 0; i < framesPerBuffer * NUM_CHANNELS; i++)
    {
        if (in[i] > THRESHOLD * 32767.0 || in[i] < -THRESHOLD * 32767.0)
        {
            aboveThreshold = 1;
            break;
        }
    }

    if (data->recording)
    {
        file = fopen(data->fileName, "ab");
        if (file == NULL)
        {
            fprintf(stderr, "Could not open file %s.\n", data->fileName);
            return paAbort;
        }
    }

    if (aboveThreshold)
    {
        time(&data->lastRecordedTime);
        if (!data->recording)
        {
            data->recording = 1;
            file = openFileForRecording(data);
            if (file == NULL)
            {
                return paAbort;
            }
        }
    }
    else
    {
        time_t currentTime;
        time(&currentTime);
        if (data->recording && difftime(currentTime, data->lastRecordedTime) > MAX_SILENCE_SECONDS)
        {
            data->recording = 0;
            printf("Stopping recording: %s\n", data->fileName);
        }
    }

    if (data->recording && file != NULL)
    {
        fwrite(inputBuffer, sizeof(short), framesPerBuffer * NUM_CHANNELS, file);
    }

    if (file != NULL)
    {
        fclose(file);
    }

    return paContinue;
}

/**
 * @brief Main function
 * 
 * Initializes mic data for each microphone, initializes Portaudio, opens streams for each microphone,
 * starts recording, stops recording when ENTER is pressed, and closes streams and terminates Portaudio.
 * 
 * @return int output status
 */
int main(void)
{
    PaStreamParameters inputParametersMic1, inputParametersMic2;
    PaStream *streamMic1, *streamMic2;
    PaError err;
    paData dataMic1, dataMic2;

    dataMic1.recording = 0;
    dataMic1.fileIndex = 0;
    strcpy(dataMic1.micName, "Mic1");
    time(&dataMic1.lastRecordedTime);

    dataMic2.recording = 0;
    dataMic2.fileIndex = 0;
    strcpy(dataMic2.micName, "Mic2");
    time(&dataMic2.lastRecordedTime);

    err = Pa_Initialize();
    if (err != paNoError)
    {
        fprintf(stderr, "Error initializing PortAudio: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    inputParametersMic1.device = MIC1_INDEX;
    printf("Recording from device: %s\n", Pa_GetDeviceInfo(inputParametersMic1.device)->name);
    if (inputParametersMic1.device == paNoDevice)
    {
        fprintf(stderr, "No default input device found.\n");
        Pa_Terminate();
        return 1;
    }
    inputParametersMic1.channelCount = NUM_CHANNELS;
    inputParametersMic1.sampleFormat = SAMPLE_FORMAT;
    inputParametersMic1.suggestedLatency = INPUT_LATENCY;
    inputParametersMic1.hostApiSpecificStreamInfo = NULL;

    inputParametersMic2.device = MIC2_INDEX;
    printf("Recording from device: %s\n", Pa_GetDeviceInfo(inputParametersMic2.device)->name);
    if (inputParametersMic2.device == paNoDevice)
    {
        fprintf(stderr, "No default input device found.\n");
        Pa_Terminate();
        return 1;
    }
    inputParametersMic2.channelCount = NUM_CHANNELS;
    inputParametersMic2.sampleFormat = SAMPLE_FORMAT;
    inputParametersMic2.suggestedLatency = INPUT_LATENCY;
    inputParametersMic2.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&streamMic1, &inputParametersMic1, NULL, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, recordCallback, &dataMic1);
    if (err != paNoError)
    {
        fprintf(stderr, "Error opening audio stream: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        return 1;
    }

    err = Pa_OpenStream(&streamMic2, &inputParametersMic2, NULL, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, recordCallback, &dataMic2);
    if (err != paNoError)
    {
        fprintf(stderr, "Error opening audio stream: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        return 1;
    }

    err = Pa_StartStream(streamMic1);
    if (err != paNoError)
    {
        fprintf(stderr, "Error starting recording: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        return 1;
    }

    err = Pa_StartStream(streamMic2);
    if (err != paNoError)
    {
        fprintf(stderr, "Error starting recording: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        return 1;
    }

    printf("Recording in both mics... Press ENTER to stop.\n");
    getchar();

    err = Pa_StopStream(streamMic1);
    if (err != paNoError)
    {
        fprintf(stderr, "Error stopping recording: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        return 1;
    }

    err = Pa_StopStream(streamMic2);
    if (err != paNoError)
    {
        fprintf(stderr, "Error stopping recording: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        return 1;
    }

    err = Pa_CloseStream(streamMic1);
    if (err != paNoError)
    {
        fprintf(stderr, "Error closing stream: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        return 1;
    }

    err = Pa_CloseStream(streamMic2);
    if (err != paNoError)
    {
        fprintf(stderr, "Error closing stream: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        return 1;
    }

    Pa_Terminate();
    printf("Recording finished.\n");

    return 0;
}
