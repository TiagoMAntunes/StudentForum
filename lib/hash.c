#include "list.h"
#include "hash.h"

Hash *createTable(size_t tableSize, size_t structSize)
{
    int i;
    Hash *table = malloc(sizeof(Hash));
    table->table = (List **)malloc(tableSize * structSize);
    for (i = 0; i < tableSize; i++)
        table->table[i] = newList();
    table->size = tableSize;
    return table;
}

void insertInTable(Hash *table, void *element, unsigned long id)
{
    addEl(table->table[id % table->size], element);
}

void removeFromTable(Hash *table, List *element, unsigned long id)
{
    removeEl(table->table[id % table->size], element);
}

List *findInTable(Hash *table, unsigned long id)
{
    return table->table[id % table->size];
}

void deleteTable(Hash *table)
{
    int i;
    /* Free all sublists */
    for (i = 0; i < table->size; i++)
        listFree(table->table[i]);
    free(table->table);
    free(table);
}