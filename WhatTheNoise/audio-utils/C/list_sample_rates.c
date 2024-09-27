/**
 * **********************************
 * ********* list_sample_rates.c *********
 * **********************************
 *
 * Displays the available sample rates for the audio input devices
 * in the system.
 *
 * ~ Author: rubennmg
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include "portaudio.h"

/**
 * Function to handle PortAudio errors
 */
void checkPaError(PaError err, const char *msg)
{
    if (err != paNoError)
    {
        fprintf(stderr, "Error %s: %s\n", msg, Pa_GetErrorText(err));
        Pa_Terminate();
        exit(err);
    }
}

int main(void)
{
    PaError err;

    // Initialize PortAudio
    err = Pa_Initialize();
    checkPaError(err, "Error initializing PortAudio");

    // Get number of devices
    int numDevices = Pa_GetDeviceCount();
    if (numDevices < 0)
        checkPaError(numDevices, "Error getting device count");

    // Iterate over devices to check supported sample rates
    for (int i = 0; i < numDevices; i++)
    {
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo->maxInputChannels == 1)
        {
            printf("----------------------------------------------------");
            printf("\nDevice: %s\n", deviceInfo->name);
            printf("Supported sample rates (Hz):\n");

            // Common sample rates
            double sampleRates[] = {8000, 11025, 16000, 22050, 32000, 44100, 48000, 88200, 96000, 176400, 192000, 384000};
            PaStreamParameters inputParameters;
            inputParameters.device = i;
            inputParameters.channelCount = 1;
            inputParameters.sampleFormat = paInt16;
            inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
            inputParameters.hostApiSpecificStreamInfo = NULL;

            for (int j = 0; j < sizeof(sampleRates) / sizeof(sampleRates[0]); j++)
            {
                err = Pa_IsFormatSupported(&inputParameters, NULL, sampleRates[j]);
                if (err == paFormatIsSupported)
                {
                    printf("%.0f\n", sampleRates[j]);
                }
            }
        }
    }

    // Terminate PortAudio
    Pa_Terminate();

    return 0;
}