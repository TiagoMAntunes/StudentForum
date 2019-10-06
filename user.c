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
#define PREFIX      "./topics/"
#define PREFIX_LEN  9
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
            printf("Unexpected server response.\n");;
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
    if (questions_titles != NULL) {
        Iterator * it = createIterator(questions_titles);
        while (hasNext(it)) free(current(next(it)));
        killIterator(it);
        listFree(questions_titles);
    }

    questions_titles = newList();
    //display the questions available for the topic and save them
    printf("Topic : %s -> Questions:\n", topic);

    int n = 0;
    while ((token = strtok(NULL, " ")) != NULL) {
        char question[11], userID[6];
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

        printf("%d - %s (%s)\n", ++n, question, userID);
        addEl(questions_titles, strdup(question));
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

int getNumFromAnswer(char *answer) {
    char *aux = strdup(answer);
    char *token = strtok(aux, " ");
    if (is_number(token)) {
        int num = atoi(token);
        free(aux);
        return num;
    }
    free(aux);
    return -1;
}

char *verify_and_reset(char *answer, char *aux, int *bytes_read, int fd_tcp) {
    if (aux - answer >= 1024) {
        bzero(answer, 1024);
        *bytes_read = read(fd_tcp, answer, 1024);
        aux = answer;
    }
    return aux;
}

void receive_input(char * hostname, char* buffer, int fd_udp, struct addrinfo *res_udp) {
    char *token, *stringID;
    int n, user_exists, short_cmmd = 0;
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
                n = sendto(fd_udp, message, 11, 0, res_udp->ai_addr, res_udp->ai_addrlen);
                if (n == -1) {
                    exit(ERROR);
                }

                n = recvfrom(fd_udp, answer, 1024, 0, res_udp->ai_addr, &res_udp->ai_addrlen);
                if (errno == EAGAIN) {
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
            if (errno == EAGAIN) {
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
                if (errno == EAGAIN) {
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
                if (errno == EAGAIN) {
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
        else if (user_exists && topic != NULL && (strcmp(token, "question_get") == 0 || strcmp(token, "qg") == 0)) {
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
                    //printf("Question is: %s\n", question_title);

                    int msg_size = strlen(question_title) + strlen(topic) + 7;
                    char * message = malloc(sizeof(char) * (msg_size+1));
                    int k = sprintf(message, "GQU %s %s", topic, question_title);
                    message[k] = '\n';

                    fd_tcp = create_TCP(hostname,  &res_tcp);
                    n = connect(fd_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen);
                    if (n == -1) exit(1);

                    write_TCP(fd_tcp, message, msg_size);
               

                    //get answer
                    // read QGR user ID
                    char answer[1024];
                    bzero(answer, 1024);
                    int bytes_read = read(fd_tcp, answer, 10);
                    if (bytes_read == -1) {
                        exit(ERROR);
                    }

                    char *token = strtok(answer, " ");
                    if (strcmp(token, "QGR") == 0) {
                        token = strtok(NULL, " \n");
                        if (is_number(token)) {
                            int userID = atoi(token);

                            validateDirectories(topic, question);

                            bzero(answer, 1024);
                            bytes_read = read(fd_tcp, answer, 1024);

                            // sacar qsize
                            int qsize = getNumFromAnswer(answer);

                            // sacar qdata
                            char *answer_aux = answer + ndigits(qsize) + 1;
                            int data_read = 0;
                            int aux_len = strlen(answer_aux);

                            while (data_read != qsize && data_read < aux_len) {
                                data_read++;
                            }

                            int more_than_bufsize = 0;
                            int offset = readServerAndWriteToFile(&more_than_bufsize, question, topic, answer, answer_aux, data_read, qsize, bytes_read, fd_tcp, 0, NULL);
                            answer_aux = answer;
                            printf("answer_aux = |%s|\n", answer_aux);
                            answer_aux +=  (more_than_bufsize ? offset : offset + ndigits(qsize) + 2);  // removes rest of qdata and a " "
                            printf("answer_aux = |%s|\n", answer_aux);
                            // if answer full, read more 
                            answer_aux = verify_and_reset(answer, answer_aux, &bytes_read, fd_tcp);


                            // sacar qIMG
                            printf("answer_aux = |%s|\n", answer_aux);
                            char *helper = strdup(answer_aux);
                            int qIMG = atoi(strtok(helper, " "));
                            answer_aux += 2;  // "qIMG "
                            answer_aux = verify_and_reset(answer, answer_aux, &bytes_read, fd_tcp);
                            printf("answer_aux = |%s|\n", answer_aux);

                            if (qIMG) {
                                char *ext = strtok(NULL, " ");
                                answer_aux += strlen(ext) + 1;      // "ext "
                                printf("answer_aux = |%s|\n", answer_aux);
                                answer_aux = verify_and_reset(answer, answer_aux, &bytes_read, fd_tcp);

                                int qisize = atoi(strtok(NULL, " "));
                                answer_aux += ndigits(qisize) + 1;   // "qisize "
                                answer_aux = verify_and_reset(answer, answer_aux, &bytes_read, fd_tcp); 

                                data_read = 0;
                                aux_len = answer + 1024 - answer_aux;
                                while (data_read != qisize && data_read < aux_len) {
                                    data_read++;
                                }

                                more_than_bufsize = 0;
                                printf("Gonna write: %d\n", qisize);
                                offset = readServerAndWriteToFile(&more_than_bufsize, question, topic, answer, answer_aux, data_read, qisize, bytes_read, fd_tcp, 1, ext);
                            }
                            else {

                            }

                            free(helper);

                        }
                        else if (strcmp(token, "EOF")) {
                            printf("No such query or topic available. Try again.\n");
                        }
                        else if (strcmp(token, "ERR")) {
                            printf("Invalid request.\nquestion_get question / qg question_number\n");
                        }
                    }
                    else {
                        printf("Server didn't respond. Try again.\n");
                    }


                    // TODO por aqui um while  message[len -1] != \n le e poe no buffer?

         //           writeAuthorInformation(topic, question, qUserID, ext);

                    //TODO if reply is not 'QGR EOF' or 'QGR ERR', save question_title as currently selected question
                    close(fd_tcp);
                    free(message);
                } 
                else {
                    printf("Invalid command.\nquestion_get question / qg question_number\n");
                }
            }
            else {
                printf("Cannot select question.\nYou must request question list first.\n"); 
            }
        }

        else if (strcmp(token, "question_submit") == 0 || strcmp(token, "qs") == 0) {
            char * submit_question, * text_file, * image_file = NULL, * message;
            char *qdata, *idata, ext_txt[5] = ".txt";
            int msg_size, qsize, i, n;
            int isize;
            int qIMG; //flag
            char * aux = to_validate + strlen(token) + 1;
            if (validate_qs_as_input(to_validate, 4)) {
                token = strtok(aux, " ");
                submit_question = strdup(token);
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

                qsize = get_filesize(text_file);
                qdata = (char*) malloc (sizeof(char) * qsize + 1);

                get_txtfile(text_file, qdata, qsize);

                if(qIMG) {
                    isize = get_filesize(image_file);
                    idata = (char*) malloc (sizeof(char) * (isize));
                    char ext[4];

                    getExtension(image_file, ext);
                    get_img(image_file, idata, isize);

                    msg_size = 21 + strlen(topic) + strlen(submit_question) + ndigits(qsize) + qsize + ndigits(qIMG) + ndigits(isize) + isize;

                    message = calloc(msg_size,sizeof(char));
                    bzero(message, msg_size);
                    n = sprintf(message, "QUS %d %s %s %d %s %d %s %d ", userID, topic, submit_question, qsize, qdata, qIMG, ext,isize);
                    memcpy(message + n, idata, isize); //copy image to message
                    printf("Image size: %d\n", isize);
                    message[n + isize + 1] = '\n';

                    free(idata);
                    free(image_file);
                }

                else{
                    msg_size = 17 + strlen(topic) + strlen(submit_question) + ndigits(qsize) + qsize;

                    message = malloc(sizeof(char) * (msg_size));
                    int k = sprintf(message, "QUS %d %s %s %d %s 0", userID, topic, submit_question, qsize, qdata);
                    message[k] = '\n';
                }

                fd_tcp = create_TCP(hostname,  &res_tcp);
                n = connect(fd_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen);
                if (n == -1) exit(1);

                write_TCP(fd_tcp, message, msg_size);
                close(fd_tcp);
                free(text_file);
                free(submit_question);
                free(qdata);
                free(message);
            }
            else {
                printf("Invalid command.\nsubmit_question_submit submit_question text_file [image_file.ext] /\nqs submit_question test_file [image_file.ext]\n");
            }
        }
        else if (strcmp(token, "answer_submit") == 0 || strcmp(token, "as") == 0) {
            char * text_file, * image_file = NULL, * message;
            char *adata, *idata;
            int msg_size, asize, isize, i;
            int qIMG; //flag
            char * aux = to_validate + strlen(token) + 1;
            if (validate_qs_as_input(to_validate, 3)) {
                token = strtok(aux, " ");
                text_file = strdup(token);
                token = strtok(NULL, " ");

                printf("curr topic: %s curr question: %s\n", topic, question);

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

                asize = get_filesize(text_file);
                adata = (char*) malloc (sizeof(char) * asize + 1);

                get_txtfile(text_file, adata, asize);

                if(qIMG) {
                    isize = get_filesize(image_file);
                    idata = (char*) malloc (sizeof(char) * (isize));
                    char ext[4];

                    getExtension(image_file, ext);
                    get_img(image_file, idata, isize);

                    msg_size = 21 + ndigits(userID) + strlen(topic) + strlen(question) + ndigits(asize) + asize + ndigits(qIMG) + ndigits(isize) + isize;

                    message = calloc(msg_size,sizeof(char));
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
                    int k = sprintf(message, "ANS %d %s %s %d %s 0", userID, topic, question, asize, adata);
                    message[k] = '\n';
                }

                fd_tcp = create_TCP(hostname,  &res_tcp);
                n = connect(fd_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen);
                if (n == -1) exit(1);

                write_TCP(fd_tcp, message, msg_size);
                printf("Enviado!\n");
                close(fd_tcp);
                free(adata);
                free(text_file);
                free(message);
            }
            else {
                printf("Invalid command.\nanswer_submit text_file [image_file.ext] /\nas question test_file [image_file.ext]\n");
            }
        }


        else if (strcmp(token, "exit\n") == 0) {
            deleteTable(topics_hash);
            freeTopics(topics);
            free(to_validate);
            freeaddrinfo(res_tcp);
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
