#include <stdio.h>
#include <stdlib.h>
#include "portaudio.h"

#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (512)
#define NUM_SECONDS (5)
#define NUM_CHANNELS (2)
#define PA_SAMPLE_TYPE paFloat32
typedef float SAMPLE;

typedef struct
{
    int frameIndex; /* Index into sample array. */
    int maxFrameIndex;
    SAMPLE *recordedSamples;
} paTestData;

static int playCallback(const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *userData)
{
    paTestData *data = (paTestData *)userData;
    SAMPLE *rptr = &data->recordedSamples[data->frameIndex * NUM_CHANNELS];
    SAMPLE *wptr = (SAMPLE *)outputBuffer;
    unsigned long i;
    int finished = paContinue;
    unsigned int framesLeft = data->maxFrameIndex - data->frameIndex;

    if (framesLeft < framesPerBuffer)
    {
        for (i = 0; i < framesLeft; i++)
        {
            *wptr++ = *rptr++; /* Left channel */
            *wptr++ = *rptr++; /* Right channel */
        }
        for (; i < framesPerBuffer; i++)
        {
            *wptr++ = 0; /* Left channel */
            *wptr++ = 0; /* Right channel */
        }
        data->frameIndex += framesLeft;
        finished = paComplete;
    }
    else
    {
        for (i = 0; i < framesPerBuffer; i++)
        {
            *wptr++ = *rptr++; /* Left channel */
            *wptr++ = *rptr++; /* Right channel */
        }
        data->frameIndex += framesPerBuffer;
    }
    return finished;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Uso: %s <archivo.raw>\n", argv[0]);
        return 1;
    }

    const char *filePath = argv[1];
    FILE *file = fopen(filePath, "rb");
    if (!file) {
        fprintf(stderr, "No se pudo abrir el archivo: %s\n", filePath);
        return 1;
    }

    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;
    paTestData data;

    data.maxFrameIndex = NUM_SECONDS * SAMPLE_RATE; /* Total number of frames to play */
    data.frameIndex = 0;
    int numSamples = data.maxFrameIndex * NUM_CHANNELS; /* Total number of samples */
    int numBytes = numSamples * sizeof(SAMPLE); /* Size of the sample array in bytes */
    data.recordedSamples = (SAMPLE *)malloc(numBytes); /* Allocate memory for the sample array */
    if (data.recordedSamples == NULL)
    {
        fprintf(stderr, "Could not allocate record array.\n");
        fclose(file);
        return 1;
    }

    size_t bytesRead = fread(data.recordedSamples, sizeof(SAMPLE), numSamples, file); /* Read samples from file */
    fclose(file);
    if (bytesRead < (size_t)numSamples)
    {
        fprintf(stderr, "Archivo leÃ­do incompleto: se esperaban %d muestras, se leyeron %zu\n", numSamples, bytesRead);
        free(data.recordedSamples);
        return 1;
    }

    err = Pa_Initialize();
    if (err != paNoError) goto error;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* Default output device */
    if (outputParameters.device == paNoDevice) {
        fprintf(stderr, "Error: No default output device.\n");
        goto error;
    }
    outputParameters.channelCount = NUM_CHANNELS; /* Stereo output */
    outputParameters.sampleFormat = PA_SAMPLE_TYPE; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &stream,
              NULL, /* No input */
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff, /* We won't output out of range samples so don't bother clipping them */
              playCallback,
              &data);
    if (err != paNoError) goto error;

    if (stream) {
        err = Pa_StartStream(stream);
        if (err != paNoError) goto error;

        printf("Waiting for playback to finish.\n");

        while ((err = Pa_IsStreamActive(stream)) == 1) Pa_Sleep(100);
        if (err < 0) goto error;

        err = Pa_CloseStream(stream);
        if (err != paNoError) goto error;

        printf("Playback finished.\n");
    }

    Pa_Terminate();
    free(data.recordedSamples);

    return err;

error:
    Pa_Terminate();
    fprintf(stderr, "An error occurred while using the PortAudio stream\n");
    fprintf(stderr, "Error number: %d\n", err);
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
    if (data.recordedSamples) free(data.recordedSamples);
    return err;
}