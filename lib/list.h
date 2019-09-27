#ifndef __LIST_H
#define __LIST_H

#include <stdlib.h>

typedef struct list
{
    struct list *next;
    void *current;
    struct list *prev;
} List;

/**
 * Removes the specified element from the list
 * Receives a pointer to the list head
 * Receives a pointer to the element to be removed
*/
void removeEl(List *list, List *el);

/**
 * Creates a list struct element and returns a pointer to it 
 * Receives a pointer to the element to be part of the list
 * Returns a pointer to a list node
*/
void addEl(List *list, void *elToAdd);

/**
 * Appends an element to the list
 * Receives a pointer to the list head and to the element to be added
*/
List *createNode(void *el);

/** Prints the given element given the print function 
 * Receives a pointer to the list element and a pointer to the print function 
 * 
 *  Example function:
 *  void printStr(void * el) {
 *      printf("%s\n", (char *)el);
 *  }
 */
void print(List *el, void (*fn)());

/**
 * Creates a newList
 * Returns a pointer to the head
 */
List *newList();

/**
 * Checks if list is empty
 * Receives a pointer to the head of the list
 * Returns true if list is empty
 */
int isEmpty(List *l);

/**
 * Frees list and all its nodes
 * Receives a pointer to the head
 */
void listFree(List *head);

/**
 * Returns the element of a given node
 */
void *current(List *el);

#endif