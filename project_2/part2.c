#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "string_parser.c"

#define INITIAL_SIZE 16;

void signaler(pid_t* pid_arr, int size, int signal) {
    for  (int i=0; i < size; i++) {
        kill(pid_arr[i], signal);
    }
}

command_line* read_file_to_command_lines(const char* filename, int* num_lines) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open file");
        exit(1);
    }

    int capacity = INITIAL_SIZE;
    *num_lines = 0;
    command_line* cmd_array = malloc(sizeof(command_line) * capacity); 

    if (cmd_array == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        exit(1);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    // read the file line by line
    while ((read = getline(&line, &len, file)) != -1) {
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }

        // resize the array if needed
        if (*num_lines >= capacity) {
            capacity *= 2;
            command_line* temp = realloc(cmd_array, sizeof(command_line) * capacity);
            if (temp == NULL) {
                perror("Memory allocation failed");
                free(line);
                fclose(file);
                free(cmd_array);
                exit(1);
            }
            cmd_array = temp;
        }

        // tokenize the line and store it in cmd_array
        cmd_array[*num_lines] = str_filler(line, " ");
        (*num_lines)++;
    }

    free(line);
    fclose(file);

    return cmd_array;
}

int main(int argc,char*argv[])
{
    if (strcmp(argv[1], "-f") != 0) { 
        fprintf(stderr, "Usage: %s -f <filename>\n", argv[0]);
        exit(1);
    }

    int size = INITIAL_SIZE;
    command_line *cmd_arr = read_file_to_command_lines(argv[2], &size);

    // sigset init
    sigset_t sigset;
    int sig;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    pid_t *pid_arr = (pid_t *)malloc(sizeof(pid_t) * size);

    for (int i = 0; i < size; i++) {
        pid_arr[i] = fork();

        if (pid_arr[i] < 0) {
            fprintf(stderr, "fork failed\n");
            exit(-1);
        } else if (pid_arr[i] == 0) {
            sigwait(&sigset, &sig);
            execvp(cmd_arr[i].command_list[0], cmd_arr[i].command_list);
            perror("execvp failed");
            exit(0);
        }
    }

    signaler(pid_arr, size, SIGUSR1);
    signaler(pid_arr, size, SIGSTOP);
    signaler(pid_arr, size, SIGCONT);

    for (int i=0; i<size; ++i) {
        waitpid(pid_arr[i], NULL, 0);
    }
    
    // freedom!
    free(pid_arr);

    for (int i = 0; i < size; i++) {
        free_command_line(&cmd_arr[i]);
    }
    free(cmd_arr); 

	return 0;
}