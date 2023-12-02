/**
 * @file assignment5.c
 * @author Ryan Hecht
 * @brief This assignment simulates a bank with multiple accounts and clients and uses mutual exclusion techniques to ensure that the accounts are updated correctly.
 * 
 * @version 0.1
 * @date 2023-12-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define INPUT_FILE_NAME "assignment5_input.txt"
#define MAX_NUMBER_OF_CLIENT_TRANSACTIONS 50

/**
 * Bank account structure
 */
typedef struct {
    char account_name[10];
    int balance;
    pthread_mutex_t mutex; // Mutex for each account
} BankAccount;

/**
 * Transaction structure
 */
typedef struct {
    char type[10]; // "deposit" or "withdraw"
    char account_name[10];
    int amount;
} Transaction;

/**
 * Client operation structure
 */
typedef struct {
    char client_name[10];
    Transaction *transactions; // Array of transactions
    int num_transactions; // Number of transactions
} ClientOperation;


/**
 * Global variables
 */
BankAccount *accounts;
int number_of_accounts = 0;
ClientOperation *client_operations;
int number_of_clients = 0;

/**
 * Function prototypes
 */
void initialize();

void destroy();

void *client_thread(void *arg);

void process_transaction(const Transaction *transaction);

/**
 * Main function
 */
int main() {
    initialize();

    // Create and start threads for each client
    pthread_t threads[number_of_clients];
    for (int i = 0; i < number_of_clients; i++) {
        if (pthread_create(&threads[i], NULL, client_thread, &client_operations[i])) {
            fprintf(stderr, "Error creating thread\n");
            return 1;
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < number_of_clients; i++) {
        pthread_join(threads[i], NULL);
    }

    // Print results
    printf("No. of Accounts: %d\nNo. of Clients: %d\n", number_of_accounts, number_of_clients);
    for (int i = 0; i < number_of_accounts; i++) {
        printf("%s balance %d\n", accounts[i].account_name, accounts[i].balance);
    }

    destroy();
    return EXIT_SUCCESS;
}

/**
 * Initializes the bank accounts and client operations from the input file
 */
void initialize() {
    FILE *file = fopen(INPUT_FILE_NAME, "r");

    if (file == NULL) {
        perror("Error opening the input file");
        exit(EXIT_FAILURE);
    }

    char line[200];
    char identifier; // Use a char to identify 'A' or 'C'
    char account_name[10];
    int balance;

    // First pass: Count the number of accounts and clients
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%c", &identifier);
        if (identifier == 'A') {
            number_of_accounts++;
        } else if (identifier == 'C') {
            number_of_clients++;
        }
    }

    // Allocate memory for accounts and client operations
    accounts = malloc(number_of_accounts * sizeof(BankAccount));
    client_operations = malloc(number_of_clients * sizeof(ClientOperation));

    // Initialize mutexes for each account
    for (int i = 0; i < number_of_accounts; i++) {
        pthread_mutex_init(&accounts[i].mutex, NULL);
    }

    // Reset file pointer to beginning
    fseek(file, 0, SEEK_SET);

    int account_index = 0;
    int client_index = 0;

    // Second pass: Parse the account balances and client operations
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'A') {
            sscanf(line, "%s balance %d", account_name, &balance);
            strcpy(accounts[account_index].account_name, account_name);
            accounts[account_index].balance = balance;
            account_index++;
        } else if (line[0] == 'C') {
            printf("Client line: %s\n",
                   line);
            client_operations[client_index].transactions = malloc(
                    sizeof(Transaction) * MAX_NUMBER_OF_CLIENT_TRANSACTIONS);
            client_operations[client_index].num_transactions = 0;

            char *ptr = line;

            // Get client name
            sscanf(ptr, "%s ", client_operations[client_index].client_name);

            // Move pointer past the client name
            ptr = strchr(ptr, ' ');
            if (ptr) ptr++;

            // transaction_number = client_operations[client_index].num_transactions
            // need to set: client_operations[client_index].transactions[transaction_number].type
            // need to set: client_operations[client_index].transactions[transaction_number].account_name
            // need to set: client_operations[client_index].transactions[transaction_number].amount

            char transaction_type[10];
            char account[10];
            int amount;

            // Parse transactions
            while (ptr && *ptr) {
                // Parse transaction type, account name, and amount
                int read = sscanf(ptr, "%s %s %d", transaction_type, account, &amount);
                if (read == 3) { // Successfully read a transaction
                    int transaction_number = client_operations[client_index].num_transactions;
                    strcpy(client_operations[client_index].transactions[transaction_number].type, transaction_type);
                    strcpy(client_operations[client_index].transactions[transaction_number].account_name, account);
                    client_operations[client_index].transactions[transaction_number].amount = amount;


                    // Increment the number of transactions
                    client_operations[client_index].num_transactions++;

                    // Move pointer past the current transaction
                    for (int i = 0; i < 3; i++) {
                        while (*ptr && *ptr != ' ') ptr++; // Skip current word
                        if (*ptr == ' ') ptr++; // Skip the space
                    }
                } else {
                    // Break the loop if the transaction is not properly formatted
                    break;
                }
            }

            client_index++;
        }
    }

    fclose(file);
}

/**
 * Lifecycle hook for destroying mutexes and freeing memory
 */
void destroy() {
    // Destroy mutexes for each account
    for (int i = 0; i < number_of_accounts; i++) {
        pthread_mutex_destroy(&accounts[i].mutex);
    }

    // Free memory for accounts and client operations
    free(accounts);
    for (int i = 0; i < number_of_clients; i++) {
        free(client_operations[i].transactions);
    }
    free(client_operations);
}

/**
 * Client thread function
 * @param arg ClientOperation
 * @return NULL
 */
void *client_thread(void *arg) {
    ClientOperation *op = (ClientOperation *) arg;
    for (int i = 0; i < op->num_transactions; i++) {
        process_transaction(&op->transactions[i]);
    }
    return NULL;
}

/**
 * Processes a transaction
 * @param transaction Transaction to process
 */
void process_transaction(const Transaction *transaction) {
    // Find the account involved in the transaction
    for (int i = 0; i < number_of_accounts; i++) {
        if (strcmp(accounts[i].account_name, transaction->account_name) == 0) {
            pthread_mutex_lock(&accounts[i].mutex);
            if (strcmp(transaction->type, "deposit") == 0) {
                accounts[i].balance += transaction->amount;
            } else if (strcmp(transaction->type, "withdraw") == 0) {
                if (accounts[i].balance >= transaction->amount) {
                    accounts[i].balance -= transaction->amount;
                }
            }

            pthread_mutex_unlock(&accounts[i].mutex);
            break;
        }
    }
}