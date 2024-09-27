#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "portaudio.h"

#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (128)
#define NUM_CHANNELS (1)
#define NUM_MICROPHONES (2)

/* select sample format */
#if 0
#define PA_SAMPLE_TYPE paFloat32
typedef float SAMPLE;
#define SAMPLE_SILENCE (0.0f)
#define PRINTF_S_FORMAT "%.8f"
#elif 1
#define PA_SAMPLE_TYPE paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE (0)
#define PRINTF_S_FORMAT "%d"
#elif 0
#define PA_SAMPLE_TYPE paInt8
typedef char SAMPLE;
#define SAMPLE_SILENCE (0)
#define PRINTF_S_FORMAT "%d"
#else
#define PA_SAMPLE_TYPE paUInt8
typedef unsigned char SAMPLE;
#define SAMPLE_SILENCE (128)
#define PRINTF_S_FORMAT "%d"
#endif

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;

typedef struct
{
    int micIndex;
    PaStream *stream;
    int frameIndex;
    int maxFrameIndex;
    SAMPLE *buffer;
} paData;

void *recording_thread(void *args)
{
    paData *data = (paData *)args;
    int err, numSamples, ret;
    FILE *file;
    char filename[20];

    sprintf(filename, "audio-%d.raw", data->micIndex);

    ret = pthread_mutex_lock(&mutex1);
    if (ret != 0)
    {
        fprintf(stderr, "Error locking mutex.\n");
        return NULL;
    }

    // start recording
    err = Pa_StartStream(data->stream);
    if (err != paNoError)
    {   
        fprintf(stderr, "Error starting stream: %s\n", Pa_GetErrorText(err));
        return NULL;
    }
    printf("\n=== [mic: %d] Now recording!! Please speak into the microphone. ===\n", data->micIndex);
    fflush(stdout);

    ret = pthread_mutex_unlock(&mutex1);
    if (ret != 0)
    {
        fprintf(stderr, "Error unlocking mutex.\n");
        return NULL;
    }

    while (1)
    {
        // read data from microphone
        err = Pa_ReadStream(data->stream, data->buffer, FRAMES_PER_BUFFER);
        if (err != paNoError)
        {
            fprintf(stderr, "Error reading data from microphone: %s\n", Pa_GetErrorText(err));
            break;
        }

        ret = pthread_mutex_lock(&mutex2);
        if (ret != 0)
        {
            fprintf(stderr, "Error locking mutex.\n");
            break;
        }

        // open file for writing (append and binary mode)
        file = fopen(filename, "ab");
        if (file == NULL)
        {
            fprintf(stderr, "Error opening file for writing.\n");
            return NULL;
        }

        // write data to file
        fwrite(data->buffer, sizeof(SAMPLE), FRAMES_PER_BUFFER, file);

        // close file
        fclose(file);

        ret = pthread_mutex_unlock(&mutex2);
        if (ret != 0)
        {
            fprintf(stderr, "Error unlocking mutex.\n");
            break;
        }
    }

    // stop recording
    err = Pa_StopStream(data->stream);
    if (err != paNoError)
    {
        fprintf(stderr, "Error stopping stream: %s\n", Pa_GetErrorText(err));
    }
}

int main(void)
{
    PaStreamParameters inputParameters[NUM_MICROPHONES];
    PaStream *stream[NUM_MICROPHONES];
    paData data[NUM_MICROPHONES];
    pthread_t threads[NUM_MICROPHONES];
    PaError err = paNoError;
    int i, ret;
    int totalFrames;
    int numSamples;
    int numBytes;

    // console info
    printf("Sample rate: %d\n", SAMPLE_RATE);
    printf("Frames per buffer: %d\n", FRAMES_PER_BUFFER);
    printf("Number of channels: %d\n", NUM_CHANNELS);
    printf("Sample type: %d\n", PA_SAMPLE_TYPE);

    // portaudio initialization
    err = Pa_Initialize();
    if (err != paNoError)
    {
        fprintf(stderr, "Error initializing portaudio: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    // initialize input parameters
    for (i = 0; i < NUM_MICROPHONES; i++)
    {
        inputParameters[i].device = i + 1; // first and second microphones
        inputParameters[i].channelCount = NUM_CHANNELS;
        inputParameters[i].sampleFormat = paInt16;
        inputParameters[i].suggestedLatency = Pa_GetDeviceInfo(inputParameters[i].device)->defaultLowInputLatency;
        inputParameters[i].hostApiSpecificStreamInfo = NULL;
    }

    // open audio streams
    for (i = 0; i < NUM_MICROPHONES; i++)
    {
        err = Pa_OpenStream(&stream[i], &inputParameters[i], NULL, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, NULL, NULL);
        if (err != paNoError)
        {
            fprintf(stderr, "Error opening stream %d: %s\n", i, Pa_GetErrorText(err));
            return 1;
        } else {
            printf("\n=== [mic: %d] Stream opened. ===\n", i);
        }
    }

    // initialize mutex
    ret = pthread_mutex_init(&mutex1, NULL);
    if (ret != 0)
    {
        fprintf(stderr, "Error initializing mutex.\n");
        return 1;
    }
    
    ret = pthread_mutex_init(&mutex2, NULL);
    if (ret != 0)
    {
        fprintf(stderr, "Error initializing mutex.\n");
        return 1;
    }

    // initialize data structures
    for (i = 0; i < NUM_MICROPHONES; i++)
    {
        data[i].micIndex = i;
        data[i].frameIndex = 0;
        data[i].stream = stream[i];
        data[i].buffer = malloc(sizeof(SAMPLE) * FRAMES_PER_BUFFER);
    }

    // create recording threads
    for (i = 0; i < NUM_MICROPHONES; i++)
    {
        ret = pthread_create(&threads[i], NULL, (void *)recording_thread, &data[i]);
        if (ret != 0)
        {
            fprintf(stderr, "Error creating thread %d.\n", i);
            return 1;
        }
    }

    // wait for threads to finish
    for (i = 0; i < NUM_MICROPHONES; i++)
    {
        ret = pthread_join(threads[i], NULL);
        if (ret != 0)
        {
            fprintf(stderr, "Error joining thread %d.\n", i);
            return 1;
        }
    }

    // close streams
    for (i = 0; i < NUM_MICROPHONES; i++)
    {
        err = Pa_CloseStream(stream[i]);
        if (err != paNoError)
        {
            fprintf(stderr, "Error closing stream %d: %s\n", i, Pa_GetErrorText(err));
        }
    }

    // free memory
    for (i = 0; i < NUM_MICROPHONES; i++)
    {
        free(data[i].buffer);
    }

    // destroy mutex
    ret = pthread_mutex_destroy(&mutex1);
    if (ret != 0)
    {
        fprintf(stderr, "Error destroying mutex.\n");
        return 1;
    }
    
    ret = pthread_mutex_destroy(&mutex2);
    if (ret != 0)
    {
        fprintf(stderr, "Error destroying mutex.\n");
        return 1;
    }

    // terminate portaudio
    Pa_Terminate();

    return 0;
}
