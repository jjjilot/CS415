#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

void signaler(pid_t* pid_ary, int size, int signal);

int main(int argc,char*argv[])
{
	if (argc < 2)
	{
		printf ("Wrong number of arguments\n");
		exit (0);
	}

    int size = atoi(argv[1]);

    pid_t *pid_ary = (pid_t *)malloc(sizeof(pid_t) * size);

    char *args[] = {"./iobound", "-seconds", "10", 0};

    // sigset init
    sigset_t sigset;
    int sig;

    // create empty sigset
    sigemptyset(&sigset);

    // add signal to set
    sigaddset(&sigset, SIGUSR1);

    // add signals in sigset to blocked set (holding in pending state until unblocked)
    sigprocmask(SIG_BLOCK, &sigset, NULL);


    for (int i =0; i<size; ++i) {
        pid_ary[i] = fork();

        if (pid_ary[i] < 0) {
            fprintf(stderr, "fork failed\n");
            exit(-1);
        } else if (pid_ary[i] == 0) {
            printf("CHILD %d WAITING ON SIGSUR1 SIGNAL...\n", getpid());
            sigwait(&sigset, &sig);

            printf("CHILD %d RECEIVED SIGSUR1 SIGNAL SIGSUR1 - CALLING EXEC \n", getpid());
            execvp(args[0], args);
            exit(0);
        }
    }

    signaler(pid_ary, size, SIGUSR1);
    
    signaler(pid_ary, size, SIGSTOP);

    signaler(pid_ary, size, SIGCONT);

    signaler(pid_ary, size, SIGINT);

    free(pid_ary);

	return 0;
}

void signaler(pid_t* pid_ary, int size, int signal) {
    sleep(3);

    for  (int i=0; i < size; i++) {
        printf("PARENT SENDING SIGNAL %d TO CHILD %d...\n", signal, pid_ary[i]);
        kill(pid_ary[i], signal);
    }
}
