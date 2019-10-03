#include <stdio.h>
#include "topic.h"
#include "list.h"

#ifndef __FILE_MANAGEMENT__
#define __FILE_MANAGEMENT__

FILE * getQuestionText(Topic * topic, char * question);
FILE * getQuestionImage(Topic * topic, char * question);
void createQuestion(char * topic, char * question, char * text, int text_size, char * image, int image_size, char * ext);
void createAnswer(char * topic, char * question, char * text, int text_size, char * image, int image_size, char * ext);
List * getAnswers(char * topic, char * question, int * count); //change return value later
List * getTopicQuestions(char * dir_name, int* list_size);
void validateDirectories(char * topic, char * question);
void writeTextFile(char * question, char * topic, char * buffer, int buffer_size, int qsize, int fd, int * changed);
void writeImageFile(char * question, char * topic, char * buffer, int buffer_size, int qsize, int fd, int * changed, char * ext);
int answerDirectoriesValidation(char * topic, char * question);
void answerWriteTextFile(char * question, char * topic, char * buffer, int buffer_size, int qsize, int fd, int * changed, int answer_number);
void answerWriteImageFile(char * question, char * topic, char * buffer, int buffer_size, int qsize, int fd, int * changed, char * ext, int answer_number);
void writeAuthorInformation(char * topic, char * question, char * userID);
void getAuthorInformation(char * topic, char * question, char * userID, char * ext);
char * getQuestionPath(char * topic, char * question);
char * getImagePath(char * topic, char * question, char * ext);
void readFromFile(char * filename, char * buffer, int buffer_size, int total_size, int fd);
void getAnswerInformation(char * answer_dir, char * userID, char * ext);
char * getAnswerQuestionPath(char * answer_dir);
char * getAnswerImagePath(char * answer_dir, char * ext);
#endif