/**
 * *******************************
 * ****** alsa_htimestamp_test.c ******
 * *******************************
 * 
 * This program demonstrates how to get the high resolution timestamp of the last sample captured by an ALSA PCM device
 * Checked and tested on USB PnP Sound Device: Audio (hw:2,0).
 * 
 * ~ Author: rubennmg
 * 
 */

#include <alsa/asoundlib.h>
#include <stdio.h>
#include <time.h>

int main()
{
    int err;
    char *pcm_name = "hw:2,0";
    snd_pcm_t *pcm_handle;
    snd_pcm_status_t *status;
    struct timespec htstamp;

    // Open the PCM device
    if ((err = snd_pcm_open(&pcm_handle, pcm_name, SND_PCM_STREAM_CAPTURE, 0)) < 0)
    {
        fprintf(stderr, "Error opening PCM interface %s: %s\n", pcm_name, snd_strerror(err));
        return 1;
    }

    // Allocate memory for the status structure
    if ((err = snd_pcm_status_malloc(&status)) < 0)
    {
        fprintf(stderr, "Error allocating memory for status: %s\n", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        return 1;
    }

    // Get the current state of the PCM device
    if ((err = snd_pcm_status(pcm_handle, status)) < 0)
    {
        fprintf(stderr, "Error getting PCM state: %s\n", snd_strerror(err));
        snd_pcm_status_free(status);
        snd_pcm_close(pcm_handle);
        return 1;
    }

    // Get the high resolution timestamp
    snd_pcm_status_get_htstamp(status, &htstamp);
    printf("High resolution timestamp: %ld.%09ld\n", htstamp.tv_sec, htstamp.tv_nsec);

    // Clean up
    snd_pcm_status_free(status);
    snd_pcm_close(pcm_handle);

    return 0;
}
