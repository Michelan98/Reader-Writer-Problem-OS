/*
ECSE 427 - Fall 2019
ASSIGNMENT 2 - READER-WRITER PROBLEM
MICHEL ABDEL NOUR
260725050
*/

// Pre-processor directives 
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <float.h>
#include <inttypes.h>

// User-defined variables
#define NUM_WTHREADS 10
#define NUM_RTHREADS 500
#define SLEEP_MAX 101           // 100 milliseconds is the max
// max/min functions
#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)

// semaphores
static sem_t rw_mutex, mutex, queue_mutex;

// shared data variable
static int shared_int;

// function calls
void *reader(void*);
void *writer(void*);
void rand_sleep();

static int read_count = 0;

// variables for waiting times
static double min_w, max_w, min_r, max_r, avg_w, avg_r; 


/*The main function below takes the number of reads & writes as command arguments, creates and runs
the reader writer threads, and prints thread statistics for reading and writing */
int main(int argc, char * argv[]){

    char *endptr;

    // Initializing pthreads
    pthread_t writers[NUM_WTHREADS];
    pthread_t readers[NUM_RTHREADS];

    // assigning variables for user inputs
    int w_input = strtoimax(argv[1], &endptr, 10);
    int r_input = strtoimax(argv[2], &endptr, 10);

    read_count = 0;         // initializing read_count to zero

    // Assigning default values to the waiting times
    max_w = 0;
    min_w = 10000;
    avg_w = 0;

    max_r = 0;
    min_r = 10000;
    avg_r = 0;    

    // checking for valid number of arguments
    if(argc < 3 || argc > 3){
        fprintf(stderr, "Error! Invalid number of arguments. The expected number of arguments is 2 \n");
        exit(1);
    }
    
    // checking for positive value of number of writes / reads
    if((w_input < 0) || (r_input < 0)){
        fprintf(stderr, "\n Error! Arguments must be positive integers.");
        exit(1);
    }

    // Checks if w_input is an int
    if (sscanf (argv[1], "%i", &w_input) != 1) {     
        fprintf(stderr, "\n Error! Number of writes is not an integer \n");
        exit(1);
    }

    // Checks if r_input is an int
    if (sscanf (argv[2], "%i", &r_input) != 1) {      
        fprintf(stderr, "\n Error! Number of reads is not an integer \n");
        exit(1);
    }

    // Checking semaphores were initialized
    if (sem_init(&rw_mutex, 0, 1) == -1) {
        printf("Error initializing rw_mutex semaphore \n");
        exit(1);
    }

    if (sem_init(&mutex, 0, 1) == -1) {
        printf("Error initializing mutex semaphore\n");
        exit(1);
    }
    // Checking if added queue_mutex initialized
    if (sem_init(&queue_mutex, 0, 1) == -1) {
        printf("Error initializing mutex semaphore\n");
        exit(1);
    }
    // creating reader threads
    for(int i=0; i < NUM_RTHREADS; i++){
        if(pthread_create(&readers[i], NULL, reader, (void *)&r_input)){
            printf("Error, creating threads\n");
            exit(1);
        }
    }
    // creating writer threads
    for(int i=0; i < NUM_WTHREADS; i++){
        if(pthread_create(&writers[i], NULL, writer, (void *)&w_input)){
            printf("Error, creating threads\n");
            exit(1);
        }
    }

    // joining reader and writer threads
    for (int i=0; i < NUM_RTHREADS; i++){          
        if(pthread_join(readers[i], NULL)){
            printf("Error, creating threads\n");
            exit(1);
        }
    }

    for (int i=0; i < NUM_WTHREADS; i++){          
        if(pthread_join(writers[i], NULL)){
            printf("Error, creating threads\n");
            exit(1);
        }
    }
    
    // getting total read/write counts
    avg_r /= r_input * NUM_RTHREADS;
    avg_w /= w_input * NUM_WTHREADS;

    printf("Updated value of shared data object :  %d \n\n", shared_int);  // Prints shared data

    printf("===================== Read Statistics======================\n");
    printf("Read minimum wait time (in microseconds) : %f \n", min_r);      
    printf("Read maximum wait time (in microseconds) : %f \n", max_r);
    printf("Read average wait time (in microseconds) : %f \n\n", avg_r);

    printf("===================== Write Statistics======================\n");
    printf("Write minimum wait time in microseconds : %f \n", min_w);       
    printf("Write maximum wait time in microseconds : %f \n", max_w);
    printf("Write average wait time in microseconds : %f \n\n", avg_w);  

    printf("Ratio (avg write wait time/avg read wait time) : %f \n\n", (avg_w/avg_r));

    exit(0);            // when run is successful
}

/* This function is run by the writer threads*/
void *writer(void * arg){
    int w_iterations = *((int *)arg);
    double w_time;
    int loc;

    while(w_iterations > 0){
        clock_t w_start = clock();

        sem_wait(&queue_mutex);         // lock on rw_mutex

        sem_wait(&rw_mutex);            // lock on rw_mutex

        clock_t w_end = clock();

        w_time = (double) (w_end - w_start) / CLOCKS_PER_SEC * 1000;

        min_w = MIN(min_w, w_time);
        max_w = MAX(max_w, w_time);
        avg_w += w_time;

        // updating value of shared_data
        loc = shared_int;
        loc += 10;
        shared_int = loc;

        sem_post(&rw_mutex);            // unlock rw_mutex 

        sem_post(&queue_mutex);         // release queue_mutex

        rand_sleep();                   // sleep for a random time

        w_iterations--;
    }
}

/* This function is run by the reader threads*/
void *reader(void * arg){
    double r_time;
    int r_iterations = *((int *)arg);

    while(r_iterations > 0){
        clock_t r_start = clock();

        sem_wait(&queue_mutex);     // lock on queue_mutex
        
        sem_wait(&mutex);        // lock for reader

        read_count++;

        if(read_count == 1){
            sem_wait(&rw_mutex);    // lock for reader & writer when read_count is 1
        }

        sem_post(&mutex);

        sem_post(&queue_mutex);        // release queue_mutex

        clock_t r_end = clock();

        r_time = (double) (r_end - r_start) / CLOCKS_PER_SEC * 1000;

        sem_wait(&mutex);
        
        read_count--;

        // printf("\n %d", shared_int);

        min_r = MIN(min_r, r_time);
        max_r = MAX(max_r, r_time);
        avg_r += r_time;

        if (read_count == 0) {
            sem_post(&rw_mutex);
        }

        sem_post(&mutex);

        rand_sleep();               // sleep for a random time

        r_iterations--;
    }
}

// helper function: sleep for a random time between 1 and 100 milliseconds
void rand_sleep(){
    int time = (rand() % SLEEP_MAX) * 1000;
    usleep(time);
}