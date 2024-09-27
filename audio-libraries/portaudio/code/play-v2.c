#include <stdio.h>
#include <stdlib.h>
#include "portaudio.h"

#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (512)
#define NUM_CHANNELS (1)

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

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("USE: %s <file.raw>\n", argv[0]);
        return 1;
    }

    const char *filePath = argv[1];
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err = paNoError;
    int i;

    // console info
    printf("Sample rate: %d\n", SAMPLE_RATE);
    printf("Frames per buffer: %d\n", FRAMES_PER_BUFFER);
    printf("Number of channels: %d\n", NUM_CHANNELS);
    printf("Sample type: %d\n", PA_SAMPLE_TYPE);

    // portaudio initialization
    err = Pa_Initialize();
    if (err != paNoError)
        goto done;

    // output device parameters initialization
    outputParameters.device = Pa_GetDefaultOutputDevice(); // use default output device
    if (outputParameters.device == paNoDevice)
    {
        fprintf(stderr, "Error: No default output device.\n");
        goto done;
    }
    outputParameters.channelCount = NUM_CHANNELS;
    outputParameters.sampleFormat = PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    // open audio stream
    err = Pa_OpenStream(
        &stream,
        NULL,
        &outputParameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff,
        NULL,
        NULL);
    if (err != paNoError)
        goto done;

    // start audio stream
    err = Pa_StartStream(stream);
    if (err != paNoError)
        goto done;
    printf("\n=== Now playing audio from file. ===\n");
    fflush(stdout);

    // open audio file
    FILE *file = fopen(filePath, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Error opening file for reading.\n");
        return 1;
    }

    // read and play audio data from file
    SAMPLE buffer[FRAMES_PER_BUFFER];
    while (fread(buffer, sizeof(SAMPLE), FRAMES_PER_BUFFER, file) == FRAMES_PER_BUFFER)
    {
        err = Pa_WriteStream(stream, buffer, FRAMES_PER_BUFFER);
        if (err != paNoError)
        {
            fprintf(stderr, "Error playing audio: %s\n", Pa_GetErrorText(err));
            break;
        }
    }

    // close file
    fclose(file);

    // stop audio stream
    err = Pa_StopStream(stream);
    if (err != paNoError)
        goto done;

    // close audio stream
    err = Pa_CloseStream(stream);
    if (err != paNoError)
        goto done;

    // terminate portaudio
    Pa_Terminate();

done:
    if (err != paNoError)
    {
        fprintf(stderr, "An error occurred while using the portaudio stream\n");
        fprintf(stderr, "Error number: %d\n", err);
        fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
        err = 1;
    }
    return err;
}
