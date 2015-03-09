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


#ifndef _ES1XTEXTUREHASHTABLE_H
#define _ES1XTEXTUREHASHTABLE_H

#ifndef _ES1X_TEXTURE_H
#   include "es1xTexture.h"
#endif

#ifndef _ES1X_HASHTABLE_H
#include "es1xHashTable.h"
#endif

/*---------------------------------------------------------------------------*/

typedef struct
{
  void*           next;
  GLuint          key;
  es1xTexture2D*  value;
} es1xTextureHashTableKeyValuePair;

/*---------------------------------------------------------------------------*/

typedef struct
{
  es1xTextureHashTableKeyValuePair*       allocationTable;        /* allocation table     */
  es1xTextureHashTableKeyValuePair*       firstFree;              /* first free entry in the allocation table */
  es1xTextureHashTableKeyValuePair**      hashTable;              /* hash table */
  GLuint                                                          size;        /* number of elements stored in the table */
  GLuint                                                          capacity;    /* current maximum capacity */
} es1xTextureHashTable;

/*---------------------------------------------------------------------------*/

void                    es1xTextureHashTable_init(es1xTextureHashTable* table, deUint32 capacity);
es1xTextureHashTable*   es1xTextureHashTable_create(void);
void                    es1xTextureHashTable_resize(es1xTextureHashTable* table, deUint32 capacity);
void                    es1xTextureHashTable_uninit(es1xTextureHashTable* table);
void                    es1xTextureHashTable_destroy(es1xTextureHashTable* table);
void                    es1xTextureHashTable_insert(es1xTextureHashTable* table, GLuint key, es1xTexture2D* value);
es1xTexture2D*      es1xTextureHashTable_remove(es1xTextureHashTable* table, GLuint key);
es1xTexture2D*      es1xTextureHashTable_get(es1xTextureHashTable* table, GLuint key);

#endif /* _ES1XTEXTUREHASHTABLE_H */
