/**
 * ******************************
 * ****** record_threads_v1.c ********
 * ******************************
 *
 * Records audio into two microphones simultaneously when detected above a threshold.
 * Stores audio samples and timestamps in separate files for each recording of each microphone.
 * Stops recording after a period of silence.
 * Made with PortAudio.
 *
 * - Implemented threadings for recording from each microphone. 
 * - recordCallback function has been modified to handle timestamps storing.
 * 
 * ~ Author: rubennmg
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include "portaudio.h"

/**
 * @brief Constants
 * 
 * Parameters for the audio recording. Chosen according to the best 
 * combination for the USB microphones used in testing.
 */
#define SAMPLE_RATE (44100)
#define SAMPLE_FORMAT (paInt16)
#define FRAMES_PER_BUFFER (128)
#define NUM_CHANNELS (1)
#define THRESHOLD (0.04)              // % of max amplitude value
#define MAX_SILENCE_SECONDS (0.6) // seconds to wait before stopping recording
#define INPUT_LATENCY (0.008707)   // default USB microphones low input latency
#define MIC1_INDEX (0)               // USB PnP Sound Device: Audio (hw:2,0)
#define MIC2_INDEX (4)               // USB PnP Sound Device: Audio (hw:3,0)

/**
 * @brief Struct to hold data
 * 
 * Stores data for each microphone recording.
 */
typedef struct
{
    int recording;                     /**< Mic is recording or not */  
    time_t lastRecordedTime;        /**< Last time audio was recorded */
    int fileIndex;                     /**< Index of the current audio file */
    char fileName[100];              /**< Name of the current audio file */
    char timestampFileName[100];   /**< Name of the current timestamp file */
    char micName[20];               /**< Name of the microphone */
    int micIndex;                     /**< Index of the microphone */     
    pthread_t threadId;              /**< Thread ID for the microphone */
    int sample_counter;              /**< Counter for the current sample (used in debugging) */
} paData;

/**
 * @brief Function to open and initialize files for each recording
 * 
 * This function is called when a new recording is started.
 * It opens a new file to sotre audio samples and creates the name 
 * for the timestamps file.
 * 
 * @param data struct with data for the microphone
 * @return FILE pointer to the opened file
 */
FILE *openFileForRecording(paData *data)
{
    data->fileIndex++;
    sprintf(data->fileName, "samples_threads_%s/samples_%s_%d.raw", data->micName, data->micName, data->fileIndex);
    sprintf(data->timestampFileName, "samples_threads_%s/timestamps_%s_%d.ts", data->micName, data->micName, data->fileIndex);
    FILE *file = fopen(data->fileName, "wb");
    if (file == NULL)
    {
        fprintf(stderr, "Could not open file %s.\n", data->fileName);
        return NULL;
    }
    printf("Starting new recording: %s\n", data->fileName);
    return file;
}

/**
 * @brief Function to encode raw audio files with specific sample params to mp4 using ffmpeg
 * 
 * @param arg Directory with raw files
 * @return NULL
 */
void *encodeRawFilesToMp4(void *arg)
{
    const char *directory = (const char *)arg;
    DIR *dir;
    struct dirent *ent;
    char inputFilePath[256];
    char outputFilePath[256];
    char command[512];

    if ((dir = opendir(directory)) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            if (strstr(ent->d_name, ".raw") != NULL)
            {
                snprintf(inputFilePath, sizeof(inputFilePath), "%s/%s", directory, ent->d_name);
                snprintf(outputFilePath, sizeof(outputFilePath), "%s/%s.mp4", directory, strtok(ent->d_name, "."));
                snprintf(command, sizeof(command), "ffmpeg -f s16le -ar %d -ac %d -i %s %s", SAMPLE_RATE, NUM_CHANNELS, inputFilePath, outputFilePath);
                printf("Encoding file: %s to %s\n", inputFilePath, outputFilePath);
                system(command);
            }
        }
        closedir(dir);
    }
    else
    {
        perror("Could not open directory");
    }
    return NULL;
}

/**
 * @brief Function to handle the recording logic for each microphone
 * 
 * This function is called by PortAudio when audio is detected.
 * 
 * @param inputBuffer Audio buffer when audio is captured
 * @param outputBuffer Audio buffer when audio is played
 * @param framesPerBuffer Number of frames per buffer
 * @param timeInfo Struct with time information
 * @param statusFlags Flags for the callback
 * @param userData Struct with data for the microphone
 * @return int Status of the callback
 */
static int recordCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    paData *data = (paData *)userData;
    const short *in = (const short *)inputBuffer;
    FILE *file = NULL;
    FILE *timestampFile = NULL;
    int aboveThreshold = 0;

    // Avoid unused variable warnings
    (void)outputBuffer;
    (void)statusFlags;

    // Check if inputBuffer is NULL (happens at the beginning of the stream)
    if (inputBuffer == NULL)
    {
        return paContinue;
    }

    // Check if audio data is above threshold
    for (int i = 0; i < framesPerBuffer * NUM_CHANNELS; i++)
    {
        if (in[i] > THRESHOLD * 32767.0 || in[i] < -THRESHOLD * 32767.0)
        {
            aboveThreshold = 1;
            break;
        }
    }

    // Open file in append mode if recording is in progress
    if (data->recording)
    {
        file = fopen(data->fileName, "ab");
        timestampFile = fopen(data->timestampFileName, "ab");
        if (file == NULL || timestampFile == NULL)
        {
            fprintf(stderr, "Could not open file %s or %s.\n", data->fileName, data->timestampFileName);
            return paAbort;
        }
    }

    // Open file for the first time or stop recording if silence is detected
    if (aboveThreshold)
    {
        time(&data->lastRecordedTime);
        if (!data->recording)
        {
            data->recording = 1;
            file = openFileForRecording(data);
            timestampFile = fopen(data->timestampFileName, "wb");
            if (file == NULL || timestampFile == NULL)
            {
                return paAbort;
            }
        }
    }
    else
    {
        time_t currentTime;
        time(&currentTime);
        if (data->recording && difftime(currentTime, data->lastRecordedTime) > MAX_SILENCE_SECONDS)
        {
            data->recording = 0;
            data->sample_counter = 0;
            printf("Stopping recording: %s\n", data->fileName);
        }
    }

    // Write current timestamp in the separate timestamp file
    if (data->recording && file != NULL && timestampFile != NULL)
    {
        double timestamp = timeInfo->currentTime - INPUT_LATENCY;
        data->sample_counter++;
        fwrite(&timestamp, sizeof(double), 1, timestampFile);
        fwrite(inputBuffer, sizeof(short), framesPerBuffer * NUM_CHANNELS, file);
    }

    // Close files
    if (file != NULL)
    {
        fclose(file);
    }
    if (timestampFile != NULL)
    {
        fclose(timestampFile);
    }

    return paContinue;
}

/**
 * @brief Function to start recording from a microphone
 * 
 * This function is called by a thread to start recording from a microphone.
 * Configures input parameters and open and starts the corresponding PortAudio stream.
 * 
 * @param arg Struct with data for the microphone
 * @return NULL
 */
void *startRecording(void *arg)
{
    paData *data = (paData *)arg;
    PaStreamParameters inputParameters;
    PaStream *stream;
    PaError err;

    inputParameters.device = data->micIndex;
    inputParameters.channelCount = NUM_CHANNELS;
    inputParameters.sampleFormat = SAMPLE_FORMAT;
    inputParameters.suggestedLatency = INPUT_LATENCY;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    printf("Recording from device: %s\n", Pa_GetDeviceInfo(inputParameters.device)->name);
    if (inputParameters.device == paNoDevice)
    {
        fprintf(stderr, "No default input device found.\n");
        pthread_exit(NULL);
    }

    err = Pa_OpenStream(&stream, &inputParameters, NULL, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, recordCallback, data);
    if (err != paNoError)
    {
        fprintf(stderr, "Error opening audio stream: %s\n", Pa_GetErrorText(err));
        pthread_exit(NULL);
    }

    err = Pa_StartStream(stream);
    if (err != paNoError)
    {
        fprintf(stderr, "Error starting recording: %s\n", Pa_GetErrorText(err));
        pthread_exit(NULL);
    }

    // Keep the thread running until Enter is pressed
    printf("Recording started for %s... Press ENTER to stop.\n", data->micName);
    getchar();

    err = Pa_StopStream(stream);
    if (err != paNoError)
    {
        fprintf(stderr, "Error stopping recording: %s\n", Pa_GetErrorText(err));
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError)
    {
        fprintf(stderr, "Error closing stream: %s\n", Pa_GetErrorText(err));
    }

    printf("Recording stopped for %s.\n", data->micName);
    pthread_exit(NULL);
}

/**
 * @brief Main function to record audio from two microphones
 * 
 * Initializes PortAudio, creates directories for saving raw audio files, and creates threads for recording from each microphone.
 * Waits for threads to finish and terminates PortAudio. Finally, encodes raw audio files to mp4 using threads.
 * 
 * @return int Status of the program
 */
int main(void)
{
    PaError err;
    paData dataMic1, dataMic2;
    char dir1[256];
    char dir2[256];

    // Create directories for saving raw audio files
    sprintf(dir1, "samples_threads_Mic%d", 1);
    sprintf(dir2, "samples_threads_Mic%d", 2);

    if (mkdir(dir1, 0777) != 0 && errno != EEXIST)
    {
        perror("Error creating directory for Mic1");
        return 1;
    }
    if (mkdir(dir2, 0777) != 0 && errno != EEXIST)
    {
        perror("Error creating directory for Mic2");
        return 1;
    }

    dataMic1.recording = 0;
    dataMic1.fileIndex = 0;
    dataMic1.sample_counter = 0;
    dataMic1.micIndex = MIC1_INDEX;
    sprintf(dataMic1.micName, "Mic%d", 1);
    time(&dataMic1.lastRecordedTime);

    dataMic2.recording = 0;
    dataMic2.fileIndex = 0;
    dataMic2.sample_counter = 0;
    dataMic2.micIndex = MIC2_INDEX;
    sprintf(dataMic2.micName, "Mic%d", 2);
    time(&dataMic2.lastRecordedTime);

    err = Pa_Initialize();
    if (err != paNoError)
    {
        fprintf(stderr, "Error initializing PortAudio: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    // Create threads for recording from each microphone
    if (pthread_create(&dataMic1.threadId, NULL, startRecording, (void *)&dataMic1) != 0)
    {
        fprintf(stderr, "Error creating thread for Mic1.\n");
        Pa_Terminate();
        return 1;
    }

    if (pthread_create(&dataMic2.threadId, NULL, startRecording, (void *)&dataMic2) != 0)
    {
        fprintf(stderr, "Error creating thread for Mic2.\n");
        Pa_Terminate();
        return 1;
    }

    // Wait for threads to finish
    pthread_join(dataMic1.threadId, NULL);
    pthread_join(dataMic2.threadId, NULL);

    // Terminate PortAudio
    Pa_Terminate();
    printf("Recording finished.\n");

    // Encode files using threads
    if (pthread_create(&dataMic1.threadId, NULL, encodeRawFilesToMp4, dir1) != 0)
    {
        fprintf(stderr, "Error creating thread for encoding Mic1.\n");
        return 1;
    }

    if (pthread_create(&dataMic2.threadId, NULL, encodeRawFilesToMp4, dir2) != 0)
    {
        fprintf(stderr, "Error creating thread for encoding Mic2.\n");
        return 1;
    }

    // Wait for threads to finish
    pthread_join(dataMic1.threadId, NULL);
    pthread_join(dataMic2.threadId, NULL);

    return 0;
}
