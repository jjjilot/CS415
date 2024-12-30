#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include "command.h"
#include "string_parser.h"
#include <unistd.h>
#include <fcntl.h>

FILE* f_in;
FILE* f_out;
int error;

int run_commands(char** comm_arr) {
    // parse input array and deal with errors
    error = 0;
    char* command = comm_arr[0];

    if (strcmp(command, "exit") == 0) { 
        return 2; 
    } else if (strcmp(command, "cat") == 0) {
        if (comm_arr[1] == 0) { 
            fputs("Error! Unsupported parameters for command: cat\n", f_out); 
            fflush(f_out);
            return 1; }
        displayFile(comm_arr[1]);
        if (error) { 
            fputs("Error! Unsupported parameters for command: cat\n", f_out); 
            fflush(f_out);
            return 1; }
    } else if (strcmp(command, "ls") == 0) {
        if (comm_arr[1] != 0) { 
            fputs("Error! Unsupported parameters for command: ls\n", f_out);
            fflush(f_out);
            return 1; }
        listDir();
    } else if (strcmp(command, "cp") == 0) {
        copyFile(comm_arr[1], comm_arr[2]);
        if (error) {
            fputs("Error! Unsupported parameters for command: cp\n", f_out); 
            fflush(f_out);
            return 1; }
    } else if (strcmp(command, "mv") == 0) {
        moveFile(comm_arr[1], comm_arr[2]);
        if (error) {
            fputs("Error! Unsupported parameters for command: mv\n", f_out); 
            fflush(f_out);
            return 1; }
    } else if (strcmp(command, "pwd") == 0) {
        if (comm_arr[1] != 0) { 
            fputs("Error! Unsupported parameters for command: pwd\n", f_out);
            fflush(f_out); 
            return 1; }
        showCurrentDir();
    } else if (strcmp(command, "rm") == 0) {
        if (comm_arr[1] == 0) { 
            fputs("Error! Unsupported parameters for command: rm\n", f_out); 
            fflush(f_out);
            return 1; }
        deleteFile(comm_arr[1]);
        if (error) { 
            fputs("Error! Unsupported parameters for command: rm\n", f_out); 
            fflush(f_out);
            return 1; }
    } else if (strcmp(command, "mkdir") == 0) {
        if (comm_arr[1] == 0) { 
            fputs("Error! Unsupported parameters for command: mkdir\n", f_out); 
            fflush(f_out);
            return 1; }
        makeDir(comm_arr[1]);
        if (error) {
            fputs("Directory already exists!\n", f_out);
            fflush(f_out);
            return 1;
        }
    } else if (strcmp(command, "cd") == 0) {
        if (comm_arr[1] == 0) { 
            fputs("Error! Unsupported parameters for command: cd\n", f_out); 
            fflush(f_out);
            return 1; }
        changeDir(comm_arr[1]);
        if (error) {
            fputs("Error! Unsupported parameters for command: cd\n", f_out); 
            fflush(f_out);
            return 1; }
    } else {
        fprintf(f_out, "Error! Unrecognized command: %s\n", command);
        fflush(f_out);
        return 1;
    }
    return 0;
}

int input(char** comm_arr)
{
    // buffer time
    size_t len = 128;
    char* line_buf = malloc(len);

    command_line large_token_buffer;
    command_line small_token_buffer;

    while (getline(&line_buf, &len, f_in) != -1) {

        large_token_buffer = str_filler(line_buf, ";");
        for (int i = 0; large_token_buffer.command_list[i] != NULL; i++) {

            // Check for empty commands and skip them
            if (strlen(large_token_buffer.command_list[i]) == 0) {
                continue;
            }

            small_token_buffer = str_filler(large_token_buffer.command_list[i], " ");
            // reset comm_arr
            for (int j = 0; j < 3; j++) {
                comm_arr[j] = 0;
            }

            // copy tokens to comm_arr for command execution
            for (int j = 0; small_token_buffer.command_list[j] != NULL; j++) {
                comm_arr[j] = small_token_buffer.command_list[j];
            }

            // Ensure comm_arr[0] is not NULL before executing
            if (comm_arr[0] == NULL) {
                free_command_line(&small_token_buffer);
                continue;
            }

            // execute commands
            if (run_commands(comm_arr) == 2) {
                free_command_line(&large_token_buffer);
                free_command_line(&small_token_buffer);
                free(line_buf);
                return 2;
            }
            free_command_line(&small_token_buffer);
        }
        free_command_line(&large_token_buffer);
    }
    // free line buffer
    free(line_buf);
    return 0;
}


int main(int argc, char* argv[]) {
    // command array
    char** comm_arr = malloc(sizeof(char*)*3);

    if (argc == 1) {
        // Interactive mode
        char inp[128];
        int repeat = 0;

        f_out = stdout;

        while (repeat != 2) {
            // Create tmp file
            f_in = fopen("tmp", "w+");

            printf(">>> ");
            fgets(inp, sizeof(inp), stdin);
            fprintf(f_in, "%s", inp); 
            rewind(f_in);
            repeat = input(comm_arr);

            // close tmp file
            fclose(f_in);
            remove("tmp");
        }
    } else {
        if (strcmp(argv[1], "-f") != 0 || argc != 3) {
            printf("Usage: ./pseudo-shell -f <file_in>\n");
            exit(EXIT_FAILURE);
        }

        char* filename = argv[2];

        // Open the input file
        f_in = fopen(filename, "r");
        if (f_in == NULL) {
            printf("Unable to locate file: %s\n", filename);
            exit(EXIT_FAILURE);
        }

        // Open the output file
        f_out = fopen("output.txt", "w");
        if (f_out == NULL) {
            printf("Unable to create output file\n");
            fclose(f_in); 
            exit(EXIT_FAILURE);
        }

        // Input time
        input(comm_arr);

        // Close input and output files
        fclose(f_in);
        fclose(f_out);
    }
    free(comm_arr);
    return 0;
}