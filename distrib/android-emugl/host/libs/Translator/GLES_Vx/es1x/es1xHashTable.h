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

 
#ifndef _ES1XHASHTABLE_H
#define _ES1XHASHTABLE_H

#ifndef _ES1XDEFS_H
#	include "es1xDefs.h"
#endif

#ifndef _ES1XMEMORY_H
#	include "es1xMemory.h"
#endif

/*---------------------------------------------------------------------------*/

ES1X_INLINE deUint32 es1xHashu(deUint32 key, deUint32 size)
{
	ES1X_ASSERT((size & (size-1)) == 0);
	return key & (size-1);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE deBool es1xEqualsu(deUint32 a, deUint32 b)
{
	return a == b;	
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE deBool es1xEqualsp(void* a, void* b)
{
	return a == b;
}

/*---------------------------------------------------------------------------*/

#define ES1X_INITIAL_HASH_TABLE_SIZE 32

/* Size must be power of 2 */
DE_STATIC_ASSERT((ES1X_INITIAL_HASH_TABLE_SIZE & (ES1X_INITIAL_HASH_TABLE_SIZE-1)) == 0);

#define ES1X_DECLARE_HASH_TABLE(ES1X_TEMPLATE_NAME, ES1X_TEMPLATE_KEY_TYPE, ES1X_TEMPLATE_VALUE_TYPE, ES1X_HASH_FUNCTION, ES1X_COMPARE_FUNCTION)	\
																												\
																												\
/*---------------------------------------------------------------------------*/									\
																												\
typedef struct																									\
{																												\
	void*						next;																			\
	ES1X_TEMPLATE_KEY_TYPE		key;																			\
	ES1X_TEMPLATE_VALUE_TYPE	value;																			\
} ES1X_TEMPLATE_NAME##KeyValuePair;																				\
																												\
/*---------------------------------------------------------------------------*/									\
																												\
typedef struct																									\
{																												\
	ES1X_TEMPLATE_NAME##KeyValuePair*	allocationTable;	/* allocation table	*/								\
	ES1X_TEMPLATE_NAME##KeyValuePair*	firstFree;			/* first free entry in the allocation table */		\
	ES1X_TEMPLATE_NAME##KeyValuePair**	hashTable;			/* hash table */									\
	GLuint								size;				/* number of elements stored in the table */		\
	GLuint								capacity;			/* current maximum capacity */						\
} ES1X_TEMPLATE_NAME;																							\
																												\
ES1X_INLINE void						ES1X_TEMPLATE_NAME##_init	(ES1X_TEMPLATE_NAME* table, deUint32 capacity);												\
ES1X_INLINE ES1X_TEMPLATE_NAME*			ES1X_TEMPLATE_NAME##_create	(void);																						\
ES1X_INLINE void						ES1X_TEMPLATE_NAME##_resize	(ES1X_TEMPLATE_NAME* table, deUint32 capacity);												\
ES1X_INLINE void						ES1X_TEMPLATE_NAME##_uninit	(ES1X_TEMPLATE_NAME* table);																\
ES1X_INLINE void						ES1X_TEMPLATE_NAME##_destroy(ES1X_TEMPLATE_NAME* table);																\
ES1X_INLINE void						ES1X_TEMPLATE_NAME##_insert	(ES1X_TEMPLATE_NAME* table, ES1X_TEMPLATE_KEY_TYPE key, ES1X_TEMPLATE_VALUE_TYPE value);	\
ES1X_INLINE ES1X_TEMPLATE_VALUE_TYPE	ES1X_TEMPLATE_NAME##_remove	(ES1X_TEMPLATE_NAME* table, ES1X_TEMPLATE_KEY_TYPE key);									\
ES1X_INLINE ES1X_TEMPLATE_VALUE_TYPE	ES1X_TEMPLATE_NAME##_get	(ES1X_TEMPLATE_NAME* table, ES1X_TEMPLATE_KEY_TYPE key);									\
																												\
/*---------------------------------------------------------------------------*/									\
																												\
ES1X_INLINE void ES1X_TEMPLATE_NAME##_init(ES1X_TEMPLATE_NAME* table, deUint32 capacity)						\
{																												\
	deUint32 i;																									\
																												\
	ES1X_ASSERT(table);																							\
																												\
	table->capacity			= capacity;																			\
	table->allocationTable	= (ES1X_TEMPLATE_NAME##KeyValuePair*)es1xMalloc(sizeof(ES1X_TEMPLATE_NAME##KeyValuePair) * table->capacity);	\
	table->firstFree		= table->allocationTable;															\
																												\
	for(i=0;i<table->capacity;i++)																				\
	{																											\
		table->allocationTable[i].next	= &table->allocationTable[i+1];											\
		table->allocationTable[i].key	= 0;																	\
		table->allocationTable[i].value	= 0;																	\
	}																											\
	table->allocationTable[table->capacity-1].next = 0;															\
																												\
	table->hashTable = es1xMalloc(sizeof(ES1X_TEMPLATE_NAME##KeyValuePair**) * table->capacity);				\
	es1xMemZero(table->hashTable, sizeof(ES1X_TEMPLATE_NAME##KeyValuePair*) * table->capacity);					\
	table->size = 0;																							\
}																												\
																												\
/*---------------------------------------------------------------------------*/									\
																												\
ES1X_INLINE ES1X_TEMPLATE_NAME* ES1X_TEMPLATE_NAME##_create(void)												\
{																												\
	ES1X_TEMPLATE_NAME* hashTable = (ES1X_TEMPLATE_NAME*) es1xMalloc(sizeof(ES1X_TEMPLATE_NAME));				\
	ES1X_TEMPLATE_NAME##_init(hashTable, ES1X_INITIAL_HASH_TABLE_SIZE);											\
	return hashTable;																							\
}																												\
																												\
/*---------------------------------------------------------------------------*/									\
																												\
ES1X_INLINE void ES1X_TEMPLATE_NAME##_resize(ES1X_TEMPLATE_NAME* table, deUint32 capacity)						\
{																												\
	ES1X_TEMPLATE_NAME##KeyValuePair*	prevEntries		= table->allocationTable;								\
	ES1X_TEMPLATE_NAME##KeyValuePair**	prevTable		= table->hashTable;										\
	deUint32							prevCapacity	= table->capacity;										\
	deUint32							i;																		\
																												\
	ES1X_TEMPLATE_NAME##_init(table, capacity);																	\
																												\
	for(i=0;i<prevCapacity;i++)																					\
		ES1X_TEMPLATE_NAME##_insert(table, prevEntries[i].key, prevEntries[i].value);							\
																												\
	es1xFree(prevTable);																						\
	es1xFree(prevEntries);																						\
}																												\
																												\
/*---------------------------------------------------------------------------*/									\
																												\
ES1X_INLINE void ES1X_TEMPLATE_NAME##_uninit(ES1X_TEMPLATE_NAME* table)											\
{																												\
	es1xFree(table->hashTable);																					\
	es1xFree(table->allocationTable);																			\
}																												\
																												\
/*---------------------------------------------------------------------------*/									\
																												\
ES1X_INLINE void ES1X_TEMPLATE_NAME##_destroy(ES1X_TEMPLATE_NAME* table)										\
{																												\
	ES1X_TEMPLATE_NAME##_uninit(table);																			\
	es1xFree(table);																							\
}																												\
																												\
/*---------------------------------------------------------------------------*/									\
																												\
ES1X_INLINE void ES1X_TEMPLATE_NAME##_insert(ES1X_TEMPLATE_NAME* table, ES1X_TEMPLATE_KEY_TYPE key, ES1X_TEMPLATE_VALUE_TYPE value)	\
{																												\
	ES1X_ASSERT(table);																							\
																												\
	if (table->firstFree == 0)																					\
		ES1X_TEMPLATE_NAME##_resize(table, table->capacity * 2);												\
																												\
	/* Calculate hash value */																					\
	{																											\
		deUint32 hash = ES1X_HASH_FUNCTION(key, table->capacity);												\
																												\
		/* Allocate key-value pair */																			\
		ES1X_TEMPLATE_NAME##KeyValuePair* pair = table->firstFree;												\
		table->firstFree = pair->next;																			\
																												\
		/* Insert key-value pair to the hash table */															\
		pair->key				= key;																			\
		pair->value				= value;																		\
		pair->next				= table->hashTable[hash];														\
		table->hashTable[hash]	= pair;																			\
		table->size				= table->size + 1;																\
	}																											\
}																												\
																												\
/*---------------------------------------------------------------------------*/									\
																												\
ES1X_INLINE ES1X_TEMPLATE_VALUE_TYPE ES1X_TEMPLATE_NAME##_remove(ES1X_TEMPLATE_NAME* table, ES1X_TEMPLATE_KEY_TYPE key)	\
{																												\
	ES1X_ASSERT(table);																							\
																												\
	/*	Find corresponding key-value pair by iterating through the list of pairs. 								\
		Also keep previous entry in memory */																	\
	{																											\
		deUint32 hash = ES1X_HASH_FUNCTION(key, table->capacity);																\
																												\
		ES1X_TEMPLATE_NAME##KeyValuePair* pair = table->hashTable[hash];										\
		ES1X_TEMPLATE_NAME##KeyValuePair* prev = 0;																\
																												\
		while(pair)																								\
		{																										\
			if (ES1X_COMPARE_FUNCTION(key, pair->key))															\
			{																									\
				ES1X_TEMPLATE_VALUE_TYPE retVal = pair->value;													\
																												\
				/* Link previous entry to the next so that this 												\
				   pair drops out from the list */																\
				if (prev)																						\
					prev->next = pair->next;																	\
				else																							\
					table->hashTable[hash] = pair->next;														\
																												\
				/* Insert key-value pair back to the allocation table */										\
				pair->key			= 0;																		\
				pair->value			= 0;																		\
				pair->next			= table->firstFree;															\
				table->firstFree	= pair; 																	\
																												\
				return retVal;																					\
			}																									\
																												\
			prev = pair;																						\
			pair = pair->next; 																					\
		}																										\
	}																											\
																												\
	/* A program matching the key was not found */																\
	return 0;																									\
}																												\
																												\
/*---------------------------------------------------------------------------*/									\
																												\
ES1X_INLINE ES1X_TEMPLATE_VALUE_TYPE ES1X_TEMPLATE_NAME##_get(ES1X_TEMPLATE_NAME* table, ES1X_TEMPLATE_KEY_TYPE key)	\
{																												\
	deUint32 hash = ES1X_HASH_FUNCTION(key, table->capacity);													\
																												\
	ES1X_TEMPLATE_NAME##KeyValuePair* pair = table->hashTable[hash];											\
																												\
	while(pair)																									\
	{																											\
		if (ES1X_COMPARE_FUNCTION(key, pair->key))																\
			return pair->value;																					\
																												\
		pair = pair->next; 																						\
	}																											\
																												\
	return 0;																									\
}																												\
																												\
/*---------------------------------------------------------------------------*/									\
																												\

#endif 																												
																												
																												
