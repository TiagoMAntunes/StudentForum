#ifndef __ITERATOR_H__
#define __ITERATOR_H__

#include "list.h"

typedef struct
{
    List *next;
} Iterator;

/**
 * Creates an iterator of the list 
 * Receives a pointer to the list head
 * Returns a pointer to the iterator
 */
Iterator *createIterator(List *head);

/**
 * Receives a pointer to an iterator
 * Returns true if next element of iterator exists
 */
int hasNext(Iterator *);

/**
 * Advances the iterator and returns the next element
 * Receives a pointer to an iterator
 * Returns a pointer to the next element
 */
List *next(Iterator *);

/**
 * Deletes the iterator from the program
 * Receives a pointer to an iterator
 */
void killIterator(Iterator *it);

#endif