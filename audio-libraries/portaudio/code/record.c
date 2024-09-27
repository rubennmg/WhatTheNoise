#include <stdio.h>
#include <stdlib.h>
#include "portaudio.h"

#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (512)
#define NUM_SECONDS (5)
#define NUM_CHANNELS (1)
#define DITHER_FLAG (0)
#define WRITE_TO_FILE (1)

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

int main(void)
{
    PaStreamParameters inputParameters;
    PaStream *stream;
    PaError err = paNoError;
    paData data; /* struct to hold data */
    int i;
    int totalFrames;
    int numSamples;
    int numBytes;
    SAMPLE max, val;
    double average;

    // console info
    printf("Sample rate: %d\n", SAMPLE_RATE);
    printf("Frames per buffer: %d\n", FRAMES_PER_BUFFER);
    printf("Number of channels: %d\n", NUM_CHANNELS);
    printf("Sample type: %d\n", PA_SAMPLE_TYPE);

    // data initialization
    data.maxFrameIndex = totalFrames = NUM_SECONDS * SAMPLE_RATE;
    data.frameIndex = 0;
    numSamples = totalFrames * NUM_CHANNELS;
    numBytes = numSamples * sizeof(SAMPLE);
    data.recordedSamples = (SAMPLE *)malloc(numBytes);
    if (data.recordedSamples == NULL)
    {
        printf("Could not allocate record array.\n");
        goto done;
    }
    for (i = 0; i < numSamples; i++)
        data.recordedSamples[i] = 0;

    // portaudio initialization
    err = Pa_Initialize();
    if (err != paNoError)
        goto done;

    // list available sound apis
    printf("\nAPI COUNT: %d\n", Pa_GetHostApiCount());
    for (i = 0; i < Pa_GetHostApiCount(); i++)
    {
        printf("API NAME: %s\n", Pa_GetHostApiInfo(i)->name);
    }

    // input device parameters initialization
    printf("\nSELECTED INPUT DEVICE: %s\n", Pa_GetDeviceInfo(2)->name); // second usb micorphone
    inputParameters.device = inputParameters.device = 2; // seocnd usb microphone
    if (inputParameters.device == paNoDevice)
    {
        fprintf(stderr, "Error: No default input device.\n");
        goto done;
    }
    inputParameters.channelCount = NUM_CHANNELS;
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    // open audio stream
    err = Pa_OpenStream(
        &stream,
        &inputParameters,
        NULL,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff,
        recordCallback,
        &data);
    if (err != paNoError)
        goto done;

    // start audio stream
    err = Pa_StartStream(stream);
    if (err != paNoError)
        goto done;
    printf("\n=== Now recording!! Please speak into the microphone. ===\n");
    fflush(stdout);

    // wait for stream to finish
    i = 0;
    while ((err = Pa_IsStreamActive(stream)) == 1)
    {
        Pa_Sleep(1000);
        printf("index (%d) = %d\n", i, data.frameIndex);
        fflush(stdout);
        i++;
    }
    if (err < 0)
        goto done;

    // close audio stream
    err = Pa_CloseStream(stream);
    if (err != paNoError)
        goto done;

    // calculate max amplitude and average amplitude
    max = 0;
    average = 0.0;
    for (i = 0; i < numSamples; i++)
    {
        val = data.recordedSamples[i];
        if (val < 0)
            val = -val;
        if (val > max)
        {
            max = val;
        }
        average += val;
    }

    average = average / (double)numSamples;

    printf("Sample max amplitude = " PRINTF_S_FORMAT "\n", max);
    printf("Sample average = %lf\n", average);

// write to file
#if WRITE_TO_FILE
    {
        FILE *fid;
        fid = fopen("recorded.raw", "wb");
        if (fid == NULL)
        {
            printf("Could not open file.");
        }
        else
        {
            fwrite(data.recordedSamples, NUM_CHANNELS * sizeof(SAMPLE), totalFrames, fid);
            fclose(fid);
            printf("Wrote data to 'recorded.raw'\n");
        }
    }
#endif

done:
    Pa_Terminate();
    if (data.recordedSamples)
        free(data.recordedSamples);
    if (err != paNoError)
    {
        fprintf(stderr, "An error occured while using the portaudio stream\n");
        fprintf(stderr, "Error number: %d\n", err);
        fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
        err = 1;
    }
    return err;
}
