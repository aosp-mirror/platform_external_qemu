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

 
#include "es1xShaderProgramInfoHashTable.h"
#include "es1xDefs.h"
#include "es1xShaderContextKey.h"
#include "es1xShaderPrograms.h"

/*---------------------------------------------------------------------------*/

DE_STATIC_ASSERT(ES1X_SHADER_PROGRAM_HASH_TABLE_SIZE >= 1);
DE_STATIC_ASSERT((ES1X_SHADER_PROGRAM_HASH_TABLE_SIZE & (ES1X_SHADER_PROGRAM_HASH_TABLE_SIZE - 1)) == 0);

/*---------------------------------------------------------------------------*/

int es1xHashShaderContextKey(es1xShaderContextKey* key, deUint32 size)
{
  /* \todo [090326:christian] Replace with better hashing function */
  int hash = 0;
  unsigned int i = 0;

  for(i=0;i<key->curIndex;i++)
    hash ^= key->data[i];

  return hash & (size - 1);
}

deBool es1xShaderContextKeyEquals(es1xShaderContextKey* a, es1xShaderContextKey* b)
{
  unsigned int i;

  if (a->curIndex != b->curIndex)
    return DE_FALSE;

  if (a->curLeft != b->curLeft)
    return DE_FALSE;

  for(i=0;i<a->curIndex+1;i++)
    if (a->data[i] != b->data[i])
      return DE_FALSE;

  return DE_TRUE;
}


/*---------------------------------------------------------------------------*/

void es1xShaderProgramHashTable_init(es1xShaderProgramHashTable* table, deUint32 capacity)
{
  deUint32 i;

  ES1X_ASSERT(table);

  table->capacity         = capacity;
  table->allocationTable  = (es1xShaderProgramHashTableKeyValuePair*) es1xMalloc(sizeof(es1xShaderProgramHashTableKeyValuePair) * table->capacity);
  table->firstFree        = table->allocationTable;

  for(i=0;i<table->capacity;i++)
    {
      table->allocationTable[i].next  = &table->allocationTable[i+1];
      table->allocationTable[i].key   = 0;
      table->allocationTable[i].value = 0;
    }
  table->allocationTable[table->capacity-1].next = 0;

  table->hashTable = (es1xShaderProgramHashTableKeyValuePair**) es1xMalloc(sizeof(es1xShaderProgramHashTableKeyValuePair*) * table->capacity);
  es1xMemZero(table->hashTable, sizeof(es1xShaderProgramHashTableKeyValuePair*) * table->capacity);
  table->size = 0;
}

/*---------------------------------------------------------------------------*/

es1xShaderProgramHashTable* es1xShaderProgramHashTable_create(void)
{
  es1xShaderProgramHashTable* hashTable = (es1xShaderProgramHashTable*) es1xMalloc(sizeof(es1xShaderProgramHashTable));
  es1xShaderProgramHashTable_init(hashTable, ES1X_INITIAL_HASH_TABLE_SIZE);
  return hashTable;
}

/*---------------------------------------------------------------------------*/

void es1xShaderProgramHashTable_resize(es1xShaderProgramHashTable* table, deUint32 capacity)
{
  es1xShaderProgramHashTableKeyValuePair*   prevEntries     = table->allocationTable;
  es1xShaderProgramHashTableKeyValuePair**  prevTable       = table->hashTable;
  deUint32                            prevCapacity    = table->capacity;
  deUint32                            i;

  es1xShaderProgramHashTable_init(table, capacity);

  for(i=0;i<prevCapacity;i++)
    es1xShaderProgramHashTable_insert(table, prevEntries[i].key, prevEntries[i].value);

  es1xFree(prevTable);
  es1xFree(prevEntries);
}

/*---------------------------------------------------------------------------*/

void es1xShaderProgramHashTable_uninit(es1xShaderProgramHashTable* table)
{
  es1xFree(table->hashTable);
  es1xFree(table->allocationTable);
}

/*---------------------------------------------------------------------------*/

void es1xShaderProgramHashTable_destroy(es1xShaderProgramHashTable* table)
{
  es1xShaderProgramHashTable_uninit(table);
  es1xFree(table);
}

/*---------------------------------------------------------------------------*/

void es1xShaderProgramHashTable_insert(es1xShaderProgramHashTable* table, es1xShaderContextKey* key, es1xShaderProgramInfo* value)
{
  ES1X_ASSERT(table);

  if (table->firstFree == 0)
    es1xShaderProgramHashTable_resize(table, table->capacity * 2);

  /* Calculate hash value */
  {
    deUint32 hash = es1xHashShaderContextKey(key, table->capacity);

    /* Allocate key-value pair */
    es1xShaderProgramHashTableKeyValuePair* pair = table->firstFree;
    table->firstFree = pair->next;

    /* Insert key-value pair to the hash table */
    pair->key               = key;
    pair->value             = value;
    pair->next              = table->hashTable[hash];
    table->hashTable[hash]  = pair;
    table->size             = table->size + 1;
  }
}

/*---------------------------------------------------------------------------*/

es1xShaderProgramInfo* es1xShaderProgramHashTable_remove(es1xShaderProgramHashTable* table, es1xShaderContextKey* key)
{
  ES1X_ASSERT(table);

  /*  Find corresponding key-value pair by iterating through the list of pairs.
      Also keep previous entry in memory */
  {
    deUint32 hash = es1xHashShaderContextKey(key, table->capacity);

    es1xShaderProgramHashTableKeyValuePair* pair = table->hashTable[hash];
    es1xShaderProgramHashTableKeyValuePair* prev = 0;

    while(pair)
      {
        if (es1xShaderContextKeyEquals(key, pair->key))
          {
            es1xShaderProgramInfo* retVal = pair->value;

            /* Link previous entry to the next so that this
               pair drops out from the list */
            if (prev)
              prev->next = pair->next;
            else
              table->hashTable[hash] = pair->next;

            /* Insert key-value pair back to the allocation table */
            pair->key           = 0;
            pair->value         = 0;
            pair->next          = table->firstFree;
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

es1xShaderProgramInfo* es1xShaderProgramHashTable_get(es1xShaderProgramHashTable* table, es1xShaderContextKey* key)
{
  deUint32 hash = es1xHashShaderContextKey(key, table->capacity);

  es1xShaderProgramHashTableKeyValuePair* pair = table->hashTable[hash];

  while(pair)
    {
      if (es1xShaderContextKeyEquals(key, pair->key))
        return pair->value;

      pair = pair->next;
    }

  return 0;
}

/*---------------------------------------------------------------------------*/
