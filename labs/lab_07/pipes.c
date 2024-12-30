#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define NUM_WORKERS 10

pthread_t *thread_ids;

pthread_mutex_t counter_lock;

pthread_mutex_t pipe_lock;

int counter = 0;

int pipe_fd[2];		// our pipe :)

void* simulate_work(void* arg);


int main(int argc, char* argv[]){


	thread_ids = (pthread_t *)malloc(sizeof(pthread_t) * NUM_WORKERS);		// create our array of threads :)

	pthread_mutex_init(&counter_lock, NULL);		// initialize the lock
	pthread_mutex_init(&pipe_lock, NULL);		// initialize the lock

	if (pipe(pipe_fd) == -1) {			// init the pipe!
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

	int numbers[NUM_WORKERS] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};


	for(int i = 0; i < NUM_WORKERS; i++){

		pthread_create(&thread_ids[i], NULL, simulate_work, (void*)&(numbers[i]));		// create thread, give job, and cast args to void pointer!

	}

	for (int j = 0; j < NUM_WORKERS; ++j){

		pthread_join(thread_ids[j], NULL);			// wait on our threads to rejoin main thread

	}

	close(pipe_fd[1]);		// close write end of pipe

	char buffer[100];
    ssize_t bytes_read;
    while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer))) > 0)
        fwrite(buffer, 1, bytes_read, stdout);

	pthread_mutex_destroy(&counter_lock);
	pthread_mutex_destroy(&pipe_lock);
	close(pipe_fd[0]);			// close read end
    free(thread_ids);



}


void* simulate_work(void* arg){

	int *numbers = (int *)arg;		// cast back to the type we are expecting!

	
	for (int j = 0; j < 100; j++){
		usleep(100);		// will cause threads to share cs more often bc ordering is more random (just here to make output make more sense and see that its not happening in serial)
		pthread_mutex_lock(&counter_lock);	// lock the critical section to prevent race condition!
		counter++;
		pthread_mutex_unlock(&counter_lock);	
	}
	
	pthread_mutex_lock(&pipe_lock);	// lock the critical section to prevent race condition!
	char message[100];
    snprintf(message, sizeof(message), "Thread %d finished work, counter = %d\n", *numbers, counter);
    write(pipe_fd[1], message, strlen(message)); 	// write to the pipe	
    pthread_mutex_unlock(&pipe_lock);	

	pthread_exit(NULL);

}










