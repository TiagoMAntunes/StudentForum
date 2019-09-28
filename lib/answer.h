#ifndef __ANSWER_H__
#define __ANSWER_H__

typedef struct answer {
    char *text;
    char *image;
} Answer;


// t or i is null if not specified
Answer *newAnswer(char *t, char *i);
void deleteAnswer(Answer *a);



#endif