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

volatile sig_atomic_t shutdownFlag = 0;

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

void sigusr1Handler(int signum) {
    shutdownFlag = 1;

    printf("MAIN: SIGUSR1 rcvd; Wordle server shutting down...\n");
    printf("MAIN: Wordle server shutting down...\n\n");
    printf("MAIN: guesses: %d\n", total_guesses);
    printf("MAIN: wins: %d\n", total_wins);
    printf("MAIN: losses: %d\n\n", total_losses);
    printf("MAIN: word(s) played:");

    printWords();
}




struct serverData {
    int listenerPort;
    int seed;
    char * wordFileName;
    int numWords;
    FILE * wordFile;
};

struct clientData {
    struct serverData * S;
    int sd;
};

int parseArgs(int * argc, char ** argv, struct serverData * S);
int countWords(char * fileName);
int server(struct serverData * S);
void * play(void * arg);
void convertToLower(char * str);
void convertToUpper(char * str);
char validWord(char * word, struct serverData * S);
char * getRandomWord(struct serverData * server);
void wordle(int * correctCount, char * result, char * guess, char * actual);

int wordle_server(int argc, char ** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    struct serverData S;
    parseArgs(&argc, argv, &S);
    return server(&S);
}



int main(int argc, char ** argv) {

    // Initialize global variables
    total_guesses = 0;
    total_wins = 0;
    total_losses = 0;

    words = calloc(1, sizeof(char *));

    if (words == NULL) {
        perror( "calloc() failed" );
        return EXIT_FAILURE;
    }










    for (char **ptr = words; *ptr; ++ptr)
        free(*ptr);
    free(words);

    return EXIT_SUCCESS;
}








int parseArgs(int * argc, char ** argv, struct serverData * S) {
    if (*argc != 5) {
        fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: hw4.out <listener-port> <seed> <word-filename> <num-words>\n");
        exit(EXIT_FAILURE);
    }
    S->listenerPort = atoi(*(argv + 1));
    if (S->listenerPort < 0 || S->listenerPort > 65535) {
        fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: hw4.out <listener-port> <seed> <word-filename> <num-words>\n");
        exit(EXIT_FAILURE);
    }
    S->seed = atoi(*(argv + 2));
    if (S->seed < 0) {
        fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: hw4.out <listener-port> <seed> <word-filename> <num-words>\n");
        exit(EXIT_FAILURE);
    }
    S->wordFileName = *(argv + 3);
    if (access(S->wordFileName, F_OK) != 0) {
        fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: hw4.out <listener-port> <seed> <word-filename> <num-words>\n");
        exit(EXIT_FAILURE);
    }
    S->numWords = atoi(*(argv + 4));
    /*int ref = countWords(S->wordFileName);
    if (ref == -1 || ref != S->numWords) {
        printf("%d\n", ref);
        fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: hw4.out <listener-port> <seed> <word-filename> <num-words>\n");
        exit(EXIT_FAILURE);
    }*/
    return EXIT_SUCCESS;
}

int countWords(char * fileName) {
    FILE * file = fopen(fileName, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }
    int wordCount = 0;
    char * word = calloc(7, sizeof(char));
    if (word == NULL) {
        perror("Error allocating memory");
        fclose(file);
        return -1;
    }
    while (fgets(word, 7, file))
        ++wordCount;
    fclose(file);
    free(word);
    return wordCount;
}

int server(struct serverData * S) {
    srand(S->seed);
    FILE * file = fopen(S->wordFileName, "r");
    if (file == NULL) {
        perror("Failed to open the file");
        return(EXIT_FAILURE);
    }
    S->wordFile = file;
    int serverSD;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(serverAddr);
    if ((serverSD = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return(EXIT_FAILURE);
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(S->listenerPort);
    if (bind(serverSD, (struct sockaddr *)&serverAddr, addrLen) == -1) {
        perror("Bind failed");
        return(EXIT_FAILURE);
    }
    if (listen(serverSD, 10) == -1) {
        perror("Listen failed");
        return(EXIT_FAILURE);
    }
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    signal(SIGUSR1, sigusr1Handler);
    printf("MAIN: opened %s (%d words)\nMAIN: Wordle server listening on port {%d}\n", S->wordFileName, S->numWords, S->listenerPort);
    while (!shutdownFlag) {
        int clientSD = accept(serverSD, (struct sockaddr *)&clientAddr, &addrLen);
        struct clientData * C = (struct clientData *)calloc(1, sizeof(struct clientData));
        C->S = S;
        C->sd = clientSD;
        pthread_t threadId;
        if (pthread_create(&threadId, NULL, play, (void *)C) != 0) {
            perror("Thread creation failed");
            close(clientSD);
            free(C);
            return(EXIT_FAILURE);
        }
        pthread_detach(threadId);
    }
    fclose(file);
    return EXIT_SUCCESS;
}

char * getRandomWord(struct serverData * server) {
    rewind(server->wordFile);
    int chosenLine = rand() % server->numWords;
    char * word = calloc(7, sizeof(char));
    for (int line = 0; line <= chosenLine; ++line)
        fgets(word, 7, server->wordFile);
    *(word + 5) = '\0';
    convertToLower(word);
    rewind(server->wordFile);
    return word;
}

void convertToLower(char * str) {
    while (*str != '\0') {
        *str = tolower(*str);
        ++str;
    }
}

void convertToUpper(char * str) {
    while (*str != '\0') {
        *str = toupper(*str);
        ++str;
    }
}

void printWords() {
    char ** ptr = words;
    char ** end = words + words_index;
    do {
        printf(" %s", *ptr);
        ptr++;
    } while (ptr < end);
    printf("\n");
    /*
    char * word = *words;
    while (word != NULL)
        printf(" %s", *(word++));
    printf("\n");*/
}



void * play(void * arg) {
    struct clientData * client = (struct clientData *)arg;
    char * chosenWord = getRandomWord(client->S);
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

char validWord(char * word, struct serverData * S) {
    rewind(S->wordFile);
    char * word2 = calloc(7, sizeof(char));
    for (int i = 0; i < S->numWords; ++i) {
        fgets(word2, 7, S->wordFile);
        *(word2 + 5) = '\0';
        convertToLower(word2);
        if (strcmp(word, word2) == 0) {
            free(word2);
            return 'Y';
        }
    }
    free(word2);
    rewind(S->wordFile);
    return 'N';
}

void wordle(int * correctCount, char * result, char * strGuess, char * strActual) {
    for (int i = 0; i < 5; ++i)
        if (*(strGuess + i) == *(strActual + i)) {
            *(result + i) = toupper(*(strActual + i));
            ++(*correctCount);
        } else if (strchr(strActual, *(strGuess + i)) != NULL)
            *(result + i) = *(strGuess + i);
}
