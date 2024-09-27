/**
 * ******************************
 * ******** record_ALSA.c **********
 * ******************************
 * 
 * This is a version of ../../audio-utils/C/record_v5.c adapted to be 
 * launched from the Flask server.
 * 
 * ~ Author: rubennmg
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdatomic.h>

#define SAMPLE_FORMAT SND_PCM_FORMAT_S16_LE
#define MAX_AMPLITUDE 32768
#define CHANNELS 1
#define FRAMES_PER_BUFFER 128
#define LATENCY 8707

int sample_rate;
float threshold_percentage;
float min_silence_time;
int threshold;
int min_silence_frames;

/**
 * @brief Node in the buffer queue that stores audio buffers and their timestamps.
 */
typedef struct BufferNode
{
    int16_t buffer[FRAMES_PER_BUFFER];
    struct timespec timestamp;
    struct BufferNode *next;
} BufferNode;

/**
 * @brief Queue to store audio buffers.
 */
typedef struct
{
    BufferNode *front;
    BufferNode *rear;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int stop;
} BufferQueue;

/**
 * @brief Structure to store data for each microphone.
 */
typedef struct
{
    snd_pcm_t *pcm_handle;
    int recording;
    int silenceCounter;
    int fileIndex;
    char fileName[100];
    char timestampFileName[100];
    char micName[20];
    FILE *file;
    FILE *timestampFile;
    pthread_t threadId;
    pthread_mutex_t *startMutex;
    pthread_cond_t *startCond;
    int *startFlag;
    int *stopFlag;
    BufferQueue bufferQueue;
    pthread_mutex_t fileMutex;
    pthread_cond_t fileCond;
    int newRecording;
    int recordingFinished;
} MicData;

/**
 * @brief Initializes the buffer queue.
 *
 * @param queue Pointer to the buffer queue.
 */
void bufferQueueInit(BufferQueue *queue)
{
    queue->front = queue->rear = NULL;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
    queue->stop = 0;
}

/**
 * @brief Adds a buffer to the queue.
 *
 * @param queue Pointer to the buffer queue.
 * @param buffer Pointer to the audio buffer.
 * @param timestamp Timestamp associated with the buffer.
 */
void bufferQueuePush(BufferQueue *queue, int16_t *buffer, struct timespec timestamp)
{
    BufferNode *newNode = (BufferNode *)malloc(sizeof(BufferNode));
    if (!newNode)
    {
        perror("Failed to allocate memory for buffer node");
        return;
    }
    memcpy(newNode->buffer, buffer, FRAMES_PER_BUFFER * sizeof(int16_t));
    newNode->timestamp = timestamp;
    newNode->next = NULL;

    pthread_mutex_lock(&queue->mutex);
    if (queue->rear == NULL)
    {
        queue->front = queue->rear = newNode;
    }
    else
    {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

/**
 * @brief Pops a buffer from the queue.
 *
 * @param queue Pointer to the buffer queue.
 * @param buffer Pointer to the audio buffer where data will be copied.
 * @param timestamp Pointer to where the timestamp of the buffer will be copied.
 * @return 0 if a buffer is popped, -1 if the queue is empty and should stop.
 */
int bufferQueuePop(BufferQueue *queue, int16_t *buffer, struct timespec *timestamp)
{
    pthread_mutex_lock(&queue->mutex);
    while (queue->front == NULL && !queue->stop)
    {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    if (queue->stop && queue->front == NULL)
    {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    BufferNode *temp = queue->front;
    memcpy(buffer, temp->buffer, FRAMES_PER_BUFFER * sizeof(int16_t));
    *timestamp = temp->timestamp;
    queue->front = queue->front->next;

    if (queue->front == NULL)
    {
        queue->rear = NULL;
    }
    free(temp);
    pthread_mutex_unlock(&queue->mutex);

    return 0;
}

/**
 * @brief Cleans the buffer queue, freeing associated memory.
 *
 * @param queue Pointer to the buffer queue.
 */
void cleanBufferQueue(BufferQueue *queue)
{
    pthread_mutex_lock(&queue->mutex);
    BufferNode *current = queue->front;
    while (current != NULL)
    {
        BufferNode *temp = current;
        current = current->next;
        free(temp);
    }
    queue->front = queue->rear = NULL;
    pthread_mutex_unlock(&queue->mutex);
}

/**
 * @brief Opens files for recording audio and timestamps.
 *
 * @param data Pointer to the microphone data structure.
 */
void openFilesForRecording(MicData *data)
{
    data->fileIndex++;
    sprintf(data->fileName, "samples_threads_%s/samples_%s_%d.raw", data->micName, data->micName, data->fileIndex);
    sprintf(data->timestampFileName, "samples_threads_%s/timestamps_%s_%d.ts", data->micName, data->micName, data->fileIndex);
    data->file = fopen(data->fileName, "wb");
    data->timestampFile = fopen(data->timestampFileName, "w");
    if (data->file == NULL || data->timestampFile == NULL)
    {
        fprintf(stderr, "Could not open files %s or %s.\n", data->fileName, data->timestampFileName);
        exit(EXIT_FAILURE);
    }
    printf("Starting new recording: %s\n", data->fileName);
}

/**
 * @brief Closes the recording files.
 *
 * @param data Pointer to the microphone data structure.
 */
void closeFilesForRecording(MicData *data)
{
    if (data->file != NULL)
    {
        fclose(data->file);
        data->file = NULL;
    }
    if (data->timestampFile != NULL)
    {
        fclose(data->timestampFile);
        data->timestampFile = NULL;
    }
    printf("Recording stopped: %s\n", data->fileName);
}

/**
 * @brief Encodes raw files to mp4 using ffmpeg.
 *
 * @param arg Pointer to the directory name.
 * @return NULL.
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
                snprintf(command, sizeof(command), "ffmpeg -f s16le -ar %d -ac %d -i %s %s", sample_rate, CHANNELS, inputFilePath, outputFilePath);
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
 * @brief Records audio from a microphone.
 * 
 * This functions handles the recording logic for each microphone.
 *
 * @param arg Pointer to the microphone data structure.
 * @return NULL.
 */
void *recordAudio(void *arg)
{
    MicData *data = (MicData *)arg;
    snd_pcm_uframes_t frames = FRAMES_PER_BUFFER;
    int pcm;
    int16_t buffer[FRAMES_PER_BUFFER];
    int aboveThreshold;
    struct timespec hw_timestamp;
    snd_pcm_status_t *status;
    snd_pcm_status_alloca(&status);

    bufferQueueInit(&data->bufferQueue);

    while (1)
    {
        pthread_mutex_lock(data->startMutex);
        while (!(*data->startFlag))
        {
            pthread_cond_wait(data->startCond, data->startMutex);
        }
        pthread_mutex_unlock(data->startMutex);

        while (!(*data->stopFlag))
        {
            aboveThreshold = 0;
            pcm = snd_pcm_readi(data->pcm_handle, buffer, frames);
            if (pcm == -EPIPE)
            {
                fprintf(stderr, "XRUN.\n");
                snd_pcm_prepare(data->pcm_handle);
                continue;
            }
            else if (pcm < 0)
            {
                fprintf(stderr, "ERROR: Can't read from PCM device. %s\n", snd_strerror(pcm));
                break;
            }
            else if (pcm != (int)frames)
            {
                fprintf(stderr, "Short read: read %d frames\n", pcm);
                continue;
            }

            snd_pcm_status(data->pcm_handle, status);
            snd_pcm_status_get_htstamp(status, &hw_timestamp);

            for (int i = 0; i < FRAMES_PER_BUFFER; i++)
            {
                if (abs(buffer[i]) > threshold)
                {
                    aboveThreshold = 1;
                    break;
                }
            }

            if (aboveThreshold)
            {
                if (!data->recording)
                {
                    data->recording = 1;
                    data->silenceCounter = 0;
                    pthread_mutex_lock(&data->fileMutex);
                    data->newRecording = 1;
                    pthread_cond_signal(&data->fileCond);
                    pthread_mutex_unlock(&data->fileMutex);
                }
            }

            if (data->recording)
            {
                bufferQueuePush(&data->bufferQueue, buffer, hw_timestamp);
            }

            if (!aboveThreshold)
            {
                data->silenceCounter++;
                if (data->recording && data->silenceCounter > min_silence_frames)
                {
                    pthread_mutex_lock(&data->fileMutex);
                    data->recordingFinished = 1;
                    data->recording = 0;
                    pthread_cond_signal(&data->fileCond);
                    pthread_mutex_unlock(&data->fileMutex);
                }
            }
            else
            {
                data->silenceCounter = 0;
            }

            if (*data->stopFlag)
            {
                if (data->recording)
                {
                    pthread_mutex_lock(&data->fileMutex);
                    data->recordingFinished = 1;
                    data->recording = 0;
                    pthread_cond_signal(&data->fileCond);
                    pthread_mutex_unlock(&data->fileMutex);
                }
                break;
            }
        }
    }

    snd_pcm_close(data->pcm_handle);
    pthread_exit(NULL);
}

/**
 * @brief Thread function for writing to files.
 * 
 * This function is called from separate threads to write audio buffers and timestamps to corresponding files.
 *
 * @param arg Pointer to the MicData structure.
 * @return NULL.
 */
void *writeAudioToFile(void *arg)
{
    MicData *data = (MicData *)arg;
    int16_t buffer[FRAMES_PER_BUFFER];
    struct timespec timestamp;

    while (!(*data->stopFlag) || data->bufferQueue.front != NULL)
    {
        pthread_mutex_lock(&data->fileMutex);
        while (!data->newRecording && !data->recordingFinished && !(*data->stopFlag))
        {
            pthread_cond_wait(&data->fileCond, &data->fileMutex);
        }

        if (data->newRecording)
        {
            openFilesForRecording(data);
            data->newRecording = 0;
        }

        pthread_mutex_unlock(&data->fileMutex);

        while (bufferQueuePop(&data->bufferQueue, buffer, &timestamp) == 0 && !data->recordingFinished)
        {
            if (data->file != NULL)
            {
                fwrite(buffer, sizeof(int16_t), FRAMES_PER_BUFFER, data->file);
                fprintf(data->timestampFile, "%ld.%09ld\n", timestamp.tv_sec, timestamp.tv_nsec);
            }
        }

        pthread_mutex_lock(&data->fileMutex);
        if (data->recordingFinished)
        {
            closeFilesForRecording(data);
            data->recordingFinished = 0;
        }
        pthread_mutex_unlock(&data->fileMutex);
    }

    return NULL;
}

/**
 * @brief Sets up the PCM device for audio capture.
 *
 * This function configures the PCM device with the specified parameters for audio capture, including 
 * sample format, sample rate, channels, and latency. It also sets the timestamp mode and type for the PCM device.
 *
 * @param data Pointer to the MicData structure that holds the PCM handle.
 * @param device Name of the PCM device to be opened.
 * @return 0 on success, or a negative error code on failure.
 */
int setup_pcm(MicData *data, const char *device)
{
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_uframes_t frames = FRAMES_PER_BUFFER;
    unsigned int sample_rate = sample_rate;
    unsigned int latency = LATENCY;
    int err;

    if ((err = snd_pcm_open(&data->pcm_handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0)
    {
        fprintf(stderr, "ERROR: Can't open \"%s\" PCM device. %s\n", device, snd_strerror(err));
        return err;
    }

    snd_pcm_hw_params_any(data->pcm_handle, params);
    snd_pcm_hw_params_set_access(data->pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(data->pcm_handle, params, SAMPLE_FORMAT);
    snd_pcm_hw_params_set_channels(data->pcm_handle, params, CHANNELS);
    snd_pcm_hw_params_set_rate_near(data->pcm_handle, params, &sample_rate, 0);
    snd_pcm_hw_params_set_period_size_near(data->pcm_handle, params, &frames, 0);
    snd_pcm_hw_params_set_buffer_time_near(data->pcm_handle, params, &latency, 0);
    if ((err = snd_pcm_hw_params(data->pcm_handle, params)) < 0)
    {
        fprintf(stderr, "ERROR: Can't set hardware parameters for PCM device. %s\n", snd_strerror(err));
        return err;
    }

    snd_pcm_sw_params_t *swparams;
    snd_pcm_sw_params_alloca(&swparams);
    snd_pcm_sw_params_current(data->pcm_handle, swparams);
    snd_pcm_sw_params_set_tstamp_mode(data->pcm_handle, swparams, SND_PCM_TSTAMP_ENABLE);
    snd_pcm_sw_params_set_tstamp_type(data->pcm_handle, swparams, SND_PCM_TSTAMP_TYPE_GETTIMEOFDAY);
    if ((err = snd_pcm_sw_params(data->pcm_handle, swparams)) < 0)
    {
        fprintf(stderr, "ERROR: Can't set software parameters for PCM device. %s\n", snd_strerror(err));
        return err;
    }

    return 0;
}

/**
 * @brief Initializes the microphone data structure.
 *
 * This function sets the initial values for the MicData structure, including recording flags, file names, 
 * mutexes, and condition variables.
 *
 * @param data Pointer to the MicData structure to be initialized.
 * @param micNumber Microphone number (identifier).
 * @param startMutex Pointer to the start mutex.
 * @param startCond Pointer to the start condition variable.
 * @param startFlag Pointer to the start flag.
 * @param stopFlag Pointer to the stop flag.
 */
void initializeMicData(MicData *data, int micNumber, pthread_mutex_t *startMutex, pthread_cond_t *startCond, int *startFlag, int *stopFlag)
{
    data->recording = 0;
    data->fileIndex = 0;
    data->silenceCounter = 0;
    sprintf(data->micName, "Mic%d", micNumber);
    data->startMutex = startMutex;
    data->startCond = startCond;
    data->startFlag = startFlag;
    data->stopFlag = stopFlag;
    data->file = NULL;
    data->timestampFile = NULL;
    pthread_mutex_init(&data->fileMutex, NULL);
    pthread_cond_init(&data->fileCond, NULL);
    data->newRecording = 0;
    data->recordingFinished = 0;
}

/**
 * @brief Starts the recording and writing threads.
 *
 * This function creates and starts the threads for recording audio and writing audio to files for both microphones.
 *
 * @param dataMic1 Pointer to the MicData structure for microphone 1.
 * @param dataMic2 Pointer to the MicData structure for microphone 2.
 */
void startThreads(MicData *dataMic1, MicData *dataMic2)
{
    pthread_t writeThreadMic1, writeThreadMic2;

    if (pthread_create(&dataMic1->threadId, NULL, recordAudio, (void *)dataMic1) != 0)
    {
        fprintf(stderr, "Error creating thread for Mic1.\n");
        exit(1);
    }

    if (pthread_create(&dataMic2->threadId, NULL, recordAudio, (void *)dataMic2) != 0)
    {
        fprintf(stderr, "Error creating thread for Mic2.\n");
        exit(1);
    }

    if (pthread_create(&writeThreadMic1, NULL, writeAudioToFile, (void *)dataMic1) != 0)
    {
        fprintf(stderr, "Error creating thread for writing Mic1.\n");
        exit(1);
    }

    if (pthread_create(&writeThreadMic2, NULL, writeAudioToFile, (void *)dataMic2) != 0)
    {
        fprintf(stderr, "Error creating thread for writing Mic2.\n");
        exit(1);
    }

    dataMic1->bufferQueue.stop = 0;
    dataMic2->bufferQueue.stop = 0;
}

/**
 * @brief Stops the recording threads for both microphones.
 *
 * @param dataMic1 Pointer to the MicData structure for microphone 1.
 * @param dataMic2 Pointer to the MicData structure for microphone 2.
 */
void stopRecordingThreads(MicData *dataMic1, MicData *dataMic2)
{
    *(dataMic1->stopFlag) = 1;
    *(dataMic2->stopFlag) = 1;

    pthread_cond_broadcast(&dataMic1->bufferQueue.cond);
    pthread_cond_broadcast(&dataMic2->bufferQueue.cond);

    pthread_join(dataMic1->threadId, NULL);
    pthread_join(dataMic2->threadId, NULL);
}

/**
 * @brief Cleans up the buffer queues for both microphones.
 *
 * @param dataMic1 Pointer to the MicData structure for microphone 1.
 * @param dataMic2 Pointer to the MicData structure for microphone 2.
 */
void cleanUp(MicData *dataMic1, MicData *dataMic2)
{
    cleanBufferQueue(&dataMic1->bufferQueue);
    cleanBufferQueue(&dataMic2->bufferQueue);
}

/**
 * @brief Main function.
 *
 * Initializes the microphone data structures, sets up the PCM devices, and launches the recording and writing threads.
 * Waits for user input to stop recording and then starts encoding the stored files to MP4.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return int Exit code of the program.
 */
int main(int argc, char *argv[])
{
    if (argc != 6) 
    {
        fprintf(stderr, "Usage: %s <mic1_device> <mic2_device> <sample_rate> <threshold> <min_silence_time>\n", argv[0]);
        return 1;
    }

    const char *mic1_device = argv[1];
    const char *mic2_device = argv[2];
    sample_rate = atoi(argv[3]);
    threshold_percentage = atof(argv[4]);
    min_silence_time = atof(argv[5]);

    threshold = MAX_AMPLITUDE * threshold_percentage;
    min_silence_frames = sample_rate / FRAMES_PER_BUFFER * min_silence_time;

    int err;
    MicData dataMic1, dataMic2;
    char dir1[256];
    char dir2[256];
    pthread_mutex_t startMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t startCond = PTHREAD_COND_INITIALIZER;
    int stopFlag = 0;
    int startFlag = 0;

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

    initializeMicData(&dataMic1, 1, &startMutex, &startCond, &startFlag, &stopFlag);
    initializeMicData(&dataMic2, 2, &startMutex, &startCond, &startFlag, &stopFlag);

    if ((err = setup_pcm(&dataMic1, mic1_device)) != 0)
    {
        return 1;
    }

    if ((err = setup_pcm(&dataMic2, mic2_device)) != 0)
    {
        return 1;
    }

    startThreads(&dataMic1, &dataMic2);

    pthread_mutex_lock(&startMutex);
    startFlag = 1;
    pthread_cond_broadcast(&startCond);
    pthread_mutex_unlock(&startMutex);

    printf("Press ENTER to stop recording...\n");
    getchar();

    pthread_mutex_lock(&startMutex);
    stopFlag = 1;
    pthread_cond_broadcast(&startCond);
    pthread_mutex_unlock(&startMutex);

    stopRecordingThreads(&dataMic1, &dataMic2);
    cleanUp(&dataMic1, &dataMic2);

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

    pthread_join(dataMic1.threadId, NULL);
    pthread_join(dataMic2.threadId, NULL);

    return 0;
}
