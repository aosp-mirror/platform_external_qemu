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

 
#include "es1xShaderCache.h"
#include "es1xShaderContext.h"
#include "es1xMemory.h"

/**
   #include <stdio.h>
   #define ES1X_SHADER_PRIORITY_LOG(P) printf P
   /*/
#define ES1X_SHADER_PRIORITY_LOG(P)
/**/

/*-------------------------------------------------------------------------*/

ES1X_INLINE void es1xTestSelfIntegrity(es1xShaderCache* cache)
{
  /**/
  ES1X_UNREF(cache);
  /*
    if (cache->highestPriorityEntry)
    {
    es1xShaderPriorityEntry* current = cache->highestPriorityEntry;

    ES1X_ASSERT(cache->lowestPriorityEntry);

    while(current)
    {
    ES1X_ASSERT((current->prev == 0 && current == cache->lowestPriorityEntry)  || current->prev->next == current);
    ES1X_ASSERT((current->next == 0 && current == cache->highestPriorityEntry) || current->next->prev == current);
    ES1X_SHADER_PRIORITY_LOG((" -> 0x%.8X", current));
    current = current->prev;
    }

    ES1X_SHADER_PRIORITY_LOG(("\n"));
    }
    else
    {
    ES1X_ASSERT(cache->lowestPriorityEntry == 0);
    }
    */
}

/*-------------------------------------------------------------------------*/

es1xShaderCache* es1xShaderCache_create()
{
  es1xShaderCache* cache = (es1xShaderCache*) es1xMalloc( sizeof(es1xShaderCache) );
  es1xShaderProgramHashTable_init(&cache->programs, ES1X_SHADER_PROGRAM_HASH_TABLE_SIZE);

  cache->keyAllocator                           = es1xShaderContextKeyAllocator_create(ES1X_NUM_MAX_CACHED_SHADER_PROGRAMS);
  cache->programInfoAllocator           = es1xShaderProgramInfoAllocator_create(ES1X_NUM_MAX_CACHED_SHADER_PROGRAMS);
  cache->priorityEntryAllocator = es1xShaderPriorityEntryAllocator_create(ES1X_NUM_MAX_CACHED_SHADER_PROGRAMS);

  cache->highestPriorityEntry   = 0;
  cache->lowestPriorityEntry    = 0;

  es1xTestSelfIntegrity(cache);

  return cache;
}

/*-------------------------------------------------------------------------*/

void es1xShaderCache_destroy(es1xShaderCache* cache)
{
  es1xShaderProgramHashTable_uninit(&cache->programs);
  es1xShaderContextKeyAllocator_destroy(cache->keyAllocator);
  es1xShaderProgramInfoAllocator_destroy(cache->programInfoAllocator);
  es1xShaderPriorityEntryAllocator_destroy(cache->priorityEntryAllocator);

  es1xFree(cache);
}

/*-------------------------------------------------------------------------*/

es1xShaderContextKey* es1xShaderCache_allocateContextKey(es1xShaderCache* cache)
{
  ES1X_ASSERT(cache);
  ES1X_ASSERT(cache->keyAllocator);

  /* Allocate */
  {
    es1xShaderContextKey* key = es1xShaderContextKeyAllocator_allocate(cache->keyAllocator);

    if (key)
      es1xShaderContextKey_init(key);

    return key;
  }
}

/*-------------------------------------------------------------------------*/

void es1xShaderCache_releaseContextKey(es1xShaderCache* cache, es1xShaderContextKey* key)
{
  ES1X_ASSERT(cache);
  ES1X_ASSERT(cache->keyAllocator);
  ES1X_ASSERT(key);

  es1xShaderContextKeyAllocator_release(cache->keyAllocator, key);
}

es1xShaderContext* es1xShaderCache_allocateContext(es1xShaderCache* cache)
{
  return &cache->shaderContext;
}

void es1xShaderCache_releaseContext(es1xShaderCache* cache, es1xShaderContext* context)
{
  ES1X_UNREF(cache);
  ES1X_UNREF(context);
}


/*-------------------------------------------------------------------------*/

es1xShaderProgramInfo* es1xShaderCache_allocateProgramInfo(es1xShaderCache* cache)
{
  ES1X_ASSERT(cache);
  ES1X_ASSERT(cache->programInfoAllocator);
  ES1X_ASSERT(cache->priorityEntryAllocator);

  /* Allocate */
  {
    es1xShaderProgramInfo* info = es1xShaderProgramInfoAllocator_allocate(cache->programInfoAllocator);
    ES1X_ASSERT(info);
    es1xShaderProgramInfo_init(info);
    return info;
  }
}

/*-------------------------------------------------------------------------*/

void es1xShaderCache_releaseProgramInfo(es1xShaderCache* cache, es1xShaderProgramInfo* info)
{
  ES1X_ASSERT(cache);
  ES1X_ASSERT(cache->programInfoAllocator);
  ES1X_ASSERT(info);

  es1xShaderProgramInfoAllocator_release(cache->programInfoAllocator, info);
}

/*-------------------------------------------------------------------------*/

void es1xShaderCache_registerProgram(es1xShaderCache* cache, es1xShaderContextKey* key, es1xShaderProgramInfo* value)
{
  es1xShaderPriorityEntry* priorityEntry = es1xShaderPriorityEntryAllocator_allocate(cache->priorityEntryAllocator);
  ES1X_ASSERT(value->priorityData == 0);

  es1xTestSelfIntegrity(cache);

  ES1X_ASSERT(priorityEntry);
  value->priorityData = priorityEntry;

  ES1X_SHADER_PRIORITY_LOG(("es1xShaderCache: registered new program: 0x%.8X\n", value->priorityData));

  /* First inserted entry? */
  if (cache->lowestPriorityEntry == 0)
    cache->lowestPriorityEntry = priorityEntry;
  else
    cache->highestPriorityEntry->next = priorityEntry;

  /* This program now has top priority */
  priorityEntry->prev                   = cache->highestPriorityEntry;
  priorityEntry->next                   = 0;
  priorityEntry->key                    = key;

  cache->highestPriorityEntry   = priorityEntry;
  es1xTestSelfIntegrity(cache);
  es1xShaderProgramHashTable_insert(&cache->programs, key, value);
}

/*-------------------------------------------------------------------------*/

void es1xShaderCache_adjustProgramPriority(es1xShaderCache* cache, es1xShaderProgramInfo* program)
{
  es1xShaderPriorityEntry* current = (es1xShaderPriorityEntry*) program->priorityData;
  ES1X_ASSERT(current);

  if (current->next)
  {
    es1xShaderPriorityEntry* next = current->next;
    es1xShaderPriorityEntry* prev = current->prev;

    if (next->next)
      next->next->prev = current;
    else
      cache->highestPriorityEntry = current;

    if (prev)
      prev->next = next;

    current->next               = next->next;
    current->prev               = next;
    next->prev                  = prev;
    next->next                  = current;

    if (cache->lowestPriorityEntry == current)
      cache->lowestPriorityEntry = next;
  }
}

/*-------------------------------------------------------------------------*/

es1xShaderProgramInfo* es1xShaderCache_useProgram(es1xShaderCache* cache, es1xShaderContextKey* key)
{
  es1xShaderProgramInfo* info = es1xShaderProgramHashTable_get(&(cache->programs), key);

  if (info)
  {
    es1xTestSelfIntegrity(cache);
    ES1X_SHADER_PRIORITY_LOG(("es1xShaderCache: Adjusting program priority: 0x%.8X\n", info->priorityData));
    es1xShaderCache_adjustProgramPriority(cache, info);

    es1xTestSelfIntegrity(cache);
  }

  return info;
}

/*-------------------------------------------------------------------------*/

es1xShaderContextKey* es1xShaderCache_unregisterMostRarelyUsedProgram(es1xShaderCache* cache, es1xShaderProgramInfo** info)
{
  ES1X_ASSERT(cache->lowestPriorityEntry);
  {
    es1xShaderPriorityEntry*    priorityEntry   = cache->lowestPriorityEntry;
    es1xShaderContextKey*               key                             = cache->lowestPriorityEntry->key;

    es1xTestSelfIntegrity(cache);

    *info = es1xShaderProgramHashTable_remove(&cache->programs, key);
    es1xShaderContextKey_init(cache->lowestPriorityEntry->key);

    ES1X_SHADER_PRIORITY_LOG(("es1xShaderCache: removed most rarely used program: 0x%.8X\n", (*info)->priorityData));

    cache->lowestPriorityEntry                  = priorityEntry->next;
    cache->lowestPriorityEntry->prev    = 0;
    es1xShaderPriorityEntryAllocator_release(cache->priorityEntryAllocator, priorityEntry);

    es1xTestSelfIntegrity(cache);
    return key;
  }
}

/*-------------------------------------------------------------------------*/
