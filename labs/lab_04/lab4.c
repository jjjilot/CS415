#include<stdio.h>
#include <sys/types.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>

void script_print (pid_t* pid_ary, int size);

int main(int argc,char*argv[])
{
	if (argc != 2)
	{
		printf ("Wrong number of arguments\n");
		exit (0);
	}

    int size = atoi(argv[1]);

    pid_t *pid_ary = (pid_t *)malloc(sizeof(pid_t) * size);

    char *args[] = {"./iobound", "-seconds", "5", 0};

    for (int i =0; i<size; ++i) {
        pid_ary[i] = fork();

        if (pid_ary[i] < 0) {
            fprintf(stderr, "fork failed\n");
            exit(-1);
        } else if (pid_ary[i] == 0) {
            execvp("./iobound", args);
            exit(0);
        }
    }

    script_print(pid_ary, size);

    for (int i=0; i<size; ++i) {
        waitpid(pid_ary[i], NULL, 0);
    }

    free(pid_ary);

	return 0;
}

void script_print (pid_t* pid_ary, int size)
{
	FILE* fout;
	fout = fopen ("top_script.sh", "w");
	fprintf(fout, "#!/bin/bash\ntop");
	for (int i = 0; i < size; i++)
	{
		fprintf(fout, " -p %d", (int)(pid_ary[i]));
	}
	fprintf(fout, "\n");
	fclose (fout);

	char* top_arg[] = {"gnome-terminal", "--", "bash", "top_script.sh", NULL};
	pid_t top_pid;

	top_pid = fork();
	{
		if (top_pid == 0)
		{
			if(execvp(top_arg[0], top_arg) == -1)
			{
				perror ("top command: ");
			}
			exit(0);
		}
	}
}


