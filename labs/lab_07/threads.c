#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define NUM_WORKERS 10

pthread_t *thread_ids;

pthread_mutex_t counter_lock;

int counter = 0;

void* simulate_work(void* arg);


int main(int argc, char* argv[]){


	thread_ids = (pthread_t *)malloc(sizeof(pthread_t) * NUM_WORKERS);		// create our array of threads :)

	pthread_mutex_init(&counter_lock, NULL);		// initialize the lock

	int numbers[NUM_WORKERS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};		// give some "ids"!


	for(int i = 0; i < NUM_WORKERS; i++){

		pthread_create(&thread_ids[i], NULL, simulate_work, (void*)&(numbers[i]));		// create thread, give job, and cast args to void pointer!

	}

	for (int j = 0; j < NUM_WORKERS; ++j){

		pthread_join(thread_ids[j], NULL);			// wait on our threads to rejoin main thread

	}

	printf("counter = %d\n", counter);



}


void* simulate_work(void* arg){

	int *numbers = (int *)arg;		// cast back to the type we are expecting!

	printf("Thread %d going to sleep... zzzzzzz\n", *numbers);

	sleep(1);

	pthread_mutex_lock(&counter_lock);	// lock the critical section to prevent race condition!
	for (int j = 0; j < 100; j++)
		counter++;
	pthread_mutex_unlock(&counter_lock);	

	pthread_exit(NULL);

}



