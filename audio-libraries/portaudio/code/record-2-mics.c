#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "portaudio.h"

#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (512)
#define NUM_SECONDS (5)
#define NUM_CHANNELS (1)
#define WRITE_TO_FILE (1)

pthread_mutex_t mutex_start = PTHREAD_MUTEX_INITIALIZER;

/* select sample format */
#if 0
#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;
#define SAMPLE_SILENCE  (0.0f)
#define PRINTF_S_FORMAT "%.8f"
#elif 1
#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#elif 0
#define PA_SAMPLE_TYPE  paInt8
typedef char SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#else
#define PA_SAMPLE_TYPE  paUInt8
typedef unsigned char SAMPLE;
#define SAMPLE_SILENCE  (128)
#define PRINTF_S_FORMAT "%d"
#endif

typedef struct
{
    int frameIndex; /* index into sample array */
    int maxFrameIndex;
    SAMPLE *recordedSamples;
} paData;

typedef struct
{
    PaStream *stream;
    paData *data;
    PaStreamParameters *inputParameters;
} ThreadArgs;

static int recordCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData)
{
    paData *data = (paData *)userData;
    const SAMPLE *rptr = (const SAMPLE *)inputBuffer;
    SAMPLE *wptr = &data->recordedSamples[data->frameIndex * NUM_CHANNELS];
    long framesToCalc;
    long i;
    int finished;
    unsigned long framesLeft = data->maxFrameIndex - data->frameIndex;

    (void)outputBuffer;
    (void)timeInfo;
    (void)statusFlags;
    (void)userData;

    if (framesLeft < framesPerBuffer)
    {
        framesToCalc = framesLeft;
        finished = paComplete;
    }
    else
    {
        framesToCalc = framesPerBuffer;
        finished = paContinue;
    }

    if (inputBuffer == NULL)
    {
        for (i = 0; i < framesToCalc; i++)
        {
            *wptr++ = SAMPLE_SILENCE;
            if (NUM_CHANNELS == 2)
                *wptr++ = SAMPLE_SILENCE;
        }
    }
    else
    {
        for (i = 0; i < framesToCalc; i++)
        {
            *wptr++ = *rptr++;
            if (NUM_CHANNELS == 2)
                *wptr++ = *rptr++;
        }
    }
    
    data->frameIndex += framesToCalc;

    return finished;
}

void *recordThread(void *args)
{
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    PaError err = paNoError;
    FILE *fid;
    int i = 0;

    // open stream
    err = Pa_OpenStream(
        &threadArgs->stream,
        threadArgs->inputParameters,
        NULL,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff,
        recordCallback,
        threadArgs->data);
    if (err != paNoError)
        goto done;

    // start stream
    err = Pa_StartStream(threadArgs->stream);
    if (err != paNoError)
        goto done;

    pthread_mutex_lock(&mutex_start);

    // recording loop
    while ((err = Pa_IsStreamActive(threadArgs->stream)) == 1)
    {
        Pa_Sleep(1000);
        printf("index (%d)  = %d\n", i, threadArgs->data->frameIndex);
        fflush(stdout);
        i++;
    }

    pthread_mutex_unlock(&mutex_start);

    // close stream
    err = Pa_CloseStream(threadArgs->stream);
    if (err != paNoError)
        goto done;

    // end thread
    pthread_exit(NULL);

done:
    Pa_Terminate();

    if (threadArgs->data->recordedSamples)
        free(threadArgs->data->recordedSamples);

    if (err != paNoError)
    {
        fprintf(stderr, "An error occured while using the portaudio stream\n");
        fprintf(stderr, "Error number: %d\n", err);
        fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
        err = 1;
    }
}

void writeToFile(char* filename, paData data)
{
    FILE *fid;

    fid = fopen(filename, "wb");
    if (fid == NULL)
    {
        printf("Could not open file.");
    }
    else
    {
        fwrite(data.recordedSamples, NUM_CHANNELS * sizeof(SAMPLE), data.maxFrameIndex, fid);
        fclose(fid);
        printf("Wrote data to '%s'.\n", filename);
    }
}

void initPaData(paData *data, int *totalFrames, int *numSamples, int *numBytes)
{
    *totalFrames = NUM_SECONDS * SAMPLE_RATE;
    data->maxFrameIndex = *totalFrames;
    data->frameIndex = 0;
    *numSamples = *totalFrames * NUM_CHANNELS;
    *numBytes = *numSamples * sizeof(SAMPLE);
    data->recordedSamples = (SAMPLE *)malloc(*numBytes);
    
    if (data->recordedSamples == NULL)
    {
        printf("Could not allocate record array.\n");
        exit(1);
    }
    
    for (int i = 0; i < *numSamples; i++)
        data->recordedSamples[i] = 0;
}

void initPaStreamParams(PaStreamParameters *inputParameters, PaDeviceIndex device)
{
    inputParameters->device = device;
    if (inputParameters->device == paNoDevice)
    {
        fprintf(stderr, "Error: No input device was found.\n");
        exit(1); // goto done
    }
    inputParameters->channelCount = NUM_CHANNELS;
    inputParameters->sampleFormat = PA_SAMPLE_TYPE;
    inputParameters->suggestedLatency = Pa_GetDeviceInfo(inputParameters->device)->defaultLowInputLatency;
    inputParameters->hostApiSpecificStreamInfo = NULL;
}

int main(void)
{
    PaStreamParameters inputParameters1, inputParameters2;
    PaError err = paNoError;
    paData data1, data2; /* struct to hold data */
    int totalFrames;
    int numSamples;
    int numBytes;
    const PaDeviceInfo *deviceInfo;

    // console info
    printf("Sample rate: %d\n", SAMPLE_RATE);
    printf("Frames per buffer: %d\n", FRAMES_PER_BUFFER);
    printf("Number of channels: %d\n", NUM_CHANNELS);
    printf("Sample type: %d\n", PA_SAMPLE_TYPE);

    // initialize data
    initPaData(&data1, &totalFrames, &numSamples, &numBytes);
    initPaData(&data2, &totalFrames, &numSamples, &numBytes);

    // initialize portaudio
    err = Pa_Initialize();
    if (err != paNoError)
        goto done;

    // selectded input devices info
    deviceInfo = Pa_GetDeviceInfo(1);
    printf("\nDEVICE NAME (i = 1): %s\n", deviceInfo->name);
    deviceInfo = Pa_GetDeviceInfo(2);
    printf("DEVICE NAME (i = 2): %s\n", deviceInfo->name);

    // initialize input parameters
    initPaStreamParams(&inputParameters1, 1); /* microphone #1 */
    initPaStreamParams(&inputParameters2, 2); /* microphone #2 */

    // create threads to open and start streams
    pthread_t thread1, thread2;
    ThreadArgs args1 = {NULL, &data1, &inputParameters1};
    ThreadArgs args2 = {NULL, &data2, &inputParameters2};

    pthread_create(&thread1, NULL, recordThread, &args1);
    pthread_create(&thread2, NULL, recordThread, &args2);

    // wait for threads to finish recording
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    // write data to file
    if (WRITE_TO_FILE)
    {
        writeToFile("recorded-mic1.raw", data1);
        writeToFile("recorded-mic2.raw", data2);
    }

done:
    Pa_Terminate();
    
    if (data1.recordedSamples)
        free(data1.recordedSamples);
    if (data2.recordedSamples)
        free(data2.recordedSamples);

    if (err != paNoError)
    {
        fprintf(stderr, "An error occured while using the portaudio stream\n");
        fprintf(stderr, "Error number: %d\n", err);
        fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
        err = 1;
    }
    return err;
}