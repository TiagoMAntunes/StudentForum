#include <stdlib.h>
#include <string.h>
#include "answer.h"

Answer *newAnswer(char *t, char *i) {
    Answer *new = (Answer *) malloc(sizeof(struct answer));
    new->text = strdup(t);
    new->image = strdup(i);

    return new;
}

void deleteAnswer(Answer *a) {
    free(a->text);
    free(a->image);
    free(a);
}