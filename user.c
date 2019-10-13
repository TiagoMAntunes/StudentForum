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
#include "lib/file_management.h"

#define ERROR       1
#define TRUE        1
#define FALSE       0
#define MAX_REQ_LEN 1024
#define ERR_MSG     "ERR"
#define ERR_LEN     4
#define MIN(A,B) ((A) < (B) ? (A) : (B))

int topic_number = -1, question_number = -1;
int n_topics = 0;
char *topic = NULL;
char *question = NULL;

List * questions_titles;
int n_questions = 0;

int userID;

char port[6] = "58017";

int create_TCP(char* hostname, struct addrinfo **res) {
    int n, fd;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    n = getaddrinfo(hostname, port, &hints, res);
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

    n = getaddrinfo(hostname, port, &hints, res);
    if (n != 0)
        exit(ERROR);

    fd = socket((*res)->ai_family, (*res)->ai_socktype, (*res)->ai_protocol);
    if (fd == -1)
        exit(ERROR);

    return fd;
}

int is_number(char *text) {
    int len = strlen(text);

    for (int i = 0; i < len; i++) {
        if (text[i] < '0' || text[i] > '9')
            return 0;
    }

    return 1;
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

            if (topic != NULL) free(topic);
            topic = strdup(getTopicTitle(current(next(it))));
            killIterator(it);
            return 1;
        }

        killIterator(it);
        return 0;
    }
    else {      // temp_topic is a title
        char *temp_title = strtok(temp_topic, " \n");
        if (topic_exists(topics_hash, temp_title)) {
            int n = 0;
            char *title;
            while(hasNext(it)) {
                n++;
                title = getTopicTitle(current(next(it)));
                if (strcmp(temp_title, title) == 0)
                    break;
            }

            topic_number = n;
            if (topic != NULL) free(topic);
            topic = strdup(title);
            question_number = -1;
            question = NULL;

            killIterator(it);
            return 1;
        }

        killIterator(it);
        return 0;
    }
}

void receive_RGR(char* answer) {
    char *token;

    token = strtok(answer, " ");
    if (strcmp(token, "RGR") != 0)
        printf("Unexpected server response.\n");

    else {
        token = strtok(NULL, " \n");
        if (strcmp(token, "OK") == 0) {
            printf("Successfully registered.\n");
        }
        else if (strcmp(token, "NOK") == 0) {
            printf("Register not successful\n");
        }
        else
            printf("Unexpected server response.\n");       
    }
}

void receive_PTR(char* answer, int *tl_available, char *propose_topic, int userID, List *topics) {
    char *token;

    token = strtok(answer, " ");
    if (strcmp(token, "PTR") != 0) 
        printf("Unexpected server response.\n");
    
    else {
        token = strtok(NULL, " \n");
        if (strcmp(token, "OK") == 0) {
            *tl_available = FALSE;
            if (topic != NULL) free(topic);
            topic = strdup(propose_topic);
            printf("Topic successfully proposed.\nCurrent topic: %s\n", topic);
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
            printf("Unexpected server response.\n");
    }
}

int receive_LTR_LQR(char *answer, char *text) {
    char *aux = strdup(answer);
    char *token = strtok(aux, " ");

    if (strcmp(token, text) != 0) {
        printf("Invalid server response.\n");
        free(aux);
        return 0;
    }
    free(aux);
    return 1;
}

void receive_QUR(char *answer, char* submit_question) {
    char *token;

    token = strtok(answer, " ");
    if (token != NULL && strcmp(token, "QUR") != 0)
        printf("Unexpected server response.\n");

    else if (token != NULL) {
        token = strtok(NULL, " \n");
        if (token != NULL && strcmp(token, "OK") == 0) {
            if (question != NULL) free(question);
            question = strdup(submit_question);
            printf("Question successfully submitted.\nCurrent question: %s\n", question);       
        }
        else if (token != NULL && strcmp(token, "NOK") == 0) {
            printf("Submission not successful\n");
        }
        else if (token != NULL && strcmp(token, "FUL") == 0) {
            printf("Question list is full. Question not submitted.\n");
        }
        else if (token != NULL && strcmp(token, "DUP") == 0) {
            printf("Question already exists. Question not submitted.\n");
        }
        else
            printf("Unexpected server response.\n");       
    }
    else {
         printf("Unexpected server response.\n"); 
    }
}

void receive_ANR(char *answer) {
    char *token = strtok(answer, " ");

    if (token != NULL && strcmp(token, "ANR") != 0)
        printf("Unexpected server response.\n");

    else if (token != NULL) {
        token = strtok(NULL, " \n");
        if (token != NULL && strcmp(token, "OK") == 0) {
            printf("Answer successfully submitted.\n");       
        }
        else if (token != NULL && strcmp(token, "NOK") == 0) {
            printf("Submission not successful\n");
        }
        else if (token != NULL && strcmp(token, "FUL") == 0) {
            printf("Answer list is full. Answer not submitted.\n");
        }
        else
            printf("Unexpected server response.\n");       
    }
    else {
         printf("Unexpected server response.\n"); 
    }
}

int read_TCP(int fd, char** full_msg, int msg_size, int current_offset){
    int n = read(fd, *full_msg + current_offset, msg_size - current_offset);
    int c;
    if (n == 0) return msg_size;
    if (n == msg_size) {
        msg_size *= 2;
        *full_msg = realloc(*full_msg, msg_size);
    }
    while( (c = read(fd, *full_msg + n + current_offset, msg_size - current_offset - n)) != 0) {
        //printf("Value of c: %d\n", c);
        if (c + n >= msg_size)
            msg_size *= 2;
        *full_msg = realloc(*full_msg, msg_size);
        n += c;
        printf("New size: %d\n", msg_size);
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
        free(buffer);
        fclose (f);
    }
    return;
}

int get_filesize(char* filename){
    FILE* f;
    int length = 0;

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
    killIterator(it);
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

void freeQuestions(List *questions) {
    Iterator *it = createIterator(questions);

    while(hasNext(it)) {
        free(current(next(it)));
    }

    listFree(questions);
    killIterator(it);
}

void update_question_list(Hash * topics_hash, char *answer) {
    char *token = strtok(answer, " ");    // LQU
    token = strtok(NULL, " ");  // N
    n_questions = atoi(token);

    List *topic_list = findInTable(topics_hash, hash(topic));
    Topic *current_topic;
    Iterator *it = createIterator(topic_list);
    while (hasNext(it)) {
        current_topic = current(next(it));
        if (strcmp(getTopicTitle(current_topic), topic) == 0)
            break;
    }

    killIterator(it);

    if (n_questions <= 0) {
        printf("Topic \"%s\" has no questions available.\n", topic);
        return;
    }

    // free outdated list of questions
    if (questions_titles != NULL) 
        freeQuestions(questions_titles);

    questions_titles = newList();
    //display the questions available for the topic and save them
    printf("Topic : %s -> Questions:\n", topic);

    int n = 0;
    while ((token = strtok(NULL, " ")) != NULL) {
        char question[11], userID[6], n_answers[3];
        int i = 0;
        while (token[i] != ':') {
            question[i] = token[i];
            i++;
        }

        token += i+1;
        question[i] = 0;
        i = 0;

        while (token[i] != ':') {
            userID[i] = token[i];
            i++;
        }

        token += i+1;
        userID[i] = 0;
        i = 0;

        while (i < 2) {
            n_answers[i] = token[i];
            i++;
        }
        
        n_answers[i] = 0;


        printf("%d - %s (by %s - %s answers)\n", ++n, question, userID, n_answers);
        addEl(questions_titles, strdup(question));
    }
}

void freeTopics(List *topics) {
    Iterator *it = createIterator(topics);

    while(hasNext(it)) {
        deleteTopic(current(next(it)));
    }

    listFree(topics);
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

int validate_qs_as_input(char *input, int n_parts) {
    int n = 0;
    char * aux = strdup(input);
    char *token = strtok(aux, " ");

    while (token != NULL) {
        token = strtok(NULL, " ");
        n++;
    }

    free(aux);

    return (n == n_parts) || (n == n_parts - 1);
}



void receive_input(char * hostname, char* buffer, int fd_udp, struct addrinfo *res_udp) {
    char *token, *stringID;
    int n, user_exists, short_cmmd = 0, used_tcp = 0;
    char answer[1024];
    int tl_available = FALSE;   // true if user asked for topic list
    int ql_available = FALSE;   // true if user asked for question list
    List *topics = newList();
    Hash *topics_hash = createTable(1024, sizeof(List));
    int fd_tcp;
    struct addrinfo *res_tcp;

    while (1) {
        bzero(buffer, 1024);
        bzero(answer, 1024);
        fgets(buffer, 1024, stdin);
        char *to_validate = strdup(buffer);
        token = strtok(buffer, " ");

        if (strcmp(token, "reg") == 0 || strcmp(token, "register") == 0) {
            token = strtok(NULL, " ");
            stringID = strdup(token);

            if (verify_ID(stringID)) {
                userID = atoi(stringID);
                free(stringID);

                char *message = malloc(sizeof(char) * 11);
                sprintf(message, "REG %d\n", userID);
                n = sendto(fd_udp, message, strlen(message), 0, res_udp->ai_addr, res_udp->ai_addrlen);
                if (n == -1) {
                    exit(ERROR);
                }

                n = recvfrom(fd_udp, answer, 1024, 0, res_udp->ai_addr, &res_udp->ai_addrlen);
                if (n < 0 &&errno == EAGAIN) {
                    printf("Server didn't respond. Try again.\n");
                }
                else if (n == -1 && errno != EAGAIN) {
                    exit(ERROR);
                }
                else {
                    user_exists = 1; //depende da resposta do server
                    receive_RGR(answer);
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
            if (n < 0 && errno == EAGAIN) {
                printf("Server didn't respond. Try again.\n");
            }
            else if (n == -1 && errno != EAGAIN) {
                exit(ERROR);
            }
            else {
                if (receive_LTR_LQR(answer, "LTR")) 
                    update_topic_list(topics, topics_hash, answer);
            }

            free(message);
        }

        else if (user_exists && (strcmp(token, "topic_select") == 0 || strcmp(token, "ts") == 0)) {
            if (tl_available) {
                short_cmmd = strcmp(token, "ts") == 0 ? 1 : 0;
                token = strtok(NULL, " ");
                char *temp_topic = strdup(token);

                if (!select_topic(topics, topics_hash, temp_topic, short_cmmd)) {
                    printf("Invalid topic selected.\n");
                }
                else {                
                    ql_available = FALSE;
                    printf("Current topic: %s\n", topic);
                }

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
                if (n < 0 && errno == EAGAIN) {
                    printf("Server didn't respond. Try again.\n");
                }
                else if (n == -1 && errno != EAGAIN) {
                    exit(ERROR);
                }
                else {
                    receive_PTR(answer, &tl_available, propose_topic, userID, topics); 
                }     
                free(message);
            }
            else
                printf("Invalid command.\ntopic_propose topic / tp topic\n");

            free(propose_topic);
        }

        else if (user_exists && (strcmp(token, "question_list\n") == 0|| strcmp(token, "ql\n")) == 0) {
            if (topic != NULL) {
                char * message;
                int topic_len = strlen(topic);
                message = malloc(sizeof(char) * (topic_len + 6));
                sprintf(message, "LQU %s\n", topic);

                n = sendto(fd_udp, message, strlen(message), 0, res_udp->ai_addr, res_udp->ai_addrlen);
                if (n == -1) {
                    exit(ERROR);
                }

                n = recvfrom(fd_udp, answer, 1024, 0, res_udp->ai_addr, &res_udp->ai_addrlen);
                if (n < 0 && errno == EAGAIN) {
                    printf("Server didn't respond. Try again.\n");
                }
                else if (n == -1 && errno != EAGAIN) {
                    exit(ERROR);
                }
                else {
                    int j = strlen(answer);
                    if (answer[j-1] == '\n') answer[j-1] = '\0';

                    if (receive_LTR_LQR(answer, "LQR")) {
                        update_question_list(topics_hash, answer);
                        ql_available = TRUE;
                    }
                }

                free(message);
            }

            else {
                printf("No topic selected.\n");
            }
        }

// TCP FROM NOW ON
        else if (user_exists && (strcmp(token, "question_get") == 0 || strcmp(token, "qg") == 0)) {
            if (ql_available) {
                char question_title[11];
                short_cmmd = strcmp(token, "qg") == 0 ? 1 : 0;

                token = strtok(NULL, " \n");
                if (token != NULL) {
                    if(short_cmmd && is_number(token) && atoi(token) <= n_questions) {
                        int n = n_questions - atoi(token) + 1;
                        Iterator * it = createIterator(questions_titles);
                        char * helper;
                        while (n-- > 0) {
                            helper = current(next(it));
                        }
                        strcpy(question_title, helper);
                        killIterator(it);
                    }

                    else 
                        strcpy(question_title, token);

                    question = strdup(question_title);

                    int msg_size = strlen(question_title) + strlen(topic) + 7;
                    char * message = malloc(sizeof(char) * (msg_size+1));
                    int k = sprintf(message, "GQU %s %s\n", topic, question_title);
                    //message[k] = '\n';

                    fd_tcp = create_TCP(hostname,  &res_tcp);
                    n = connect(fd_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen);
                    if (n == -1) exit(1);

                    write_TCP(fd_tcp, message, msg_size - 1);
                    
                    //get answer
                    int n;
                    bzero(answer, 1024);
                    while ((n = read(fd_tcp, answer, 1024)) == 0) ;
                    token = strtok(answer, " ");
                    if (strcmp(token, "QGR") != 0) 
                        printf("Unexpected server response.\n");

                    else {
                        token = strtok(NULL, " ");
                        // verify server response
                        if (token != NULL && strcmp(token, "EOF\n") == 0) {
                            printf("No such query or topic available.\n");
                        }
                        else if (token != NULL && strcmp(token, "ERR\n") == 0) {
                            printf("Request not properly formulated.\nquestion_get question / reg question_number\n");
                        }
                        else {
                            char qUserID[6];
                            sprintf(qUserID, "%s", token);
                            
                            token = strtok(NULL, " ");
                            int qsize = atoi(token);

                            char * aux = token + ndigits(qsize) + 1;

                            validateDirectories(topic, question);
                            if (aux - answer >= n) {
                                bzero(answer, 1024);
                                while ((n = read(fd_tcp, answer, 1024)) == 0) ;
                                aux = answer;
                            }
                            char qdata[1024];
                            bzero(qdata, 1024);
                            memcpy(qdata, aux, MIN(qsize, 1024 - (aux - answer)));
                            
                            int changed = 0;
                            writeTextFile(question, topic, qdata, 1024, qsize, fd_tcp, &changed);

                            if (changed) {
                                bzero(answer, 1024);
                                while ((n = read(fd_tcp, answer, 1024)) == 0) ;
                                aux = answer + 1;
                            } else {
                                aux += qsize + 1;
                            }
                            token = strtok(aux, " ");
                            if (token == NULL) {
                                bzero(answer, 1024);
                                while ((n = read(fd_tcp, answer, 1024)) == 0) ;
                                aux = answer;
                                token = strtok(aux, " ");
                            }
                            int qIMG = atoi(token);
                            
                            char ext[4] = {0};
                            if (qIMG) {
                                token = strtok(NULL, " ");
                                sprintf(ext, "%s", token);
                                token = strtok(NULL, " ");
                                int isize = atoi(token);
                                aux = token + ndigits(isize) + 1;
                                //reuse qdata for less memory
                                bzero(qdata, 1024);
                                if (aux - answer >= n) {
                                    //re-read
                                    bzero(answer, 1024);
                                    while ((n = read(fd_tcp, answer, MIN(1024, isize))) == 0) ;
                                    aux = answer;
                                    memcpy(qdata, answer, n);
                                } else {
                                    int offset = n - (aux - answer);
                                    memcpy(qdata, aux, offset);
                                    if ( offset < isize)
                                        //read what it can to the buffer
                                        while ( (n = read(fd_tcp, qdata + offset, MIN(1024 - offset, isize))) == 0 );
                                }
                                changed = 0;
                                writeImageFile(question, topic, qdata, 1024, isize, fd_tcp, &changed, ext);
                                if(changed) {
                                    bzero(answer, 1024);
                                    while ((n = read(fd_tcp, answer, 1024)) == 0) ;
                                    aux = answer + 1;
                                } else 
                                    aux += isize + 1;
                            }
                            token = strtok(aux, " ");
                            int N = atoi(token);
                            aux = token + strlen(token) + 1;
                            printf("I've found: %d answers!\n", N);
                            while (N-- > 0) {
                                
                                if (aux - answer >= n) {
                                    bzero(answer, 1024);
                                    while ((n = read(fd_tcp, answer, 1024)) == 0) ;
                                    aux = answer;
                                }

                                
                                token = strtok(aux, " ");
                                printf("Token is: %s; with length %ld\n", token, strlen(token));
                                int answer_number = atoi(token);
                                printf("Answer number is now: %d\n", answer_number);
                                aux = token + strlen(token) + 1;
                                printf("Moved aux. It is now: %s\n", aux);

                                if (aux - answer >= n) {
                                    bzero(answer, 1024);
                                    while ((n = read(fd_tcp, answer, 1024)) == 0) ;
                                    aux = answer;
                                    printf("Re-read. it is now: %s\n", aux);
                                }

                                token = strtok(aux , " ");
                                printf("Token is: %s; with length %ld\n", token, strlen(token));
                                strcpy(qUserID, token);
                                printf("User ID: %s\n", qUserID);
                                printf("Token is: %s; with a strlen of %ld\n", token, strlen(token));
                                aux = token +  strlen(token) + 1;
                                
                                if (aux - answer >= n) {
                                    bzero(answer, 1024);
                                    while ((n = read(fd_tcp, answer, 1024)) == 0) ;
                                    aux = answer;
                                }
                                printf("Aux is now: %s\n", aux);
                                token = strtok(aux, " ");
                                qsize = atoi(token);
                                printf("Qsize is %d\n", qsize);
                                aux += strlen(token) + 1;
                                
                                if (aux - answer >= n) {
                                    bzero(answer, 1024);
                                    while ((n = read(fd_tcp, answer, 1024)) == 0) ;
                                    aux = answer;
                                }

                                answerDirectoriesValidationWithNumber(topic, question, answer_number);
                                bzero(qdata, 1024);
                                memcpy(qdata, aux, MIN(qsize, 1024 - (aux - answer)));
                                

                                if (1024 - (aux - answer) < qsize) {
                                    read(fd_tcp, qdata + MIN(qsize, 1024 - (aux - answer)), 1024 - MIN(qsize, 1024 - (aux - answer)));
                                }

                                changed = 0;
                                answerWriteTextFile(question, topic, qdata, 1024, qsize, fd_tcp, &changed, answer_number);

                                if (changed){
                                    read(fd_tcp, answer, 1024);
                                    aux = answer + 1;
                                } else
                                    aux += qsize + 1; 

                                if (aux - answer >= n) {
                                    bzero(answer, 1024);
                                    while ((n = read(fd_tcp, answer, 1024)) == 0) ;
                                    aux = answer;
                                }

                                token = strtok(aux, " ");
                                qIMG = atoi(token);
                                aux = token + strlen(token) + 1;
                                

                                if (qIMG) {
                                    if (aux - answer >= n) {
                                        bzero(answer, 1024);
                                        while ((n = read(fd_tcp, answer, 1024)) == 0) ;
                                        aux = answer;
                                    }
                                    token = strtok(aux, " ");
                                    strcpy(ext, token);
                                    printf("Extension is now: %s\n", ext);
                                    aux = token + strlen(token) + 1;
                                    if (aux - answer >= n) {
                                        printf("Gonna read from answer: \n");
                                        write(1, answer, 1024);
                                        printf("\n----------------END\n\n\n");
                                        bzero(answer, 1024);
                                        while ((n = read(fd_tcp, answer, 1024)) == 0) ;
                                        aux = answer;
                                    }
                                    
                                    token = strtok(aux, " ");
                                    int aisize = atoi(token);
                                    printf("Isize: %d\n", aisize);
                                    aux = token + strlen(token) + 1;

                                    if (aux - answer >= n) {
                                        bzero(answer, 1024);
                                        while ((n = read(fd_tcp, answer, 1024)) == 0) ;
                                        aux = answer;
                                    }
                                    printf("\n\n\n----------------AUX CONTENT---------------\n");
                                    write(1, aux, 1024);
                                    printf("\n-----------------------------------------");
                                    bzero(qdata, 1024);
                                    memcpy(qdata, aux, MIN(aisize, 1024 - (aux - answer)));
                                    

                                    if (1024 - (aux - answer) < aisize) {
                                        read(fd_tcp, qdata + MIN(aisize, 1024 - (aux - answer)), 1024 - MIN(aisize, 1024 - (aux - answer)));
                                    }

                                    printf("\n\n\n----------------QDATA CONTENT----------------\n");
                                    write(1, qdata, 1024);
                                    printf("--------------------------------------------");
                                    changed = 0;
                                    answerWriteImageFile(question, topic, qdata, 1024, aisize, fd_tcp, &changed, ext, answer_number);
                                    n = 0;
                                }
                                answerWriteAuthorInformation(topic, question, qUserID, ext, answer_number);
                            }

                            writeAuthorInformation(topic, question, qUserID, ext);

                            if (question != NULL) free(question);
                            question = strdup(question_title);
                            printf("Current question: %s\n", question);
                            close(fd_tcp);
                            used_tcp = 1;
                            free(message);
                        }
                    }
                } 
                else {
                    printf("Invalid command.\nquestion_get question / reg question_number\n");
                }
            }
            else {
                printf("Cannot select question.\nYou must request question list first.\n"); 
            }
        }

        else if (strcmp(token, "question_submit") == 0 || strcmp(token, "qs") == 0) {
            if (topic != NULL) {
                char * submit_question, * text_file, * image_file = NULL, * message;
                char *qdata, *idata;
                int msg_size, qsize, i, n;
                int isize;
                int qIMG; //flag
                char * aux = to_validate + strlen(token) + 1;
                if (validate_qs_as_input(to_validate, 4)) {
                    int exists;

                    // question title
                    token = strtok(aux, " ");
                    submit_question = strdup(token);

                    // text_file
                    token = strtok(NULL, " ");
                    int len_text_filename = strlen(token) + 5;
                    text_file = malloc(sizeof(char) * len_text_filename);
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

                    //if filename has no ext, add .txt
                    char *text_noext = strtok(text_file, ".");
                    char aux[strlen(text_file) + 1];
                    strcpy(aux, text_file);
                    bzero(text_file, len_text_filename);
                    sprintf(text_file, "%s.txt", aux);

                    // proceed only if file.txt exists
                    if (fileExists(text_file)) {
                        qsize = get_filesize(text_file);
                        qdata = (char*) malloc (sizeof(char) * qsize + 1);

                        get_txtfile(text_file, qdata, qsize);
                        int image_exists = fileExists(image_file);
                        if (qIMG && image_exists) {
                            image_exists = 1;
                            isize = get_filesize(image_file);
                            idata = (char*) malloc (sizeof(char) * (isize));
                            char ext[4];

                            getExtension(image_file, ext);
                            get_img(image_file, idata, isize);

                            msg_size = 21 + strlen(topic) + strlen(submit_question) + ndigits(qsize) + qsize + ndigits(qIMG) + ndigits(isize) + isize;

                            message = calloc(msg_size + 1,sizeof(char));
                            n = sprintf(message, "QUS %d %s %s %d %s %d %s %d ", userID, topic, submit_question, qsize, qdata, qIMG, ext,isize);
                            memcpy(message + n, idata, isize); //copy image to message
                            printf("Image size: %d\n", isize);
                            message[n + isize + 1] = '\n';

                            free(image_file);
                            free(idata);
                                 
                        }
                        else if (qIMG && !image_exists) {
                            free(idata);
                            printf("Image file doesn't exist.\n");         
                        }
                        else {
                            msg_size = 17 + strlen(topic) + strlen(submit_question) + ndigits(qsize) + qsize;
                            message = malloc(sizeof(char) * (msg_size));

                            int k = sprintf(message, "QUS %d %s %s %d %s 0", userID, topic, submit_question, qsize, qdata);
                            message[k] = '\n';
                        }

                        if ((qIMG && image_exists) || !qIMG) {
                            fd_tcp = create_TCP(hostname,  &res_tcp);
                            n = connect(fd_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen);
                            if (n == -1) exit(1);

                            //send request
                            write_TCP(fd_tcp, message, msg_size);

                            //receive answer
                            if (read(fd_tcp, answer, 1024) == -1) {
                                exit(ERROR);
                            }
                            receive_QUR(answer, submit_question);

                            close(fd_tcp);
                            used_tcp = 1;
                            free(text_file);
                            free(submit_question);
                            free(qdata);
                            free(message);
                        }
                    }
                    else {
                        printf("Text file doesn't exist.\n");
                    }
                }
                else {
                    printf("Invalid command.\nsubmit_question_submit submit_question text_file [image_file.ext] /\nqs submit_question test_file [image_file.ext]\n");
                }
            }
            else {
                printf("You must request question list first.\n");
            }
        }
        else if (strcmp(token, "answer_submit") == 0 || strcmp(token, "as") == 0) {
            if (question != NULL) {
                char * text_file, * image_file = NULL, * message;
                char *adata, *idata;
                int msg_size, asize, isize, i;
                int qIMG; //flag
                char * aux = to_validate + strlen(token) + 1;
                if (validate_qs_as_input(to_validate, 3)) {
                    // answer text file
                    token = strtok(aux, " ");
                    if (token[strlen(token) -1] == '\n')
                        token[strlen(token) -1] = '\0';

                    text_file = calloc(strlen(token) + 5, sizeof(char));    
                    sprintf(text_file, "%s.txt", token);  

                    // answer image file
                    token = strtok(NULL, " ");
                    if ((qIMG = token != NULL)){
                        image_file = strdup(token);
                        i = strlen(image_file) - 1;
                        if (image_file[i] == '\n')
                            image_file[i] = '\0';
                        printf("image file is: %s\n", image_file);
                    }

                    printf("Text file is: %s\n", text_file);
                    asize = get_filesize(text_file);
                    adata = calloc(asize + 1, sizeof(char));

                    get_txtfile(text_file, adata, asize);

                    if(qIMG) {
                        isize = get_filesize(image_file);
                        idata = (char*) malloc (sizeof(char) * (isize));
                        char ext[4];

                        getExtension(image_file, ext);
                        get_img(image_file, idata, isize);

                        msg_size = 21 + ndigits(userID) + strlen(topic) + strlen(question) + ndigits(asize) + asize + ndigits(qIMG) + ndigits(isize) + isize;

                        message = calloc(msg_size,sizeof(char));
                        bzero(message, 1024);
                        n = sprintf(message, "ANS %d %s %s %d %s %d %s %d ", userID, topic, question, asize, adata, qIMG, ext,isize);
                        memcpy(message + n, idata, isize); //copy image to message
                        printf("Image size: %d\n", isize);
                        message[n + isize + 1] = '\n';

                        free(idata);
                        free(image_file);
                    }

                    else{
                        msg_size = 17 + strlen(topic) + strlen(question) + ndigits(asize) + asize;

                        message = malloc(sizeof(char) * (msg_size));
                        bzero(message, 1024);
                        int k = sprintf(message, "ANS %d %s %s %d %s 0", userID, topic, question, asize, adata);
                        message[k] = '\n';
                    }

                    fd_tcp = create_TCP(hostname,  &res_tcp);
                    n = connect(fd_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen);
                    if (n == -1) exit(1);

                    // send request
                    write_TCP(fd_tcp, message, msg_size); //TODO receive server confirmation
                    
                    // get answer
                    if(read(fd_tcp, answer, 1024) < 0) exit(ERROR);
                    receive_ANR(answer);

                    close(fd_tcp);
                    used_tcp = 1;
                    free(adata);
                    free(text_file);
                    free(message);
                }
                else {
                    printf("Invalid command.\nanswer_submit text_file [image_file.ext] /\nas question test_file [image_file.ext]\n");
                }
            }
            else {
                printf("You must request a question first.\n");
            }
        }


        else if (strcmp(token, "exit\n") == 0) {
            deleteTable(topics_hash);
            freeTopics(topics);
            if (questions_titles != NULL) 
                freeQuestions(questions_titles);
            free(to_validate);
            if (topic != NULL) free(topic);
            if (question != NULL) free(question);
            if (used_tcp) freeaddrinfo(res_tcp);
            printf("Goodbye!\n");
            break;
        }

        else if (!user_exists) {
            printf("You need to login.\n");
        }
        else {
            printf("%s\n", token);
            printf("Unknown command. Try again.\n");
        }

        printf(">>> ");
        free(to_validate);
    }
}

void parseArgs(int argc, char * argv[], char *hostname){
    int n = argc, i = 1, hashost = 0;

    if(argc>1){
        while(n > 1){
            if(strcmp(argv[i], "-p")==0){
                strcpy(port, argv[i+1]);
                printf("custom port was set as %s\n", port);
            }
            else if(strcmp(argv[i], "-n")==0){
                strcpy(hostname, argv[i+1]);
                printf("custom hostname was set as %s\n", hostname);
                hashost = 1;
            }
            n -= 2;
            i += 2;
        }
    }

    if(!hashost){
        gethostname(hostname, 1023);
    }
}

int main(int argc, char * argv[]) {
    char buffer[1024];

    int fd_udp;
    struct addrinfo hints_udp, *res_udp;

    char hostname[1024];
    hostname[1023] = '\0';

    parseArgs(argc, argv, hostname);

    fd_udp = create_UDP(hostname, hints_udp, &res_udp);

    struct timeval time;
    time.tv_sec = 2;
    time.tv_usec = 0;   
    int n;
    if ((n = setsockopt(fd_udp, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(time))) < 0) {
        exit(ERROR);
    }

    printf("=== Welcome to RC Forum! ===\n\n>>> ");
    receive_input(hostname, buffer, fd_udp, res_udp);

    freeaddrinfo(res_udp);
    close(fd_udp);

    return 0;
}
