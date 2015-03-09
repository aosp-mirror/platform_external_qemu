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

 
#ifndef _ES1XSHADERPROGRAMHASHTABLE_H
#define _ES1XSHADERPROGRAMHASHTABLE_H

#ifndef _ES1XSHADERPROGRAMKEY_H
#   include "es1xShaderContextKey.h"
#endif

int es1xHashShaderContextKey(es1xShaderContextKey* key, deUint32 size);
deBool es1xShaderContextKeyEquals(es1xShaderContextKey* a, es1xShaderContextKey* b);  

/*---------------------------------------------------------------------------*/

#ifndef _ES1XHASHTABLE_H
#   include "es1xHashTable.h"
#endif

#ifndef _ES1XSHADERPROGRAMS_H 
#include "es1xShaderPrograms.h"
#endif

/*---------------------------------------------------------------------------*/

#define ES1X_SHADER_PROGRAM_HASH_TABLE_SIZE (ES1X_NUM_MAX_CACHED_SHADER_PROGRAMS / 2)

typedef struct
{
  void*                       next;
  es1xShaderContextKey*      key;
  es1xShaderProgramInfo*    value;
} es1xShaderProgramHashTableKeyValuePair;

/*---------------------------------------------------------------------------*/

typedef struct
{
  es1xShaderProgramHashTableKeyValuePair*   allocationTable;    /* allocation table */
  es1xShaderProgramHashTableKeyValuePair*   firstFree;          /* first free entry in the allocation table */
  es1xShaderProgramHashTableKeyValuePair**  hashTable;          /* hash table */
  GLuint                              size;               /* number of elements stored in the table */
  GLuint                              capacity;           /* current maximum capacity */
} es1xShaderProgramHashTable;

void                        es1xShaderProgramHashTable_init   (es1xShaderProgramHashTable* table, deUint32 capacity);
es1xShaderProgramHashTable* es1xShaderProgramHashTable_create (void);
void                        es1xShaderProgramHashTable_resize (es1xShaderProgramHashTable* table, deUint32 capacity);
void                        es1xShaderProgramHashTable_uninit (es1xShaderProgramHashTable* table);
void                        es1xShaderProgramHashTable_destroy(es1xShaderProgramHashTable* table);
void                        es1xShaderProgramHashTable_insert (es1xShaderProgramHashTable* table, es1xShaderContextKey* key, es1xShaderProgramInfo* value);
es1xShaderProgramInfo*    es1xShaderProgramHashTable_remove (es1xShaderProgramHashTable* table, es1xShaderContextKey* key);
es1xShaderProgramInfo*    es1xShaderProgramHashTable_get    (es1xShaderProgramHashTable* table, es1xShaderContextKey* key);

#endif /* _ES1XSHADERPROGRAMHASHTABLE_H */
