#include <arpa/inet.h>
#include <ctype.h>
#include "helper.h"
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/*
 * These global variables will be updated by multiple
 * concurrent threads. So it's important to lock them
 * during updates so only one thread has access at a time.
 * This is mutual exclusion (mutex).
 */

int total_guesses;
int total_wins;
int total_losses;
char ** words_used;
int words_used_index;

pthread_mutex_t total_guesses_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t total_wins_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t total_losses_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t words_used_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * These functions update the global variables, using
 * the 'mutex' locking mechanism.
 */

void incrementTotalGuesses() {
    pthread_mutex_lock(&total_guesses_mutex);

    ++total_guesses;

    pthread_mutex_unlock(&total_guesses_mutex);
}

void incrementTotalWins() {
    pthread_mutex_lock(&total_wins_mutex);

    ++total_wins;

    pthread_mutex_unlock(&total_wins_mutex);
}

void incrementTotalLosses() {
    pthread_mutex_lock(&total_losses_mutex);

    ++total_losses;

    pthread_mutex_unlock(&total_losses_mutex);
}

void addToWordsUsed(char * new_word) {
    pthread_mutex_lock(&words_used_mutex);

    // Increase size by 1 to fit new word in
    words_used = realloc(words_used, (words_used_index + 2) * sizeof(char *));

    // Copy word to new address
    *(words_used + words_used_index) = calloc(1, strlen(new_word) + 1);
    strcpy(*(words_used + words_used_index), new_word);

    ++words_used_index;

    pthread_mutex_unlock(&words_used_mutex);
}

/*
 * This variable and function are responsible for
 * shutting down the server when a 'SIGUSR1' signal
 * is sent to the running server.
 */

volatile sig_atomic_t shutdown_flag = 0;

// Print out all the words in 'words_used'
void printWords() {
    char ** curr = words_used;
    char ** end = words_used + words_used_index;

    do {
        printf(" %s", *curr);
        curr++;
    } while (curr < end);

    printf("\n");
}

void sigHandler(int signum) {
    shutdown_flag = 1;

    printf("MAIN: SIGUSR1 rcvd; Wordle server shutting down...\n");
    printf("MAIN: Wordle server shutting down...\n\n");
    printf("MAIN: guesses: %d\n", total_guesses);
    printf("MAIN: wins: %d\n", total_wins);
    printf("MAIN: losses: %d\n\n", total_losses);
    printf("MAIN: word(s) played:");

    printWords();
}

/*
 * Structures to hold important information for
 * both the server and client
 */

struct serverData {
    int port;
    int seed;
    char * file_name;
    int word_count;
    FILE * file;
};

struct clientData {
    int sd;
    struct serverData * server;
};

void * play(void * arg);
void wordle(int * correctCount, char * result, char * guess, char * actual);

/*
 * Welcome to Swordle!
 * Takes in a file containing 5 letter words
 * seperated by a new line, and acts as a
 * server to distribute games to incoming
 * clients. Every client gets a differnt word.
 */
int main(int argc, char ** argv) {

    // Initialize global variables
    total_guesses = 0;
    total_wins = 0;
    total_losses = 0;

    words_used = calloc(1, sizeof(char *));

    if (words_used == NULL) {
        perror( "calloc() failed" );
        return EXIT_FAILURE;
    }

    // Unbuffer output for instantaneous results
    setvbuf(stdout, NULL, _IONBF, 0);

    // Store important server data
    struct serverData * server;
    parseArgs(&argc, argv, server);

    // Seed the random word picker
    srand(server->seed);

    // Open file for parsing
    FILE * file = fopen(server->file_name, "r");
    if (file == NULL) {
        perror("Failed to open the file");
        return EXIT_FAILURE;
    }

    server->file = file;

    // Initialize server and client socket information
    int server_sd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(server_addr);

    if ((server_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(server->port);

    // Bind the descriptor to address and open connection
    if (bind(server_sd, (struct sockaddr *)&server_addr, addr_len) == -1) {
        perror("Bind failed");
        return(EXIT_FAILURE);
    }
    if (listen(server_sd, 10) == -1) {
        perror("Listen failed");
        return(EXIT_FAILURE);
    }

    // Custom handler for termination signals
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
    signal(SIGUSR2, sigHandler);
    signal(SIGUSR1, sigHandler);

    printf("MAIN: opened %s (%d words)\n");
    printf("MAIN: Wordle server listening on port {%d}\n", server->file_name, server->word_count, server->port);

    while (!shutdown_flag) {
        // Get client descriptor and address
        int client_sd = accept(server_sd, (struct sockaddr *)&client_addr, &addr_len);

        struct clientData * client = (struct clientData *)calloc(1, sizeof(struct clientData));
        client->server = server;
        client->sd = client_sd;

        pthread_t thread_id;
        // Execute 'play' for new threads
        if (pthread_create(&thread_id, NULL, play, (void *)client) != 0) {
            perror("Thread creation failed");

            close(client_sd);
            free(client);

            return EXIT_FAILURE;
        }

        // Make thread run independently
        pthread_detach(thread_id);
    }

    // Clean memory
    fclose(file);

    for (char ** ptr = words_used; *ptr; ++ptr)
        free(*ptr);
    free(words_used);

    return EXIT_SUCCESS;
}

/*
 * Game logic for every client!
 */
void * play(void * arg) {
    struct clientData * client = (struct clientData *)arg;
    char * chosen_word = getRandomWord(client->server);
    convertToUpper(chosenWord);
    addWord(chosenWord);
    convertToLower(chosenWord);

    pthread_t threadID = pthread_self();

    printf("MAIN: rcvd incoming connection request\n");

    char * buffer = calloc(6, sizeof(char));
    if (buffer == NULL) {
        perror("Failed to allocate memory for message");
        exit(EXIT_FAILURE);
    }
    *(buffer + 5) = '\0';

    int bytes_received, total_bytes_received = 0;
    short remainingGuesses = 6;
    int correctCount;
    char * result = calloc(5, sizeof(char));
    char * reply = calloc(8, 1);

    do {
        printf("THREAD %lu: waiting for guess\n", (unsigned long) threadID);

        while (total_bytes_received < 5) {
            bytes_received = recv(client->sd, buffer + bytes_received, 6 - bytes_received, 0);
            if (bytes_received == -1) {
                perror("recv");
                exit(EXIT_FAILURE);
            } else if (bytes_received == 0) {
                printf("THREAD %lu: client gave up; closing TCP connection...\n", (unsigned long) threadID);
                convertToUpper(chosenWord);
                printf("THREAD %lu: game over; word was %s!\n", (unsigned long) threadID, chosenWord);
                incrementTotalLosses();
                break;
            } else
                total_bytes_received += bytes_received;
        }
        if (total_bytes_received < 5)
            break;
        else if (total_bytes_received == 5) {
            printf("THREAD %lu: rcvd guess: %s\n", (unsigned long) threadID, buffer);

            --remainingGuesses;
            correctCount = 0;

            convertToLower(buffer);

            char validGuess = validWord(buffer, client->S);
            if (validGuess == 'Y') {
                incrementTotalGuesses();
                memcpy(result, "-----", 5);
                wordle(&correctCount, result, buffer, chosenWord);
            } else {
                memcpy(result, "?????", 5);
                ++remainingGuesses;
            }

            unsigned short value = htons(remainingGuesses);
            memcpy(reply, &validGuess, 1);
            memcpy(reply + 1, &value, 2);
            memcpy(reply + 3, result, 5);

            if (validGuess == 'Y')
                printf("THREAD %lu: sending reply: %s (%hd guess%s left)\n", (unsigned long) threadID, result, remainingGuesses, remainingGuesses == 1 ? "" : "es");
            else
                printf("THREAD %lu: invalid guess; sending reply: %s (%hd guess%s left)\n", (unsigned long) threadID, result, remainingGuesses, remainingGuesses == 1 ? "" : "es");

            int rc = send(client->sd, reply, 8, 0);
            if (rc == -1) {
                perror("Failed to send data to client.");
                pthread_exit(NULL);
            }

            if (correctCount == 5 || remainingGuesses == 0) {
                convertToUpper(chosenWord);
                printf("THREAD %lu: game over; word was %s!\n", (unsigned long) threadID, chosenWord);
                if (correctCount == 5)
                    incrementTotalWins();
                if (remainingGuesses == 0)
                    incrementTotalLosses();
                break;
            }
        }
        bytes_received = total_bytes_received = 0;
    } while(shutdownFlag == 0);

    close(client->sd);

    free(chosenWord);
    free(reply);
    free(result);
    free(buffer);
    free(client);

    pthread_exit(NULL);
}


void wordle(int * correctCount, char * result, char * strGuess, char * strActual) {
    for (int i = 0; i < 5; ++i)
        if (*(strGuess + i) == *(strActual + i)) {
            *(result + i) = toupper(*(strActual + i));
            ++(*correctCount);
        } else if (strchr(strActual, *(strGuess + i)) != NULL)
            *(result + i) = *(strGuess + i);
}
