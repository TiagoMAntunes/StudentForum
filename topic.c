#include "topic.h"
#include "hash.h"
#include <string.h>


Topic* createTopic(char* title) {
    Topic* new = (Topic*) malloc(sizeof(Topic));
    new->title = strdup(title);
    new->questions = createTable(1024, sizeof(List));
}

char* getTopicTitle(Topic* topic) {
    return topic->title;
}

void addQuestion(Topic *topic, char* question, unsigned long id) {
    insertInTable(topic->questions, question, id);
}

void deleteTopic(Topic* topic) {
    deleteTable(topic->questions);
    free(topic->title);
    free(topic);
}