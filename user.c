#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

#define PORT "58017"

int main(int argc, char * argv[]) {
    char buffer[1024];
    char * token;

    int fd, addrlen, n;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char sockBuffer[128];

    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype= SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    n = getaddrinfo(hostname, PORT, &hints, &res);
    if (n != 0) exit(1);

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd==-1) exit(1);

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) exit(1);

    n = read(fd, sockBuffer, 128);
    if (n == -1) exit(1);

    write(1, "echo: ", 6); write(1, sockBuffer, n);


    //get input
    
    fgets(buffer, 1024, stdin);
    if ((token = strchr(buffer, '\n')) != NULL)
        *token = '\0'; //remove last \n if it exists


    token = strtok(buffer, " ");
    
    if (strcmp(token, "reg") == 0 || strcmp(token, "register") == 0) {
        token = strtok(NULL, " ");
        while (token != NULL) {
            printf("%s\n", token);
            token = strtok(NULL, " ");
        }       
    }

    freeaddrinfo(res);
    close(fd);

    return 0;
}