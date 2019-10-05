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
#define BUF_SIZE 1024

#define MIN(A,B) ((A) < (B) ? (A) : (B))

FILE * getQuestionText(Topic * topic, char * question) {
    return NULL;
}

FILE * getQuestionImage(Topic * topic, char * question) {
    return NULL;
}

void validateDirectories(char * topic, char * question) {
    char * dir = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 3,sizeof(char));
    struct stat sb;

    printf("Checking directory %s\n", dir);
    if (stat(dir, &sb) == -1) {
        printf("Directories missing. Creating...\n");

        int check = mkdir(PREFIX, 0700);

        sprintf(dir, "%s%s/", PREFIX, topic);
        check = mkdir(dir, 0700);

        sprintf(dir, "%s%s/%s/", PREFIX, topic, question);
        check = mkdir(dir, 0700);
    }
}

void writeToFile(char * filename, char * buffer, int buffer_size, int total_size, int fd, int * changed) {
    FILE * f = fopen(filename, "w+");
    printf("Opened: %s\n", filename);
    while (total_size > 0) {
        total_size -= fwrite(buffer, sizeof(char), MIN(total_size, buffer_size), f);
        if (total_size > 0) { 
            read(fd, buffer, MIN(buffer_size, total_size));
            *changed = 1;
        }
    }
    
    fclose(f);
}

void writeTextFileNew(char *question, char *topic, char *buffer, int n_bytes, int append_flag) {
    char * filename = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 3 + QUESTION_LEN , sizeof(char)); //textfilename
    sprintf(filename, "%s%s/%s/question.txt", PREFIX, topic, question);

    FILE *f = fopen(filename, (append_flag ? "a" : "w+"));
    printf("Opened: %s\n", filename);
    fwrite(buffer, sizeof(char), n_bytes, f);

    fclose(f);
}

void readFromFile(char * filename, char * buffer, int buffer_size, int total_size, int fd) {
    FILE * f = fopen(filename, "r");
    fread(buffer, buffer_size, sizeof(char), f);
    while(total_size > 0) {
        total_size -= write(fd, buffer, buffer_size);
        if (total_size > 0) {
            fread(buffer, buffer_size, sizeof(char), f);
        }
    }
    fclose(f);
}


void writeAuthorInformation(char * topic, char * question, char * userID, char * ext) {
    char * filename = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 2 + 12 /* .information */ + 1, sizeof(char));
    sprintf(filename, "%s%s/%s/.information", PREFIX, topic, question);
    
    FILE * f = fopen(filename, "w+");
    printf("%d\n", f);
    fwrite(userID, sizeof(char), 5, f);
    fwrite(" ", sizeof(char), 1, f);
    fwrite(ext, sizeof(char), 3, f);

    fclose(f);
    free(filename);
}

void getAuthorInformation(char * topic, char * question, char * userID, char * ext) {
    char * filename = calloc(PREFIX_LEN + strlen(topic) + strlen(question) + 2 + 12 + 1, sizeof(char));
    sprintf(filename, "%s%s/%s/.information", PREFIX, topic, question);
    FILE * f = fopen(filename, "r");
    
    fread(userID, 5, sizeof(char), f);
    getc(f);
    fread(ext, 3, sizeof(char), f);

    fclose(f);
    free(filename);
}

void getAnswerInformation(char * answer_dir, char * userID, char * ext) {
    char * filename = calloc(strlen(answer_dir) + 12 + 1, sizeof(char));
    sprintf(filename, "%s.information", answer_dir);
    FILE * f = fopen(filename, "r");
    
    fread(userID, 5, sizeof(char), f);
    getc(f);
    fread(ext, 3, sizeof(char), f);

    fclose(f);
    free(filename);
}

void writeTextFile(char * question, char * topic, char * buffer, int buffer_size, int qsize, int fd, int * changed) {
    char * filename = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 3 + QUESTION_LEN , sizeof(char)); //textfilename
    sprintf(filename, "%s%s/%s/question.txt", PREFIX, topic, question);
    
    writeToFile(filename, buffer, buffer_size, qsize, fd, changed);
}

void writeImageFile(char * question, char * topic, char * buffer, int buffer_size, int qsize, int fd, int * changed, char * ext) {
    char * filename = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 3 + IMAGE_LEN + EXT_LEN, sizeof(char));
    sprintf(filename, "%s%s/%s/image.%s", PREFIX, topic, question, ext);
    
    writeToFile(filename, buffer, buffer_size, qsize, fd, changed);
}

void createQuestion(char * topic, char * question, char * text, int text_size, char * image, int image_size, char * ext) {
    char * file_text = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 3 + QUESTION_LEN , sizeof(char));
    char * file_img = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 3 + IMAGE_LEN + EXT_LEN, sizeof(char));
    char * dir = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 3,sizeof(char));
    sprintf(file_text, "%s%s/%s/question.txt", PREFIX, topic, question);
    sprintf(file_img, "%s%s/%s/image.%s", PREFIX, topic, question, ext);
    sprintf(dir, "%s%s/%s/", PREFIX, topic, question);
    printf("%s\n%s\n", file_text, file_img);

    struct stat sb;

    printf("Checking directory %s\n", dir);
    if (stat(dir, &sb) == -1) {
        printf("Directories missing. Creating...\n");

        int check = mkdir(PREFIX, 0700);

        sprintf(dir, "%s%s/", PREFIX, topic);
        check = mkdir(dir, 0700);

        sprintf(dir, "%s%s/%s/", PREFIX, topic, question);
        check = mkdir(dir, 0700);
    }


    FILE * f = fopen(file_text, "w+");
    fwrite(text, sizeof(char), text_size, f);
    fclose(f);

    f = fopen(file_img, "w+");
    fwrite(image, sizeof(char), image_size, f);
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
    char * dir = calloc(PREFIX_LEN + strlen(topic) + 1 + strlen(question) * 2 + 5 + 1,sizeof(char));
    validateDirectories(topic, question);

    struct stat sb;
    if (stat(dir, &sb) == -1) {
        sprintf(dir, "%s%s/%s/%s_%02d/", PREFIX, topic, question, question, answer_number);
        int check = mkdir(dir, 0700);
    }
    return answer_number;
}

void answerWriteTextFile(char * question, char * topic, char * buffer, int buffer_size, int qsize, int fd, int * changed, int answer_number) {
    char * file_text = calloc(PREFIX_LEN + strlen(topic) + 1 + strlen(question) * 2 + 5+ strlen("answer.") + EXT_LEN + 1, sizeof(char));
    sprintf(file_text, "%s%s/%s/%s_%02d/answer.txt", PREFIX, topic, question, question, answer_number);
    writeToFile(file_text, buffer, buffer_size, qsize, fd, changed);
}

void answerWriteImageFile(char * question, char * topic, char * buffer, int buffer_size, int qsize, int fd, int * changed, char * ext, int answer_number) {
    char * file_img = calloc(PREFIX_LEN + strlen(topic) + 1 + strlen(question) * 2 + 5 + strlen("image.") + EXT_LEN + 1, sizeof(char));
    sprintf(file_img, "%s%s/%s/%s_%02d/image.%s", PREFIX, topic, question, question, answer_number, ext);
    writeToFile(file_img, buffer, buffer_size, qsize, fd, changed);
}

void createAnswer(char * topic, char * question, char * text, int text_size, char * image, int image_size, char * ext) {

    int answer_number = getNumberOfAnswers(topic, question) + 1;

    char * file_text = calloc(PREFIX_LEN + strlen(topic) + 1 + strlen(question) * 2 + 5+ strlen("answer.") + EXT_LEN + 1, sizeof(char));
    char * file_img = calloc(PREFIX_LEN + strlen(topic) + 1 + strlen(question) * 2 + 5 + strlen("image.") + EXT_LEN + 1, sizeof(char));
    char * dir = calloc(PREFIX_LEN + strlen(topic) + 1 + strlen(question) * 2 + 5 + 1,sizeof(char));
    sprintf(file_text, "%s%s/%s/%s_%02d/answer.txt", PREFIX, topic, question, question, answer_number);
    sprintf(file_img, "%s%s/%s/%s_%02d/image.%s", PREFIX, topic, question, question, answer_number, ext);
    sprintf(dir, "%s%s/%s/%s_%02d/", PREFIX, topic, question, question, answer_number);
    printf("%s\n%s\n", file_text, file_img);

    struct stat sb;

    printf("Checking directory %s\n", dir);
    if (stat(dir, &sb) == -1) {
        printf("Directories missing. Creating...\n");

        int check = mkdir(PREFIX, 0700);

        sprintf(dir, "%s%s/", PREFIX, topic);
        check = mkdir(dir, 0700);

        sprintf(dir, "%s%s/%s/", PREFIX, topic, question);
        check = mkdir(dir, 0700);

        sprintf(dir, "%s%s/%s/%s_%02d/", PREFIX, topic, question, question, answer_number);
        check = mkdir(dir, 0700);
    }


    FILE * f = fopen(file_text, "w+");
    fwrite(text, sizeof(char), text_size, f);
    fclose(f);

    f = fopen(file_img, "w+");
    fwrite(image, sizeof(char), image_size, f);
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
        perror("Error in scandir");
        exit(EXIT_FAILURE);
    }

    List * res = newList();

    int dir_count = 0;
    int dir_size = strlen(dir_name);
    while (n-- > 0 && dir_count < 10)
      if (namelist[n]->d_type == DT_DIR && strcmp(namelist[n]->d_name, ".") && strcmp(namelist[n]->d_name, "..")) {
              char * answer = calloc(dir_size + strlen(namelist[n]->d_name) + 2, sizeof(char));
              sprintf(answer, "%s%s/", dir_name, namelist[n]->d_name);
              addEl(res, answer);
              dir_count++;
      }

    free(namelist);
    *count = dir_count;
    return res;
}

List * getTopicQuestions(char * topic, int * list_size) {
    struct dirent * de;
    char * dir_name = calloc(PREFIX_LEN + strlen(topic) + 1, sizeof(char));
    sprintf(dir_name, "%s%s", PREFIX, topic);
    printf("Opening: %s\n", dir_name);
    DIR * dir = opendir(dir_name);

    if (dir == NULL) return NULL;

    //Create return list
    List * list = newList();
    *list_size = 0;
    while((de = readdir(dir)) != NULL)
        if (!strstr(de->d_name, ".") && !strstr(de->d_name, "..")) {
                printf("Found Question: %s\n", de->d_name);
                (*list_size)++;
                addEl(list, strdup(de->d_name));
            }

    closedir(dir);
    free(dir_name);
    return list;
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
    char * filename = calloc(strlen(answer_dir) + 12 + 1, sizeof(char));
    sprintf(filename, "%squestion.txt", answer_dir);
    return filename;
}

char * getAnswerImagePath(char * answer_dir, char * ext) {
    char * filename = calloc(strlen(answer_dir) + 6 + strlen(ext) + 1, sizeof(char));
    sprintf(filename, "%simage.%s", answer_dir, ext);
    return filename;
}

int readTextFileAndSend(char *message, char *filename, int file_size, int *offset, int fd) {
    int bytes_left = file_size;
    int space_left = BUF_SIZE - (*offset) * sizeof(char);
    int read;
    FILE * f = fopen(filename, "r");
    while (bytes_left != 0) {
        read = fread(message + ((*offset) * sizeof(char)), sizeof(char), MIN(space_left, bytes_left), f);
        space_left -= read;
        bytes_left -= read;
        if (bytes_left != 0) {
            write(fd, message, BUF_SIZE);
            bzero(message, BUF_SIZE);
            *offset = 0;
            space_left = BUF_SIZE;
        }
    }

    fclose(f);

    return space_left;
}

char *readServerAndWriteToFile(char *question, char *topic, char *answer, char *answer_aux, int data_read, int qsize, int bytes_read, int fd_tcp) {
    int append_flag = 0;
    writeTextFileNew(question, topic, answer_aux, data_read, append_flag);
    int total_data_writen = data_read;
    while (total_data_writen != qsize) {
        bzero(answer, 1024);
        answer_aux = answer;
        bytes_read = read(fd_tcp, answer, 1024);
        int data_left = qsize - total_data_writen;
        data_read = (data_left > bytes_read ? 1024 : data_left);
        total_data_writen += data_read;
        append_flag = 1;
        writeTextFileNew(question, topic, answer_aux, data_read, append_flag);

/*  DEBUGG
        printf("data_read = %d\n", data_read);
        printf("total_data_writen = %d\n", total_data_writen);
*/ 
    }
    return answer_aux;
}