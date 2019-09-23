#ifndef __TOPIC_H__
#define __TOPIC_H__

#include "hash.h"

typedef struct {
    char *title;
    int userID;
    Hash *questions;
} Topic;

Topic* createTopic(char* title, int userID);
char* getTopicTitle(Topic* topic);
void addQuestion(Topic *topic, char* question, unsigned long id) ;
void deleteTopic(Topic* topic);
unsigned long hash(char *str);

#endif