#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>
//#include <string.h>
#include <errno.h>
#include <semaphore.h>

#define ERROR_MSG(x) fprintf(stderr, "%s\n", (x))

#define THREAD_NUM 2
#define QUEUE_SIZE 30

// Shared buffer
int queue[QUEUE_SIZE];
// Shared count
int count = 0;

int queue_empty_count = 0;
int queue_full_count = 0;

int total_produced_count = 0;
int total_consumed_count = 0;

sem_t semEmpty;
sem_t semFull;

pthread_mutex_t mutex_buffer;

void* producer_routine(void *thread_args) {
    while ((total_produced_count < 500)) {
        // Produce 

        int x = rand() % 100;

        if (sem_wait(&semEmpty) == 0) { 
            queue_empty_count++;
            }
        pthread_mutex_lock(&mutex_buffer);
        if (count < QUEUE_SIZE) {
            queue[count] = x;
            count++;
            total_produced_count++;
        }
        pthread_mutex_unlock(&mutex_buffer);
        sem_post(&semFull);
    }
 
    pthread_exit(NULL);

}

void* consumer_routine(void *thread_args) {

    while ((total_consumed_count < 500)) {
        //int y = -1;

        if (sem_wait(&semFull) == 0) {
            queue_full_count++;
        }
        pthread_mutex_lock(&mutex_buffer);
        if (count > 0) {
            //y = queue[count - 1];
            count--;
            total_consumed_count++;
        }
        pthread_mutex_unlock(&mutex_buffer);
        sem_post(&semEmpty);

        //printf("Got %d\n",y);
    }
    pthread_exit(NULL);

}



int main(int argc, char **argv) {
    srand(time(NULL));

   // if (argc != 2) {
   //     ERROR_MSG("Usage: ./producer_consumer Q \nQ : Queue size\n");
   //     exit(-1);
   // }

    pthread_t th[THREAD_NUM];

    // Initialize semaphore for if the queue is empty and full, as well as a mutex for the shared queue
    pthread_mutex_init(&mutex_buffer, NULL);
    sem_init(&semEmpty, 0, QUEUE_SIZE);
    sem_init(&semFull, 0, 0);

    // Create threads
    for (int i = 0; i < THREAD_NUM; i++) {
        if (i % 2 == 0) {
            if (pthread_create(&th[i], NULL, &producer_routine, NULL) != 0) { ERROR_MSG("Failed to create producer thread\n"); }
        }
        else {
            if (pthread_create(&th[i], NULL, &consumer_routine, NULL) != 0) { ERROR_MSG("Failed to create consumer thread\n"); }
        }
    }

    // Join threads
    for (int j = 0; j < THREAD_NUM; j++) {
        if (pthread_join(th[j], NULL) != 0) { ERROR_MSG("Failed to join thread\n"); }
    }
    printf("Consumer sleep count: %d\n", queue_full_count);
    printf("Producer sleep count: %d\n", queue_empty_count);
    printf("Total items consumed: %d\n", total_consumed_count);
    printf("Total items produced: %d\n", total_produced_count);

    // Clean up semaphores and mutex
    sem_destroy(&semEmpty);
    sem_destroy(&semFull);
    pthread_mutex_destroy(&mutex_buffer);

    return 0;
}