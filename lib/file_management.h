#include <stdio.h>
#include "topic.h"
#include "list.h"

#ifndef __FILE_MANAGEMENT__
#define __FILE_MANAGEMENT__

FILE * getQuestionText(Topic * topic, char * question);
FILE * getQuestionImage(Topic * topic, char * question);
void createQuestion(char * topic, char * question, char * text, int text_size, char * image, int image_size, char * ext);
void createAnswer(char * topic, char * question, char * text, int text_size, char * image, int image_size, char * ext);
List * getAnswers(char * topic, char * question); //change return value later
List * getTopicQuestions(char * dir_name, int* list_size);


#endif