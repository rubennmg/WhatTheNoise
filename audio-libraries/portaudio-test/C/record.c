#include <stdio.h>
#include <stdlib.h>
#include "portaudio.h"

#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (512)
#define NUM_SECONDS (5)
#define NUM_CHANNELS (2)
#define DITHER_FLAG (0)
#define WRITE_TO_FILE (1)

#define PA_SAMPLE_TYPE paFloat32
typedef float SAMPLE;
#define SAMPLE_SILENCE (0.0f)
#define PRINTF_S_FORMAT "%.8f"

typedef struct
{
    int frameIndex; /* Index into sample array. */
    int maxFrameIndex;
    SAMPLE *recordedSamples;
} paTestData;

static int recordCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData)
{
    paTestData *data = (paTestData *)userData;
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

int main(void);

int main(void)
{
    PaStreamParameters inputParameters,
        outputParameters;
    PaStream *stream;
    PaError err = paNoError;
    paTestData data; // struct to hold data
    int i;
    int totalFrames;
    int numSamples;
    int numBytes;
    SAMPLE max, val;
    double average;

    printf("patest_record.c\n");
    fflush(stdout);

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

    err = Pa_Initialize();
    if (err != paNoError)
        goto done;

    inputParameters.device = Pa_GetDefaultInputDevice();
    if (inputParameters.device == paNoDevice)
    {
        fprintf(stderr, "Error: No default input device.\n");
        goto done;
    }
    inputParameters.channelCount = 2;
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

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

    err = Pa_StartStream(stream);
    if (err != paNoError)
        goto done;
    printf("\n=== Now recording!! Please speak into the microphone. ===\n");
    fflush(stdout);

    while ((err = Pa_IsStreamActive(stream)) == 1)
    {
        Pa_Sleep(1000);
        printf("index = %d\n", data.frameIndex);
        fflush(stdout);
    }
    if (err < 0)
        goto done;

    err = Pa_CloseStream(stream);
    if (err != paNoError)
        goto done;

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

    printf("sample max amplitude = " PRINTF_S_FORMAT "\n", max);
    printf("sample average = %lf\n", average);

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
