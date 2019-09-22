#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define PORT "58017"
#define ERROR   1

int topic_number = -1;
char * topic;
int userID;

int create_TCP(char* hostname, struct addrinfo hints, struct addrinfo *res) {
    int n, fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    n = getaddrinfo(hostname, PORT, &hints, &res);
    if (n != 0)
        exit(ERROR);

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd==-1) exit(1);

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) exit(1);

    return fd;
}

int create_UDP(char* hostname, struct addrinfo hints, struct addrinfo **res) {
    int n, fd;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICSERV;

    n = getaddrinfo(hostname, PORT, &hints, res);
    if (n != 0)
        exit(ERROR);

    fd = socket((*res)->ai_family, (*res)->ai_socktype, (*res)->ai_protocol);
    if (fd == -1)
        exit(ERROR);

    return fd;
}

int verify_ID(char *stringID) {

    int i = 0;
    while (stringID[i] != '\n') {
        if (stringID[i] > '9' || stringID[i] < 0)
            return 0;
        i++;
    }

    if (i != 5) 
        return 0;

    return 1;

    
}

int select_topic(char *temp_topic, int short_cmmd) {
    /* TODO:
            - if short cmd, verify that its a nbr
            - verify if given topic exists
    */
}

void receive_input(char* buffer, int fd_udp, struct addrinfo *res_udp) {
    char *token, *stringID;
    int n, user_exists, short_cmmd = 0;
    char answer[1024];

    while (1) {
        bzero(buffer, 1024);
        fgets(buffer, 1024, stdin);
        token = strtok(buffer, " ");
        
        if (strcmp(token, "reg") == 0 || strcmp(token, "register") == 0) {
            token = strtok(NULL, " ");
            stringID = strdup(token);

            if (verify_ID(stringID)) {
                userID = atoi(stringID);
                free(stringID);

                char *message = malloc(sizeof(char) * 11);
                sprintf(message, "REG %d\n", userID);
                n = sendto(fd_udp, message, 11, 0, res_udp->ai_addr, res_udp->ai_addrlen);
                if (n == -1) {
                    exit(ERROR);
                }
                
                n = recvfrom(fd_udp, answer, 1024, 0, res_udp->ai_addr, &res_udp->ai_addrlen);
                printf("%s\n", answer);
                user_exists = 1; //depende da resposta do server

                free(message);
            } 
            else 
                printf("Invalid command.\nregister userID / reg userID (5 digits)\n");

            bzero(buffer, 1024);  
        }

        else if (user_exists && (strcmp(token, "topic_list\n") == 0 || strcmp(token, "tl\n") == 0)) {
            char *message = malloc(sizeof(char) * 5);
            sprintf(message, "LTP\n");
            n = sendto(fd_udp, message, 4, 0, res_udp->ai_addr, res_udp->ai_addrlen);
            if (n == -1) {
                exit(ERROR);
            }
            free(message);

            // receive list of topics
        }

        else if (user_exists && (strcmp(token, "topic_select") == 0 || strcmp(token, "ts")) == 0) {
            short_cmmd = (strcmp(token, "ts") == 0 ? 1 : 0);
            token = strtok(NULL, " ");
            char *temp_topic = strdup(token);
            if (!select_topic(topic, short_commd)) {
                printf("Invalid topic selected.\n");
            }

            free(temp_topic);

    /*       
            char type = strcmp(token, "ts") == 0 ? 1 : 0; //1 if must read number
            if (topic != NULL) {free(topic); topic = NULL;}
            topic_number = -1;
            token = strtok(NULL, " ");
            if (type)
                topic_number = atoi(token);
            else
                topic = strdup(token);
            //TODO: needs better parsing
     */   }

        else if (user_exists && (strcmp(token, "topic_propose") == 0 || strcmp(token, "tp") == 0)) {
            char *propose_topic, *message;

            token = strtok(NULL, " ");
            propose_topic = strdup(token);
            message = malloc(sizeof(char) * (strlen(propose_topic) + 12));
            sprintf(message, "PTP %d %s\n", userID, propose_topic);
            n = sendto(fd_udp, message, strlen(message), 0, res_udp->ai_addr, res_udp->ai_addrlen);
            if (n == -1) {
                exit(ERROR);
            }


            //needs to get answer

            free(message);
            free(propose_topic);
        }

        else if (strcmp(token, "question_list\n") == 0|| strcmp(token, "ql\n") == 0) {
            printf("Entered with token: %p\n", topic);
            char * message;
            if (topic != NULL) {
                message = malloc(sizeof(char) * (strlen(topic) + 5));
                sprintf(message, "LQU %s\n", topic);   
            }
            else { 
                message = malloc(sizeof(char) * 17);
                sprintf(message, "LQU %d\n", topic_number);  
            }
            n = sendto(fd_udp, message, strlen(message), 0, res_udp->ai_addr, res_udp->ai_addrlen);
            if (n == -1) {
                exit(ERROR);
            }

            //get answer

            free(message);
        }
        else if (strcmp(token, "exit\n") == 0) {
            printf("Goodbye!\n");
            break;
        }

        else {
            printf("%s\n", token);
            printf("Unknown command. Try again.\n");
        }
        printf(">>> "); 
    }
}

int main(int argc, char * argv[]) {
    char buffer[1024];

    int fd_udp;
//    socklen_t addrlen;
    struct addrinfo hints_udp, *res_udp;
//   struct sockaddr_in addr;
//   char sockBuffer[128];

    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);


    fd_udp = create_UDP(hostname, hints_udp, &res_udp);


    printf("=== Welcome to RC Forum! ===\n\n>>> ");
    receive_input(buffer, fd_udp, res_udp);

    freeaddrinfo(res_udp);
    close(fd_udp);

    return 0;
}