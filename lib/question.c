#include <stdlib.h>
#include <string.h>
#include "question.h"
#include "list.h"
#include "iterator.h"
#include "answer.h"

Question *newQuestion(char *text, int image, int id) {
    Question *new = (Question *) malloc(sizeof(struct question));

    new->title = strdup(text);
    new->image = image;
    new->answers = newList();
    new->n_answers = 0;
    new->userID = id;

    return new;
}

void deleteAnswers(Question *q) {
    if (q->n_answers > 0) {
        Iterator *it = createIterator(q->answers);

        while(hasNext(it)) {
            deleteAnswer(current(next(it)));
        }
        killIterator(it);
    }
    free(q->answers);
}

char *getQuestionTitle(Question *q) { return q->title; }
int getQuestionID(Question *q) {return q->userID; }

void deleteQuestion(Question *q) {
    deleteAnswers(q);
    if (q->title != NULL) free(q->title);
    free(q);
} 
