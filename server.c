#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>

#define PORT "58017"

int main()
{
    int fd, addrlen, n, newfd, pid;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;

    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);

    //Create TCP Socket
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    n = getaddrinfo(hostname, PORT, &hints, &res);
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

        pid = fork();
		if (pid < 0)
		{
			printf("Unable to fork");
			exit(1);
		}
		else if (pid == 0){ //Child process
            printf("dei fork\n");

            int fdUDP, addrlenUDP, nUDP, pid;
            struct addrinfo hintsUDP, *resUDP;
            struct sockaddr_in addrUDP;

            memset(&hintsUDP, 0, sizeof(hintsUDP));
            hintsUDP.ai_family = AF_INET;
            hintsUDP.ai_socktype= SOCK_DGRAM;
            hintsUDP.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

            //--------------------------------------
            
            write(newfd, "Oi babyyy\n", 11);

            //---------------------------------------

            nUDP = getaddrinfo(hostname, PORT, &hintsUDP, &resUDP);
            if (nUDP != 0) exit(1);

            fdUDP = socket(resUDP->ai_family, resUDP->ai_socktype, resUDP->ai_protocol);//create user-dedicated udp socket
            if (fdUDP==-1) exit(1);

            nUDP = bind(fdUDP, resUDP->ai_addr, resUDP->ai_addrlen);
            if(nUDP==-1) exit(1);

            addrlenUDP = sizeof(addrUDP);

            nUDP = sendto(fdUDP, "Oi\n", 4, 0, (struct sockaddr*) &addrUDP, addrlenUDP);
            if (nUDP==-1) exit(1);

            printf("UDP done!\n");

		}
		else{ //parent process
		}
    }
}