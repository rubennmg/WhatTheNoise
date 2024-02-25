#include <stdio.h>
#include <stdlib.h>
#include "portaudio.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (512)
#define NUM_SECONDS (5)
#define NUM_CHANNELS (2)
#define PA_SAMPLE_TYPE paFloat32
typedef float SAMPLE;

typedef struct
{
    int frameIndex; /* Index into sample array. */
    int maxFrameIndex;
    SAMPLE *recordedSamples;
} paTestData;

static int playCallback(const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *userData)
{
    paTestData *data = (paTestData *)userData;
    SAMPLE *wptr = (SAMPLE *)outputBuffer;
    unsigned int framesLeft = data->maxFrameIndex - data->frameIndex;

    if (framesLeft < framesPerBuffer)
    {
        for (unsigned long i = 0; i < framesLeft; i++)
        {
            *wptr++ = data->recordedSamples[data->frameIndex++]; /* Left channel */
            *wptr++ = data->recordedSamples[data->frameIndex++]; /* Right channel */
        }
        for (unsigned long i = framesLeft; i < framesPerBuffer; i++)
        {
            *wptr++ = 0; /* Left channel */
            *wptr++ = 0; /* Right channel */
        }
        return paComplete;
    }
    else
    {
        for (unsigned long i = 0; i < framesPerBuffer; i++)
        {
            *wptr++ = data->recordedSamples[data->frameIndex++]; /* Left channel */
            *wptr++ = data->recordedSamples[data->frameIndex++]; /* Right channel */
        }
        return paContinue;
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("USO: %s <archivo.mp4>\n", argv[0]);
        return 1;
    }

    const char *filePath = argv[1];
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;
    paTestData data;

    // Inicializar FFmpeg
    av_register_all();
    avcodec_register_all();

    // Abrir archivo de formato
    AVFormatContext *formatContext = NULL;
    if (avformat_open_input(&formatContext, filePath, NULL, NULL) != 0)
    {
        fprintf(stderr, "Error: No se pudo abrir el archivo: %s\n", filePath);
        return 1;
    }

    // Obtener información del archivo
    if (avformat_find_stream_info(formatContext, NULL) < 0)
    {
        fprintf(stderr, "Error: No se pudo encontrar información del archivo: %s\n", filePath);
        avformat_close_input(&formatContext);
        return 1;
    }

    // Encontrar el flujo de audio
    int audioStreamIndex = -1;
    for (int i = 0; i < formatContext->nb_streams; i++)
    {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audioStreamIndex = i;
            break;
        }
    }

    if (audioStreamIndex == -1)
    {
        fprintf(stderr, "Error: No se encontró flujo de audio en el archivo: %s\n", filePath);
        avformat_close_input(&formatContext);
        return 1;
    }

    // Obtener contexto del códec
    AVCodecParameters *codecParameters = formatContext->streams[audioStreamIndex]->codecpar;
    AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
    if (!codec)
    {
        fprintf(stderr, "Error: No se encontró el códec de audio adecuado.\n");
        avformat_close_input(&formatContext);
        return 1;
    }

    AVCodecContext *codecContext = avcodec_alloc_context3(codec);
    if (!codecContext)
    {
        fprintf(stderr, "Error: No se pudo asignar memoria para el contexto del códec.\n");
        avformat_close_input(&formatContext);
        return 1;
    }

    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0)
    {
        fprintf(stderr, "Error: No se pudo asignar parámetros al contexto del códec.\n");
        avformat_close_input(&formatContext);
        avcodec_free_context(&codecContext);
        return 1;
    }

    if (avcodec_open2(codecContext, codec, NULL) < 0)
    {
        fprintf(stderr, "Error: No se pudo abrir el códec.\n");
        avformat_close_input(&formatContext);
        avcodec_free_context(&codecContext);
        return 1;
    }

    // Configurar parámetros de salida para PortAudio
    data.maxFrameIndex = NUM_SECONDS * SAMPLE_RATE * NUM_CHANNELS; /* Total number of frames to play */
    data.frameIndex = 0;
    int numSamples = data.maxFrameIndex; /* Total number of samples */
    int numBytes = numSamples * sizeof(SAMPLE);        /* Size of the sample array in bytes */
    data.recordedSamples = (SAMPLE *)malloc(numBytes); /* Allocate memory for the sample array */
    if (data.recordedSamples == NULL)
    {
        fprintf(stderr, "Could not allocate record array.\n");
        avcodec_close(codecContext);
        avformat_close_input(&formatContext);
        avcodec_free_context(&codecContext);
        return 1;
    }

    // Leer audio del archivo y almacenarlo en data.recordedSamples
    AVPacket packet;
    int dataSize = 0;
    while (av_read_frame(formatContext, &packet) >= 0)
    {
        if (packet.stream_index == audioStreamIndex)
        {
            int gotFrame;
            AVFrame *frame = av_frame_alloc();
            err = avcodec_decode_audio4(codecContext, frame, &gotFrame, &packet);
            if (err < 0)
            {
                fprintf(stderr, "Error: No se pudo decodificar el audio.\n");
                av_frame_free(&frame);
                break;
            }

            if (gotFrame)
            {
                int dataSize = frame->nb_samples * codecParameters->channels * av_get_bytes_per_sample(codecContext->sample_fmt);
                memcpy(data.recordedSamples + data.frameIndex, frame->data[0], dataSize);
                data.frameIndex += frame->nb_samples * codecParameters->channels;
            }

            av_frame_free(&frame);
        }
        av_packet_unref(&packet);
    }

    // Inicializar PortAudio
    err = Pa_Initialize();
    if (err != paNoError)
    {
        fprintf(stderr, "Error: No se pudo inicializar PortAudio.\n");
        avcodec_close(codecContext);
        avformat_close_input(&formatContext);
        avcodec_free_context(&codecContext);
        free(data.recordedSamples);
        return err;
    }

    // Configurar parámetros de salida de PortAudio
    outputParameters.device = Pa_GetDefaultOutputDevice(); /* Default output device */
    if (outputParameters.device == paNoDevice)
    {
        fprintf(stderr, "Error: No se encontró el dispositivo de salida predeterminado.\n");
        Pa_Terminate();
        avcodec_close(codecContext);
        avformat_close_input(&formatContext);
        avcodec_free_context(&codecContext);
        free(data.recordedSamples);
        return 1;
    }
    outputParameters.channelCount = NUM_CHANNELS;         /* Stereo output */
    outputParameters.sampleFormat = PA_SAMPLE_TYPE;       /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    // Abrir el stream de PortAudio
    err = Pa_OpenStream(
        &stream,
        NULL, /* No input */
        &outputParameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff,
        playCallback,
        &data);
    if (err != paNoError)
    {
        fprintf(stderr, "Error: No se pudo abrir el stream de PortAudio.\n");
        Pa_Terminate();
        avcodec_close(codecContext);
        avformat_close_input(&formatContext);
        avcodec_free_context(&codecContext);
        free(data.recordedSamples);
        return err;
    }

    // Iniciar el stream de PortAudio
    err = Pa_StartStream(stream);
    if (err != paNoError)
    {
        fprintf(stderr, "Error: No se pudo iniciar el stream de PortAudio.\n");
        Pa_CloseStream(stream);
        Pa_Terminate();
        avcodec_close(codecContext);
        avformat_close_input(&formatContext);
        avcodec_free_context(&codecContext);
        free(data.recordedSamples);
        return err;
    }

    printf("Reproducción de audio desde el archivo: %s\n", filePath);
    printf("Presione Ctrl+C para detener la reproducción.\n");

    // Esperar a que finalice la reproducción
    while (Pa_IsStreamActive(stream))
        Pa_Sleep(100);

    // Cerrar el stream de PortAudio
    err = Pa_CloseStream(stream);
    if (err != paNoError)
    {
        fprintf(stderr, "Error: No se pudo cerrar el stream de PortAudio correctamente.\n");
        Pa_Terminate();
        avcodec_close(codecContext);
        avformat_close_input(&formatContext);
        avcodec_free_context(&codecContext);
        free(data.recordedSamples);
        return err;
    }

    // Terminar PortAudio
    Pa_Terminate();

    // Liberar recursos
    avcodec_close(codecContext);
    avformat_close_input(&formatContext);
    avcodec_free_context(&codecContext);
    free(data.recordedSamples);

    return 0;
}
