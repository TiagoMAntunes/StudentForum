#ifndef __QUESTION_H__
#define __QUESTION_H__

#include "list.h"

typedef struct question {
    char *text;
    char *image;
    List *answers;
    int n_answers;

} Question;


// text or image is NULL if not specified
Question *newQuestion(char *text, char *image);
void deleteAnswers(Question *q);
void deleteQuestion(Question *q);

#endif