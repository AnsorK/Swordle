#ifndef HELPER_H
#define HELPER_H

#include <stdio.h>

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

char * getRandomWord(struct serverData * server);
char isValidWord(char * word, struct serverData * server);
void makeLowercase(char * str);
void makeUppercase(char * str);
int parseArgs(int * argc, char ** argv, struct serverData * S);

#endif
