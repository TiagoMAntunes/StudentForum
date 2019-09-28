#ifndef __QUESTION_H__
#define __QUESTION_H__

#include "list.h"

typedef struct question {
    char *title;
    char *image;    // image extention
    List *answers;
    int n_answers;
    int userID;

} Question;


// text or image is NULL if not specified
Question *newQuestion(char *text, int image, int id);
char *getQuestionTitle(Question *q);
int getQuestionID(Question *q);
void deleteAnswers(Question *q);
void deleteQuestion(Question *q);

#endif