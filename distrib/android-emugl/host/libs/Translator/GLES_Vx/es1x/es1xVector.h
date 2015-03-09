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

 
#ifndef _ES1XVECTOR_H
#define _ES1XVECTOR_H

#ifndef _ES1XDEFS_H
#	include "es1xDefs.h"
#endif

#ifndef _ES1XMATH_H
#	include "es1xMath.h"
#endif

typedef struct 
{
	GLfloat x;
	GLfloat y;
} es1xVec2;

typedef struct 
{
	GLfloat x;
	GLfloat y;
	GLfloat z;
} es1xVec3;

typedef struct 
{
	GLfloat x;
	GLfloat y;
	GLfloat z;
	GLfloat w;
} es1xVec4;

#ifdef __cplusplus
extern "C"
{
#endif

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xVec2_set(es1xVec2* dst, GLfloat x, GLfloat y)
{
	ES1X_ASSERT(dst);
	
	dst->x = x;
	dst->y = y;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xVec2_setVec2(es1xVec2* dst, const es1xVec2* src)
{
	ES1X_ASSERT(src);
	es1xVec2_set(dst, src->x, src->y);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xVec2_setVec3(es1xVec2* dst, const es1xVec3* src)
{
	ES1X_ASSERT(src);
	es1xVec2_set(dst, src->x, src->y);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xVec2_setVec4(es1xVec2* dst, const es1xVec4* src)
{
	ES1X_ASSERT(src);
	es1xVec2_set(dst, src->x, src->y);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xVec3_set(es1xVec3* dst, GLfloat x, GLfloat y, GLfloat z)
{
	ES1X_ASSERT(dst);
	
	dst->x = x;
	dst->y = y;
	dst->z = z;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xVec3_setVec2(es1xVec3* dst, const es1xVec2* src)
{
	ES1X_ASSERT(src);
	es1xVec3_set(dst, src->x, src->y, 0.f);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xVec3_setVec3(es1xVec3* dst, const es1xVec3* src)
{
	ES1X_ASSERT(src);
	es1xVec3_set(dst, src->x, src->y, src->z);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xVec3_setVec4(es1xVec3* dst, const es1xVec4* src)
{
	ES1X_ASSERT(src);
	es1xVec3_set(dst, src->x, src->y, src->z);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xVec4_set(es1xVec4* dst, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	ES1X_ASSERT(dst);
	
	dst->x = x;
	dst->y = y;
	dst->z = z;
	dst->w = w;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xVec4_setVec2(es1xVec4* dst, const es1xVec2* src)
{
	ES1X_ASSERT(src);
	es1xVec4_set(dst, src->x, src->y, 0.f, 0.f);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xVec4_setVec3(es1xVec4* dst, const es1xVec3* src)
{
	ES1X_ASSERT(src);
	es1xVec4_set(dst, src->x, src->y, src->z, 0.f);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xVec4_setVec4(es1xVec4* dst, const es1xVec4* src)
{
	ES1X_ASSERT(src);
	es1xVec4_set(dst, src->x, src->y, src->z, src->w);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xVec4_normalize(es1xVec4* dst)
{
	es1xMath_normalize(&dst->x, &dst->y, &dst->z);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xVec3_normalize(es1xVec3* dst)
{
	es1xMath_normalize(&dst->x, &dst->y, &dst->z);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _ES1XVECTOR_H */
