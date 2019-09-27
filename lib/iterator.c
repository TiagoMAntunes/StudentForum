#include "iterator.h"
#include "list.h"

Iterator *createIterator(List *head)
{
    Iterator *t = malloc(sizeof(Iterator));
    t->next = head->next;
    t->prev = head->prev;
    return t;
}

int hasNext(Iterator *it)
{
    return it->next != NULL;
}

int hasPrev(Iterator *it) {
    return it->prev != NULL;
}

List *next(Iterator *it)
{
    List *tmp = it->next;
    it->next = it->next->next;
    it->prev = tmp;

    return tmp;
}

List *prev(Iterator *it) {
    List *tmp = it->prev;
    it->prev = it->prev->prev;
    it->next = tmp;
    return tmp;
}


void killIterator(Iterator *it)
{
    free(it);
}