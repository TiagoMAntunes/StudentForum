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

#include "lib/hash.h"
#include "lib/list.h"
#include "lib/topic.h"
#include "lib/iterator.h"
#include "lib/file_management.h"
#include "lib/question.h"
#include "lib/answer.h"

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

int read_TCP(int fd, char** full_msg, int msg_size){
    int n = read(fd, *full_msg, msg_size);
    
    while((*full_msg)[n-2] != '\n') {
        msg_size *= 2;
        *full_msg = realloc(*full_msg, msg_size);
        n += read(fd, *full_msg + n, msg_size - n);
        
    }
    
    return msg_size;
}

void write_TCP(int fd, char* reply, int msg_size){
    int n;

    n = write(fd, reply, msg_size);
    if(n==-1) exit(1);

    
    while(n < msg_size){
        n = write(fd, reply + n, msg_size - n);
        if(n==-1) exit(1);
    }
    write(1, reply, msg_size);
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

int ndigits(int i){
    int count = 0;

    while(i != 0)
    {
        i /= 10;
        ++count;
    }

    return count;
}

void TCP_input_validation(char * message, int msg_size) {
    char * token, * prefix;
    token = strtok(message, " ");
    prefix = strdup(token);

    if (strcmp(prefix, "QUS") == 0) {
        char * topic, * question, * userID, * qdata, * ext, * img_data;
        int qsize, qIMG, isize;

        token = strtok(NULL, " ");
        userID = strdup(token);

        token = strtok(NULL, " ");
        topic = strdup(token);

        token = strtok(NULL, " ");
        question = strdup(token);

        token = strtok(NULL, " ");
        qsize = atoi(token);


        //manually copy
        token += ndigits(qsize) + 1;
        qdata = calloc(qsize, sizeof(char));
        memcpy(qdata, token, qsize);

        //strtok after skipping
        token += qsize + 1;
        token = strtok(token , " ");
        qIMG = atoi(token);

        token = strtok(NULL, " ");
        ext = strdup(token);

        token = strtok(NULL, " ");
        isize = atoi(token);

        //reposition to avoid destroying data
        token += ndigits(isize)+1;
        img_data = calloc(isize, sizeof(char));
        memcpy(img_data, token, isize);

        free(topic);
        free(question);
        free(userID);
        free(qdata);
        free(ext);
        free(img_data);

    }
}

int main(int argc, char *argv[])
{
    int fd_tcp, fd_udp, newfd, pid;
    socklen_t user_addrlen;
    struct addrinfo hints_tcp, hints_udp, *res_tcp, *res_udp;
    struct sockaddr_in user_addr;
    fd_set set;
    List *topics = newList();
    Hash *topics_hash = createTable(1024, sizeof(List));
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
                    
                    int msg_size = MAX_REQ_LEN;
                    char * message = malloc(sizeof(char) * msg_size);
                    int reply_len;

                    msg_size = read_TCP(newfd, &message, msg_size);
                    printf("Message received: %s\n", message);
                    TCP_input_validation(message, msg_size);
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
                        insertInTable(topics_hash, new, hash(topic_title));
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
                    token = strtok(to_token, " ");
                    token = strtok(NULL, " ");
                    
                    //remove \n
                    int j = strlen(token);
                    if (token[j-1] == '\n') token[j-1] = '\0';
                    
                    //get list of topic questions
                    List * questions_list = getTopicQuestions(token, &j);

                    //Create message data
                    char * message = calloc(7 + ndigits(j) + j * (10 + 1 + 5 + 1 + 2), sizeof(char));
                    char * msg_help = message;

                    //LQR N
                    memcpy(msg_help, "LQR ", 4);
                    msg_help += 4;
                    sprintf(msg_help, "%d ", j);
                    msg_help += ndigits(j) + 1;
                    
                    //populate with the amount of questions
                    Iterator * it = createIterator(questions_list);
                    while (j > 0) {
                        char * question = (char * ) current(next(it));
                        sprintf(msg_help, "%s:%s:%s ", question, "12345", "NA");
                        msg_help += 10 + strlen(question);
                        j--;
                        free(question);
                    }

                    //Finish string
                    *msg_help = '\n';
                    *(++msg_help) = 0;

                    n = sendto(fd_udp, message, strlen(message), 0, (struct sockaddr *) &user_addr, user_addrlen);

                    killIterator(it);
                    listFree(questions_list);

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
