#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>

int main()
{
    int fd, addrlen, n, newfd;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;

    //Create TCP Socket
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
    n = getaddrinfo(NULL, "58000", &hints, &res);
    if (n != 0)
        exit(1);
    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1)
        exit(1);
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1)
        exit(1);

    if (listen(fd, 5) == -1) //mark connection socket as passive
        exit(1);
    
    while ((newfd = accept(fd, (struct sockaddr *)&addr, &addrlen)) != -1) {
        printf("Hello!\n");
    }
}