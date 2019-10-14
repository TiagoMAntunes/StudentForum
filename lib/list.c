
#include "iterator.h"
#include "list.h"

void removeEl(List *head, List *el)
{
    List *iterator = head, *tmp;

    /* Find el to remove previous pointer */
    while (iterator != NULL && current(iterator->next) != current(el))
        iterator = iterator->next;

    /* Removes from list */
    if (iterator != NULL)
    {
        tmp = iterator->next;
        iterator->next = tmp->next;
        free(tmp);
    }
}

List *createNode(void *el)
{
    List *tmp = malloc(sizeof(List));
    tmp->next = NULL;
    tmp->current = el;
    return tmp;
}

void addEl(List *head, void *elToAdd)
{
    List *el = createNode(elToAdd);
    el->next = head->next;
    if (head->next != NULL) {
        head->next->prev = el;
    }
    head->next = el;
    el->prev = head;
}

void print(List *el, void (*fn)())
{
    fn(el->current);
}

List *newList()
{
    List *head = malloc(sizeof(List));
    head->next = NULL;
    head->current = NULL;
    head->prev = NULL;
    return head;
}

int isEmpty(List *l)
{
    return l->next == NULL;
}

int listSize(List *l) {
    Iterator *it = createIterator(l);
    int n = 0;

    while(hasNext(it)){
        n++;
        next(it);
    }
    killIterator(it);
    return n;
    
}

void listFree(List *head)
{
    List *tmp;
    while (head->next != NULL)
    {
        tmp = head;
        head = head->next;
        free(tmp);
    }
    free(head);
}

void *current(List *el)
{
    return el->current;
}

