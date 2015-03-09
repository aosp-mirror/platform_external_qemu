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

 
#ifndef _ES1XPOOLALLOCATOR_H
#define _ES1XPOOLALLOCATOR_H

#ifndef _ES1XDEFS_H
#	include "es1xDefs.h"
#endif

#define ES1X_DECLARE_POOL_ALLOCATOR(ES1X_TEMPLATE_NAME, ES1X_TEMPLATE_TYPENAME)				\
																							\
DE_HEADER_STATIC_ASSERT(ES1X_TEMPLATE_NAME, sizeof(ES1X_TEMPLATE_TYPENAME) >= 4);	/* Must be able to store one ptr */	\
																							\
/*-----------------------------------------------------------------------*/					\
																							\
typedef struct																				\
{																							\
	ES1X_TEMPLATE_TYPENAME*	objects;														\
	ES1X_TEMPLATE_TYPENAME* first;															\
	deUint32				size;															\
} ES1X_TEMPLATE_NAME;																		\
																							\
/*-----------------------------------------------------------------------*/					\
																							\
ES1X_INLINE ES1X_TEMPLATE_NAME* ES1X_TEMPLATE_NAME##_create(deUint32 size)					\
{																							\
	deUint32 i;																							\
	ES1X_TEMPLATE_NAME* allocator = (ES1X_TEMPLATE_NAME*) es1xMalloc(sizeof(ES1X_TEMPLATE_NAME));		\
																										\
	allocator->objects	= (ES1X_TEMPLATE_TYPENAME*) es1xMalloc(sizeof(ES1X_TEMPLATE_TYPENAME) * size);	\
	allocator->first	= allocator->objects;												\
	allocator->size		= size;																\
																							\
	for(i=0;i<size-1;i++)																	\
		*(ES1X_TEMPLATE_TYPENAME**)(&allocator->objects[i]) = &(allocator->objects[i+1]);	\
																							\
	*(ES1X_TEMPLATE_TYPENAME**)(&allocator->objects[size-1]) = 0;							\
	return allocator;																		\
}																							\
																							\
/*-----------------------------------------------------------------------*/					\
																							\
ES1X_INLINE void ES1X_TEMPLATE_NAME##_destroy(ES1X_TEMPLATE_NAME* allocator)				\
{																							\
	ES1X_ASSERT(allocator);																	\
	ES1X_ASSERT(allocator->objects);														\
																							\
	es1xFree(allocator->objects);															\
	es1xFree(allocator);																	\
}																							\
																							\
/*-----------------------------------------------------------------------*/					\
																							\
ES1X_INLINE ES1X_TEMPLATE_TYPENAME*	ES1X_TEMPLATE_NAME##_allocate(ES1X_TEMPLATE_NAME* allocator)	\
{																							\
	ES1X_ASSERT(allocator);																	\
																							\
	/* Return first free object */															\
	{																						\
		ES1X_TEMPLATE_TYPENAME* retVal = allocator->first;									\
																							\
		if (allocator->first)																\
			allocator->first =  *((ES1X_TEMPLATE_TYPENAME**)allocator->first);				\
																							\
		return retVal;																		\
	}																						\
}																							\
																							\
/*-----------------------------------------------------------------------*/					\
																							\
ES1X_INLINE void ES1X_TEMPLATE_NAME##_release(ES1X_TEMPLATE_NAME* allocator, ES1X_TEMPLATE_TYPENAME* object) \
{																							\
	ES1X_ASSERT(allocator);																	\
	ES1X_ASSERT(object);																	\
																							\
	*(ES1X_TEMPLATE_TYPENAME**)object = allocator->first;									\
	allocator->first = object;																\
}																							\
																							\
/*-----------------------------------------------------------------------*/					\
																							\
ES1X_INLINE GLboolean ES1X_TEMPLATE_NAME##_contains(ES1X_TEMPLATE_NAME* allocator, ES1X_TEMPLATE_TYPENAME* object)	\
{																							\
	ES1X_ASSERT(allocator);																	\
																							\
	if (object >= allocator->objects && object <= allocator->objects + allocator->size - 1)	\
		return GL_TRUE;																		\
																							\
	return GL_FALSE;																		\
}																							
																							
/*-----------------------------------------------------------------------*/					

#endif /* es1xPoolAllocator.h */
