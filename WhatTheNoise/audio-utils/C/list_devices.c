/**
 * **********************************
 * *********** list_devices.c ***********
 * **********************************
 *
 * Lists the available devices and displays relevant features for each one.
 *
 * ~ Author: rubennmg
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>

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
    const PaDeviceInfo *deviceInfo;
    PaError err;

    // Initialize PortAudio
    err = Pa_Initialize();
    checkPaError(err, "Error initializing PortAudio");

    // Print information for each device
    int numDevices = Pa_GetDeviceCount();
    if (numDevices < 0)
    {
        checkPaError(numDevices, "Error getting device count");
    }
    printf("----------------------------------------------------\n");
    printf("Number of devices: %d\n", numDevices);

    for (int i = 0; i < numDevices; i++)
    {
        deviceInfo = Pa_GetDeviceInfo(i);
        printf("----------------------------------------------------\n");
        printf("Device Index: %d\n", i);
        printf("    Name: %s\n", deviceInfo->name);
        printf("    Max Input Channels: %d\n", deviceInfo->maxInputChannels);
        printf("    Default Sample Rate: %lf\n", deviceInfo->defaultSampleRate);
        printf("    Default Low Input Latency: %lf\n", deviceInfo->defaultLowInputLatency);
        printf("    Default High Input Latency: %lf\n", deviceInfo->defaultHighInputLatency);
    }

    // Terminate PortAudio
    Pa_Terminate();

    return 0;
}
