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
#include <sys/wait.h>
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
#define MAX_ANSWERS		99
#define MAX_REQ_LEN 	1024
#define ERR_MSG     	"ERR"
#define ERR_LEN     	4
#define BUF_SIZE    	1024

#define MIN(A,B) ((A) < (B) ? (A) : (B))
#define max(x, y) (x > y ? x : y)

char port[6] = "58017";


// ERROR MANAGEMENT
	void error_on(char *alloc, char *function) {
		char msg[9 + strlen(alloc) + 3 + strlen(function) + 3];
		if (sprintf(msg, "Error on %s - %s.\n", alloc, function) < 0) {
			perror("Error on sprintf - error_on.\n");
			exit(ERROR);
		}
		perror(msg);
		exit(ERROR);
	}

	void error_file(char *function) {
		char msg[21 + strlen(function) + 2];
		if (sprintf(msg, "File related error - %s.\n", function) < 0) {
			perror("Error on sprintf - error_file.\n");
			exit(ERROR);
		}
		perror(msg);
		exit(ERROR);
	}

void get_img(char* filename, char* img, int size){
    FILE* f;
    char buffer[size];

    f = fopen(filename, "r");

    if(f!=NULL){
    	clearerr(f);
        while(!feof(f)) {
            fread(buffer, 1, sizeof(buffer), f);
            validateReadWrite(f, filename);
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
    if (n != 0) error_on("getaddrinfo", "create_TCP");

    fd = socket((*res)->ai_family, (*res)->ai_socktype, (*res)->ai_protocol);
    if (fd == -1) error_on("socket creation", "create_TCP");

    n = bind(fd, (*res)->ai_addr, (*res)->ai_addrlen);
    if (n == -1) error_on("bind", "create_TCP");

    if (listen(fd, 5) == -1) error_on("listen", "create_TCP");

    return fd;
}

int create_UDP(char* hostname, struct addrinfo hints, struct addrinfo **res) {
    int n, fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    n = getaddrinfo(NULL, port, &hints, res);
    if (n != 0) error_on("getaddrinfo", "create_UDP");

    fd = socket((*res)->ai_family, (*res)->ai_socktype, (*res)->ai_protocol);
    if (fd == -1) error_on("socket creation", "create_UDP");

    n = bind(fd, (*res)->ai_addr, (*res)->ai_addrlen);
    if (n == -1) error_on("bind", "create_UDP");

    return fd;
}

void send_ERR_MSG_UDP(int fd, struct addrinfo **res) {
    int n;
    n = sendto(fd, ERR_MSG, ERR_LEN, 0, (*res)->ai_addr, (*res)->ai_addrlen);
    if (n == -1) error_on("sento", "send_ERR_MSG_UDP");
}

int registered_user(Hash *users, int userID) {

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
    if (n < 0) error_on("read", "read_TCP");

    if (n == 0) return msg_size;

    if (n == msg_size) {
        msg_size *= 2;
        *full_msg = realloc(*full_msg, msg_size);
        if (*full_msg == NULL) error_on("realloc", "read_TCP");
    }
    while( (c = read(fd, *full_msg + n + current_offset, msg_size - current_offset - n)) != 0) {
        if (c < 0) error_on("read", "read_TCP");

        if (c + n >= msg_size)
            msg_size *= 2;
        *full_msg = realloc(*full_msg, msg_size);
        if (*full_msg == NULL) error_on("realloc", "read_TCP");
        n += c;
    }
    (*full_msg)[n-1] = '\0';
    return msg_size;
}

void write_TCP(int fd, char* reply, int msg_size){
    int n;

    n = write(fd, reply, msg_size);
    if(n==-1) error_on("write", "write_TCP");


    while(n < msg_size){
        n = write(fd, reply + n, msg_size - n);
        if(n==-1) error_on("write", "write_TCP");;
    }
    if (write(1, reply, msg_size) < 0) error_on("write", "write_TCP");
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

void freeAnswers(List *l) {
	Iterator *it = createIterator(l);

    while(hasNext(it)) {
        free(current(next(it)));
    }

    listFree(l);
    killIterator(it);
}

void freeTopics(List *topics) {
	Iterator *it = createIterator(topics);

    while(hasNext(it)) {
        deleteTopic(current(next(it)));
    }

    listFree(topics);
    killIterator(it);
}

// PROTOCOL VALIDATION
	int validate_REG(char *msg) {
		if (msg[strlen(msg) - 1] != '\n')
			return 0;

	    char *token = strtok(msg, " \n"); // REG
	    if (token == NULL) {
	    	return 0;
	    }

	    token = strtok(NULL, " "); 	// userID
	    if (token == NULL) {
	        return 0;
	    }

	    return verify_ID(token);
	}

	int validate_LTP(char *msg) {
		if (msg[strlen(msg) - 1] != '\n')
			return 0;

	    char *token = strtok(msg, " \n"); // LTP
	    if (token == NULL)
	    	return 0;

	    token = strtok(NULL, " "); 	// nothing more

	    return token == NULL;
	}

	int validate_PTP(char *msg) {
		if (msg[strlen(msg) - 1] != '\n')
			return 0;

	    char *token = strtok(msg, " \n");  // PTP
	    if (token == NULL) {
	    	return 0;
	    }

	    token = strtok(NULL, " "); 	// userID
	    if (token == NULL || !verify_ID(token)) {
	        return 0;
	    }

	    token = strtok(NULL, " \n");	// topic
	    if (token == NULL || strlen(token) > 10)
	        return 0;

	    token = strtok(NULL, " ");	// nothing more

	    return token == NULL;
	}

	int validate_LQU(char *msg) {
		if (msg[strlen(msg) - 1] != '\n')
			return 0;

	    char *token = strtok(msg, " \n"); // LQU
	    if (token == NULL)
	    	return 0;

	    token = strtok(NULL, " \n"); // topic
	    if (token == NULL || strlen(token) > 10)
	        return 0;

	    token = strtok(NULL, " ");	// nothing more
	    return token == NULL;
	}

	// validates QUS userID topic question qsize
	int validate_QUS_ANS(char *msg, int is_qus) {
		char *aux = strdup(msg);
		if (aux == NULL) error_on("strdup", "validate_QUS_ANS");

		char *token = strtok(aux, " ");	// QUS
		if (token == NULL || (is_qus && strcmp(token, "QUS") != 0) || (!is_qus && strcmp(token, "ANS") != 0)) {
			free(aux);
			return 0;
		}

		//userID
		token = strtok(NULL, " "); 	// userID
		if (token == NULL || !verify_ID(token)) {
			free(aux);
			return 0;
		}

		// topic
		token = strtok(NULL, " ");	// topic
		if (token == NULL || strlen(token) > 10) {
			free(aux);
			return 0;
		}

		// question
		token = strtok(NULL, " ");	// question
		if (token == NULL || strlen(token) > 10) {
			free(aux);
			return 0;
		}

		// qsize
		token = strtok(NULL, " "); 	// qsize
		if (token == NULL || !is_number(token)) {
			free(aux);
			return 0;
		}

		free(aux);
		return 1;
	}

	// validate qIMG [iext isize]
	int validate_extra_QUS(char *msg) {
		char *aux = strdup(msg);
		if (aux == NULL) error_on("strdup", "validate_extra_QUS");
		char *token = strtok(aux, " ");

		// qIMG
		if (strcmp(token, "0\n") != 0 && strcmp(token, "1") != 0) {
			free(aux);
			return 0;
		}

		// there's an image
		if (strcmp(token, "1") == 0) {
			// 3 byte extension
			token = strtok(NULL, " ");
			if (strlen(token) != 3) {
				free(aux);
				return 0;
			}

			// isize
			token = strtok(NULL, " ");
			int flag = token != NULL && is_number(token) && msg[6 + ndigits(atoi(token))] == ' ';
			free(aux);
			return flag;
		}

		// no image
		else if (strcmp(token, "0\n") == 0){
			free(aux);
			return 1;
		}

		free(aux);
		return 0;
	}

	int validate_GQU(char *msg) {
		char *aux = strdup(msg);
		if (aux == NULL) error_on("strdup", "validate_GQU");

		char *token = strtok(aux, " ");	// GQU
		if (token == NULL || strcmp(token, "GQU") != 0)  {
			free(aux);
			return 0;
		}

		token = strtok(NULL, " ");
		if (token == NULL || strlen(token) > 10) {
			free(aux);
			return 0;
		}
		int len_topic = strlen(token);

		token = strtok(NULL, " ");
		if (token == NULL || strlen(token) > 10) {
			free(aux);
			return 0;
		}
		int len_question = strlen(token);
		free(aux);
		return msg[4 + len_topic + len_question] == '\n';
	}

	// validate aIMG [iext isize]
	int validate_extra_ANS(char *msg) {
		return validate_extra_QUS(msg);
	}



void TCP_input_validation(int fd) {
    char message[BUF_SIZE], prefix[4];
    char * aux = message, * token;

    bzero(message, BUF_SIZE);
    bzero(prefix, 4);
    if (read(fd, message, BUF_SIZE) == -1) error_on("read", "TCP_input_validation");
    printf("Message: %s\n", message);
    memcpy(prefix, aux, 3);
    aux += 4;
    printf("Prefix: %s\n", prefix);
    if (strcmp(prefix, "QUS") == 0) {
    	if (validate_QUS_ANS(message, 1)) {
	        char topic[11], question[11], userID[6], * qdata, ext[4];
	        int qsize, qIMG, isize;

	        bzero(topic, 11);
	        bzero(question, 11);
	        bzero(userID, 6);
	        bzero(ext, 4);

	        // userID
	        token = strtok(aux, " ");
	        sprintf(userID, "%s",token);

	    	// topic
	        token = strtok(NULL, " ");
	        sprintf(topic, "%s", token);

	        int n_questions = getNumberOfQuestions(topic);
	        if (n_questions < MAX_QUESTIONS) {

	        	// question
		        token = strtok(NULL, " ");
		        sprintf(question, "%s", token);

		        // qsize
		        token = strtok(NULL, " ");
		        qsize = atoi(token);

		        aux = token + ndigits(qsize) + 1;

		        if (topicDirExists(topic)) {

			        if (validateDirectories(topic, question)) {

				        qdata = calloc(BUF_SIZE, sizeof(char));
				        if (qdata == NULL) error_on("calloc", "TCP_input_validation");

						int initial_size = MIN(qsize, BUF_SIZE - (aux - message));
				        //sprintf(qdata, "%s", aux);
				        memcpy(qdata, aux, initial_size);

				        //in case there's some space available in the buffer, fill it in to avoid bad writes
				        if (BUF_SIZE - (aux - message) < qsize) {
				        	int bytes_read = read(fd, qdata + initial_size, BUF_SIZE - initial_size);
				        	if (bytes_read < 0) error_on("read", "TCP_input_validation");
				            initial_size += bytes_read;
				        }

				        int changed = 0;
				        writeTextFile(question, topic, qdata, BUF_SIZE, qsize, fd, &changed, initial_size);

				        if (changed){
				            if (read(fd, message, BUF_SIZE) < 0) error_on("read", "TCP_input_validation");
				            aux = message + 1;
				        } else
				            aux += qsize + 1;

				        if (validate_extra_QUS(aux)) {
				        	printf("after extra validation\n");
				        	int all_clear = 1;
					        token = strtok(aux, " ");
					        qIMG = atoi(token);
					        if (qIMG) {
					            token = strtok(NULL, " ");
					            if (sprintf(ext, "%s", token) < 0) error_on("sprintf", "TCP_input_validation");

					            token = strtok(NULL, " ");
					            isize = atoi(token);
					            printf("Prepare to write: %d\n", isize);
					            //reuse qdata for less memory
					            aux = token + ndigits(isize) + 1;
								initial_size = MIN(isize, BUF_SIZE - (aux - message));
					            memcpy(qdata, aux, initial_size);

					            //in case there's some space available in the buffer, fill it in to avoid bad writes
					            if (BUF_SIZE - (aux - message) < isize) {
					                printf("Sizes: %d %ld\n", isize, initial_size);

					                if ((initial_size += read(fd, qdata + initial_size, BUF_SIZE - initial_size)) < 0)
					                	error_on("read", "TCP_input_validation");
					            }

					            changed = 0;
					            writeImageFile(question, topic, qdata, BUF_SIZE, isize, fd, &changed, ext, initial_size);

					            // validate final \n
					            token = strtok(qdata, "\n");
					            all_clear = (token == NULL ? 0 : 1);
					        }
					        if (all_clear) {
					        	writeAuthorInformation(topic, question, userID, ext);
					        	if (write(fd, "QUR OK\n", 7) < 0) error_on("write", "TCP_input_validation");
					        	printf("Returned OK information.\n");
					        }
					        else {
					        	eraseDirectory(topic, question);
					        	if (write(fd, "ERR\n", 4) < 0) error_on("write", "TCP_input_validation");
					    		printf("Returned wrong protocol information.\n");
					        }
					        free(qdata);
					    }
					    else {
					    	eraseDirectory(topic, question);
					    	if (write(fd, "ERR\n", 4) < 0) error_on("write", "TCP_input_validation");
					    	printf("Returned wrong protocol information.\n");
					    }
				    }
				    else {
				    	if (write(fd, "QUR DUP\n", 8) < 0) error_on("write", "TCP_input_validation");
				    	printf("Returned duplication information.\n");
				    }
				}
				else {
					if (write(fd, "QUR NOK\n", 8) < 0) error_on("write", "TCP_input_validation");
					printf("Returned invalid request information.\n");
				}
		    }
			else {
	    		if (write(fd, "QUR FUL\n", 8) < 0) error_on("write", "TCP_input_validation");
	    		printf("Returned full list information.\n");
	    	}
		}
		else {
			if (write(fd, "ERR\n", 4) < 0) error_on("write", "TCP_input_validation");
			printf("Returned wrong protocol information.\n");
		}

    } else if(strcmp("GQU", prefix) == 0) {
    	if (validate_GQU(message)) {
	        char topic[11], question[11], userID[6], ext[4];
	        bzero(topic, 11);
	        bzero(question, 11);
	        bzero(userID, 6);
	        bzero(ext, 4);

	        token = strtok(aux, " ");
	        sprintf(topic, "%s", token);

	        token = strtok(NULL, " \n");
	        sprintf(question, "%s", token);

	        if (questionDirExists(topic, question)) {
		        int answers_number;
		        List * answers = getAnswers(topic, question, &answers_number);

		        Iterator * it = createIterator(answers);
		        printf("Available answers:\n");
		        while (hasNext(it)) {
		            printf("%s\n", (char*) current(next(it)));
		        }
		        killIterator(it);

		        bzero(message, BUF_SIZE);
		        getAuthorInformation(topic, question, userID, ext);
		        char * txtfile = getQuestionPath(topic, question);
		        char * imgfile = getImagePath(topic, question, ext);

		        int txt_size = get_filesize(txtfile);
		        sprintf(message, "QGR %s %d ", userID, get_filesize(txtfile));
		        if (write(fd, message, strlen(message)) < 0) error_on("write", "TCP_input_validation");

		        readFromFile(txtfile, message, BUF_SIZE, txt_size, fd);

		        int qIMG = imgfile != NULL ? 1 : 0;
		        sprintf(message, " %d ", qIMG);
		        if (qIMG) {
		            int image_size = get_filesize(imgfile);
		            if (sprintf(message + 3, "%s %d ", ext, image_size) < 0) error_on("sprintf", "TCP_input_validation");
		            if (write(fd, message, 3 + strlen(ext) + ndigits(image_size) + 2) < 0) error_on("write", "TCP_input_validation");
		            if (write(1, message, 3 + strlen(ext) + ndigits(image_size) + 2) < 0) error_on("write", "TCP_input_validation");
		            readFromFile(imgfile, message, BUF_SIZE, image_size, fd);
		        }
		        char * aux = message;
		        if (answers_number > 0){
		            if (sprintf(message, " %d", answers_number) < 0) error_on("sprintf", "TCP_input_validation");
		            aux += 1 + ndigits(answers_number);
		        }
		        else {
		            if (sprintf(message, " 0\n") < 0) error_on("sprintf", "TCP_input_validation");
		            aux += 3;
		        }

		        if (write(fd, message, aux - message) < 0) error_on("write", "TCP_input_validation");

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
		            if (sprintf(message, " %02d %s %d ", question_count++, userID, txt_size) < 0) error_on("sprintf", "TCP_input_validation");

		            if (write(fd, message, strlen(message)) < 0) error_on("write", "TCP_input_validation");
		            printf("I'm gonna read from %s\n", txtfile);
		            readFromFile(txtfile, message, BUF_SIZE, txt_size, fd);

		            qIMG = (ext[0] != '\0' ? 1 : 0);
		            if (sprintf(message, " %d", qIMG) < 0) error_on("sprintf", "TCP_input_validation");
		            printf("message: %s\n", message);
		            if (write(fd, message, strlen(message)) < 0) error_on("write", "TCP_input_validation");

		            if (qIMG) {
		                int image_size = get_filesize(imgfile);
		                if (sprintf(message, " %s %d ", ext, image_size) < 0) error_on("sprintf", "TCP_input_validation");
		                if (write(fd, message, strlen(message)) < 0) error_on("write", "TCP_input_validation");

		                readFromFile(imgfile, message, BUF_SIZE, image_size, fd);

		            }
		        }
		        if (write(fd, "\n", 1) < 0) error_on("write", "TCP_input_validation");
		        if (imgfile != NULL) free(imgfile);
		        free(txtfile);
		        freeAnswers(answers);
		        killIterator(it);
		    }
	    	else {
	    		if (write(fd, "QGR ERR\n", 8) < 0) error_on("write", "TCP_input_validation");
	    		printf("Returned invalid request information.\n");
	    	}
	    }
	    else {
	    	if (write(fd, "ERR\n", 4) < 0) error_on("write", "TCP_input_validation");
	    	printf("Returned wrong protocol information.\n");
	    }

    } else if (strcmp("ANS", prefix) == 0) {
    	if (validate_QUS_ANS(message, 0)) {
	        char topic[11], question[11], userID[6], * qdata, ext[4];
	        int qsize, qIMG, isize;
	        printf("ANS inside!\n");

	        bzero(topic, 11);
	        bzero(question, 11);
	        bzero(userID, 6);
	        bzero(ext, 4);

	        token = strtok(aux, " ");
	        if (sprintf(userID, "%s", token) < 0) error_on("sprintf", "TCP_input_validation");

	        token = strtok(NULL, " ");
	        if (sprintf(topic, "%s", token) < 0) error_on("sprintf", "TCP_input_validation");

	        token = strtok(NULL, " ");
	        if (sprintf(question, "%s", token) < 0) error_on("sprintf", "TCP_input_validation");

	        token = strtok(NULL, " ");
	        qsize = atoi(token);

	        aux = token + ndigits(qsize) + 1;

	        // make sure topic and question already exist
	        int answer_number;
		    if (questionDirExists(topic, question) && (answer_number = answerDirectoriesValidation(topic, question)) < MAX_ANSWERS) {
		        qdata = calloc(BUF_SIZE, sizeof(char));
		        if (qdata == NULL) error_on("calloc", "TCP_input_validation");

		        //sprintf(qdata, "%s", aux);
				int initial_size = MIN(qsize, BUF_SIZE - (aux - message));
		        memcpy(qdata, aux, initial_size);

		        //in case there's some space available in the buffer, fill it in to avoid bad writes
		        if (BUF_SIZE - (aux - message) < qsize)
		            if ((initial_size += read(fd, qdata + initial_size, BUF_SIZE - initial_size)) < 0)
		            	error_on("read", "TCP_input_validation");

		        int changed = 0;
		        answerWriteTextFile(question, topic, qdata, BUF_SIZE, qsize, fd, &changed, answer_number, initial_size);


		        if (changed){
		            if (read(fd, message, BUF_SIZE) < 0) error_on("read", "TCP_input_validation");
		            aux = message + 1;
		        } else
		            aux += qsize + 1;

		        if (validate_extra_ANS(aux)) {

			        token = strtok(aux, " ");
			        qIMG = atoi(token);

			        if (qIMG) {
			            token = strtok(NULL, " ");
			            if (sprintf(ext, "%s", token) < 0) error_on("sprintf", "TCP_input_validation");

			            token = strtok(NULL, " ");
			            isize = atoi(token);
			            printf("Prepare to write: %d\n", isize);
			            //reuse qdata for less memory
			            aux = token + ndigits(isize) + 1;

						int initial_size = MIN(isize, BUF_SIZE - (aux - message));
			            memcpy(qdata, aux, initial_size);

			            //in case there's some space available in the buffer, fill it in to avoid bad writes
			            if (BUF_SIZE - (aux - message) < isize) {
			                printf("Sizes: %d %ld\n", isize, BUF_SIZE - (aux - message));
			                if ((initial_size += read(fd, qdata + initial_size, BUF_SIZE - initial_size)) < 0)
			                	error_on("read", "TCP_input_validation");
			            }

			            changed = 0;
			            answerWriteImageFile(question, topic, qdata, BUF_SIZE, isize, fd, &changed, ext, answer_number, initial_size);
			        }
			        // validate final \n
		            token = strtok(qdata, "\n");
		            int all_clear = (token == NULL ? 0 : 1);

			        if (all_clear) {
			        	answerWriteAuthorInformation(topic, question, userID, ext, answer_number);
			        	if (write(fd, "ANR OK\n", 7) < 0) error_on("write", "TCP_input_validation");
			        	printf("Returned OK information.\n");
			        }
			        else {
			        	answerEraseDirectory(topic, question, answer_number);
			        	if (write(fd, "ERR\n", 4) < 0) error_on("write", "TCP_input_validation");
			    		printf("Returned wrong protocol information.\n");
			        }
			        free(qdata);
			    }
			    else {	// qIMG [iext isize] are wrong
			    	if (write(fd, "ANR NOK\n", 8) < 0) error_on("write", "TCP_input_validation");
		    		printf("Returned invalid request information.\n");
			    }
		    }
		    // answer list is full
		    else if (answer_number >= 99) {
		    	if (write(fd, "ANR FUL\n", 8) < 0) error_on("write", "TCP_input_validation");
		    	printf("Returned full list information.\n");
		    }
		    else {	// topic or question non existent
		    	if (write(fd, "ANR NOK\n", 8) < 0) error_on("write", "TCP_input_validation");
		    	printf("Returned invalid request information.\n");
		    }
	    }
	    else {	// wrong formulation
	    	if (write(fd, "ANR NOK\n", 8) < 0) error_on("write", "TCP_input_validation");
	    	printf("Returned invalid request information.\n");
	    }
    }
    else {	// wrong protocol
    	if (write(fd, "ERR\n", 4) < 0) error_on("write", "TCP_input_validation");
    	printf("Returned wrong protocol informarion.\n");
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
	int status = 0;

    for (;;) {
		while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
				printf("[DEBUG] Child process %d has finished and been gotten back with status: %d\n", pid, status);
		}

        FD_SET(fd_tcp, &set);
        FD_SET(fd_udp, &set);
        int fd_ready = select(maxfd, &set, NULL, NULL, NULL);
        if (fd_ready < 0) error_on("select", "main");

        if (FD_ISSET(fd_tcp, &set)) {
            printf("Receiving from TCP client.\n");
            user_addrlen = sizeof(user_addr);
            if ((newfd = accept(fd_tcp, (struct sockaddr *)&user_addr, &user_addrlen)) == -1)
                error_on("accept", "main");

            else {
                pid = fork();
                if (pid < 0) {
                    error_on("fork", "main");
                }
                else if (pid == 0){ //Child process
                    printf("dei fork\n");
                    //write(newfd, "Oi babyyy\n", 11);

                    int msg_size = MAX_REQ_LEN;
                    char * message = calloc(msg_size, sizeof(char));
                    if (message == NULL) error_on("calloc", "main");

                    /*msg_size = read_TCP(newfd, &message, msg_size, 0);
                    printf("Message received: %s\n", message);*/
                    freeaddrinfo(res_udp);
                    TCP_input_validation(newfd);
                    free(message);
                    freeaddrinfo(res_tcp);
                    deleteTable(users);
                    deleteTable(topics_hash);
                    freeTopics(topics);
                    return 0;

                }
                else { //parent process
                    close(newfd);
                }
            }
        }
        if (FD_ISSET(fd_udp, &set)) {
            printf("Receiving from UDP client.\n");
            user_addrlen = sizeof(user_addr);

            bzero(buffer, 1024);
            int n = recvfrom(fd_udp, buffer, 1024, 0, (struct sockaddr *) &user_addr, &user_addrlen);
            if (n == -1) error_on("recvfrom", "main");

            printf("Message received: %s\n", buffer);
            char *to_validate = strdup(buffer);
            char *to_token = strdup(buffer);
            if (to_validate == NULL || to_token == NULL) error_on("strdup", "main");

            char * token = strtok(buffer, " \n");
            if (token != NULL && strcmp(token, "REG") == 0) {
                if (validate_REG(to_validate)) {
                    n = sendto(fd_udp, "RGR OK\n", 7, 0, (struct sockaddr *) &user_addr, user_addrlen);
                	if (n == -1) error_on("sendto", "main");
                }
                else {
                    send_ERR_MSG_UDP(fd_udp, &res_udp);
                }
            } else if (token != NULL && strcmp(token, "LTP") == 0) {
                if (validate_LTP(to_validate)) {
                    if (n_topics > 0) {
                        char *list = (char*) malloc(sizeof(char) * 17 * n_topics);
                        if (list == NULL) error_on("malloc", "main");
                        int list_size = list_topics(topics, n_topics, list);

                        char *message = malloc(sizeof(char) * (7 + list_size));
                        if (message == NULL) error_on("malloc", "main");

                        if (sprintf(message, "LTR %d %s\n", n_topics, list) < 0) error_on("sprintf", "main");

                        n = sendto(fd_udp, message, 6 + list_size, 0, (struct sockaddr *) &user_addr, user_addrlen);
                        if (n < 0) error_on("sendto", "main");

                        free(list);
                        free(message);
                    }
                    else {
                        n = sendto(fd_udp, "LTR 0\n", 6, 0, (struct sockaddr *) &user_addr, user_addrlen);
                    	if (n < 0) error_on("sento", "main");
                    }
                }
                 else {
                    send_ERR_MSG_UDP(fd_udp, &res_udp);
                }

            } else if (strcmp(token, "PTP") == 0) {
                if (validate_PTP(to_validate)) {
                    token = strtok(to_token, " ");

                    // userID
                    token = strtok(NULL, " ");
                    char *stringID = strdup(token);
                    if (stringID == NULL) error_on("strdup", "main");

                    // topic
                    token = strtok(NULL, " \n");
                    char *topic_title = strdup(token);
                    if (topic_title == NULL) error_on("strdup", "main");


                    if (n_topics == MAX_TOPICS) {
                        n = sendto(fd_udp, "PTR FUL\n", 8, 0, (struct sockaddr *) &user_addr, user_addrlen);
                        if (n == -1) error_on("sendto", "main");

                        printf("Returned full list information.\n");
                    }

                    else if (!registered_user(users, atoi(stringID)) || strlen(topic_title) > 10) {
                        n = sendto(fd_udp, "PTR NOK\n", 8, 0, (struct sockaddr *) &user_addr, user_addrlen);
                        if (n == -1) error_on("sendto", "main");

                        printf("Returned invalid request information.\n");
                    }

                    else if (topic_exists(topics, topic_title)) {
                        n = sendto(fd_udp, "PTR DUP\n", 8, 0, (struct sockaddr *) &user_addr, user_addrlen);
                        if (n == -1) error_on("sendto", "main");

                        printf("Returned duplication information\n");
                    }

                    else {
                        Topic* new = createTopic(topic_title, atoi(stringID));
                        addEl(topics, new);
                        insertInTable(topics_hash, new, hash(topic_title));
                        createTopicDir(topic_title);
                        n_topics++;

                        n = sendto(fd_udp, "PTR OK\n", 7, 0, (struct sockaddr *) &user_addr, user_addrlen);
                        if (n == -1) error_on("sendto", "main");

                        printf("Returned OK information\n");
                    }

                    free(stringID);
                    free(topic_title);
                }
                else {
                    send_ERR_MSG_UDP(fd_udp, &res_udp);
                    printf("Returned wrong protocol information.\n");
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
                    char * message = calloc(7 + ndigits(n_questions) + n_questions * (10 + 1 + 5 + 1 + 2), sizeof(char));
                    if (message == NULL) error_on("calloc", "main");
                    char * msg_help = message;

                    //LQR N
                    if (sprintf(msg_help, "LQR %d", (n_questions == 0 ? 0 : n_questions)) < 0) error_on("sprintf", "main");
                    msg_help += (n_questions == 0 ? 5 : 4 + ndigits(n_questions));

                    //populate with the amount of questions
                    if (n_questions > 0) {
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
                            	if (sprintf(n_answers, "0%d", n) < 0) error_on("sprintf", "main");
                            }
                            else {
                            	if (sprintf(n_answers, "%d", n) < 0) error_on("sprintf", "main");
                            }

                            if (sprintf(msg_help, " %s:%s:%s", question, userID, n_answers) < 0) error_on("sprintf", "main");
                            msg_help += 10 + strlen(question);
                            j--;
                            free(question);
                        }
                        killIterator(it);
                    }
                    else {
                    	msg_help++;
                    }

                    *(msg_help) = '\n';
                    printf("Char: %c with value %d\n", *msg_help, *msg_help);
                    printf("String: \"%s\" with size: %ld\n", message, strlen(message));
                    n = sendto(fd_udp, message, strlen(message), 0, (struct sockaddr *) &user_addr, user_addrlen);
                    if (n < 0) error_on("sendto", "main");

                    free(message);
                    if (questions_list !=NULL)
                    	listFree(questions_list);
                }
                else {
                    send_ERR_MSG_UDP(fd_udp, &res_udp);
                }

            }

            free(to_validate);
            free(to_token);

        }

	FD_SET(fd_tcp, &set);
	FD_SET(fd_udp, &set);
	}
}
