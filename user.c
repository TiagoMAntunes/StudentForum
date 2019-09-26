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
#include "list.h"
#include "topic.h"
#include "iterator.h"

#define PORT        "58017"
#define ERROR       1
#define TRUE        1
#define FALSE       0
#define MAX_REQ_LEN 1024
#define ERR_MSG     "ERR"
#define ERR_LEN     4

int topic_number = -1, question_number = -1;
int n_topics = 500;     // TODO ir atualizando
char *topic = "teste";
char *question = NULL;


int userID;

int create_TCP(char* hostname, struct addrinfo **res) {
    int n, fd;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    n = getaddrinfo(hostname, PORT, &hints, res);
    if (n != 0)
        exit(ERROR);

    fd = socket((*res)->ai_family, (*res)->ai_socktype, (*res)->ai_protocol);
    if (fd==-1) exit(1);

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

int topic_exists(char *topic) {
    // TODO falta guardar a lista de topicos
    return 1;
}

int select_topic(char *temp_topic, int short_cmmd) {
    if (short_cmmd) {
        int len = strlen(temp_topic);
        for (int i = 0; i < len; i++) {
            if (temp_topic[i] > '9' || temp_topic[i] < 0)
                return 0;
            i++;
        }

        if (atoi(temp_topic) < n_topics) {
            topic_number = atoi(temp_topic);

            question_number = -1;
            free(question);
            question = NULL;
            // TODO set topic according to its number
            return 1;
        }
        return 0;
    }
    else {
        if (topic_exists(temp_topic)) {
            if (topic != NULL) free(topic);
            topic = strdup(temp_topic);
            question_number = -1;
            free(question);
            question = NULL;
            // TODO set topic_number according to its name
            return 1;
        }
    }
    return 0;
}

int receive_RGR(char* answer) {
    char *token;

    token = strtok(answer, " ");
    if (strcmp(token, "RGR") != 0) 
        return 0;

    else {
        token = strtok(NULL, " \n");
        if (strcmp(token, "OK") == 0) {
            printf("Successfully registered.\n");
        }
        else if (strcmp(token, "NOK") == 0) {
            printf("Register not successful\n");
        }
        else // wrong protocol
            return 0;     
    }
    return 1;
}

int receive_PTR(char* answer) {
    char *token;

    token = strtok(answer, " ");
    if (strcmp(token, "PTR") != 0) 
        return 0;

    else {
        token = strtok(NULL, " \n");
        if (strcmp(token, "OK") == 0) {
            printf("Topic successfully proposed.\n");
        }
        else if (strcmp(token, "DUP") == 0) {
            printf("Topic already exists.\n");
        }
        else if (strcmp(token, "FUL") == 0) {
            printf("Topic list is full.\n");
        }
        else if (strcmp(token, "NOK") == 0){
            printf("Invalid request.\n");
        }
        else // wrong protocol
            return 0;
    }
    return 1;
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

void write_TCP(int fd, char* reply){
    int n, sent = 0;
    int to_send = strlen(reply), len = to_send;

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

void send_ERR_MSG_UDP(int fd, struct addrinfo **res) {
    int n;
    n = sendto(fd, ERR_MSG, ERR_LEN, 0, (*res)->ai_addr, (*res)->ai_addrlen);
    if (n == -1) {
        exit(ERROR);
    } 
}

void get_txtfile(char* filename, char* txt, int length){
    FILE* f;
    char *buffer;
    int i = 0;

    f = fopen(filename, "r");

    if(f!=NULL){
        buffer = (char*) malloc (sizeof(char) * length + 1);

        while(i<length){//usei isto pq tava a dar merda mas podem mudar
            buffer[i] = fgetc(f);
            i++;
        }

        buffer[i] = '\0';

        strcpy(txt, buffer);

        fclose (f);
    }
    return;
}

int get_filesize(char* filename){
    FILE* f;
    int length;

    f = fopen(filename, "r");

    if(f!=NULL){
        fseek(f, 0, SEEK_END);
        length = ftell(f);

        fseek(f, 0, SEEK_SET);

        fclose (f);
    }

    return length;

}

int ndigits(int i){
    int count;

    while(i != 0)
    {
        i /= 10;
        ++count;
    }

    return count;
}

void receive_input(char * hostname, char* buffer, int fd_udp, struct addrinfo *res_udp) {
    char *token, *stringID;
    int n, user_exists, short_cmmd = 0;
    char answer[1024];
    int tl_available = FALSE;

    int fd_tcp; 
    struct addrinfo *res_tcp;
    while (1) {
        bzero(buffer, 1024);
        bzero(answer, 1024);
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
                printf("%s", answer);
                user_exists = 1; //depende da resposta do server

                if (!receive_RGR(answer)) {
                    send_ERR_MSG_UDP(fd_udp, &res_udp);
                }

                free(message);
            }
            else
                printf("Invalid command.\nregister userID / reg userID (5 digits)\n");
        }

        else if (user_exists && (strcmp(token, "topic_list\n") == 0 || strcmp(token, "tl\n") == 0)) {
            tl_available = TRUE;
            char *message = malloc(sizeof(char) * 5);
            sprintf(message, "LTP\n");
            n = sendto(fd_udp, message, 4, 0, res_udp->ai_addr, res_udp->ai_addrlen);
            if (n == -1) {
                exit(ERROR);
            }

            // TODO receive list of topics
            n = recvfrom(fd_udp, answer, 1024, 0, res_udp->ai_addr, &res_udp->ai_addrlen);
            printf("%s\n", answer);

            // TODO validar protocolo de LTR

            free(message);  
        }

        else if (user_exists && (strcmp(token, "topic_select") == 0 || strcmp(token, "ts")) == 0) {
            if (tl_available) {
                short_cmmd = strcmp(token, "ts") == 0 ? 1 : 0;
                token = strtok(NULL, " ");
                char *temp_topic = strdup(token);

                if (!select_topic(temp_topic, short_cmmd)) {
                    printf("Invalid topic selected.\n");
                }

                printf("Current topic: %s\n", topic);
                free(temp_topic);
            }
            else
                printf("Cannot select topic.\nYou must request topic list first.\n");
        }

        else if (user_exists && (strcmp(token, "topic_propose") == 0 || strcmp(token, "tp") == 0)) {
            char *propose_topic, *message;
            token = strtok(NULL, " \n");
            propose_topic = strdup(token);
            token = strtok(NULL, " ");
            if (token == NULL) {
                message = malloc(sizeof(char) * (strlen(propose_topic) + 12));
                sprintf(message, "PTP %d %s\n", userID, propose_topic);

                n = sendto(fd_udp, message, strlen(message), 0, res_udp->ai_addr, res_udp->ai_addrlen);
                if (n == -1) {
                    exit(ERROR);
                }

                n = recvfrom(fd_udp, answer, 1024, 0, res_udp->ai_addr, &res_udp->ai_addrlen);
                if (n == -1) 
                    exit(ERROR);

                if (!receive_PTR(answer)) {
                    send_ERR_MSG_UDP(fd_udp, &res_udp);
                }

                free(message);
            }
            else
                printf("Invalid command.\ntopic_propose topic / tp topic\n");
            
            free(propose_topic);

        }

        else if (user_exists && (strcmp(token, "question_list\n") == 0|| strcmp(token, "ql\n")) == 0) {
            printf("Entered with topic: %p\n", topic);
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

            n = recvfrom(fd_udp, answer, 1024, 0, res_udp->ai_addr, &res_udp->ai_addrlen);
            printf("%s\n", answer);

            free(message);
        }

// TCP FROM NOW ON
        else if (user_exists && topic != NULL && (strcmp(token, "question_get") == 0 || strcmp(token, "qg") == 0)) {
            char question_title[11];
            short_cmmd = strcmp(token, "qg") == 0 ? 1 : 0;
            token = strtok(NULL, " ");

            if(short_cmmd){
                //TODO get question title from question list, by number
            }

            else
                strcpy(question_title, token);
            
            int msg_size = strlen(question_title) + strlen(topic) + 5;
            char * message = malloc(sizeof(char) * (msg_size+1));
            sprintf(message, "GQU %s %s", topic, question_title);
            fd_tcp = create_TCP(hostname,  &res_tcp);
            n = connect(fd_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen);
            if (n == -1) exit(1);

            //n = sendto(fd_tcp, message, msg_size, 0, res_tcp->ai_addr, res_tcp->ai_addrlen);


            write_TCP(fd_tcp, message);

            //TODO if reply is not 'QGR EOF' or 'QGR ERR', save question_title as currently selected question
            close(fd_tcp);
        }

        else if (strcmp(token, "question_submit") == 0 || strcmp(token, "qs") == 0) {
            char * question, * text_file, * image_file = NULL, * message;
            char *qdata, ext_txt[5] = ".txt";
            int msg_size, qsize, i;
            char qIMG; //flag
            token = strtok(NULL, " ");
            question = strdup(token);
            token = strtok(NULL, " ");
            text_file = malloc(sizeof(char) * strlen(token) + 5);
            strcpy(text_file, token);
            token = strtok(NULL, " ");

            if ((qIMG = token != NULL))
                image_file = strdup(token);

            else{//tirar o \n
                i = strlen(text_file) - 1;
                text_file[i] = '\0';
            }

            strcat(text_file, ext_txt);

            qsize = get_filesize(text_file);
            qdata = (char*) malloc (sizeof(char) * qsize + 1);
            
            get_txtfile(text_file, qdata, qsize);

            msg_size = 15 + strlen(topic) + strlen(question) + ndigits(qsize) + qsize + qIMG;
            
            message = malloc(sizeof(char) * msg_size);
            sprintf(message, "QUS %d %s %s %d %s\n", userID, topic, question, qsize, qdata /*qIMG*/ );
            printf("Message: %sSize: %d %d\n", message, strlen(message), msg_size);
            
            fd_tcp = create_TCP(hostname,  &res_tcp);
            n = connect(fd_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen);
            if (n == -1) exit(1);

            write_TCP(fd_tcp, message);
            close(fd_tcp);

        }

        else if (strcmp(token, "answer_submit") == 0 || strcmp(token, "as") == 0) {
            char * text_file, * image_file = NULL, * message;
            int msg_size;
            char qIMG; //flag
            token = strtok(NULL, " ");
            text_file = strdup(token);
            token = strtok(NULL, " ");
            if ((qIMG = token != NULL))
                image_file = strdup(token);

            msg_size = 15 + strlen(topic)  /* + size of txt file in bytes  + txt file data*/ + qIMG;
            
            message = malloc(sizeof(char) * msg_size);
            sprintf(message, "ANS %d %s %d\n", userID, topic, /* qsize, qdata, */ qIMG );
            
            fd_tcp = create_TCP(hostname,  &res_tcp);
            n = connect(fd_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen);
            if (n == -1) exit(1);

            write_TCP(fd_tcp, message);
            printf("Enviado!\n");
            close(fd_tcp);
        }

        else if (strcmp(token, "exit\n") == 0) {
            printf("Goodbye!\n");
            break;
        }

        else if (!user_exists) {
            printf("You need to login.\n");
        }
        else {
        //    printf("%s\n", token);
            printf("Unknown command. Try again.\n");
        }
        printf(">>> ");
    }
}

int main(int argc, char * argv[]) {
    char buffer[1024];

    int fd_udp;
    struct addrinfo hints_udp, *res_udp;

    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);


    fd_udp = create_UDP(hostname, hints_udp, &res_udp);
    

    printf("=== Welcome to RC Forum! ===\n\n>>> ");
    receive_input(hostname, buffer, fd_udp, res_udp);

    freeaddrinfo(res_udp);
    close(fd_udp);

    return 0;
}
