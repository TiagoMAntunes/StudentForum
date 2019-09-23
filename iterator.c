#include "iterator.h"
#include "list.h"

Iterator *createIterator(List *head)
{
    Iterator *t = malloc(sizeof(Iterator));
    t->next = head->next;
    return t;
}

int hasNext(Iterator *it)
{
    return it->next != NULL;
}

List *next(Iterator *it)
{
    List *tmp = it->next;
    it->next = it->next->next;
    return tmp;
}

void killIterator(Iterator *it)
{
    free(it);
}