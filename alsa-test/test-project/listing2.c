#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>

/*
 * This example lists the PCM devices list, opens the "hdmi" PCM device, sets some 
 * parameters, and then displays the vallue of most of the hardware parameters.
 * It does not perform any sound playback or recording.
 * Error checking has been omitted to keep the code simple.
 * ~ rubennmg
 */

int main()
{
    int rc;
    snd_pcm_t *handle = NULL;
    snd_pcm_hw_params_t *params = NULL;
    unsigned int val, val2;
    int dir;
    char **hints;
    char *name;
    snd_pcm_uframes_t frames;

    // ---------  DEBUG ------------
    printf("-----------------------------\n");
    printf("rc address: %p\n", &rc);
    printf("handle address: %p\n", &handle);
    printf("params address: %p\n", &params);
    printf("val address: %p\n", &val);
    printf("val2 address: %p\n", &val2);
    printf("dir address: %p\n", &dir);
    printf("frames address: %p\n", &frames);
    printf("hints address: %p\n", &hints);
    printf("name address: %p\n", &name);
    printf("-----------------------------\n");
    // ---------  DEBUG ------------

    // Obtain PCM devices list
    rc = snd_device_name_hint(-1, "pcm", (void ***)&hints);
    if (rc < 0)
    {
        fprintf(stderr, "unable to obtain PCM device list: %s, error code: %d\n", snd_strerror(rc), rc);
        exit(1);
    }

    if (hints == NULL)
    {
        fprintf(stderr, "PCM device list is NULL\n");
        exit(1);
    }

    // Iterate through all PCM devices
    for (char **hint = hints; *hint != NULL; hint++)
    {
        name = snd_device_name_get_hint(*hint, "NAME");
        printf("PCM device: %s\n", name);
        free(name);
    }

    // Free the PCM devices list
    snd_device_name_free_hint((void **)hints);

    // Open PCM device for playback
    rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0)
    {
        fprintf(stderr, "\nERR: unable to open pcm device: %s, error code: %d \n", snd_strerror(rc), rc);
        exit(1);
    }

    // its important to ckeck all the errors, here they are not checked to keep the code simple ~ rubennmg

    // Allocate a hardware parameters object
    snd_pcm_hw_params_alloca(&params);

    // Fill it in with default values
    snd_pcm_hw_params_any(handle, params);

    /* Set the desired hardware parameters */
    // Interleaved mode - ¿Qué hace esto?
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

    // Signed 16-bit little-endian format
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

    // Two channels -> stereo
    snd_pcm_hw_params_set_channels(handle, params, 2);

    // CD quality -> 44100 bits per second
    val = 44100;
    snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);

    // Write the parameters to the driver
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0)
    {
        fprintf(stderr, "ERR: unable to set hw parameters: %s, error code: %d\n", snd_strerror(rc), rc);
        exit(1);
    }

    /* Display information about the PCM interface */
    printf("\nPCM name: '%s'\n", snd_pcm_name(handle));
    printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(handle)));

    snd_pcm_hw_params_get_access(params, (snd_pcm_access_t *)&val);
    printf("access type: %s\n", snd_pcm_access_name((snd_pcm_access_t)val));

    snd_pcm_hw_params_get_format(params, (snd_pcm_format_t *)&val);
    printf("format: %s (%s)\n", snd_pcm_format_name((snd_pcm_format_t)val), snd_pcm_format_description((snd_pcm_format_t)val));

    snd_pcm_hw_params_get_subformat(params, (snd_pcm_subformat_t *)&val);
    printf("subformat: %s (%s)\n", snd_pcm_subformat_name((snd_pcm_subformat_t)val), snd_pcm_subformat_description((snd_pcm_subformat_t)val));

    snd_pcm_hw_params_get_channels(params, &val);
    printf("channels: %u\n", val);

    snd_pcm_hw_params_get_rate(params, &val, &dir);
    printf("rate: %d bps\n", val);

    snd_pcm_hw_params_get_period_time(params, &val, &dir);
    printf("period time: %d us\n", val);

    snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    printf("period size: %d frames\n", (int)frames);

    snd_pcm_hw_params_get_buffer_time(params, &val, &dir);
    printf("buffer time: %d us\n", val);

    snd_pcm_hw_params_get_buffer_size(params, (snd_pcm_uframes_t *)&val);
    printf("buffer size: %d frames\n", val);

    printf("val address: %p\n", &val);
    printf("dir address: %p\n", &dir);
    printf("params address: %p\n", &params);
    snd_pcm_hw_params_get_periods(params, &val, &dir); // this call is causing a segfault
    printf("periods per buffer: %u\n", val);

    snd_pcm_hw_params_get_rate_numden(params, &val, &val2);
    printf("exact rate: %d/%d bps\n", val, val2);

    val = snd_pcm_hw_params_get_sbits(params);
    printf("significant bits: %d\n", val);

    // This function is deprecated
    // snd_pcm_hw_params_get_tick_time(params, &val, &dir);
    // printf("tick time = %d us\n", val);

    val = snd_pcm_hw_params_is_batch(params);
    printf("is batch = %d\n", val);

    val = snd_pcm_hw_params_is_block_transfer(params);
    printf("is block transfer = %d\n", val);

    val = snd_pcm_hw_params_is_double(params);
    printf("is double = %d\n", val);

    val = snd_pcm_hw_params_is_half_duplex(params);
    printf("is half duplex = %d\n", val);

    val = snd_pcm_hw_params_is_joint_duplex(params);
    printf("is joint duplex = %d\n", val);

    val = snd_pcm_hw_params_can_overrange(params);
    printf("can overrange = %d\n", val);

    val = snd_pcm_hw_params_can_mmap_sample_resolution(params);
    printf("can mmap = %d\n", val);

    val = snd_pcm_hw_params_can_pause(params);
    printf("can pause = %d\n", val);

    val = snd_pcm_hw_params_can_resume(params);
    printf("can resume = %d\n", val);

    val = snd_pcm_hw_params_can_sync_start(params);
    printf("can sync start = %d\n", val);

    snd_pcm_close(handle);
    snd_pcm_hw_params_free(params);

    return 0;
}