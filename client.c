#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }
    struct hostent * hp = gethostbyname("localhost");
    if (hp == NULL) {
        fprintf(stderr, "ERROR: gethostbyname() failed\n");
        return EXIT_FAILURE;
    }
    struct sockaddr_in tcp_server;
    tcp_server.sin_family = AF_INET;
    memcpy((void *)&tcp_server.sin_addr, (void *)hp->h_addr, hp->h_length);
    unsigned short server_port = 8192;
    tcp_server.sin_port = htons(server_port);
    printf("CLIENT: TCP server address is %s\n", inet_ntoa(tcp_server.sin_addr));
    printf("CLIENT: connecting to server...\n");
    if (connect(sd, (struct sockaddr *)&tcp_server, sizeof(tcp_server)) == -1) {
        perror("connect() failed");
        return EXIT_FAILURE;
    }
    while (1) {
        char * buffer = calloc(9, sizeof(char));
        printf("Enter valid word:\n");
        if (fgets(buffer, 9, stdin) == NULL)
            break;
        if (strlen(buffer) != 6 ) {
            printf("CLIENT: invalid -- try again\n");
            continue;
        }
        *(buffer + 5) = '\0';
        printf("CLIENT: Sending to server: %s\n", buffer);
        int n = write(sd, buffer, strlen(buffer));
        if (n == -1) {
            perror("write() failed");
            return EXIT_FAILURE;
        }
        n = read(sd, buffer, 9);
        if (n == -1) {
            perror("read() failed");
            return EXIT_FAILURE;
        }
        else if (n == 0) {
            printf("CLIENT: rcvd no data; TCP server socket was closed\n");
            break;
        }
        else {
            switch (*buffer) {
                case 'N':
                    printf("CLIENT: invalid guess -- try again");
                    break;
                case 'Y':
                    printf("CLIENT: response: %s", buffer + 3);
                    break;
                default:
                    break;
            }
            short guesses = ntohs(*(short *)(buffer + 1));
            printf(" -- %d guess%s remaining\n", guesses, guesses == 1 ? "" : "es");
            if (guesses == 0)
                break;
        }
        free(buffer);
    }
    printf("CLIENT: disconnecting...\n");
    close(sd);
    return EXIT_SUCCESS;
}
