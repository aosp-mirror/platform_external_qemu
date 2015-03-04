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

 

#include "es1xTypeConverter.h"
#include "es1xStateQueryLookup.h"
#include "es1xMath.h"
#include "es1xMemory.h"

/* Type conversion functions */
typedef void (*es1xGenericTypeConversionFunction)(void*, const void*, int);
typedef void (*es1xEnumTypeConversionFunction)(void*, const void*, int, es1xQueryResultMapFunction);

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_itof(void* dst, const void* src, int numElements)
{
	int				i;
	const GLint*	srcPtr = (const GLint*)src;
	GLfloat*		dstPtr = (GLfloat*)dst;

	for(i=0;i<numElements;i++)
		*dstPtr++ = (GLfloat)*srcPtr++;
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_itox(void* dst, const void* src, int numElements)
{
	int				i;
	const GLint*	srcPtr = (const GLint*)src;
	GLfixed*		dstPtr = (GLfixed*)dst;

	for(i=0;i<numElements;i++)
	{
		int value = *srcPtr++;

		value = es1xMath_mini(value, 32767);
		value = es1xMath_maxi(value, -32768);

		*dstPtr++ = value << 16;
	}
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_itob(void* dst, const void* src, int numElements)
{
	int				i;
	const GLint*	srcPtr = (const GLint*)src;
	GLboolean*		dstPtr = (GLboolean*)dst;

	for(i=0;i<numElements;i++,srcPtr++,dstPtr++)
		*dstPtr = (GLboolean)!!(*srcPtr);
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_ftoi(void* dst, const void* src, int numElements)
{
	int				i;
	const GLfloat*	srcPtr = (const GLfloat*)src;
	GLint*			dstPtr = (GLint*)dst;

	for(i=0;i<numElements;i++,srcPtr++,dstPtr++)
		*dstPtr = (GLint)*srcPtr;
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_ctoi(void* dst, const void* src, int numElements)
{
	int				i;
	const GLfloat*	srcPtr = (const GLfloat*)src;
	GLint*			dstPtr = (GLint*)dst;

	/* Special conversion method for color components, 
	   depth range values, depth clear value, normal 
	   coordinates, alpha test reference value etc.. */
	for(i=0;i<numElements;i++,srcPtr++,dstPtr++)
		*dstPtr = (GLint)(*srcPtr * 0x7FFFFFFF);
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_ftox(void* dst, const void* src, int numElements)
{
	int				i;
	const GLfloat*	srcPtr = (const GLfloat*)src;
	GLfixed*		dstPtr = (GLfixed*)dst;

	for(i=0;i<numElements;i++,srcPtr++,dstPtr++)
		*dstPtr = (GLint)((*srcPtr) * 65536.f + 0.5f);
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_ftob(void* dst, const void* src, int numElements)
{
	int				i;
	const GLfloat*	srcPtr = (const GLfloat*)src;
	GLfixed*		dstPtr = (GLfixed*)dst;

	for(i=0;i<numElements;i++)
		*dstPtr = (GLboolean)!!((GLint)*srcPtr);
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_xtoi(void* dst, const void* src, int numElements)
{
	int				i;
	const GLfixed*	srcPtr = (const GLfixed*)src;
	GLint*			dstPtr = (GLint*)dst;

	for(i=0;i<numElements;i++)
		*dstPtr = (*srcPtr) >> 16;
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_xtof(void* dst, const void* src, int numElements)
{
	int				i;
	const GLfixed*	srcPtr = (const GLfixed*)src;
	GLfloat*		dstPtr = (GLfloat*)dst;

	for(i=0;i<numElements;i++)
		*dstPtr = (GLfloat)((*srcPtr) >> 16);
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_xtob(void* dst, const void* src, int numElements)
{
	int				i;
	const GLfixed*	srcPtr = (const GLfixed*)src;
	GLboolean*		dstPtr = (GLboolean*)dst;

	for(i=0;i<numElements;i++)
		*dstPtr = (GLboolean)(!!(*srcPtr));
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_btoi(void* dst, const void* src, int numElements)
{
	int					i;
	const GLboolean*	srcPtr = (const GLboolean*)src;
	GLint*				dstPtr = (GLint*)dst;

	for(i=0;i<numElements;i++)
		*dstPtr = ((GLint)*srcPtr) != 0 ? 1 : 0;
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_btof(void* dst, const void* src, int numElements)
{
	int			i;
	GLboolean*	srcPtr = (GLboolean*)src;
	GLfloat*	dstPtr = (GLfloat*)dst;

	for(i=0;i<numElements;i++)
		*dstPtr = ((GLfloat)*srcPtr) != 0.f ? 1.f : 0.f;
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_btox(void* dst, const void* src, int numElements)
{
	int			i;
	GLboolean*	srcPtr = (GLboolean*)src;
	GLint*		dstPtr = (GLint*)dst;

	for(i=0;i<numElements;i++)
		*dstPtr = ((GLint)*srcPtr) << 16;
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_etoi(void* dst, const void* src, int numElements, es1xQueryResultMapFunction mapFunction)
{
	int				i;
	const GLint*	srcPtr = (const GLint*)src;
	GLint*			dstPtr = (GLint*)dst;

	for(i=0;i<numElements;i++,srcPtr++,dstPtr++)
		*dstPtr = (GLint)mapFunction(*srcPtr);
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_etof(void* dst, const void* src, int numElements, es1xQueryResultMapFunction mapFunction)
{
	int				i;
	const GLint*	srcPtr = (const GLint*)src;
	GLfloat*		dstPtr = (GLfloat*)dst;

	for(i=0;i<numElements;i++,srcPtr++,dstPtr++)
		*dstPtr = (GLfloat)mapFunction(*srcPtr);
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_etox(void* dst, const void* src, int numElements, es1xQueryResultMapFunction mapFunction)
{
	int				i;
	const GLint*	srcPtr = (const GLint*)src;
	GLfixed*		dstPtr = (GLfixed*)dst;

	for(i=0;i<numElements;i++)
	{
		int value = mapFunction(*srcPtr++);

		value = es1xMath_mini(value, 32767);
		value = es1xMath_maxi(value, -32768);

		*dstPtr++ = value << 16;
	}
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_etob(void* dst, const void* src, int numElements, es1xQueryResultMapFunction mapFunction)
{
	int				i;
	const GLint*	srcPtr = (const GLint*)src;
	GLboolean*		dstPtr = (GLboolean*)dst;

	for(i=0;i<numElements;i++,srcPtr++,dstPtr++)
		*dstPtr = (GLboolean)!!mapFunction(*srcPtr);
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_etoe(void* dst, const void* src, int numElements, es1xQueryResultMapFunction mapFunction)
{
	int				i;
	const GLint*	srcPtr = (const GLint*)src;
	GLenum*			dstPtr = (GLenum*)dst;

	for(i=0;i<numElements;i++,srcPtr++,dstPtr++)
		*dstPtr = mapFunction(*srcPtr);
}

/*---------------------------------------------------------------------------*/

void es1xTypeConverter_convertEntry(const es1xStateQueryEntry* entry, void* dst, es1xStateQueryParamType dstType, const void* src, es1xStateQueryParamType srcType, int numElements)
{
	ES1X_ASSERT(entry);
	
	if (srcType == PARAM_TYPE_INTERNAL_ENUM)
	{
		static const es1xEnumTypeConversionFunction conversionFunction[NUM_PARAM_TYPES] = 
		{ 
			es1xTypeConverter_etoi,	
			es1xTypeConverter_etof,	
			es1xTypeConverter_etox,	
			es1xTypeConverter_etob,	
			es1xTypeConverter_etoe,	
			ES1X_NULL,	
			ES1X_NULL 
		};

		es1xEnumTypeConversionFunction function = conversionFunction[dstType];

		ES1X_ASSERT(function);
		ES1X_ASSERT(entry->mapFunction);

		function(dst, src, numElements, entry->mapFunction);
	}
	else
		es1xTypeConverter_convert(dst, dstType, src, srcType, numElements);
}

void es1xTypeConverter_convert(void* dst, es1xStateQueryParamType dstType, const void* src, es1xStateQueryParamType srcType, int numElements)
{
	static const es1xGenericTypeConversionFunction conversionFunction[NUM_PARAM_TYPES][NUM_PARAM_TYPES] = 
	{
		{ ES1X_NULL,				es1xTypeConverter_itof,	es1xTypeConverter_itox,	es1xTypeConverter_itob,	ES1X_NULL,					ES1X_NULL },
		{ es1xTypeConverter_ftoi,	ES1X_NULL,				es1xTypeConverter_ftox,	es1xTypeConverter_ftob,	es1xTypeConverter_ftoi,		ES1X_NULL },
		{ es1xTypeConverter_xtoi,	es1xTypeConverter_xtof,	ES1X_NULL,				es1xTypeConverter_xtob,	ES1X_NULL,					ES1X_NULL },
		{ es1xTypeConverter_btoi,	es1xTypeConverter_btof,	es1xTypeConverter_btox,	ES1X_NULL,				es1xTypeConverter_btoi,		ES1X_NULL },
		{ ES1X_NULL,				es1xTypeConverter_itof,	ES1X_NULL,				es1xTypeConverter_itob,	ES1X_NULL,					ES1X_NULL },
		{ ES1X_NULL,				ES1X_NULL,				ES1X_NULL,				ES1X_NULL,				ES1X_NULL,					ES1X_NULL },
		{ es1xTypeConverter_ctoi,	ES1X_NULL,				es1xTypeConverter_ftox,	es1xTypeConverter_ftob,	es1xTypeConverter_ftoi,		ES1X_NULL },
	};

	/* value conversion methods lookup */
	ES1X_ASSERT(src);
	ES1X_ASSERT(dst);
	ES1X_ASSERT(dstType >= 0 && dstType < NUM_PARAM_TYPES);
	ES1X_ASSERT(srcType >= 0 && srcType < NUM_PARAM_TYPES);

	/* Convert */
	{
		es1xGenericTypeConversionFunction function = conversionFunction[srcType][dstType];

		if (function)
		{
			/* Perform conversion between from source type to destination type */
			function(dst, src, numElements);
		}
		else	
		{
			static const GLint s_dataTypeSizeLookup[NUM_PARAM_TYPES] = 
			{
				sizeof(GLint),		
				sizeof(GLfloat),	
				sizeof(GLfixed),	
				sizeof(GLboolean),
				sizeof(GLenum),
				sizeof(GLint),
				sizeof(GLclampf)
			};
			
			ES1X_ASSERT(s_dataTypeSizeLookup[PARAM_TYPE_INTEGER]		== sizeof(GLint));
			ES1X_ASSERT(s_dataTypeSizeLookup[PARAM_TYPE_FLOAT]			== sizeof(GLfloat));
			ES1X_ASSERT(s_dataTypeSizeLookup[PARAM_TYPE_FIXED]			== sizeof(GLfixed));
			ES1X_ASSERT(s_dataTypeSizeLookup[PARAM_TYPE_BOOLEAN]		== sizeof(GLboolean));
			ES1X_ASSERT(s_dataTypeSizeLookup[PARAM_TYPE_GL_ENUM]		== sizeof(GLenum));
			ES1X_ASSERT(s_dataTypeSizeLookup[PARAM_TYPE_INTERNAL_ENUM]	== sizeof(GLint));
			ES1X_ASSERT(s_dataTypeSizeLookup[PARAM_TYPE_CLAMPF]			== sizeof(GLclampf));

			/* perform direct memcopy */
			ES1X_ASSERT(s_dataTypeSizeLookup[srcType] == s_dataTypeSizeLookup[dstType]);
			es1xMemCopy((deInt8*)dst, (const void*)src, s_dataTypeSizeLookup[dstType] * numElements); 
		} 	
	}
}
