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
#include "lib/list.h"
#include "lib/topic.h"
#include "lib/iterator.h"
#include "lib/question.h"
#include "lib/answer.h"

#define PORT        "58017"
#define ERROR       1
#define TRUE        1
#define FALSE       0
#define MAX_REQ_LEN 1024
#define ERR_MSG     "ERR"
#define ERR_LEN     4

int topic_number = -1, question_number = -1;
int n_topics = 0;     
char *topic = NULL;
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

int topic_exists(Hash* topics_hash, char *topic) { 
    List* h =  findInTable(topics_hash, hash(topic));
    Iterator *it = createIterator(h);

    while (hasNext(it)) {
        if (strcmp(getTopicTitle(current(next(it))), topic) == 0) {
            killIterator(it);
            return 1;
        }
    }
    killIterator(it);
    return 0;
}

int select_topic(List* topics, Hash* topics_hash, char *temp_topic, int short_cmmd) {
    Iterator *it = createIterator(topics);
    if (short_cmmd) {      //temp_topic is a number
        int len = strlen(temp_topic);
        for (int i = 0; i < len; i++) {
            if (temp_topic[i] > '9' || temp_topic[i] < 0) {
                killIterator(it);
                return 0;
            }
        }

        if (atoi(temp_topic) <= n_topics) {
            topic_number = atoi(temp_topic);

            question_number = -1;
            question = NULL;

            int n = 1;
            while (n < topic_number) {
                next(it);
                n++;
            }
            topic = getTopicTitle(current(next(it)));
            killIterator(it);
            return 1;
        }

        killIterator(it);
        return 0;
    }
    else {      // temp_topic is a title 
        if (topic_exists(topics_hash, temp_topic)) {
            int n = 0;
            char *title;
            while(hasNext(it)) {
                n++;
                title = getTopicTitle(current(next(it))); 
                if (strcmp(temp_topic, title) == 0) 
                    break;   
            }

            topic_number = n;
            topic = title;
            question_number = -1;
            question = NULL;

            killIterator(it);
            return 1;
        }

        killIterator(it);
        return 0;
    }
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
    //write(1, reply, msg_size);
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
    int count = 0;

    while(i != 0)
    {
        i /= 10;
        ++count;
    }

    return count;
}

void get_img(char* filename, char* img, int size){
    FILE* f;
    char buffer[size];
    int i = 0;

    f = fopen(filename, "r");

    if(f!=NULL){

        while(!feof(f)) {
            fread(buffer, 1, sizeof(buffer), f);
        }

        memcpy(img, buffer, size);

        fclose (f);
    }

    return;
}


void print_topic_list(List *topics) {
    Iterator *it = createIterator(topics);

    printf("AVAILABLE TOPICS:\n");

    int n = 1;
    while(hasNext(it)) {
        List *topic= next(it); 
        char* title = getTopicTitle(current(topic));
        int id = getTopicID(current(topic));
        printf("%d - %s (%d)\n", n++, title, id);
    }
}

void update_topic_list(List* topics, Hash *topics_hash, char* answer) {

    char *token = strtok(answer, " \n");  // LTR
    token = strtok(NULL, " ");  //N
    n_topics = atoi(token);
    if (n_topics == 0) {
        printf("No topics available.\n");
    }
    else {
        for (int i = 0; i < n_topics; i++) {
            char* title = strtok(NULL, ":");
            char *stringID = strtok(NULL, " ");
            int id = atoi(stringID);
            if (!topic_exists(topics_hash, title)) {
                Topic *new = createTopic(title, id);
                insertInTable(topics_hash, new, hash(title));
                addEl(topics, new);
            }
        }

        print_topic_list(topics);
    }
}

void freeTopics(List *topics) {
    Iterator *it = createIterator(topics);

    while(hasNext(it)) {
        deleteTopic(current(next(it)));
    }

    free(topics);
    killIterator(it);
}

void getExtension(char * image, char * ext) {
    int i, j = 0;
    for (i = 0; image[i] != '.'; i++)
        ;
    i++;
    while (image[i] != '\0' && j < 3) {
        ext[j++] = image[i++];
    }
    ext[3] = 0;
}

void receive_input(char * hostname, char* buffer, int fd_udp, struct addrinfo *res_udp) {
    char *token, *stringID;
    int n, user_exists, short_cmmd = 0;
    char answer[1024];
    int tl_available = FALSE;
    List *topics = newList();
    Hash *topics_hash = createTable(1024, sizeof(List));

    int fd_tcp; 
    struct addrinfo *res_tcp;
    while (1) {
        bzero(buffer, 1024);
        bzero(answer, 1024);
        fgets(buffer, 1024, stdin);
        token = strtok(buffer, " ");
        printf("command = %s\n", token);

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
                //printf("%s", answer);
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

            n = recvfrom(fd_udp, answer, 1024, 0, res_udp->ai_addr, &res_udp->ai_addrlen);
            //printf("%s\n", answer);

            // TODO validar protocolo de LTR ?

            update_topic_list(topics, topics_hash, answer);

            free(message);  
        }

        else if (user_exists && (strcmp(token, "topic_select") == 0 || strcmp(token, "ts")) == 0) {
            if (tl_available) {
                short_cmmd = strcmp(token, "ts") == 0 ? 1 : 0;
                token = strtok(NULL, " ");
                char *temp_topic = strdup(token);

                if (!select_topic(topics, topics_hash, temp_topic, short_cmmd)) {
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


            write_TCP(fd_tcp, message, msg_size);

            //TODO if reply is not 'QGR EOF' or 'QGR ERR', save question_title as currently selected question
            close(fd_tcp);
        }

        else if (strcmp(token, "question_submit") == 0 || strcmp(token, "qs") == 0) {
            char * question, * text_file, * image_file = NULL, * message;
            char *qdata, *idata, ext_txt[5] = ".txt";
            int msg_size, qsize, i, n;
            int isize;
            int qIMG; //flag

            token = strtok(NULL, " ");
            question = strdup(token);
            token = strtok(NULL, " ");
            text_file = malloc(sizeof(char) * strlen(token) + 5);
            strcpy(text_file, token);
            token = strtok(NULL, " ");

            if ((qIMG = token != NULL)){
                image_file = strdup(token);
                i = strlen(image_file) - 1;
                if (image_file[i] == '\n')
                    image_file[i] = '\0';  
                printf("image file is: %s\n", image_file);
            }

            else{//tirar o \n
                i = strlen(text_file) - 1;
                text_file[i] = '\0';
            }
            //strcat(text_file, ext_txt); add '.txt'

            qsize = get_filesize(text_file);
            qdata = (char*) malloc (sizeof(char) * qsize + 1);
            
            get_txtfile(text_file, qdata, qsize);

            if(qIMG) {
                isize = get_filesize(image_file);
                idata = (char*) malloc (sizeof(char) * (isize));
                char ext[4];

                getExtension(image_file, ext);
                get_img(image_file, idata, isize);

                msg_size = 21 + strlen(topic) + strlen(question) + ndigits(qsize) + qsize + ndigits(qIMG) + ndigits(isize) + isize;

                message = calloc(msg_size,sizeof(char));
                n = sprintf(message, "QUS %d %s %s %d %s %d %s %d ", userID, topic, question, qsize, qdata, qIMG, ext,isize);
                memcpy(message + n, idata, isize); //copy image to message
                printf("Image size: %d\n", isize);
                message[msg_size-2] = '\n';
                message[msg_size-1] = '\0'; //acho q isto n e preciso?
            }

            else{
                msg_size = 17 + strlen(topic) + strlen(question) + ndigits(qsize) + qsize;
                
                message = malloc(sizeof(char) * (msg_size));
                sprintf(message, "QUS %d %s %s %d %s 0\n", userID, topic, question, qsize, qdata, qIMG);
            }
            
            
            fd_tcp = create_TCP(hostname,  &res_tcp);
            n = connect(fd_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen);
            if (n == -1) exit(1);

            write_TCP(fd_tcp, message, msg_size);
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

            write_TCP(fd_tcp, message, msg_size);
            printf("Enviado!\n");
            close(fd_tcp);
        }

        else if (strcmp(token, "exit\n") == 0) {
            deleteTable(topics_hash);
            freeTopics(topics);
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
