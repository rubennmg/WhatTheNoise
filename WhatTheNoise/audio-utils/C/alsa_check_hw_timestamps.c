/**
 * **********************************
 * ****** alsa_check_hw_timestamps.c ******
 * **********************************
 *
 * Checks if the PCM device supports hardware timestamping.
 * Usefull to get high resolution timestamps.
 * Checked on USB PnP Sound Device: Audio (hw:2,0).
 *
 * ~ Author: rubennmg
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#define PCM_DEVICE "hw:2,0" // USB PnP Sound Device: Audio (hw:2,0)

int main()
{
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *params;
    unsigned int sample_rate = 44100;
    int dir;
    int pcm;

    if ((pcm = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0)
    {
        fprintf(stderr, "ERROR: Can't open \"%s\" PCM device. %s\n", PCM_DEVICE, snd_strerror(pcm));
        return 1;
    }

    snd_pcm_hw_params_alloca(&params);

    if ((pcm = snd_pcm_hw_params_any(pcm_handle, params)) < 0)
    {
        fprintf(stderr, "ERROR: Can't initialize hardware parameter structure. %s\n", snd_strerror(pcm));
        snd_pcm_close(pcm_handle);
        return 1;
    }

    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED); // Interleaved mode
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, 1);
    snd_pcm_hw_params_set_rate_near(pcm_handle, params, &sample_rate, &dir);

    snd_pcm_uframes_t frames = 32;
    snd_pcm_hw_params_set_period_size_near(pcm_handle, params, &frames, &dir);

    if ((pcm = snd_pcm_hw_params(pcm_handle, params)) < 0)
    {
        fprintf(stderr, "ERROR: Can't set hardware parameters. %s\n", snd_strerror(pcm));
        return 1;
    }

    snd_pcm_sw_params_t *swparams;
    snd_pcm_sw_params_alloca(&swparams);
    snd_pcm_sw_params_current(pcm_handle, swparams);
    snd_pcm_sw_params_set_tstamp_mode(pcm_handle, swparams, SND_PCM_TSTAMP_ENABLE); // Enable hardware timestamping

    if (snd_pcm_sw_params(pcm_handle, swparams) < 0)
    {
        printf("PCM device does not support hardware timestamping.\n");
    }
    else
    {
        printf("PCM device supports hardware timestamping.\n");
    }

    snd_pcm_close(pcm_handle);
    
    return 0;
}
