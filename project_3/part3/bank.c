#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "account.h"
#include "string_parser.c"

#define INITIAL_SIZE 16
#define NUM_WORKERS 10
#define UPDATE_THRESHOLD 5000

int NUM_ACCS = 0;
account *account_arr;
int total_transactions = 0;

// Thread synchronization primitives
pthread_mutex_t account_mutex;
pthread_mutex_t transaction_mutex;
pthread_mutex_t counter_mutex;
pthread_mutex_t total_trans_mutex;
pthread_cond_t bank_cond;
pthread_barrier_t start_barrier;

int update_pending = 0;
int all_transactions_processed = 0;
static int completed_workers = 0;  // Track completed worker threads
static int check_balance_count = 0;

typedef struct {
    command_line *transactions;
    int start_index;
    int end_index;
} thread_data;

// pipe time
int pipe_fd[2];

void* process_transaction(void* arg);
void* update_balance(void* arg);
void auditor_process(int read_fd);

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

    while ((read = getline(&line, &len, file)) != -1) {
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }

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

        cmd_array[*num_lines] = str_filler(line, " ");
        (*num_lines)++;
    }

    free(line);
    fclose(file);
    return cmd_array;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    // fork auditor process
    // Before forking the auditor process
    if (pipe(pipe_fd) == -1) {
        perror("pipe creation failed");
        exit(1);
    }
    pid_t auditor_pid = fork();
    if (auditor_pid == -1) {
        perror("forking error");
        exit(1);
    }

    if (auditor_pid == 0) {
        // child process: auditor
        close(pipe_fd[1]);
        auditor_process(pipe_fd[0]);
        exit(0);
    }

    // parent process: Duck Bank
    close(pipe_fd[0]);

    int num_lines = 0;
    command_line *cmd_arr = read_file_to_command_lines(argv[1], &num_lines);

    NUM_ACCS = atoi(cmd_arr[0].command_list[0]);
    account_arr = malloc(sizeof(account) * NUM_ACCS);

    // Initialize mutexes, condition variable, and barrier
    pthread_mutex_init(&account_mutex, NULL);
    pthread_mutex_init(&transaction_mutex, NULL);
    pthread_mutex_init(&counter_mutex, NULL);
    pthread_mutex_init(&total_trans_mutex, NULL);
    pthread_cond_init(&bank_cond, NULL);
    pthread_barrier_init(&start_barrier, NULL, NUM_WORKERS + 1);

    command_line *transactions = NULL;
    int num_transactions = 0;

    // Process account information
    for (int i = 1; i < num_lines; i++) {
        if (strcmp(cmd_arr[i].command_list[0], "index") == 0) {
            int acc_i = atoi(cmd_arr[i].command_list[1]);
            strncpy(account_arr[acc_i].account_number, cmd_arr[++i].command_list[0], 16);
            account_arr[acc_i].account_number[16] = '\0';
            strncpy(account_arr[acc_i].password, cmd_arr[++i].command_list[0], 8);
            account_arr[acc_i].password[8] = '\0';
            account_arr[acc_i].balance = strtod(cmd_arr[++i].command_list[0], NULL);
            account_arr[acc_i].reward_rate = strtod(cmd_arr[++i].command_list[0], NULL);
            account_arr[acc_i].transaction_tracter = 0;
            snprintf(account_arr[acc_i].out_file, 64, "Output/%s.txt", account_arr[acc_i].account_number);
            FILE* out_file = fopen(account_arr[acc_i].out_file, "w");
            if (out_file != NULL) {
                fprintf(out_file, "account %d:\n", acc_i);
                fclose(out_file);
            }
        } else {
            transactions = &cmd_arr[i];
            num_transactions = num_lines - i;
            break;
        }
    }

    // Create worker and bank threads
    pthread_t worker_threads[NUM_WORKERS];
    pthread_t bank_thread;
    thread_data worker_data[NUM_WORKERS];
    int transactions_per_worker = num_transactions / NUM_WORKERS;

    // Create and start worker threads
    for (int i = 0; i < NUM_WORKERS; i++) {
        worker_data[i].transactions = transactions;
        worker_data[i].start_index = i * transactions_per_worker;
        worker_data[i].end_index = (i == NUM_WORKERS - 1) ? num_transactions : (i + 1) * transactions_per_worker;
        
        // printf("Creating worker thread %d to process transactions %d-%d\n", 
        //        i, worker_data[i].start_index, worker_data[i].end_index);
        pthread_create(&worker_threads[i], NULL, process_transaction, &worker_data[i]);
    }

    pthread_create(&bank_thread, NULL, update_balance, NULL);

    // Release all threads
    pthread_barrier_wait(&start_barrier);

    // Wait for worker threads
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_join(worker_threads[i], NULL);
    }

    // printf("All workers finished. Bank thread updating final balances\n");

    // Signal completion
    pthread_mutex_lock(&transaction_mutex);
    all_transactions_processed = 1;
    pthread_cond_signal(&bank_cond);
    pthread_mutex_unlock(&transaction_mutex);

    // Wait for bank thread
    pthread_join(bank_thread, NULL);

    // printf("All balances updated. Cleaning up...\n");

    // Cleanup
    pthread_mutex_destroy(&account_mutex);
    pthread_mutex_destroy(&transaction_mutex);
    pthread_mutex_destroy(&counter_mutex);
    pthread_mutex_destroy(&total_trans_mutex);
    pthread_cond_destroy(&bank_cond);
    pthread_barrier_destroy(&start_barrier);
    free(account_arr);
    for (int i=0; i<num_lines; i++) {
        free_command_line(&cmd_arr[i]);
    }
    free(cmd_arr);

    return 0;
}

void* process_transaction(void* arg) {
    thread_data *data = (thread_data*) arg;
    static int check_balance_count = 0; // check bal call counter (static to retain val between calls)

    pthread_barrier_wait(&start_barrier);

    for (int i = data->start_index; i < data->end_index; i++) {
        command_line *transaction = &data->transactions[i];
        int src_acc_ind = -1, dst_acc_ind = -1;
        double trans_amount = -1;
        
        pthread_mutex_lock(&account_mutex);
        
        for (int j = 0; j < NUM_ACCS; j++) {
            if (strcmp(account_arr[j].account_number, transaction->command_list[1]) == 0) {
                src_acc_ind = j;
                break;
            }
        }

        if (src_acc_ind == -1 || strcmp(account_arr[src_acc_ind].password, transaction->command_list[2]) != 0) {
            pthread_mutex_unlock(&account_mutex);
            continue;
        }
        
        pthread_mutex_unlock(&account_mutex);

        char trans_type = transaction->command_list[0][0];
        
        if (trans_type == 'C') {
            pthread_mutex_lock(&counter_mutex);
            check_balance_count++;
            pthread_mutex_unlock(&counter_mutex);
        }

        pthread_mutex_lock(&transaction_mutex);
        pthread_mutex_lock(&account_mutex);

        switch (trans_type) {
            case 'T':
                for (int j = 0; j < NUM_ACCS; j++) {
                    if (strcmp(account_arr[j].account_number, transaction->command_list[3]) == 0) {
                        dst_acc_ind = j;
                        break;
                    }
                }
                if (dst_acc_ind != -1) {
                    trans_amount = strtod(transaction->command_list[4], NULL);
                    if (account_arr[src_acc_ind].balance >= trans_amount) {
                        account_arr[src_acc_ind].balance -= trans_amount;
                        account_arr[src_acc_ind].transaction_tracter += trans_amount;
                        account_arr[dst_acc_ind].balance += trans_amount;
                        pthread_mutex_lock(&total_trans_mutex);
                        total_transactions++;
                        pthread_mutex_unlock(&total_trans_mutex);
                    }
                }
                break;

            case 'C':
                // check balance
                if (check_balance_count >= 500) {
                    pthread_mutex_lock(&counter_mutex);
                    check_balance_count -= 500;
                    pthread_mutex_unlock(&counter_mutex);
                    // set time val
                    time_t now = time(NULL);
                    char time_str[26];
                    ctime_r(&now, time_str);
                    time_str[strlen(time_str) - 1] = '\0';

                    // write to pipe for auditor
                    char message[128];
                    snprintf(message, sizeof(message), 
                            "Worker checked balance of Account %s. Balance is $%.2f. Check occured at %s\n",
                            account_arr[src_acc_ind].account_number,
                            account_arr[src_acc_ind].balance,
                            time_str);
                    write(pipe_fd[1], message, strlen(message));
                }
                break;

            case 'D':
                trans_amount = strtod(transaction->command_list[3], NULL);
                account_arr[src_acc_ind].balance += trans_amount;
                account_arr[src_acc_ind].transaction_tracter += trans_amount;
                pthread_mutex_lock(&total_trans_mutex);
                total_transactions++;
                pthread_mutex_unlock(&total_trans_mutex);
                break;

            case 'W':
                trans_amount = strtod(transaction->command_list[3], NULL);
                if (account_arr[src_acc_ind].balance >= trans_amount) {
                    account_arr[src_acc_ind].balance -= trans_amount;
                    account_arr[src_acc_ind].transaction_tracter += trans_amount;
                    pthread_mutex_lock(&total_trans_mutex);
                    total_transactions++;
                    pthread_mutex_unlock(&total_trans_mutex);
                }
                break;
        }

        if (total_transactions >= UPDATE_THRESHOLD && !update_pending) {
            update_pending = 1;
            pthread_cond_signal(&bank_cond);
        }

        pthread_mutex_unlock(&account_mutex);
        pthread_mutex_unlock(&transaction_mutex);
    }

    // Track completed workers and force final update if last worker
    pthread_mutex_lock(&transaction_mutex);
    completed_workers++;
    if (completed_workers == NUM_WORKERS) {
        update_pending = 1;
        pthread_cond_signal(&bank_cond);
    }
    pthread_mutex_unlock(&transaction_mutex);

    return NULL;
}

void* update_balance(void* arg) {
    while (1) {
        pthread_mutex_lock(&transaction_mutex);
        
        while (!update_pending && !all_transactions_processed) {
            pthread_cond_wait(&bank_cond, &transaction_mutex);
        }
        
        if (all_transactions_processed && !update_pending && total_transactions <= 0) {
            pthread_mutex_unlock(&transaction_mutex);
            break;
        }
        
        pthread_mutex_lock(&account_mutex);
        
        for (int i = 0; i < NUM_ACCS; i++) {
            double reward = account_arr[i].reward_rate * account_arr[i].transaction_tracter;
            account_arr[i].balance += reward;
            account_arr[i].transaction_tracter = 0;
            
            FILE* out_file = fopen(account_arr[i].out_file, "a");
            if (out_file != NULL) {
                fprintf(out_file, "Current Balance:\t\t%.2f\n", account_arr[i].balance);
                fclose(out_file);
            }
            // pipe time!
            // set time val
            time_t now = time(NULL);
            char time_str[26];
            ctime_r(&now, time_str);
            time_str[strlen(time_str) - 1] = '\0';
            // write balance to pipe
            char message[128];
            snprintf(message, sizeof(message), 
                    "Applied interest to account %s. New Balance: $%.2f. Time of Update: %s\n",
                    account_arr[i].account_number, 
                    account_arr[i].balance,
                    time_str);
            write(pipe_fd[1], message, strlen(message));
        }

        pthread_mutex_lock(&total_trans_mutex);
        total_transactions -= 5000;
        pthread_mutex_unlock(&total_trans_mutex);
        update_pending = 0;
        
        pthread_mutex_unlock(&account_mutex);
        pthread_mutex_unlock(&transaction_mutex);
    }

    return NULL;
}

void auditor_process(int read_fd) {
    /* write pipe info to ledger.txt */
    FILE *ledger = fopen("Output/ledger.txt", "w");

    char buffer[256];
    ssize_t bytes_read;
    while ((bytes_read = read(read_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        fprintf(ledger, "%s", buffer);
    }

    fclose(ledger);
}