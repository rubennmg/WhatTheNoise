#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>

#define SAMPLE_RATE  (44100)
#define FRAMES_PER_BUFFER (64)

static int audioCallback(const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo *timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData) {
    FILE *file = (FILE *)userData;
    const float *input = (const float*)inputBuffer;

    fwrite(input, sizeof(float), framesPerBuffer, file);

    return paContinue;
}

int main() 
{
    PaError err;
    PaStream *stream;
    FILE *file;

    file = fopen("audio.raw", "wb");
    if (file == NULL) 
    {
        printf("Error: could not open file for writing [0]\n");
        return 1;
    }

    err = Pa_Initialize();
    if (err != paNoError) 
    {
        printf("PortAudio error [1]: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    err = Pa_OpenDefaultStream(&stream, 1, 0, paFloat32, SAMPLE_RATE, FRAMES_PER_BUFFER, audioCallback, file); // 1 0 to record
    if (err != paNoError) 
    {
        printf("PortAudio error [2]: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) 
    {
        printf("PortAudio error [3]: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        Pa_Terminate(); 
        fclose(file); 
        return 1;
    }

    printf("Press ENTER to stop the audio stream...\n");
    getchar();
    err = Pa_StopStream(stream);
    if (err != paNoError) 
    {
        printf("PortAudio error [4]: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        Pa_Terminate();
        fclose(file);
        return 1;
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError) 
    {
        printf("PortAudio error [5]: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        fclose(file);
        return 1;
    }

    Pa_Terminate();
    
    fclose(file);

    return 0;
}