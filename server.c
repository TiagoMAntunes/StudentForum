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

#define ERROR       	1
#define MAX_TOPICS 		99
#define MAX_QUESTIONS	99
#define MAX_REQ_LEN 	1024
#define ERR_MSG     	"ERR"
#define ERR_LEN     	4
#define BUF_SIZE    	1024

#define MIN(A,B) ((A) < (B) ? (A) : (B))
#define max(x, y) (x > y ? x : y)

char port[6] = "58017";


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

    if (stringID[len-1] != '\n')
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
    }
    (*full_msg)[n-1] = '\0';
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

int is_number(char *text) {
    int len = strlen(text);

    for (int i = 0; i < len; i++) {
        if (text[i] < '0' || text[i] > '9')
            return 0;
    }

    return 1;
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

// PROTOCOL VALIDATION
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

	    if (token == NULL || !verify_ID(token)) {
	        return 0;
	    }

	    token = strtok(NULL, " ");
	    if (token == NULL || strlen(token) > 10)
	        return 0;

	    token = strtok(NULL, " ");

	    return token == NULL;
	}

	int validate_LQU(char *msg) {      
	    char *token = strtok(msg, " "); // LQU

	    token = strtok(NULL, " ");
	    if (token == NULL || strlen(token) > 10)
	        return 0;

	    token = strtok(NULL, " ");
	    return token == NULL;
	}	

	int validate_QUS(char *msg) {
		char *aux = strdup(msg);
		int msg_len = 0;
		while (msg_len < 1024 && msg[msg_len] != '\n') 
			msg_len++;

		char *token = strtok(aux, " ");
		if (token == NULL || strcmp(token, "QUS") != 0) {
			free(aux);
			return 0;
		}

		//userID
		token = strtok(NULL, " ");
		if (token == NULL || !verify_ID(token)) {
			free(aux);
			return 0;
		}

		// topic
		token = strtok(NULL, " ");
		if (token == NULL || strlen(token) > 10) {
			free(aux);
			return 0;
		}

		int topic_len = strlen(token);

		// question
		token = strtok(NULL, " ");
		if (token == NULL || strlen(token) > 10) {
			free(aux);
			return 0;
		}

		int question_len = strlen(token);

		// qsize
		token = strtok(NULL, " ");
		if (token == NULL || !is_number(token)) {
			free(aux);
			return 0;
		}

		if (msg_len < 1024) {
			// TODO validate rest of msg if len(msg) < 1024 otherwise validate in main function
		}


		free(aux);
		return 1;
	}




void TCP_input_validation(int fd) {
    char message[BUF_SIZE], prefix[4];
    char * aux = message, * token;

    bzero(message, BUF_SIZE);
    bzero(prefix, 4);
    if (read(fd, message, BUF_SIZE) == -1) {
    	exit(ERROR);
    }
    printf("Message: %s\n", message);
    memcpy(prefix, aux, 3);
    aux += 4;
    printf("Prefix: %s\n", prefix);
    if (strcmp(prefix, "QUS") == 0) {
    	if (validate_QUS(message)) {
	        char topic[11], question[11], userID[6], * qdata, ext[4];
	        int qsize, qIMG, isize;
	        printf("QUS inside!\n");

	        bzero(topic, 11);
	        bzero(question, 11);
	        bzero(userID, 6);
	        bzero(ext, 4);

	        token = strtok(aux, " ");
	        sprintf(userID, "%s",token);

	        token = strtok(NULL, " ");
	        sprintf(topic, "%s", token);

	        int n_questions = getNumberOfQuestions(topic);
	        if (n_questions < MAX_QUESTIONS) {

		        token = strtok(NULL, " ");
		        sprintf(question, "%s", token);

		        token = strtok(NULL, " ");
		        qsize = atoi(token);

		        aux = token + ndigits(qsize) + 1;

		        validateDirectories(topic, question);	// TODO se ja exite, entao QUR DUP

		        qdata = calloc(BUF_SIZE, sizeof(char));
		        
		        //sprintf(qdata, "%s", aux);
		        memcpy(qdata, aux, MIN(qsize, BUF_SIZE - (aux - message)));

		        //in case there's some space available in the buffer, fill it in to avoid bad writes
		        if (BUF_SIZE - (aux - message) < qsize)
		            read(fd, qdata + MIN(qsize, BUF_SIZE - (aux - message)), BUF_SIZE - MIN(qsize, BUF_SIZE - (aux - message)));

		        int changed = 0;
		        writeTextFile(question, topic, qdata, BUF_SIZE, qsize, fd, &changed);

		        if (changed){
		            read(fd, message, BUF_SIZE);
		            aux = message + 1;
		        } else
		            aux += qsize + 1; 
		        
		        
		        token = strtok(aux, " ");
		        qIMG = atoi(token);
		        
		        if (qIMG) {
		            token = strtok(NULL, " ");
		            sprintf(ext, "%s", token);

		            token = strtok(NULL, " ");
		            isize = atoi(token);
		            printf("Prepare to write: %d\n", isize);
		            //reuse qdata for less memory
		            aux = token + ndigits(isize) + 1;
		            memcpy(qdata, aux, MIN(isize, BUF_SIZE - (aux - message)));

		            //in case there's some space available in the buffer, fill it in to avoid bad writes
		            if (BUF_SIZE - (aux - message) < isize) {
		                printf("Sizes: %d %ld\n", isize, BUF_SIZE - (aux - message));
		                read(fd, qdata + MIN(isize, BUF_SIZE - (aux - message)), BUF_SIZE - MIN(isize, BUF_SIZE - (aux - message)));
		            }

		            changed = 0;
		            writeImageFile(question, topic, qdata, BUF_SIZE, isize, fd, &changed, ext);
		        }
		        writeAuthorInformation(topic, question, userID, ext);
		        write(fd, "QUR OK\n", 7);
		        free(qdata);
		    }
			else {
	    		write(fd, "QUR FUL\n", 8);
	    	}
		}
		else {
			write(fd, "QUR NOK\n", 8);
		}
	    
    } else if(strcmp("GQU", prefix) == 0) {
        char topic[11], question[11], userID[6], ext[4];
        bzero(topic, 11);
        bzero(question, 11);
        bzero(userID, 6);
        bzero(ext, 4);

        token = strtok(aux, " ");
        sprintf(topic, "%s", token);

        token = strtok(NULL, " \n");
        sprintf(question, "%s", token);
        int answers_number;
        List * answers = getAnswers(topic, question, &answers_number);

        Iterator * it = createIterator(answers);
        printf("Available answers:\n");
        while (hasNext(it))
            printf("%s\n", current(next(it)));
        killIterator(it);

        bzero(message, BUF_SIZE);
        getAuthorInformation(topic, question, userID, ext);
        char * txtfile = getQuestionPath(topic, question);
        char * imgfile = getImagePath(topic, question, ext);

        int txt_size = get_filesize(txtfile);
        sprintf(message, "QGR %s %d ", userID, get_filesize(txtfile));
        write(fd, message, strlen(message));
        
        readFromFile(txtfile, message, BUF_SIZE, txt_size, fd);

        int qIMG = imgfile != NULL ? 1 : 0;
        sprintf(message, " %d ", qIMG);
        if (qIMG) {
            int image_size = get_filesize(imgfile);
            sprintf(message + 3, "%s %d ", ext, image_size);
            write(fd, message, 3 + strlen(ext) + ndigits(image_size) + 2);
            write(1, message, 3 + strlen(ext) + ndigits(image_size) + 2);
            readFromFile(imgfile, message, BUF_SIZE, image_size, fd);
        }
        char * aux = message;
        if (answers_number > 0){
            sprintf(message, " %d", answers_number);
            aux += 2;
        }
        else {
            sprintf(message, " 0\n");
            aux += 3;
        }
        
        write (fd, message, aux - message);
        
        it = createIterator(answers);
        int question_count = 1;
        while (hasNext(it)) {
            char * answer_name = current(next(it));
            bzero(userID, 6); bzero(ext, 4);
            getAnswerInformation(answer_name, userID, ext);
            if (txtfile != NULL) free(txtfile);
            if (imgfile != NULL) free(imgfile);

            txtfile = getAnswerQuestionPath(answer_name);
            imgfile = getAnswerImagePath(answer_name, ext);
            txt_size = get_filesize(txtfile);
            sprintf(message, " %02d %s %d ", question_count++, userID, txt_size);

            write(fd, message, strlen(message));
            printf("I'm gonna read from %s\n", txtfile);
            readFromFile(txtfile, message, BUF_SIZE, txt_size, fd);

            qIMG = imgfile != NULL ? 1 : 0;
            sprintf(message, " %d", qIMG);
            write(fd, message, strlen(message));

            if (qIMG) {
                int image_size = get_filesize(imgfile);
                sprintf(message, " %s %d ", ext, image_size);
                write(fd, message, strlen(message));
                
                readFromFile(imgfile, message, BUF_SIZE, image_size, fd);
                
            }
        }
        write(fd, "\n", 1);
        
        free(txtfile);

    } else if (strcmp("ANS", prefix) == 0) {
        char topic[11], question[11], userID[6], * qdata, ext[4];
        int qsize, qIMG, isize;
        printf("ANS inside!\n");

        bzero(topic, 11);
        bzero(question, 11);
        bzero(userID, 6);
        bzero(ext, 4);

        token = strtok(aux, " ");
        sprintf(userID, token);

        token = strtok(NULL, " ");
        sprintf(topic, "%s", token);

        token = strtok(NULL, " ");
        sprintf(question, "%s", token);

        token = strtok(NULL, " ");
        qsize = atoi(token);

        aux = token + ndigits(qsize) + 1;

        int answer_number = answerDirectoriesValidation(topic, question);

        qdata = calloc(BUF_SIZE, sizeof(char));
        
        //sprintf(qdata, "%s", aux);
        memcpy(qdata, aux, MIN(qsize, BUF_SIZE - (aux - message)));

        //in case there's some space available in the buffer, fill it in to avoid bad writes
        if (BUF_SIZE - (aux - message) < qsize)
            read(fd, qdata + MIN(qsize, BUF_SIZE - (aux - message)), BUF_SIZE - MIN(qsize, BUF_SIZE - (aux - message)));

        int changed = 0;
        answerWriteTextFile(question, topic, qdata, BUF_SIZE, qsize, fd, &changed, answer_number);

        if (changed){
            read(fd, message, BUF_SIZE);
            aux = message + 1;
        } else
            aux += qsize + 1; 
        
        
        token = strtok(aux, " ");
        qIMG = atoi(token);
        
        if (qIMG) {
            token = strtok(NULL, " ");
            sprintf(ext, "%s", token);

            token = strtok(NULL, " ");
            isize = atoi(token);
            printf("Prepare to write: %d\n", isize);
            //reuse qdata for less memory
            aux = token + ndigits(isize) + 1;
            memcpy(qdata, aux, MIN(isize, BUF_SIZE - (aux - message)));

            //in case there's some space available in the buffer, fill it in to avoid bad writes
            if (BUF_SIZE - (aux - message) < isize) {
                printf("Sizes: %d %ld\n", isize, BUF_SIZE - (aux - message));
                read(fd, qdata + MIN(isize, BUF_SIZE - (aux - message)), BUF_SIZE - MIN(isize, BUF_SIZE - (aux - message)));
            }

            changed = 0;
            answerWriteImageFile(question, topic, qdata, BUF_SIZE, isize, fd, &changed, ext, answer_number);
        }
        free(qdata);
        answerWriteAuthorInformation(topic, question, userID, ext, answer_number);
    }
    printf("Son is finished!\n");

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
            user_addrlen = sizeof(user_addr);
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
                    char * message = calloc(msg_size, sizeof(char));

                    /*msg_size = read_TCP(newfd, &message, msg_size, 0);
                    printf("Message received: %s\n", message);*/
                    TCP_input_validation(newfd);
                    free(message);
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
                    char *stringID = strdup(token);
                    token = strtok(NULL, " \n");
                    char *topic_title = strdup(token);
                    token = strtok(NULL, " \n");

                    if (n_topics == MAX_TOPICS) {
                        n = sendto(fd_udp, "PTR FUL\n", 8, 0, (struct sockaddr *) &user_addr, user_addrlen);
                        if (n == -1)
                            exit(ERROR);
                    }

                    else if (token != NULL || !registered_user(users, atoi(stringID)) || strlen(topic_title) > 10) {
                        n = sendto(fd_udp, "PTR NOK\n", 8, 0, (struct sockaddr *) &user_addr, user_addrlen);
                        if (n == -1)
                            exit(ERROR);
                    }

                    else if (topic_exists(topics, topic_title)) {
                        n = sendto(fd_udp, "PTR DUP\n", 8, 0, (struct sockaddr *) &user_addr, user_addrlen);
                        printf("Returned duplication information\n");
                        if (n == -1)
                            exit(ERROR);
                    }

                    else {
                        Topic* new = createTopic(topic_title, atoi(stringID));
                        addEl(topics, new);
                        insertInTable(topics_hash, new, hash(topic_title));
                        n_topics++;

                        n = sendto(fd_udp, "PTR OK\n", 7, 0, (struct sockaddr *) &user_addr, user_addrlen);
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
                    int n_questions = (questions_list == NULL ? 0 : listSize(questions_list));

                    //Create message data
                    char * message = calloc(7 + ndigits(j) + j * (10 + 1 + 5 + 1 + 2), sizeof(char));
                    char * msg_help = message;

                    //LQR N
                    memcpy(msg_help, "LQR ", 4);
                    msg_help += 4;
                    sprintf(msg_help, "%d", n_questions);
                    msg_help += ndigits(n_questions);

                    //populate with the amount of questions
                    if (questions_list != NULL) {
                        Iterator * it = createIterator(questions_list);
                        char userID[6], ext[4];
                        bzero(userID, 6);
                        bzero(ext, 4);
                        while (j > 0) {
                            char * question = (char * ) current(next(it));
                            getAuthorInformation(token, question, userID, ext);
                            int n = getNumberOfAnswers(token, question);
                            char n_answers[3];
                            if (n < 9) {
                            	sprintf(n_answers, "0%d", n);
                            }
                            else {
                            	sprintf(n_answers, "%d", n);
                            }

                            sprintf(msg_help, " %s:%s:%s", question, userID, n_answers);
                            msg_help += 10 + strlen(question);
                            j--;
                            free(question);
                        }
                        killIterator(it);
                        listFree(questions_list);
                    } else msg_help++;
                    *(msg_help) = '\n';
                    printf("Char: %c with value %d\n", *msg_help, *msg_help);
                    printf("String: \"%s\" with size: %ld\n", message, strlen(message));
                    n = sendto(fd_udp, message, strlen(message), 0, (struct sockaddr *) &user_addr, user_addrlen);

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
