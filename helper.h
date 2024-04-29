#ifndef HELPER_H
#define HELPER_H

char * getRandomWord(struct serverData * server);
char isValidWord(char * word, struct serverData * server);
void makeLowercase(char * str);
void makeUppercase(char * str);
int parseArgs(int * argc, char ** argv, struct serverData * S);

#endif
