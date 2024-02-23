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

    // write audio samples to file
    fwrite(input, sizeof(float), framesPerBuffer, file);

    return paContinue;
}

int main() 
{
    PaError err;
    PaStream *stream;
    FILE *file;

    // open file for writing
    file = fopen("audio.raw", "wb");
    if (file == NULL) 
    {
        printf("Error: could not open file for writing [0]\n");
        return 1;
    }

    // initialize
    err = Pa_Initialize();
    if (err != paNoError) 
    {
        printf("PortAudio error [1]: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    // open audio stream
    err = Pa_OpenDefaultStream(&stream, 1, 0, paFloat32, SAMPLE_RATE, FRAMES_PER_BUFFER, audioCallback, file); // 1 0 to record
    if (err != paNoError) 
    {
        printf("PortAudio error [2]: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    // start audio stream
    err = Pa_StartStream(stream);
    if (err != paNoError) 
    {
        printf("PortAudio error [3]: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream); // close stream
        Pa_Terminate(); // terminate
        fclose(file); // close file
        return 1;
    }

    // wait for user input
    printf("Press ENTER to stop the audio stream...\n");
    getchar();

    // stop audio stream
    err = Pa_StopStream(stream);
    if (err != paNoError) 
    {
        printf("PortAudio error [4]: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        Pa_Terminate();
        fclose(file);
        return 1;
    }

    // close audio stream
    err = Pa_CloseStream(stream);
    if (err != paNoError) 
    {
        printf("PortAudio error [5]: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        fclose(file);
        return 1;
    }

    // terminate
    Pa_Terminate();
    
    // close file
    fclose(file);

    return 0;
}