#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// For reference
struct serverData {
    int port;
    int seed;
    char * file_name;
    int word_count;
    FILE * file;
};

/*
 * Parse command-line arguments and validate them.
 * Assumes that the word count passed matches the total
 * number of words in the wordle file.
 *
 * Example: server.out <listener-port> <seed> <word-filename> <num-words>
 */
int parseArgs(int * argc, char ** argv, struct serverData * server) {
    if (*argc != 5) {
        fprintf(stderr, "ERROR: Invalid number of arguments passed.\n");
        return EXIT_FAILURE;
    }

    server->port = atoi(*(argv + 1));
    if (server->port < 0 || server->port > 65535) {
        fprintf(stderr, "ERROR: Port value out of range.\n");
        return EXIT_FAILURE;
    }

    server->seed = atoi(*(argv + 2));
    if (server->seed < 0) {
        fprintf(stderr, "ERROR: Seed value out of range.\n");
        return EXIT_FAILURE;
    }

    server->file_name = *(argv + 3);
    if (access(server->file_name, F_OK) != 0) {
        fprintf(stderr, "ERROR: file inaccessible.\n");
        return EXIT_FAILURE;
    }

    // Putting faith here
    server->word_count = atoi(*(argv + 4));

    return EXIT_SUCCESS;
}
