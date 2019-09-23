#ifndef __TOPIC_H__
#define __TOPIC_H__

#include "hash.h"

typedef struct {
    char *title;
    Hash *questions;
} Topic;

Topic* createTopic(char* title);
char* getTopicTitle(Topic* topic);
void addQuestion(Topic *topic, char* question, unsigned long id) ;
void deleteTopic(Topic* topic);

#endif