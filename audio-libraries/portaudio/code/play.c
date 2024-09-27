/**
 * **********************************
 * ************* play.c ***************
 * **********************************
 * 
 * Reproduce un archivo de audio .raw.
 * Parámetros ajustados en función del programa de grabación.
 * 
 * Versión final
 * ~ rubennmg
 * 
*/

#include <stdio.h>
#include <stdlib.h>
#include "portaudio.h"

#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (256)
#define NUM_CHANNELS (1)
#define INPUT_LATENCY (0.008685)

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
    int frameIndex;
    int maxFrameIndex;
    SAMPLE *recordedSamples;
} paTestData;

static int playCallback(const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo* timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *userData)
{
    paTestData *data = (paTestData*)userData;
    SAMPLE *rptr = &data->recordedSamples[data->frameIndex * NUM_CHANNELS];
    SAMPLE *wptr = (SAMPLE*)outputBuffer;
    unsigned int i;
    int finished;
    unsigned int framesLeft = data->maxFrameIndex - data->frameIndex;

    // Prevent unused variable warnings
    (void) inputBuffer; 
    (void) timeInfo;
    (void) statusFlags;
    (void) userData;

    if (framesLeft < framesPerBuffer)
    {
        for (i = 0; i < framesLeft; i++)
        {
            *wptr++ = *rptr++;
            if (NUM_CHANNELS == 2) *wptr++ = *rptr++;
        }
        for (; i < framesPerBuffer; i++)
        {
            *wptr++ = 0;
            if (NUM_CHANNELS == 2) *wptr++ = 0;
        }
        data->frameIndex += framesLeft;
        finished = paComplete;
    }
    else
    {
        for (i = 0; i < framesPerBuffer; i++)
        {
            *wptr++ = *rptr++;
            if (NUM_CHANNELS == 2) *wptr++ = *rptr++;
        }
        data->frameIndex += framesPerBuffer;
        finished = paContinue;
    }
    return finished;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("USE: %s <file.raw>\n", argv[0]);
        return 1;
    }

    const char *filePath = argv[1];
    FILE *file = fopen(filePath, "rb");
    if (!file) {
        fprintf(stderr, "Could not open file: %s\n", filePath);
        return 1;
    }

    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;
    paTestData data;
    int i;

    // Determine the size of the file
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Calculate the number of samples in the file
    int numSamples = fileSize / sizeof(SAMPLE);
    data.maxFrameIndex = numSamples / NUM_CHANNELS;
    data.frameIndex = 0;

    data.recordedSamples = (SAMPLE *)malloc(fileSize);
    if (data.recordedSamples == NULL)
    {
        fprintf(stderr, "Could not allocate memory for the sample array.\n");
        fclose(file);
        return 1;
    }

    size_t bytesRead = fread(data.recordedSamples, sizeof(SAMPLE), numSamples, file);
    fclose(file);
    if (bytesRead < (size_t)numSamples)
    {
        fprintf(stderr, "File read incomplete: %d samples were expected, %zu were read\n", numSamples, bytesRead);
        free(data.recordedSamples);
        return 1;
    }

    // Initialize PortAudio
    err = Pa_Initialize();
    if (err != paNoError) goto error;

    // Output device initialization
    outputParameters.device = Pa_GetDefaultOutputDevice();
    if (outputParameters.device == paNoDevice) {
        fprintf(stderr, "Error: No default output device.\n");
        goto error;
    }
    outputParameters.channelCount = NUM_CHANNELS;
    outputParameters.sampleFormat = PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = INPUT_LATENCY;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    // Open audio stream
    err = Pa_OpenStream(
              &stream,
              NULL,
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,
              playCallback,
              &data);
    if (err != paNoError) goto error;

    // Start audio stream
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

    // Terminate PortAudio
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
