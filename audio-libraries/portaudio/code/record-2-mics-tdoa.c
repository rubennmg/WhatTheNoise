#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include "portaudio.h"

#define NUM_MICROPHONES (2)
#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (512)
#define NUM_SECONDS (5)
#define NUM_CHANNELS (1)
#define WRITE_TO_FILE (1)
#define SOUND_SPEED (343) 
#define MIC_DISTANCE (0.5) // 50 cm

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
    double timeStamp;
    int identifier; /* microphone/thread identifier */
} MicData;

typedef struct
{
    PaStream *stream;
    MicData *data;
    PaStreamParameters *inputParameters;
    int identifier; /* microphone/thread identifier */
} ThreadArgs;

typedef struct 
{
    double x;
    double y;
} Point2D;

static int recordCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData)
{
    MicData *data = (MicData *)userData;
    const SAMPLE *rptr = (const SAMPLE *)inputBuffer;
    SAMPLE *wptr = &data->recordedSamples[data->frameIndex * NUM_CHANNELS];
    long framesToCalc;
    long i;
    int finished;
    unsigned long framesLeft = data->maxFrameIndex - data->frameIndex;
    double currentTimeStamp = timeInfo->inputBufferAdcTime; // time when the first sample of the input buffer was captured

    (void)outputBuffer;
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
    
    data->timeStamp = currentTimeStamp; // update timestamp
    data->frameIndex += framesToCalc;

    return finished;
}

void writeToFile(char* filename, MicData data)
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

    // console: stream info
    const PaStreamInfo *streamInfo = Pa_GetStreamInfo(threadArgs->stream);
    printf("Stream info (thread %d): input latency = %.2f ms\n", threadArgs->identifier, streamInfo->inputLatency * 1000.0);
    printf("Stream info (trhead %d): output latency = %.2f ms\n", threadArgs->identifier, streamInfo->outputLatency * 1000.0);
    printf("Stream info (thread %d): sample rate = %.2f Hz\n", threadArgs->identifier, streamInfo->sampleRate);
    printf("Stream info (thread %d): struct version = %d\n", threadArgs->identifier, streamInfo->structVersion);

    // start stream
    err = Pa_StartStream(threadArgs->stream);
    if (err != paNoError)
        goto done;

    // recording loop
    while ((err = Pa_IsStreamActive(threadArgs->stream)) == 1)
    {
        Pa_Sleep(1000);
        printf("index (%d, thread: %d)  = %d\n", i, threadArgs->identifier, threadArgs->data->frameIndex);
        fflush(stdout);
        i++;
    }

    // close stream
    err = Pa_CloseStream(threadArgs->stream);

    if (err != paNoError)
        goto done;

    // write data to file
    if (WRITE_TO_FILE)
    {
        char filename[20];
        sprintf(filename, "recorded-mic%d.raw", threadArgs->identifier);
        writeToFile(filename, *threadArgs->data);
    }

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

void initMicData(MicData *data, int *totalFrames, int *numSamples, int *numBytes, int identifier)
{
    *totalFrames = NUM_SECONDS * SAMPLE_RATE;
    data->maxFrameIndex = *totalFrames;
    data->frameIndex = 0;
    *numSamples = *totalFrames * NUM_CHANNELS;
    *numBytes = *numSamples * sizeof(SAMPLE);
    data->recordedSamples = (SAMPLE *)malloc(*numBytes);
    data->timeStamp = 0.0;
    data->identifier = identifier;
    
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

void estimateSoundLocationTDOA(MicData *data1, MicData *data2)
{
    double TDOA = (data2->timeStamp - data1->timeStamp) * SAMPLE_RATE;
    double deltaDistance = TDOA / SAMPLE_RATE * SOUND_SPEED;
    double angle = atan2(MIC_DISTANCE, deltaDistance);

    printf("Estimated sound location: (%.2f, %.2f)\n", deltaDistance * cos(angle), deltaDistance * sin(angle));
}

int main(void)
{
    PaStreamParameters inputParameters[NUM_MICROPHONES];
    PaError err = paNoError;
    MicData data[NUM_MICROPHONES];
    int totalFrames;
    int numSamples;
    int numBytes;
    int i; 
    const PaDeviceInfo *deviceInfo;

    // console info
    printf("Sample rate: %d\n", SAMPLE_RATE);
    printf("Frames per buffer: %d\n", FRAMES_PER_BUFFER);
    printf("Number of channels: %d\n", NUM_CHANNELS);
    printf("Sample type: %d\n", PA_SAMPLE_TYPE);

    // initialize data
    for (i = 0; i < NUM_MICROPHONES; i++)
        initMicData(&data[i], &totalFrames, &numSamples, &numBytes, i);

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
    for (i = 0; i < NUM_MICROPHONES; i++)
        initPaStreamParams(&inputParameters[i], i + 1); /* microphone #1 and #2 */

    // create threads to open and start streams
    pthread_t threads[NUM_MICROPHONES];
    ThreadArgs args[NUM_MICROPHONES];

    for (int i = 0; i < NUM_MICROPHONES; i++) 
    {
        args[i].data = &data[i];
        args[i].inputParameters = &inputParameters[i];
        args[i].identifier = i;
        pthread_create(&threads[i], NULL, recordThread, &args[i]);
    }

    // wait for threads to finish recording
    for (int i = 0; i < NUM_MICROPHONES; i++) 
        pthread_join(threads[i], NULL);

done:
    Pa_Terminate();
    
    for (i = 0; i < NUM_MICROPHONES; i++)
        if (data[i].recordedSamples)
            free(data[i].recordedSamples);

    if (err != paNoError)
    {
        fprintf(stderr, "An error occured while using the portaudio stream\n");
        fprintf(stderr, "Error number: %d\n", err);
        fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
        err = 1;
    }
    return err;
}