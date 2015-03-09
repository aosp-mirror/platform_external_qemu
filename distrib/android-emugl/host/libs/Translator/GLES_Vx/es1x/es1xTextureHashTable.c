/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "es1xTextureHashTable.h"

void es1xTextureHashTable_init(es1xTextureHashTable* table, deUint32 capacity)
{
  deUint32 i;

  ES1X_ASSERT(table);

  table->capacity                 = capacity;
  table->allocationTable  = (es1xTextureHashTableKeyValuePair*)es1xMalloc(sizeof(es1xTextureHashTableKeyValuePair) * table->capacity);
  table->firstFree                = table->allocationTable;

  for(i=0;i<table->capacity;i++)
  {
    table->allocationTable[i].next  = &table->allocationTable[i+1];
    table->allocationTable[i].key   = 0;
    table->allocationTable[i].value = 0;
  }
  table->allocationTable[table->capacity-1].next = 0;

  table->hashTable = es1xMalloc(sizeof(es1xTextureHashTableKeyValuePair**) * table->capacity);
  es1xMemZero(table->hashTable, sizeof(es1xTextureHashTableKeyValuePair*) * table->capacity);
  table->size = 0;
}

/*---------------------------------------------------------------------------*/

es1xTextureHashTable* es1xTextureHashTable_create(void)
{
  es1xTextureHashTable* hashTable = (es1xTextureHashTable*) es1xMalloc(sizeof(es1xTextureHashTable));
  es1xTextureHashTable_init(hashTable, ES1X_INITIAL_HASH_TABLE_SIZE);
  return hashTable;
}

/*---------------------------------------------------------------------------*/

void es1xTextureHashTable_resize(es1xTextureHashTable* table, deUint32 capacity)
{
  es1xTextureHashTableKeyValuePair*       prevEntries             = table->allocationTable;
  es1xTextureHashTableKeyValuePair**      prevTable               = table->hashTable;
  deUint32                                                        prevCapacity    = table->capacity;
  deUint32                                                        i;

  es1xTextureHashTable_init(table, capacity);

  for(i=0;i<prevCapacity;i++)
    es1xTextureHashTable_insert(table, prevEntries[i].key, prevEntries[i].value);

  es1xFree(prevTable);
  es1xFree(prevEntries);
}

/*---------------------------------------------------------------------------*/

void es1xTextureHashTable_uninit(es1xTextureHashTable* table)
{
  es1xFree(table->hashTable);
  es1xFree(table->allocationTable);
}

/*---------------------------------------------------------------------------*/

void es1xTextureHashTable_destroy(es1xTextureHashTable* table)
{
  es1xTextureHashTable_uninit(table);
  es1xFree(table);
}

/*---------------------------------------------------------------------------*/

void es1xTextureHashTable_insert(es1xTextureHashTable* table, GLuint key, es1xTexture2D* value)
{
  ES1X_ASSERT(table);

  if (table->firstFree == 0)
    es1xTextureHashTable_resize(table, table->capacity * 2);

  /* Calculate hash value */
  {
    deUint32 hash = es1xHashu(key, table->capacity);

    /* Allocate key-value pair */
    es1xTextureHashTableKeyValuePair* pair = table->firstFree;
    table->firstFree = pair->next;

    /* Insert key-value pair to the hash table */
    pair->key                               = key;
    pair->value                             = value;
    pair->next                              = table->hashTable[hash];
    table->hashTable[hash]  = pair;
    table->size                             = table->size + 1;
  }
}

/*---------------------------------------------------------------------------*/

es1xTexture2D* es1xTextureHashTable_remove(es1xTextureHashTable* table, GLuint key)
{
  ES1X_ASSERT(table);

  /*    Find corresponding key-value pair by iterating through the list of pairs.
        Also keep previous entry in memory */
  {
    deUint32 hash = es1xHashu(key, table->capacity);

    es1xTextureHashTableKeyValuePair* pair = table->hashTable[hash];
    es1xTextureHashTableKeyValuePair* prev = 0;

    while(pair)
    {
      if (es1xEqualsu(key, pair->key))
      {
        es1xTexture2D* retVal = pair->value;

        /* Link previous entry to the next so that this
           pair drops out from the list */
        if (prev)
          prev->next = pair->next;
        else
          table->hashTable[hash] = pair->next;

        /* Insert key-value pair back to the allocation table */
        pair->key                   = 0;
        pair->value                 = 0;
        pair->next                  = table->firstFree;
        table->firstFree    = pair;

        return retVal;
      }

      prev = pair;
      pair = pair->next;
    }
  }

  /* A program matching the key was not found */
  return 0;
}

/*---------------------------------------------------------------------------*/

es1xTexture2D* es1xTextureHashTable_get(es1xTextureHashTable* table, GLuint key)
{
  deUint32 hash = es1xHashu(key, table->capacity);

  es1xTextureHashTableKeyValuePair* pair = table->hashTable[hash];

  while(pair)
  {
    if (es1xEqualsu(key, pair->key))
      return pair->value;

    pair = pair->next;
  }

  return 0;
}

/*---------------------------------------------------------------------------*/
