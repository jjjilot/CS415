#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#define NUM_WORKERS 10

void* bank(void* arg);

void* worker(void* arg);


pthread_mutex_t worker_lock;
pthread_cond_t bank_cond;

// pthread_barrier_t start_barrier;

pthread_t *workers;	// workers

pthread_t bank_thread;



int main(){


	// pthread_barrier_init(&start_barrier, NULL, NUM_WORKERS + 1);		// + 1 for the the bank thread
	pthread_mutex_init(&worker_lock, NULL);
	pthread_cond_init(&bank_cond, NULL);

	workers = (pthread_t *)malloc(sizeof(pthread_t) * NUM_WORKERS);

	int numbers[NUM_WORKERS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};		// give some "ids"!


	pthread_create(&bank_thread, NULL, bank, NULL);

	for (int i = 0; i < NUM_WORKERS; ++i)
	{
		pthread_create(&workers[i], NULL, worker, (void*)&(numbers[i]));		// create our workers
	}

	for (int j = 0; j < NUM_WORKERS; ++j){
			pthread_join(workers[j], NULL);				// wait on all of our workers
	}		

	pthread_join(bank_thread, NULL);			// wait on the bank thread


	printf("The bank is now closed :(\n");

	free(workers);
	pthread_mutex_destroy(&worker_lock);
	pthread_cond_destroy(&bank_cond);


}


void* worker(void* arg){

	// pthread_barrier_wait(&start_barrier);

	int *id = (int *)arg;

	pthread_mutex_lock(&worker_lock);

	printf("worker %d waiting on bank to open... \n", *id);
	pthread_cond_wait(&bank_cond, &worker_lock);

	printf("worker %d went to work! :)\n", *id);

	pthread_mutex_unlock(&worker_lock);

	pthread_exit(NULL);

}


void* bank(void* arg){

	sleep(2);
	
	// pthread_barrier_wait(&start_barrier);	// what might happen if we use a barrier instead of sleep here? 

	// pthread_cond_signal(&bank_cond);	// wake up one (chosen randomnly if multiple threads are all waiting on same condition)

	pthread_cond_broadcast(&bank_cond);		// wake up all!

	printf("bank is open for business!\n");

	pthread_exit(NULL);

}










