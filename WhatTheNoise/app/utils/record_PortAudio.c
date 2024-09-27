/**
 * ******************************
 * ****** record_PorAudio.c ********
 * ******************************
 *
 * This is a version of ../../audio-utils/C/record_v2.c adapted to be 
 * launched from the Flask server.
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
#include <portaudio.h>
#include <stdint.h>

#define SAMPLE_FORMAT (paInt16)
#define MAX_AMPLITUDE 32768
#define FRAMES_PER_BUFFER (128)
#define NUM_CHANNELS (1)

int mic1_index;
int mic2_index;
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
    double timestamp;
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
    PaStream *stream;
    int recording;
    int silenceCounter;
    int fileIndex;
    char fileName[100];
    char timestampFileName[100];
    char micName[20];
    int micIndex;
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
void bufferQueuePush(BufferQueue *queue, int16_t *buffer, double timestamp)
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
int bufferQueuePop(BufferQueue *queue, int16_t *buffer, double *timestamp)
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
    snprintf(data->fileName, sizeof(data->fileName), "samples_threads_%s/samples_%s_%d.raw", data->micName, data->micName, data->fileIndex);
    snprintf(data->timestampFileName, sizeof(data->timestampFileName), "samples_threads_%s/timestamps_%s_%d.ts", data->micName, data->micName, data->fileIndex);
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
                snprintf(command, sizeof(command), "ffmpeg -f s16le -ar %d -ac %d -i %s %s", sample_rate, NUM_CHANNELS, inputFilePath, outputFilePath);
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
 * @param inputBuffer audio buffer when audio is captured
 * @param outputBuffer audio buffer when audio is played
 * @param framesPerBuffer number of frames per buffer
 * @param timeInfo struct with time information
 * @param statusFlags flags for the callback
 * @param userData 
 * @return int status of the callback
 */
static int recordCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    MicData *data = (MicData *)userData;
    const int16_t *buffer = (const int16_t *)inputBuffer;
    int aboveThreshold = 0;
    double timestamp = timeInfo->inputBufferAdcTime;

    if (inputBuffer == NULL)
    {
        return paContinue;
    }

    for (int i = 0; i < FRAMES_PER_BUFFER; i++)
    {
        if (abs(buffer[i]) > threshold)
        {
            aboveThreshold = 1;
            break;
        }
    }

    if (aboveThreshold && !data->recording)
    {
        data->recording = 1;
        data->silenceCounter = 0;
        pthread_mutex_lock(&data->fileMutex);
        data->newRecording = 1;
        pthread_cond_signal(&data->fileCond);
        pthread_mutex_unlock(&data->fileMutex);
    }

    if (data->recording)
    {
        bufferQueuePush(&data->bufferQueue, (int16_t *)buffer, timestamp);
    }

    if (!aboveThreshold)
    {
        data->silenceCounter++;
        if (data->recording && data->silenceCounter > min_silence_frames)
        {
            data->recording = 0;
            pthread_mutex_lock(&data->fileMutex);
            data->recordingFinished = 1;
            pthread_cond_signal(&data->fileCond);
            pthread_mutex_unlock(&data->fileMutex);
        }
    }
    else
    {
        data->silenceCounter = 0;
    }

    return paContinue;
}

/**
 * @brief Thread function for recording audio.
 * 
 * Configures audio parameters for each microphone and
 * PortAudio stream recording.
 *
 * @param arg Pointer to the MicData structure.
 * @return NULL.
 */
void *recordAudio(void *arg)
{
    MicData *data = (MicData *)arg;
    PaStreamParameters inputParameters;
    PaError err;

    inputParameters.device = data->micIndex;
    inputParameters.channelCount = NUM_CHANNELS;
    inputParameters.sampleFormat = SAMPLE_FORMAT;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&data->stream, &inputParameters, NULL, sample_rate, FRAMES_PER_BUFFER, paClipOff, recordCallback, data);
    if (err != paNoError)
    {
        fprintf(stderr, "Error opening audio stream: %s\n", Pa_GetErrorText(err));
        pthread_exit(NULL);
    }

    err = Pa_StartStream(data->stream);
    if (err != paNoError)
    {
        fprintf(stderr, "Error starting audio stream: %s\n", Pa_GetErrorText(err));
        pthread_exit(NULL);
    }

    pthread_mutex_lock(data->startMutex);
    while (!(*data->startFlag))
    {
        pthread_cond_wait(data->startCond, data->startMutex);
    }
    pthread_mutex_unlock(data->startMutex);

    while (!(*data->stopFlag))
    {
        Pa_Sleep(100);
    }

    err = Pa_StopStream(data->stream);
    if (err != paNoError)
    {
        fprintf(stderr, "Error stopping audio stream: %s\n", Pa_GetErrorText(err));
    }

    err = Pa_CloseStream(data->stream);
    if (err != paNoError)
    {
        fprintf(stderr, "Error closing audio stream: %s\n", Pa_GetErrorText(err));
    }

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
    double timestamp;

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
            pthread_mutex_lock(data->startMutex); // Lock when writing to the file
            if (data->file != NULL)
            {
                fwrite(buffer, sizeof(int16_t), FRAMES_PER_BUFFER, data->file);
                fprintf(data->timestampFile, "%.9f\n", timestamp);
            }
            pthread_mutex_unlock(data->startMutex);
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
 * @brief Initializes the microphone data structure.
 *
 * @param data Pointer to the MicData structure.
 * @param micIndex Index of the microphone.
 * @param micName Name of the microphone.
 * @param startMutex Pointer to the start mutex.
 * @param startCond Pointer to the start condition variable.
 * @param startFlag Pointer to the start flag.
 * @param stopFlag Pointer to the stop flag.
 */
void initializeMicData(MicData *data, int micIndex, const char *micName, pthread_mutex_t *startMutex, pthread_cond_t *startCond, int *startFlag, int *stopFlag)
{
    data->recording = 0;
    data->fileIndex = 0;
    data->silenceCounter = 0;
    snprintf(data->micName, sizeof(data->micName), "%s", micName);
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
    data->micIndex = micIndex;
    bufferQueueInit(&data->bufferQueue);
}

/**
 * @brief Starts the recording and writing threads.
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
 * @brief Stops the recording threads for two microphones.
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
 * @brief Cleans up the buffer queues from each microphone.
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
 * Initializes the microphone data structures, sets up the recording devices, and launches the recording and file writing threads.
 * Waits for user input to stop recording and then initiates the encoding of stored files.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return int Exit code of the program.
 */
int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        fprintf(stderr, "Usage: %s <mic1_index> <mic2_index> <sample_rate> <threshold_percentage> <min_silence_time>\n", argv[0]);
        return 1;
    }

    mic1_index = atoi(argv[1]);
    mic2_index = atoi(argv[2]);
    sample_rate = atoi(argv[3]);
    threshold_percentage = atof(argv[4]);
    min_silence_time = atof(argv[5]);

    threshold = MAX_AMPLITUDE * threshold_percentage;
    min_silence_frames = sample_rate / FRAMES_PER_BUFFER * min_silence_time;

    PaError err;
    MicData dataMic1, dataMic2;
    char dir1[256];
    char dir2[256];
    pthread_mutex_t startMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t startCond = PTHREAD_COND_INITIALIZER;
    int stopFlag = 0;
    int startFlag = 0;

    snprintf(dir1, sizeof(dir1), "samples_threads_Mic%d", 1);
    snprintf(dir2, sizeof(dir2), "samples_threads_Mic%d", 2);

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

    initializeMicData(&dataMic1, mic1_index, "Mic1", &startMutex, &startCond, &startFlag, &stopFlag);
    initializeMicData(&dataMic2, mic2_index, "Mic2", &startMutex, &startCond, &startFlag, &stopFlag);

    err = Pa_Initialize();
    if (err != paNoError)
    {
        fprintf(stderr, "Error initializing PortAudio: %s\n", Pa_GetErrorText(err));
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

    Pa_Terminate();

    return 0;
}
