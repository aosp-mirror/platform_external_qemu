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

 
#ifndef _ES1XSHADERCONTEXTKEY_H
#define _ES1XSHADERCONTEXTKEY_H

#ifndef _ES1XDEFS_H
#	include "es1xDefs.h"
#endif

#define ES1X_SHADER_CONTEXT_HASH_KEY_BUFFER_SIZE (ES1X_SHADER_CONTEXT_HASH_KEY_BITS / 32 + 1)

typedef struct 
{
	deUint32	data[ES1X_SHADER_CONTEXT_HASH_KEY_BUFFER_SIZE];	
	deUint32	curIndex;					/* current element in array */
	deUint32	curLeft;					/* bits left in current element */
	ES1X_DEBUG_CODE(GLuint totalUsed;)		/* used bits */
} es1xShaderContextKey;

/* Helper struct for pushing multiple 
   elements with one function call */
typedef struct
{
	deUint32	bits;
	deUint32	numBits;
} es1xShaderContextKeyEntry;

#ifdef __cplusplus
extern "C"
{
#endif

/* BitArray API */
void es1xShaderContextKey_init		(es1xShaderContextKey* array);
void es1xShaderContextKey_destroy	(es1xShaderContextKey* array);
void es1xShaderContextKey_push		(es1xShaderContextKey* dst, deUint32 data, deUint32 numBits);
void es1xShaderContextKey_pushArray	(es1xShaderContextKey* array, const es1xShaderContextKeyEntry* entries, deUint32 numEntries);
void es1xShaderContextKey_set		(es1xShaderContextKey* array, deUint32* data, deUint32 numEntries);

ES1X_INLINE GLboolean es1xShaderContextKey_equals(es1xShaderContextKey* key0, es1xShaderContextKey* key1)
{
	ES1X_ASSERT(key0);
	ES1X_ASSERT(key1);
	ES1X_ASSERT(key0->curIndex == key1->curIndex);
	ES1X_ASSERT(key0->curLeft == key1->curLeft);

	{
		int i;
	
		deUint32* data0 = key0->data;
		deUint32* data1 = key1->data;
			
		for(i=0;i<ES1X_SHADER_CONTEXT_HASH_KEY_BUFFER_SIZE;i++)
			if (*data0++ != *data1++)
				return GL_FALSE;
		
		return GL_TRUE;
	}
} 

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _ES1XBITARRAY_H */
