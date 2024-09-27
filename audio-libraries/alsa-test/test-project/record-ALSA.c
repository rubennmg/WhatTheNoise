/**
 *************************
 ******* record-ALSA.c ******
 ************************* 
 * 
 * Graba audio desde un dispositivo y se almacena en un fichero .raw.
 * Usado para testear la librería ALSA.
 * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#define SAMPLE_RATE 44100
#define CHANNELS 2
#define SAMPLE_SIZE 2 // 16-bit audio

int main(int argc, char **argv) {
    int rc;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    unsigned int sample_rate = SAMPLE_RATE;
    int dir;
    snd_pcm_uframes_t frames = 32; 
    char *device = argv[1]; 
    char filename[100];

    // Open PCM device for recording
    if ((rc = snd_pcm_open(&handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "Error opening PCM device: %s\n", snd_strerror(rc));
        return 1;
    }

    // Allocate hardware parameters object
    snd_pcm_hw_params_alloca(&params);

    // Fill hardware parameters with default values
    snd_pcm_hw_params_any(handle, params);

    // Set desired sample format
    if ((rc = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        fprintf(stderr, "Error setting access mode: %s\n", snd_strerror(rc));
        return 1;
    }

    if ((rc = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE)) < 0) {
        fprintf(stderr, "Error setting sample format: %s\n", snd_strerror(rc));
        return 1;
    }

    // Set desired sample rate
    if ((rc = snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, &dir)) < 0) {
        fprintf(stderr, "Error setting sample rate: %s\n", snd_strerror(rc));
        return 1;
    }

    // Set desired number of channels
    if ((rc = snd_pcm_hw_params_set_channels(handle, params, CHANNELS)) < 0) {
        fprintf(stderr, "Error setting channels: %s\n", snd_strerror(rc));
        return 1;
    }

    // Set desired buffer size (in frames)
    if ((rc = snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir)) < 0) {
        fprintf(stderr, "Error setting period size: %s\n", snd_strerror(rc));
        return 1;
    }

    // Apply hardware parameters
    if ((rc = snd_pcm_hw_params(handle, params)) < 0) {
        fprintf(stderr, "Error setting hardware parameters: %s\n", snd_strerror(rc));
        return 1;
    }

    sprintf(filename, "grabacion%s.raw", argv[1]);
    // Open the output file for writing
    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Error opening output file\n");
        return 1;
    }

    // Record audio for 5 seconds
    int seconds_to_record = 5;
    int total_frames = SAMPLE_RATE * seconds_to_record;
    int frames_written = 0;

    while (frames_written < total_frames) {
        short buffer[frames * CHANNELS];
        int frames_to_read = total_frames - frames_written;
        
        if (frames_to_read > frames) {
            frames_to_read = frames;
        }
        
        if ((rc = snd_pcm_readi(handle, buffer, frames_to_read)) != frames_to_read) {
            fprintf(stderr, "Error reading from PCM device: %s\n", snd_strerror(rc));
            break;
        }
        
        fwrite(buffer, sizeof(short), frames_to_read * CHANNELS, file);
        frames_written += frames_to_read;
    }

    snd_pcm_close(handle);

    fclose(file);

    return 0;
}
