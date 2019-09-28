#include "topic.h"
#include "hash.h"
#include "list.h"
#include "question.h"
#include "iterator.h"
#include <string.h>


Topic* createTopic(char* title, int userID) {
    Topic* new = (Topic*) malloc(sizeof(Topic));
    new->title = strdup(title);
    new->userID = userID;
    new->questions_hash = createTable(1024, sizeof(List));
    new->questions = newList();
    new->n_questions = 0;

}

char* getTopicTitle(Topic* topic) {
    return topic->title;
}

int getTopicID(Topic* topic) {
    return topic->userID;
}

void addQuestion(Topic *topic, Question* question, unsigned long id) {
    insertInTable(topic->questions_hash, question, id);
    addEl(topic->questions, question);
    topic->n_questions++;
}

void deleteQuestions(Topic *topic) {
    if (topic->n_questions > 0) {
        Iterator *it = createIterator(topic->questions);

        while(hasNext(it)) {
            deleteQuestion(current(next(it)));
        }

        killIterator(it);
    }

    free(topic->questions);
}

void deleteTopic(Topic *topic) {
    deleteTable(topic->questions_hash);
    free(topic->title);
    deleteQuestions(topic);
    free(topic); 
}

unsigned long hash(char *str) {
    unsigned long hash = 5387;
    int c;  
    while (c = *str++)
        hash = ((hash << 5) + hash) + c; 

    return hash;
}