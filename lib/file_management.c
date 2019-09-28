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

void createAnswer(Topic * topic, char * question, char * text, char * image) {

}
void getAnswer(Topic * topic, char * question, int id) {

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