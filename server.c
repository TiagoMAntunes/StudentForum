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
#include "hash.h"
#include "list.h"
#include "topic.h"
#include "iterator.h"

#define ERROR       1
#define MAX_TOPICS  10
#define MAX_REQ_LEN 1024
#define ERR_MSG     "ERR"
#define ERR_LEN     4

#define max(x, y) (x > y ? x : y)

char port[6] = "58017";


int create_TCP(char* hostname, struct addrinfo hints, struct addrinfo **res) {
    int n, fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    n = getaddrinfo(NULL, port, &hints, res);
    if (n != 0)
        exit(ERROR);

    fd = socket((*res)->ai_family, (*res)->ai_socktype, (*res)->ai_protocol);
    if (fd == -1)
        exit(ERROR);

    n = bind(fd, (*res)->ai_addr, (*res)->ai_addrlen);
    if (n == -1)
        exit(ERROR);

    if (listen(fd, 5) == -1) //mark connection socket as passive
        exit(ERROR);

    return fd;
}

int create_UDP(char* hostname, struct addrinfo hints, struct addrinfo **res) {
    int n, fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    n = getaddrinfo(NULL, port, &hints, res);
    if (n != 0)
        exit(ERROR);

    fd = socket((*res)->ai_family, (*res)->ai_socktype, (*res)->ai_protocol);
    if (fd == -1)
        exit(ERROR);

    n = bind(fd, (*res)->ai_addr, (*res)->ai_addrlen);
    if (n == -1)
        exit(ERROR);

    return fd;
}

void send_ERR_MSG_UDP(int fd, struct addrinfo **res) {
    int n;
    n = sendto(fd, ERR_MSG, ERR_LEN, 0, (*res)->ai_addr, (*res)->ai_addrlen);
    if (n == -1) {
        exit(ERROR);
    } 
}

int registered_user(Hash *users, int userID) {
    // TODO verificar se o user esta registado

    return 1;
}

int verify_ID(char *stringID) {
    int len = strlen(stringID); 
    char new[len +2];
    strcpy(new, stringID);

    if (stringID[-1] != '\n')  
        strcat(new, "\n");
    
    int i = 0;
    while (new[i] != '\n') {
        if (new[i] > '9' || new[i] < 0)
            return 0;
        i++;
    }

    if (i != 5) 
        return 0;

    return 1;
}

int topic_exists(List* topics, char* topic_title) {
    Iterator *it = createIterator(topics);
    while (hasNext(it)) {
        if (strcmp(getTopicTitle(current(next(it))), topic_title) == 0) {
            killIterator(it);
            return 1;
        }
    }
    killIterator(it);
    return 0;
}

int list_topics(List* topics, int n_topics, char *list) {
    Iterator *it = createIterator(topics);


    int t = 0;
    while (hasNext(it)){
        List* next_topic = next(it);
        char* title = getTopicTitle(current(next_topic));
        int len = strlen(title);

        for (int i = 0; i < len; i++) { 
            if (title[i] != '\0')        
                list[t++] = title[i];
        }
        list[t++] = ':';

        char stringID[5];
        sprintf(stringID, "%d", getTopicID(current(next_topic)));
        for (int i = 0; i < 5; i++) {
            list[t++] = stringID[i];
        }
        if (hasNext(it)) {
            list[t++] = ' ';
        }
    }
    list[t++] = '\0';
    killIterator(it);
    return t;

}

void read_TCP(int fd, char* full_msg){
    char buffer[MAX_REQ_LEN], c;
    int n;

    n = read(fd, buffer, MAX_REQ_LEN);

    c = buffer[n-1];
    while (c != '\n') {
        n = read(fd, buffer + n, MAX_REQ_LEN - n);
        c = buffer[n-1];
    }
    
    strcpy(full_msg, buffer);

    return;
}

void write_TCP(int fd, char* reply, int len){
    int n, sent = 0;
    int to_send = len;

    n = write(fd, reply, len);
    if(n==-1) exit(1);

    sent = n;
    to_send -= n;

    while(sent<len){
        n = write(fd, reply + sent, to_send);
        if(n==-1) exit(1);

        sent += n;
        to_send -= n;
    }

    return;
}

int validate_REG(char *msg) {
    char *token = strtok(msg, " "); // REG
    token = strtok(NULL, " "); //ID

    if (token == NULL) {
        return 0;
    }

    return verify_ID(token);
}

int validate_LTP(char *msg) {
    char *token = strtok(msg, " "); // LTP
    token = strtok(NULL, " ");

    return token == NULL;
}

int validate_PTP(char *msg) {
    char *token = strtok(msg, " "); //PTP
    token = strtok(NULL, " ");  // ID

    if (!verify_ID(token)) {
        return 0;
    }

    token = strtok(NULL, " ");
    if (strlen(token) > 10)
        return 0;
    token = strtok(NULL, " ");

    return token == NULL;

}

int validate_LQU(char *msg) {   // TODO tem de validar a existencia do topico?
    char *token = strtok(msg, " "); // LQU

    token = strtok(NULL, " ");
    if (strlen(token) > 10)
        return 0;

    token = strtok(NULL, " ");
    return token == NULL;
}

int main(int argc, char *argv[])
{
    int fd_tcp, fd_udp, newfd, pid;
    socklen_t user_addrlen;
    struct addrinfo hints_tcp, hints_udp, *res_tcp, *res_udp;
    struct sockaddr_in user_addr;
    fd_set set;
    List *topics = newList();
    Hash *users = createTable(1024, sizeof(List));
    int n_topics = 0;


    char hostname[1024], buffer[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);

    if(argc==3){//custom port was set
        if(strcmp(argv[1], "-p") == 0){
            strcpy(port, argv[2]);
            printf("new port is %s\n", port);
        }
    }

    fd_tcp = create_TCP(hostname, hints_tcp, &res_tcp);
    fd_udp = create_UDP(hostname, hints_udp, &res_udp);

    //clear descriptor set
    FD_ZERO(&set);

    int maxfd = max(fd_tcp, fd_udp) + 1;

    for (;;) {
        FD_SET(fd_tcp, &set);
        FD_SET(fd_udp, &set);

        int fd_ready = select(maxfd, &set, NULL, NULL, NULL);

        if (FD_ISSET(fd_tcp, &set)) {
            printf("Receiving from TCP client.\n");
            if ((newfd = accept(fd_tcp, (struct sockaddr *)&user_addr, &user_addrlen)) == -1)
                exit(ERROR);

            else {
                pid = fork();
                if (pid < 0) {
                    printf("Unable to fork");
                    exit(ERROR);
                }
                else if (pid == 0){ //Child process
                    printf("dei fork\n");
                    //write(newfd, "Oi babyyy\n", 11);
                    
                    char message[1024];
                    int reply_len;

                    read_TCP(newfd, message);
                    printf("Message received: %s", message);
                    exit(0);

                }
                else { //parent process
                    close(newfd);
                }
            }
        }
        if (FD_ISSET(fd_udp, &set)) {
            printf("Receiving from UDP client.\n");
            user_addrlen = sizeof(user_addr);
            int n = recvfrom(fd_udp, buffer, 1024, 0, (struct sockaddr *) &user_addr, &user_addrlen);
            if (n == -1)
                exit(ERROR);

            char * token;
            printf("Message received: %s\n", buffer);
            char *to_validate = strdup(buffer); // preciso para validar protocolo
            char *to_token = strdup(buffer);    // os strtok do PTP so funcionam co esta linha
            token = strtok(buffer, " \n");  

            if (strcmp(token, "REG") == 0) {
                if (validate_REG(to_validate)) {
                    // TODO add user
                    n = sendto(fd_udp, "RGR OK\n", 7, 0, (struct sockaddr *) &user_addr, user_addrlen);
                }
                else {
                    send_ERR_MSG_UDP(fd_udp, &res_udp);
                }
            } else if (strcmp(token, "LTP") == 0) {
                if (validate_LTP(to_validate)) {
                    if (n_topics > 0) {
                        char *list = (char*) malloc(sizeof(char) * 17 * n_topics);
                        int list_size = list_topics(topics, n_topics, list);
                        char *message = malloc(sizeof(char) * (7 + list_size));
                        sprintf(message, "LTR %d %s\n", n_topics, list);

                        n = sendto(fd_udp, message, 6 + list_size, 0, (struct sockaddr *) &user_addr, user_addrlen);

                        free(list);
                        free(message);
                    }
                    else 
                        n = sendto(fd_udp, "LTR 0\n", 6, 0, (struct sockaddr *) &user_addr, user_addrlen);
                }
                 else {
                    send_ERR_MSG_UDP(fd_udp, &res_udp);
                }

            } else if (strcmp(token, "PTP") == 0) {
                if (validate_PTP(to_validate)) {
                    token = strtok(to_token, " ");
                    token = strtok(NULL, " ");
                    printf("token = %s\n", token);
                    char *stringID = strdup(token);
                    token = strtok(NULL, " \n");
                    char *topic_title = strdup(token);
                    token = strtok(NULL, " \n");

                    if (n_topics == MAX_TOPICS) {
                        n = sendto(fd_udp, "PTR FUL", 8, 0, (struct sockaddr *) &user_addr, user_addrlen);
                        if (n == -1)
                            exit(ERROR);
                    }

                    else if (token != NULL || !registered_user(users, atoi(stringID)) || strlen(topic_title) > 10) {
                        n = sendto(fd_udp, "PTR NOK", 8, 0, (struct sockaddr *) &user_addr, user_addrlen);
                        if (n == -1)
                            exit(ERROR);
                    }

                    else if (topic_exists(topics, topic_title)) {
                        n = sendto(fd_udp, "PTR DUP", 8, 0, (struct sockaddr *) &user_addr, user_addrlen);
                        printf("Returned duplication information\n");
                        if (n == -1)
                            exit(ERROR);
                    }

                    else {
                        Topic* new = createTopic(topic_title, atoi(stringID));
                        addEl(topics, new);
                        n_topics++;

                        n = sendto(fd_udp, "PTR OK", 7, 0, (struct sockaddr *) &user_addr, user_addrlen);
                        printf("Returned OK information\n");
                        if (n == -1)
                            exit(ERROR);
                    }

                    free(stringID);
                    free(topic_title);
                }
                else {
                    send_ERR_MSG_UDP(fd_udp, &res_udp);
                }

            
            } else if (strcmp(token, "LQU") == 0) {
                if (validate_LQU(to_validate)) {

                

                    n = sendto(fd_udp, "LQR 5 (question:userID:NA )\n", 28, 0, (struct sockaddr *) &user_addr, user_addrlen);
                }
                else {
                    send_ERR_MSG_UDP(fd_udp, &res_udp);
                }

            }

            printf("n:%d errno:%d\n", n, errno);
            if (n == -1)
                exit(ERROR);

            bzero(buffer, 1024);
            free(to_validate);
            free(to_token);
            
        }
    }
}
