#include <stdio.h>
#include "topic.h"
#include "list.h"

#ifndef __FILE_MANAGEMENT__
#define __FILE_MANAGEMENT__

// Topic
List * getTopicQuestions(char * dir_name, int* list_size);
int getNumberOfQuestions(char *topic);
void createTopicDir(char *topic);
int topicDirExists(char *topic);

// Question
int questionDirExists(char *topic, char *question);
void createQuestion(char * topic, char * question, char * text, int text_size, char * image, int image_size, char * ext);
FILE * getQuestionText(Topic * topic, char * question);
FILE * getQuestionImage(Topic * topic, char * question);
char * getQuestionPath(char * topic, char * question);
int getNumberOfAnswers(char * topic, char * question);

//Answer
void answerEraseDirectory(char *topic, char *question, int answer_number);
void createAnswer(char * topic, char * question, char * text, int text_size, char * image, int image_size, char * ext);
List * getAnswers(char * topic, char * question, int * count); //change return value later
int answerDirectoriesValidation(char * topic, char * question);
int answerDirectoriesValidationWithNumber(char * topic, char * question, int answer_number);
void answerWriteAuthorInformation(char * topic, char * question, char * userID, char * ext, int answer_number);
void answerWriteTextFile(char * question, char * topic, char * buffer, int buffer_size, int qsize, int fd, int * changed, int answer_number, int start_size);
void answerWriteImageFile(char * question, char * topic, char * buffer, int buffer_size, int qsize, int fd, int * changed, char * ext, int answer_number, int start_size);
void getAnswerInformation(char * answer_dir, char * userID, char * ext);
char * getAnswerQuestionPath(char * answer_dir);
char * getAnswerImagePath(char * answer_dir, char * ext);

// General
void validateReadWrite(FILE *f, char *filename);
int validateDirectories(char * topic, char * question);
void eraseDirectory(char *topic, char* question);
int fileExists(char *filename);
void writeTextFile(char * question, char * topic, char * buffer, int buffer_size, int qsize, int fd, int * changed, int start_size);
void writeImageFile(char * question, char * topic, char * buffer, int buffer_size, int qsize, int fd, int * changed, char * ext, int start_size);
void writeAuthorInformation(char * topic, char * question, char * userID, char * ext);
void getAuthorInformation(char * topic, char * question, char * userID, char * ext);
char * getImagePath(char * topic, char * question, char * ext);
void readFromFile(char * filename, char * buffer, int buffer_size, int total_size, int fd);

#endif