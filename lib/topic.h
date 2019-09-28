#ifndef __TOPIC_H__
#define __TOPIC_H__

#include "hash.h"
#include "list.h"
#include "question.h"

typedef struct {
    char *title;
    int userID;
    Hash *questions_hash;
    List *questions;
    int n_questions;
} Topic;

Topic* createTopic(char* title, int userID);
char* getTopicTitle(Topic* topic);
int getTopicID(Topic* topic);
void printTopicQuestion(Topic *topic);
void addQuestion(Topic *topic, Question* question, unsigned long id);
void deleteQuestions(Topic *topic);
void deleteTopic(Topic* topic);
unsigned long hash(char *str);

#endif