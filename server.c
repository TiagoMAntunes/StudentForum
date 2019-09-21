#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#define ERROR   1
#define max(x, y) (x > y ? x : y)

char port[6] = "58017";


int create_TCP(char* hostname, struct addrinfo hints, struct addrinfo *res) {
    int n, fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    n = getaddrinfo(hostname, port, &hints, &res);
    if (n != 0)
        exit(ERROR);

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1)
        exit(ERROR);

    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1)
        exit(ERROR);

    if (listen(fd, 5) == -1) //mark connection socket as passive
        exit(ERROR);

    return fd;
}

int create_UDP(char* hostname, struct addrinfo hints, struct addrinfo *res) {
    int n, fd;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    n = getaddrinfo(hostname, port, &hints, &res);
    if (n != 0)
        exit(ERROR);

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1)
        exit(ERROR);

    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1)
        exit(ERROR);

    return fd;
}


int main(int argc, char *argv[])
{
    int fd_tcp, fd_udp, addrlen, newfd, pid;
    struct addrinfo hints_tcp, hints_udp, *res_tcp, *res_udp;
    struct sockaddr_in addr;
    fd_set set;

    char hostname[1024], buffer[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);

    if(argc==3){//custom port was set
        if(strcmp(argv[1], "-p") == 0){
            strcpy(port, argv[2]);
            printf("new port is %s\n", port);
        }
    }

    fd_tcp = create_TCP(hostname, hints_tcp, res_tcp);
    fd_udp = create_UDP(hostname, hints_udp, res_udp);

    //clear descriptor set
    FD_ZERO(&set);

    int maxfd = max(fd_tcp, fd_udp) + 1;

    for (;;) {
        FD_SET(fd_tcp, &set);
        FD_SET(fd_udp, &set);

        int fd_ready = select(maxfd, &set, NULL, NULL, NULL);

        if (FD_ISSET(fd_tcp, &set)) {
            printf("Receiving from TCP client.\n");
            if ((newfd = accept(fd_tcp, (struct sockaddr *)&addr, &addrlen)) == -1) 
                exit(ERROR);  
            
            else {
                pid = fork();
                if (pid < 0) {
                    printf("Unable to fork");
                    exit(ERROR);
                }
                else if (pid == 0){ //Child process
                    printf("dei fork\n");
                    write(newfd, "Oi babyyy\n", 11);
                }
                else { //parent process
                }
            }
        }
        if (FD_ISSET(fd_udp, &set)) {
            printf("Receiving from UDP client.\n");
            int n = recvfrom(fd_udp, buffer, 1024, 0, (struct sockaddr *) &addr, &addrlen);
            if (n == -1) 
                exit(ERROR);
            pid = fork();
            if (pid < 0) {
                    printf("Unable to fork");
			        exit(ERROR);
            }
            else if (pid == 0){ //Child process
                printf("dei fork\n");
    
                n = sendto(newfd, "Oi babyyy\n", 11, 0, (struct sockaddr *) &addr, &addrlen);
                if (n == -1) 
                        exit(ERROR);   
            }
            else { //parent process
            }
        }		
    }
}

