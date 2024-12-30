#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "account.h"
#include "string_parser.c"

#define INITIAL_SIZE 16;
int NUM_ACCS = 0;
account *account_arr;

void* process_transaction(void* arg);
void* update_balance(void* arg);

command_line* read_file_to_command_lines(const char* filename, int* num_lines) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open file");
        exit(1);
    }

    int capacity = INITIAL_SIZE;
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

int main(int argc, char* argv[]) {
    /* process input file and do bank stuff */
    int num_lines = 0;

    command_line *cmd_arr = read_file_to_command_lines(argv[1], &num_lines);
    NUM_ACCS = atoi(cmd_arr[0].command_list[0]);
    account_arr = malloc(sizeof(account) * NUM_ACCS);

    printf("Processing transactions (single-threaded)...");

    for (int i=1; i<num_lines; i++) {
        // account block
        if (strcmp(cmd_arr[i].command_list[0], "index") == 0) {
            int acc_i = atoi(cmd_arr[i].command_list[1]);
            // acc number
            strncpy(account_arr[acc_i].account_number, cmd_arr[++i].command_list[0], 16);
            account_arr[acc_i].account_number[16] = '\0';
            // password
            strncpy(account_arr[acc_i].password, cmd_arr[++i].command_list[0], 8);
            account_arr[acc_i].password[8] = '\0';
            // balance
            account_arr[acc_i].balance = strtod(cmd_arr[++i].command_list[0], NULL);
            // reward rate
            account_arr[acc_i].reward_rate = strtod(cmd_arr[++i].command_list[0], NULL);
            // transaction tracter init to 0
            account_arr[acc_i].transaction_tracter = 0;
        
        // transaction block
        } else {
            process_transaction(&cmd_arr[i]);
        }
    }

    printf("finished\nUpdating balances\n");

    // update bal
    update_balance(0);

    printf("Balances updated. Goodbye!\n");

    free(account_arr);
    for (int i=0; i<num_lines; i++) {
        free_command_line(&cmd_arr[i]);
    }
    free(cmd_arr);

    return 0;
}

void* process_transaction(void *arg) {
    /* run by worker to handle transaction requests */
    command_line *transaction = (command_line*) arg;
    int src_acc_ind = -1;
    int dst_acc_ind = -1;
    double trans_amount = -1;
    // get account index for account_arr
    for (int i=0; i<NUM_ACCS; i++) {
        if (strcmp(account_arr[i].account_number, transaction->command_list[1]) == 0) {
            src_acc_ind = i;
            break;
        }
    }

    // check password
    if (strcmp(account_arr[src_acc_ind].password, transaction->command_list[2]) != 0) {
        return 0;
    }

    // do the transaction
    char trans = transaction->command_list[0][0];
    switch(trans) {
        case 'T':
            // transfer
            for (int i=0; i<NUM_ACCS; i++) {
                if (strcmp(account_arr[i].account_number, transaction->command_list[3]) == 0) {
                    dst_acc_ind = i;
                    break;
                }
            }

            trans_amount = strtod(transaction->command_list[4], NULL);
            account_arr[src_acc_ind].balance -= trans_amount;
            account_arr[src_acc_ind].transaction_tracter += trans_amount;
            account_arr[dst_acc_ind].balance += trans_amount;

            break;
        case 'C':
            // check balance
            // do nothing

            break;
        case 'D':
            // deposit
            trans_amount = strtod(transaction->command_list[3], NULL);
            account_arr[src_acc_ind].balance += trans_amount;
            account_arr[src_acc_ind].transaction_tracter += trans_amount;

            break;
        case 'W':
            // withdrawal
            trans_amount = strtod(transaction->command_list[3], NULL);
            account_arr[src_acc_ind].balance -= trans_amount;
            account_arr[src_acc_ind].transaction_tracter += trans_amount;

            break;
        default:
            // error handling
            printf("Oops, how did we get here");
            exit(1);
    }
    return NULL;
}

void* update_balance(void* arg) {
    /* run by bank thread to update each account.
       return number of times updated each account */
    FILE* f_out = fopen("Output/output.txt", "w");
    
    for (int i=0; i<NUM_ACCS; i++) {
        account_arr[i].balance += (account_arr[i].reward_rate * account_arr[i].transaction_tracter);
        account_arr[i].transaction_tracter = 0;
        fprintf(f_out, "%i balance:  %.2f\n", i, account_arr[i].balance);
        fprintf(f_out, "\n");
    }

    fclose(f_out);
    return NULL;
}