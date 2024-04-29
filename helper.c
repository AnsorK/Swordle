#include <ctype.h>
#include "helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Get a random word from the wordle file
 * with 'rand()'. The pseudo-random number
 * generator is already seeded at this point.
 */
char * getRandomWord(struct serverData * server) {
    // Start searching from the top
    rewind(server->file);

    int chosen_line = rand() % server->word_count;
    char * word = calloc(7, sizeof(char));

    for (int line = 0; line <= chosen_line; ++line)
        fgets(word, 7, server->file);

    *(word + 5) = '\0';

    makeLowercase(word);

    return word;
}

/*
 * Check if given word 'word' is in the
 * wordle file.
 */
char isValidWord(char * word, struct serverData * server) {
    // Start searching from the top
    rewind(server->file);

    char * valid_word = calloc(7, sizeof(char));

    for (int line = 0; line < server->word_count; ++line) {
        fgets(valid_word, 7, server->file);

        *(valid_word + 5) = '\0';

        makeLowercase(valid_word);

        if (strcmp(word, valid_word) == 0) {
            free(valid_word);
            return 'Y';
        }
    }

    free(valid_word);

    return 'N';
}

/*
 * Make every letter in 'str' lowercase.
 */
void makeLowercase(char * str) {
    while (*str != '\0') {
        *str = tolower(*str);
        ++str;
    }
}

/*
 * Make every letter in 'str' uppercase.
 */
void makeUppercase(char * str) {
    while (*str != '\0') {
        *str = toupper(*str);
        ++str;
    }
}

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
