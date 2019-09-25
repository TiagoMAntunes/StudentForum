#include "topic.h"
#include "hash.h"
#include <string.h>


Topic* createTopic(char* title, int userID) {
    Topic* new = (Topic*) malloc(sizeof(Topic));
    new->title = strdup(title);
    new->userID = userID;
    new->questions = createTable(1024, sizeof(List));
}

char* getTopicTitle(Topic* topic) {
    return topic->title;
}

int getTopicID(Topic* topic) {
    return topic->userID;
}

void addQuestion(Topic *topic, char* question, unsigned long id) {
    insertInTable(topic->questions, question, id);
}

void deleteTopic(Topic* topic) {
    deleteTable(topic->questions);
    free(topic->title);
    free(topic); 
}

unsigned long hash(char *str) {
    unsigned long hash = 5387;
    int c;  
    while (c = *str++)
        hash = ((hash << 5) + hash) + c; 

    return hash;
}