#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "string_parser.c"

#define INITIAL_SIZE 16

command_line* read_file_to_command_lines(const char* filename, int* num_lines) {
    // init tons of stuff
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

    // replace newline with null terminator (genius)
    while ((read = getline(&line, &len, file)) != -1) {
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }

        // resize buffer as needed
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

        // tokenize line and update num_lines
        cmd_array[*num_lines] = str_filler(line, " ");
        (*num_lines)++;
    }

    free(line);
    fclose(file);

    return cmd_array;
}

// scheduling stuff
pid_t *pid_arr;
int size;
int current_process = 0;
int *process_alive;

void handle_sigchld(int sig) {
    siginfo_t siginfo;

    // WNOHANG to avoid blocking, WEXITED to check for terminated children only
    // no waitpid()
    while (waitid(P_ALL, 0, &siginfo, WNOHANG | WEXITED) == 0) {
        // break loop if none have exited
        if (siginfo.si_pid == 0) break;

        // process the terminated child by PID
        for (int i = 0; i < size; i++) {
            if (pid_arr[i] == siginfo.si_pid) {
                process_alive[i] = 0;
                break;
            }
        }
    }
}

void scheduler(int sig) {
    static int current_process = 0;
    printf("Scheduler running... (current process: %d)\n", current_process);

    // stop the current process if it is still alive
    if (process_alive[current_process] && kill(pid_arr[current_process], SIGSTOP) == -1) {
        perror("Failed to stop process");
    }

    // find the next process that is still alive
    do {
        current_process = (current_process + 1) % size;
    } while (!process_alive[current_process]);

    // resume the next process
    printf("Continuing process %d\n", current_process);
    if (kill(pid_arr[current_process], SIGCONT) == -1) {
        perror("Failed to continue process");
    }

    alarm(2);
}

int main(int argc, char *argv[]) {
    if (strcmp(argv[1], "-f") != 0) { 
        fprintf(stderr, "Usage: %s -f <filename>\n", argv[0]);
        exit(1);
    }

    command_line *cmd_arr = read_file_to_command_lines(argv[2], &size);

    pid_arr = malloc(sizeof(pid_t) * size);
    process_alive = malloc(sizeof(int) * size);
    
    // initialize process_alive to 1 (alive)
    for (int i = 0; i < size; i++) {
        process_alive[i] = 1;
    }

    // set up signal handlers (child termination and alarm signal)
    signal(SIGCHLD, handle_sigchld);
    signal(SIGALRM, scheduler);
    alarm(2);

    // create child processes
    for (int i = 0; i < size; i++) {
        pid_arr[i] = fork();

        if (pid_arr[i] == 0) {
            // execute command (child only)
            execvp(cmd_arr[i].command_list[0], cmd_arr[i].command_list);
            perror("execvp failed");
            exit(1);
        }
    }

    // start the first process
    if (kill(pid_arr[0], SIGCONT) == -1) {
        perror("Failed to start first process");
        exit(1);
    }

    // continue scheduling and waiting for child processes (parent only)
    while (1) {
        int all_terminated = 1;
        for (int i = 0; i < size; i++) {
            if (process_alive[i]) {
                all_terminated = 0;
                break;
            }
        }
        if (all_terminated) { break; }
        // sleep(1)   // this line prevents breaking up ls output but rubric says no sleep()
    }

    // freedom!
    free(pid_arr);
    free(process_alive);

    for (int i = 0; i < size; i++) {
        free_command_line(&cmd_arr[i]);
    }
    free(cmd_arr);

    return 0;
}