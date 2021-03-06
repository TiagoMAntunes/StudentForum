#include <stdlib.h>
#include <stdio.h>
#include "file_management.h"
#include <string.h>
#include "list.h"
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#define PREFIX "./topics/"
#define PREFIX_LEN 9
#define QUESTION_LEN 12
#define IMAGE_LEN 6
#define EXT_LEN 3

#define MIN(A,B) ((A) < (B) ? (A) : (B))

FILE * getQuestionText(Topic * topic, char * question) {
    return NULL;
}

FILE * getQuestionImage(Topic * topic, char * question) {
    return NULL;
}

void validateReadWrite(FILE *f, char *filename) {
    if (ferror(f)) {
        if (filename != NULL) free(filename);
        fclose(f);
        perror("File related error.\n");
        exit(1);
    }
}

void validateOpen(FILE *f, char* filename) {
	char msg [21 + strlen(filename)];
	sprintf(msg, "Error opening file: %s\n", filename);
	if (f == NULL) {
		perror(msg);
		exit(EXIT_FAILURE);
	}
}

int fileExists(char *filename) {
    struct stat sb;
    return (stat(filename, &sb) == 0);
}

void createTopicDir(char *topic) {
    char *dir = calloc(PREFIX_LEN + strlen(topic) + 2,sizeof(char));
    sprintf(dir, "%s%s/", PREFIX, topic);
    mkdir(PREFIX, 0700); //requires ./topic dir to be created first
    mkdir(dir, 0700);
    free(dir);
}

int topicDirExists(char *topic) {
    char *dir = calloc(PREFIX_LEN + strlen(topic) + 2,sizeof(char));
    sprintf(dir, "%s%s/", PREFIX, topic);
    struct stat sb;

    int flag = (stat(dir, &sb) == 0 ? 1 : 0);
    free(dir);
    return flag;
}

int questionDirExists(char *topic, char *question) {
    char *dir =  calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 3,sizeof(char));
    sprintf(dir, "%s%s/%s/", PREFIX, topic, question);
    struct stat sb;

    int flag = (stat(dir, &sb) == 0 ? 1 : 0);
    free(dir);
    return flag;
}

int validateDirectories(char * topic, char * question) {
    char * dir = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 3,sizeof(char));
    struct stat sb;
    sprintf(dir, "%s%s/%s", PREFIX, topic, question);


    if (stat(dir, &sb) == -1) {


        mkdir(PREFIX, 0700);

        sprintf(dir, "%s%s/", PREFIX, topic);
        mkdir(dir, 0700);

        sprintf(dir, "%s%s/%s/", PREFIX, topic, question);
        if (mkdir(dir, 0700) < 0){
            free(dir);
            return 0;
        }
        free(dir);
        return 1;
    }
    free(dir);
    return 0;
}

void eraseDirectory(char *topic, char* question) {
    char *command =  calloc(7 + PREFIX_LEN + strlen(question) + strlen(topic) + 3,sizeof(char));
    sprintf(command, "rm -rf %s%s/%s/", PREFIX, topic, question);
    
    if (system(command) < 0) {
    	perror("Error removing directory.\n");
    	exit(1);
    }
    free(command);
}

void writeToFile(char * filename, char * buffer, int buffer_size, int total_size, int fd, int * changed, int initial_size) {
    FILE * f = fopen(filename, "w+");
    validateOpen(f, filename);


    int available_size = initial_size;
    while (total_size > 0) {
        clearerr(f);
        total_size -= fwrite(buffer, sizeof(char), MIN(total_size, available_size), f);
        validateReadWrite(f, NULL);
        if (total_size > 0) {
            bzero(buffer, buffer_size);
            if ((available_size = read(fd, buffer, MIN(buffer_size, total_size))) < 0) {
                fclose(f);
                perror("Error writing on file.\n");
                exit(1);
            }
            *changed = 1;
        }
    }

    fclose(f);
}

void readFromFile(char * filename, char * buffer, int buffer_size, int total_size, int fd) {
    FILE * f = fopen(filename, "r");
    validateOpen(f, filename);




    clearerr(f);
    int n = fread(buffer, sizeof(char), buffer_size, f);
    validateReadWrite(f, NULL);

    while(total_size > 0) {
        int n_bytes = write(fd, buffer, MIN(buffer_size, n));
        if (n_bytes < 0) {
            fclose(f);
            perror("Error reading from file.\n");
            exit(1);
        }
        total_size -= n_bytes;
        if (total_size > 0) {
            clearerr(f);
            n = fread(buffer, sizeof(char), buffer_size, f);
            validateReadWrite(f, NULL);
        }

    }

    fclose(f);
}

void writeAuthorInformation(char * topic, char * question, char * userID, char * ext) {
    char * filename = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 2 + 12 /* .information */ + 1, sizeof(char));
    sprintf(filename, "%s%s/%s/.information", PREFIX, topic, question);

    FILE * f = fopen(filename, "w+");
    validateOpen(f, filename);


    clearerr(f);
    fwrite(userID, sizeof(char), 5, f);
    validateReadWrite(f, filename);

    fwrite(" ", sizeof(char), 1, f);
    validateReadWrite(f, filename);

    fwrite(ext, sizeof(char), 3, f);
    validateReadWrite(f, filename);

    fclose(f);
    free(filename);
}

void answerWriteAuthorInformation(char * topic, char * question, char * userID, char * ext, int answer_number) {
    char * filename = calloc(PREFIX_LEN + strlen(question) * 2 + strlen(topic) + 3 + 12 /* .information */ + 4 + 1, sizeof(char));
    sprintf(filename, "%s%s/%s/%s_%02d/.information", PREFIX, topic, question, question, answer_number);

    FILE * f = fopen(filename, "w+");
    validateOpen(f, filename);


    clearerr(f);
    fwrite(userID, sizeof(char), 5, f);
    validateReadWrite(f, filename);

    fwrite(" ", sizeof(char), 1, f);
    validateReadWrite(f, filename);

    fwrite(ext, sizeof(char), 3, f);
    validateReadWrite(f, filename);

    fclose(f);
    free(filename);
}

void getAuthorInformation(char * topic, char * question, char * userID, char * ext) {
    char * filename = calloc(PREFIX_LEN + strlen(topic) + strlen(question) + 2 + 1, sizeof(char));
    sprintf(filename, "%s%s/%s/", PREFIX, topic, question);
    getAnswerInformation(filename, userID, ext);
    free(filename);
}

void getAnswerInformation(char * answer_dir, char * userID, char * ext) {
    char * filename = calloc(strlen(answer_dir) + 12 + 1, sizeof(char));
    sprintf(filename, "%s.information", answer_dir);
    FILE * f = fopen(filename, "r");
    validateOpen(f, filename);

    clearerr(f);
    fread(userID, 5, sizeof(char), f);
    validateReadWrite(f, filename);

    getc(f);

    fread(ext, 3, sizeof(char), f);
    validateReadWrite(f, filename);

    fclose(f);
    free(filename);
}

void writeTextFile(char * question, char * topic, char * buffer, int buffer_size, int qsize, int fd, int * changed, int start_size) {
    char * filename = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 3 + QUESTION_LEN , sizeof(char)); //textfilename
    sprintf(filename, "%s%s/%s/question.txt", PREFIX, topic, question);

    writeToFile(filename, buffer, buffer_size, qsize, fd, changed, start_size);
    free(filename);
}

void writeImageFile(char * question, char * topic, char * buffer, int buffer_size, int qsize, int fd, int * changed, char * ext, int start_size) {
    char * filename = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 3 + IMAGE_LEN + EXT_LEN, sizeof(char));
    sprintf(filename, "%s%s/%s/image.%s", PREFIX, topic, question, ext);

    writeToFile(filename, buffer, buffer_size, qsize, fd, changed, start_size);
    free(filename);
}

void createQuestion(char * topic, char * question, char * text, int text_size, char * image, int image_size, char * ext) {
    char * file_text = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 3 + QUESTION_LEN , sizeof(char));
    char * file_img = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 3 + IMAGE_LEN + EXT_LEN, sizeof(char));
    char * dir = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 3,sizeof(char));

    sprintf(file_text, "%s%s/%s/question.txt", PREFIX, topic, question);
    sprintf(file_img, "%s%s/%s/image.%s", PREFIX, topic, question, ext);
    sprintf(dir, "%s%s/%s/", PREFIX, topic, question);


    struct stat sb;


    if (stat(dir, &sb) == -1) {


        mkdir(PREFIX, 0700);

        sprintf(dir, "%s%s/", PREFIX, topic);
        mkdir(dir, 0700);

        sprintf(dir, "%s%s/%s/", PREFIX, topic, question);
        mkdir(dir, 0700);
    }

    FILE * f = fopen(file_text, "w+");
    validateOpen(f, file_text);

    clearerr(f);
    fwrite(text, sizeof(char), text_size, f);
    if (ferror(f)) {
        free(file_text);
        free(file_img);
        free(dir);
        fclose(f);
        perror("Error writing on file - createQuestion.\n");
        exit(1);
    }
    fclose(f);

    f = fopen(file_img, "w+");
    validateOpen(f, file_img);
    clearerr(f);
    fwrite(image, sizeof(char), image_size, f);
    if (ferror(f)) {
        free(file_text);
        free(file_img);
        free(dir);
        fclose(f);
        perror("Error writing on file - createQuestion.\n");
        exit(1);
    }
    fclose(f);

    free(file_text);
    free(file_img);
    free(dir);
}

int getNumberOfAnswers(char * topic, char * question) {
    char * dir = calloc(PREFIX_LEN + strlen(topic) + 1 + strlen(question) + 2, sizeof(char));
    sprintf(dir, "%s%s/%s/", PREFIX, topic, question);
    int n_files = 0;
    DIR * dirp;
    struct dirent * entry;

    dirp = opendir(dir);
    while ((entry = readdir(dirp)) != NULL)
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
            n_files++;

    free(dir);
    closedir(dirp);
    return n_files;
}

int answerDirectoriesValidation(char * topic, char * question) {
    int answer_number = getNumberOfAnswers(topic, question) + 1;
    return answerDirectoriesValidationWithNumber(topic, question, answer_number);
}

int answerDirectoriesValidationWithNumber(char * topic, char * question, int answer_number) {
    char * dir = calloc(PREFIX_LEN + strlen(topic) + 1 + strlen(question) * 2 + 5 + 1,sizeof(char));
    validateDirectories(topic, question);

    struct stat sb;
    if (stat(dir, &sb) == -1) {
        sprintf(dir, "%s%s/%s/%s_%02d/", PREFIX, topic, question, question, answer_number);
        mkdir(dir, 0700);
    }

    free(dir);
    return answer_number;
}

void answerEraseDirectory(char *topic, char *question, int answer_number) {
    char * command = calloc(7 + PREFIX_LEN + strlen(topic) + 1 + strlen(question) * 2 + 5 + 1,sizeof(char));
    sprintf(command, "rm -rf %s%s/%s/%s_%02d/", PREFIX, topic, question, question, answer_number);
    if (system(command) < 0) {
        free(command);
        perror("Error removing directory - answerEraseDirectory.\n");
        exit(1);
    }
    free(command);
}

void answerWriteTextFile(char * question, char * topic, char * buffer, int buffer_size, int qsize, int fd, int * changed, int answer_number, int start_size) {
    char * file_text = calloc(PREFIX_LEN + strlen(topic) + 1 + strlen(question) * 2 + 5+ strlen("answer.") + EXT_LEN + 1, sizeof(char));
    sprintf(file_text, "%s%s/%s/%s_%02d/answer.txt", PREFIX, topic, question, question, answer_number);
    writeToFile(file_text, buffer, buffer_size, qsize, fd, changed, start_size);

    free(file_text);
}

void answerWriteImageFile(char * question, char * topic, char * buffer, int buffer_size, int qsize, int fd, int * changed, char * ext, int answer_number, int start_size) {
    char * file_img = calloc(PREFIX_LEN + strlen(topic) + 1 + strlen(question) * 2 + 5 + strlen("image.") + EXT_LEN + 1, sizeof(char));
    sprintf(file_img, "%s%s/%s/%s_%02d/image.%s", PREFIX, topic, question, question, answer_number, ext);
    writeToFile(file_img, buffer, buffer_size, qsize, fd, changed, start_size);

    free(file_img);
}

void createAnswer(char * topic, char * question, char * text, int text_size, char * image, int image_size, char * ext) {

    int answer_number = getNumberOfAnswers(topic, question) + 1;

    char * file_text = calloc(PREFIX_LEN + strlen(topic) + 1 + strlen(question) * 2 + 5+ strlen("answer.") + EXT_LEN + 1, sizeof(char));
    char * file_img = calloc(PREFIX_LEN + strlen(topic) + 1 + strlen(question) * 2 + 5 + strlen("image.") + EXT_LEN + 1, sizeof(char));
    char * dir = calloc(PREFIX_LEN + strlen(topic) + 1 + strlen(question) * 2 + 5 + 1,sizeof(char));

    sprintf(file_text, "%s%s/%s/%s_%02d/answer.txt", PREFIX, topic, question, question, answer_number);
    sprintf(file_img, "%s%s/%s/%s_%02d/image.%s", PREFIX, topic, question, question, answer_number, ext);
    sprintf(dir, "%s%s/%s/%s_%02d/", PREFIX, topic, question, question, answer_number);


    struct stat sb;


    if (stat(dir, &sb) == -1) {


        mkdir(PREFIX, 0700);

        sprintf(dir, "%s%s/", PREFIX, topic);
        mkdir(dir, 0700);

        sprintf(dir, "%s%s/%s/", PREFIX, topic, question);
        mkdir(dir, 0700);

        sprintf(dir, "%s%s/%s/%s_%02d/", PREFIX, topic, question, question, answer_number);
        mkdir(dir, 0700);
    }


    FILE * f = fopen(file_text, "w+");
    validateOpen(f, file_text);

    clearerr(f);
    fwrite(text, sizeof(char), text_size, f);
    if (ferror(f)) {
        free(file_text);
        free(file_img);
        free(dir);
        fclose(f);
        perror("Error writing on file - createAnswer.\n");
        exit(1);
    }
    fclose(f);

    f = fopen(file_img, "w+");
    validateOpen(f, file_img);
    fwrite(image, sizeof(char), image_size, f);
    if (ferror(f)) {
        free(file_text);
        free(file_img);
        free(dir);
        fclose(f);
        perror("Error writing on file - createAnswer.\n");
        exit(1);
    }

    fclose(f);
    free(file_text);
    free(file_img);
    free(dir);
}

List * getAnswers(char * topic, char * question, int * count) {
    int n;
    struct dirent ** namelist;
    char * dir_name = calloc(PREFIX_LEN + strlen(topic) + 1 + strlen(question) + 2, sizeof(char));
    sprintf(dir_name, "%s%s/%s/", PREFIX, topic, question);
    n = scandir(dir_name, &namelist, NULL, alphasort);
    if (n == -1) {
        perror("Error in scandir - getAnswer.\n");
        exit(EXIT_FAILURE);
    }

    List * res = newList();

    int dir_count = 0;
    int dir_size = strlen(dir_name);
    char *answer = NULL;

    while (n-- > 0 && dir_count < 10)
		if (namelist[n]->d_type == DT_DIR && strcmp(namelist[n]->d_name, ".") && strcmp(namelist[n]->d_name, "..")) {
			answer = calloc(dir_size + strlen(namelist[n]->d_name) + 2, sizeof(char));
			if (answer == NULL) {
				perror("Error on calloc - getAnswers.\n");
				exit(EXIT_FAILURE);
			}
			sprintf(answer, "%s%s/", dir_name, namelist[n]->d_name);
			addEl(res, answer);
			dir_count++;
		}

    free(namelist);
    free(dir_name);
    *count = dir_count;

    return res;
}

List * getTopicQuestions(char * topic, int * list_size) {
    struct dirent * de;
    char * dir_name = calloc(PREFIX_LEN + strlen(topic) + 1, sizeof(char));
    sprintf(dir_name, "%s%s", PREFIX, topic);

    DIR * dir = opendir(dir_name);

    if (dir == NULL) return NULL;

    //Create return list
    List * list = newList();
    *list_size = 0;
    while((de = readdir(dir)) != NULL)
        if (!strstr(de->d_name, ".") && !strstr(de->d_name, "..")) {

            (*list_size)++;
            addEl(list, strdup(de->d_name));
        }

    closedir(dir);
    free(dir_name);
    return list;
}

void getTopicUserID(char * topic, char * userID) {
    char * filename = calloc(9 + strlen(topic) + 5 + 1, sizeof(char));
    sprintf(filename, "./topics/%s/.uid", topic);
    FILE * f = fopen(filename, "r");
    fread(userID, sizeof(char), 5, f);
    fclose(f);
    free(filename);
}

void registerTopic(char * topic, char * userID) {
    char * filename = calloc(9 + strlen(topic) + 5 + 1, sizeof(char));
    sprintf(filename, "./topics/%s/.uid", topic);
    FILE * f = fopen(filename, "w");
    fwrite(userID, sizeof(char), 5, f);
    fclose(f);
    free(filename);
}

int getTopics(List * list) {
    struct dirent * de;
    DIR * dir = opendir("./topics");
    if (dir == NULL) return 0;
    int count = 0;
    char userID[6] = {0};
    while ((de = readdir(dir)) != NULL)
        if (de->d_type == DT_DIR  && !strstr(de->d_name, ".") && !strstr(de->d_name, ".."))  {
            getTopicUserID(de->d_name, userID);
            addEl(list, createTopic(de->d_name, atoi(userID)));
            count++;
        }
    closedir(dir);
    return count;
}

int getNumberOfQuestions(char *topic) {
    struct dirent * de;
    char * dir_name = calloc(PREFIX_LEN + strlen(topic) + 1, sizeof(char));
    sprintf(dir_name, "%s%s", PREFIX, topic);

    DIR * dir = opendir(dir_name);

    if (dir == NULL) return 0;

    int list_size = 0;
    while((de = readdir(dir)) != NULL)
        if (!strstr(de->d_name, ".") && !strstr(de->d_name, "..")) {
            list_size++;
        }

    closedir(dir);
    free(dir_name);
    return list_size;
}


char * getQuestionPath(char * topic, char * question) {
    char * filename = calloc(PREFIX_LEN + strlen(topic) + strlen(question) + 2 + 12 + 1, sizeof(char));

    sprintf(filename, "%s%s/%s/question.txt", PREFIX, topic, question);
    return filename;
}

char * getImagePath(char * topic, char * question, char * ext) {
    char * filename = calloc(PREFIX_LEN + strlen(topic) + strlen(question) + 2 + 6 /* image. */ + 3 /* ext */ + 1, sizeof(char));

    sprintf(filename, "%s%s/%s/image.%s", PREFIX, topic, question, ext);
    struct stat sb;
    if (stat(filename, &sb) == -1) {
        //does not exist
        free(filename);
        return NULL;
    }
    return filename;
}

char * getAnswerQuestionPath(char * answer_dir) {
    char * filename = calloc(strlen(answer_dir) + 10 + 1, sizeof(char));
    sprintf(filename, "%sanswer.txt", answer_dir);
    return filename;
}

char * getAnswerImagePath(char * answer_dir, char * ext) {
    char * filename = calloc(strlen(answer_dir) + 6 + strlen(ext) + 1, sizeof(char));
    sprintf(filename, "%simage.%s", answer_dir, ext);
    return filename;
}
