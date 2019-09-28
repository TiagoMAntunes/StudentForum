#include <stdlib.h>
#include <string.h>
#include "question.h"
#include "list.h"
#include "iterator.h"
#include "answer.h"

Question *newQuestion(char *text, char *image) {
    Question *new = (Question *) malloc(sizeof(struct question));

    new->text = strdup(text);
    new->image = strdup(image);
    new->answers = newList();
    new->n_answers = 0;

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

void deleteQuestion(Question *q) {
    deleteAnswers(q);
    if (q->text != NULL) free(q->text);
    if (q->image != NULL) free(q->image);
    free(q);
} 
