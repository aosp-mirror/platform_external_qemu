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

 
#include "es1xShaderContextKey.h"
#include "es1xMemory.h"

/*---------------------------------------------------------------------------*/

void es1xShaderContextKey_init(es1xShaderContextKey* array)
{
	ES1X_ASSERT(array);

	array->curIndex = 0;
	array->curLeft	= 32;
	
	es1xMemZero(array->data, sizeof(deUint32) * ES1X_SHADER_CONTEXT_HASH_KEY_BUFFER_SIZE); 

	ES1X_DEBUG_CODE(array->totalUsed	 = 0);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xShaderContextKey_pushInternal(es1xShaderContextKey* dst, deInt32 bits, deUint32 numBits)
{
	#define NUM_BITS_PER_ELEMENT (sizeof(GLint) * 8)

	ES1X_ASSERT(dst);
	ES1X_ASSERT(numBits <= NUM_BITS_PER_ELEMENT);
	ES1X_DEBUG_CODE(ES1X_ASSERT(dst->totalUsed + numBits <= ES1X_SHADER_CONTEXT_HASH_KEY_BITS));
	
	/* mask data */
	bits &= (1 << numBits) - 1;
	
	if (dst->curLeft > numBits)
	{
		dst->data[dst->curIndex] |= bits << (NUM_BITS_PER_ELEMENT - dst->curLeft);
		dst->curLeft -= numBits;
		ES1X_ASSERT(dst->curLeft > 0);
	}
	else if (dst->curLeft < numBits)
	{
		dst->data[dst->curIndex] |= bits << (NUM_BITS_PER_ELEMENT - dst->curLeft);
		dst->curIndex ++;
		dst->data[dst->curIndex] = bits >> dst->curLeft;
		dst->curLeft = NUM_BITS_PER_ELEMENT - dst->curLeft;
	}
	else /* curLeft == numBits */
	{
		dst->data[dst->curIndex] |= bits << (NUM_BITS_PER_ELEMENT - dst->curLeft);
		dst->curIndex	+= 1;
		dst->curLeft	= NUM_BITS_PER_ELEMENT;	
	}

	ES1X_ASSERT(dst->curLeft > 0);
	ES1X_DEBUG_CODE(dst->totalUsed += numBits);

	#undef NUM_BITS_PER_ELEMENT 
}

/*---------------------------------------------------------------------------*/

void es1xShaderContextKey_push(es1xShaderContextKey* dst, deUint32 bits, deUint32 numBits)
{
	es1xShaderContextKey_pushInternal(dst, bits, numBits);
}

/*---------------------------------------------------------------------------*/

void es1xShaderContextKey_set(es1xShaderContextKey* array, deUint32* data, deUint32 numEntries)
{
	es1xMemCopy(array->data, data, sizeof(deUint32) * numEntries);
	array->curLeft = 0;
	array->curIndex = numEntries;
}

/*---------------------------------------------------------------------------*/

void es1xShaderContextKey_pushArray(es1xShaderContextKey* dst, const es1xShaderContextKeyEntry* entries, deUint32 numEntries)
{
	deUint32 i;
	
	ES1X_ASSERT(dst);
	ES1X_ASSERT(entries);
	
	for(i=0;i<numEntries;i++)
	{
		es1xShaderContextKey_pushInternal(dst, entries->bits, entries->numBits);
		entries += 1;
	}
} 
