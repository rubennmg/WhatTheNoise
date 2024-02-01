#include <alsa/asoundlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    int val;

    // first of all
    printf("ALSA library version: %s\n", SND_LIB_VERSION_STR); // working :)

    // list PCM stream types
    printf("\nPCM stream types:\n");
    for (val = 0; val <= SND_PCM_STREAM_LAST; val++)
        printf("  %s\n", snd_pcm_stream_name((snd_pcm_stream_t)val));

    // list PCM access types
    printf("\nPCM access types:\n");
    for (val = 0; val <= SND_PCM_ACCESS_LAST; val++)
        printf("  %s\n", snd_pcm_access_name((snd_pcm_access_t)val));

    // list PCM formats
    printf("\nPCM formats:\n");
    if (snd_pcm_format_name((snd_pcm_format_t)val) != NULL)
        printf("  %s (%s)\n", snd_pcm_format_name((snd_pcm_format_t)val), snd_pcm_format_description((snd_pcm_format_t)val));

    // list PCM subformats
    printf("\nPCM subformats:\n");
    for (val = 0; val <= SND_PCM_SUBFORMAT_LAST; val++)
        printf("  %s (%s)\n", snd_pcm_subformat_name((snd_pcm_subformat_t)val), snd_pcm_subformat_description((snd_pcm_subformat_t)val));

    // list PCM states
    printf("\nPCM states:\n");
    for (val = 0; val <= SND_PCM_STATE_LAST; val++)
        printf("  %s\n", snd_pcm_state_name((snd_pcm_state_t)val));

    return 0;
}


