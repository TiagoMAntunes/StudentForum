#include <stdio.h>
#include "topic.h"

#ifndef __FILE_MANAGEMENT__
#define __FILE_MANAGEMENT__

FILE * getQuestionText(Topic * topic, char * question);
FILE * getQuestionImage(Topic * topic, char * question);
void createQuestion(char * topic, char * question, char * text, int text_size, char * image, int image_size, char * ext);
void createAnswer(Topic * topic, char * question, char * text, char * image);
void getAnswer(Topic * topic, char * question, int id); //change return value later



#endif