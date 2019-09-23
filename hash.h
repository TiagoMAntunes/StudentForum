#ifndef __HASH_H__
#define __HASH_H__

#include "list.h"

typedef struct
{
    List **tableLastIndexes;
    List **table;
    size_t size;
} Hash;

/**
 * Creates a Hash table element and returns a pointer to it
 * Receives a size_t for the number of table rows, and a size_t for struct size
 * Returns a pointer to the new hash table
 */
Hash *createTable(size_t tableSize, size_t structSize);

/**
 * Inserts an element indexed by id in the given table
 * Receives a hash pointer, a void pointer and an unsigned long
 */
void insertInTable(Hash *table, void *element, unsigned long id);

/**
 * Removes a list element from the table given its index
 * Receives a hash pointer, a list pointer and an unsigned long
 */
void removeFromTable(Hash *table, List *element, unsigned long id);

/**
 * Returns the head of the list with the specified id
 * Receives a hash pointer and a int
 * Returns a pointer to the head of the list
 */
List *findInTable(Hash *table, unsigned long id);

/**
 * Removes the table from memory, freeing all its lists
 * Receives a hash pointer
 */
void deleteTable(Hash *table);

#endif