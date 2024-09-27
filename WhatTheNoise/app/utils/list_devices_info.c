/**
 * **********************************
 * ********* list_devices_info.c *********
 * **********************************
 *
 * Lists all available device's ID, name and supported sample rates.
 * It is required to display data of the available devices in
 * the configuration form. It is limited to displaying single-channel recording devices,
 * as this is supported by USB microphones
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
        fprintf(stderr, "[ERROR] %s: %s\n", msg, Pa_GetErrorText(err));
        Pa_Terminate();
        exit(err);
    }
}

int main(void)
{
    PaError err;
    const PaDeviceInfo *deviceInfo;
    int numDevices, i;

    // Initialize PortAudio
    freopen("/dev/null", "w", stderr); // suppress warning messages
    err = Pa_Initialize();
    freopen("/dev/tty", "w", stderr); // restore stderr
    checkPaError(err, "Error initializing PortAudio");

    // Get device count
    numDevices = Pa_GetDeviceCount();
    if (numDevices < 0)
    {
        checkPaError(numDevices, "Error getting device count");
    }

    // Print input devices names
    for (i = 0; i < numDevices; i++)
    {
        deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo->maxInputChannels == 1)
        {
            printf("ID: %d, Name: %s", i, deviceInfo->name);
            // Common sample rates
            double sampleRates[] = {8000, 11025, 16000, 22050, 32000, 44100, 48000, 88200, 96000, 176400, 192000, 384000};
            PaStreamParameters inputParameters;
            inputParameters.device = i;
            inputParameters.channelCount = 1;
            inputParameters.sampleFormat = paInt16;
            inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
            inputParameters.hostApiSpecificStreamInfo = NULL;

            printf(", Rates:");

            for (int j = 0; j < sizeof(sampleRates) / sizeof(sampleRates[0]); j++)
            {
                err = Pa_IsFormatSupported(&inputParameters, NULL, sampleRates[j]);
                if (err == paFormatIsSupported)
                {
                    printf(" %.0f", sampleRates[j]);
                }
            }
            printf("\n");
        }
    }

    // Terminate Portaudio
    Pa_Terminate();

    return 0;
}
