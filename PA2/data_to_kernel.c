#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
//#include <string.h>
#include <errno.h>

#define ERROR_MSG(x) fprintf(stderr, "%s\n", (x))

typedef struct data {
    int sent;
    int num_packets;
    int space_available;
    int last_sender;
    int num_senders;

    pthread_mutex_t mutex;
    pthread_cond_t open;
    pthread_cond_t data_available;
} data;

typedef struct thread_arg_t {
    // Thread struct
    pthread_t thread;
    int thread_num;
    data* data;
    
} thread_arg_t;

data* allocateData(int x) {
   data* buf = (data*)malloc(x * sizeof(data));
   return buf;
}


void* sender_routine(void *thread_args) {
    int run = 1;
    thread_arg_t* d = thread_args;
    data* b = d->data;
    int thread_num = d->thread_num;

    while (run) {
        pthread_mutex_lock(&b->mutex);
        if (b->sent == b->num_packets) {
            run = 0;
            pthread_mutex_unlock(&b->mutex);
            break;
        }
        if ((b->last_sender == (thread_num - 1)) && (b->space_available == 1)) {
            printf("Sender thread %d is going to put data in a packet.\n", thread_num);

            b->space_available = 0;
            if (b->num_senders == thread_num) {
                b->last_sender = 0;
            }
            else {
                b->last_sender = thread_num;
            }

            pthread_cond_broadcast(&b->data_available);
        }
        else {
            printf("No packet available or not my turn to produce, senderthread %d going to sleep.\n", thread_num);

            usleep(10000 + 10000*(thread_num*1000 % 5));
            pthread_cond_wait(&b->open, &b->mutex);
        }
        pthread_mutex_unlock(&b->mutex);
    }
    pthread_exit(NULL);
}

void* worker_routine(void *thread_args) {
    int run = 1;
    thread_arg_t* d = thread_args;
    data* b = d->data;
    int thread_num = d->thread_num;

    while (run) {
        pthread_mutex_lock(&b->mutex);
        if (b->space_available == 0) {
            printf("worker thread %d getting served.\n", thread_num);

            b->sent += 1;
            b->space_available = 1;
            
            if (b->num_packets == b->sent) {
                run = 0;
            }
            pthread_cond_broadcast(&b->open);
        }
        else {
            printf("No data available, going to sleep worker thread %d.\n", thread_num);

            pthread_cond_wait(&b->data_available, &b->mutex);
        }
        pthread_mutex_unlock(&b->mutex);
    }
    pthread_exit(NULL);
}



int main(int argc, char **argv) {
    if (argc != 4) {
        ERROR_MSG("Usage: ./data_to_kernel X N M\nX : # of packets\nN : # sender threads\nM : # worker threads\n");
        exit(-1);
    }

    int num_packets = atoi(argv[1]);
    int num_sender_threads = atoi(argv[2]);
    int num_worker_threads = atoi(argv[3]);
    int status;

    data* data = allocateData(num_packets);
    data->sent = 0;
    data->num_packets = num_packets;
    pthread_mutex_init(&data->mutex, NULL);
    data->space_available = 1;
    pthread_cond_init(&data->open, NULL);
    pthread_cond_init(&data->data_available, NULL);
    data->last_sender = 0;
    data->num_senders = num_sender_threads;

    thread_arg_t tid[num_sender_threads + num_worker_threads];

    for (int i = 0; i < num_worker_threads; i++) {
        tid[i].thread_num = i + 1;
        tid[i].data = data;
        status = pthread_create(&tid[i].thread, NULL, (void*(*)(void*))sender_routine, tid+i);
        if (status != 0) { 
            ERROR_MSG("Error creating sender threads.\n");
            exit(-1);
        }
    }
    for (int j = 0; j < num_sender_threads; j++) {
        tid[num_worker_threads + j].thread_num = j + 1;
        tid[num_worker_threads + j].data = data;
        status = pthread_create(&tid[num_worker_threads + j].thread, NULL, (void*(*)(void*))worker_routine, tid+j);
        if (status != 0) { 
            ERROR_MSG("Error creating worker threads.\n");
            exit(-1);
        }
        
    }

    for (int k = 0; k < num_worker_threads; k++) {
        status = pthread_join(tid[k].thread, NULL);
        if (status != 0) { 
            ERROR_MSG("Error joining worker threads.\n");
            exit(-1);
        }
    }
    for (int l = 0; l < num_sender_threads; l++) {
        status = pthread_join(tid[num_worker_threads + l].thread, NULL);
        if (status != 0) { 
            ERROR_MSG("Error joining sender threads.\n");
            exit(-1);
        }
    }

    free(data);

    return 0;
}