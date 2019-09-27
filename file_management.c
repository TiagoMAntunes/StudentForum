#include <stdlib.h>
#include <stdio.h>
#include "file_management.h"
#include <string.h>

#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define PREFIX "./topics/"
#define PREFIX_LEN 9
#define QUESTION_LEN 12
#define IMAGE_LEN 6
#define EXT_LEN 3

FILE * getQuestionText(Topic * topic, char * question) {
    return NULL;
}

FILE * getQuestionImage(Topic * topic, char * question) {
    return NULL;
}

void createQuestion(char * topic, char * question, char * text, int text_size, char * image, int image_size, char * ext) {
    char * file_text = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 3 + QUESTION_LEN , sizeof(char));
    char * file_img = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 3 + IMAGE_LEN + EXT_LEN, sizeof(char));
    char * dir = calloc(PREFIX_LEN + strlen(question) + strlen(topic) + 3,sizeof(char));
    sprintf(file_text, "%s%s/%s/question.txt", PREFIX, topic, question);
    sprintf(file_img, "%s%s/%s/image.%s", PREFIX, topic, question, ext);
    sprintf(dir, "%s%s/", PREFIX, topic);
    printf("%s\n%s\n", file_text, file_img);
    
    struct stat sb;

    if (stat(dir, &sb) == -1) {
        printf("Directories missing. Creating...\n");
      
        int check = mkdir(PREFIX, 0700);
      
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
}

void createAnswer(Topic * topic, char * question, char * text, char * image) {

}
void getAnswer(Topic * topic, char * question, int id) {

}