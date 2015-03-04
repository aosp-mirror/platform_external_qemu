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

 
#ifndef _ES1XMATH_H
#define _ES1XMATH_H

#ifndef _ES1XDEFS_H
#	include "es1xDefs.h"
#endif

#include <math.h>

#define ES1X_PI			3.1415926535f

#ifdef __cplusplus
extern "C"
{
#endif

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLfloat es1xMath_sinf(GLfloat value)
{
	return (GLfloat)sin(value);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLfloat es1xMath_cosf(GLfloat value)
{
	return (GLfloat)cos(value);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLfloat es1xMath_radians(GLfloat angle)
{
	return angle * ES1X_PI / 180.f;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLfloat es1xMath_sqrtf(GLfloat value)
{
	return (GLfloat)sqrt(value);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE int es1xMath_maxi(int a, int b)
{
	return a > b ? a : b;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE int es1xMath_mini(int a, int b)
{
	return a < b ? a : b;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLfloat	es1xMath_xtof(GLfixed value)
{
	return ((GLfloat)value) / (1 << 16);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLfixed es1xMath_ftox(GLfloat value)
{
	return (GLfixed)(value * (1 << 16));
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLfloat es1xMath_itof(GLint value)
{
	return (2 * value + 1) / (float)((1 << 16) - 1);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLboolean es1xMath_clampb(GLboolean value)
{
	return value ? GL_TRUE : GL_FALSE;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLfloat es1xMath_clampf(GLfloat value)
{
	if (value <= 0.f)
		return 0.f;
		
	if (value >= 1.f)
		return 1.f;
		
	return value;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLint es1xMath_ceil(GLfloat value)
{
	GLint i = (GLint)value;

	if (value-i > 0.f)
		return i+1;
		
	return i;
}

/*---------------------------------------------------------------------------*/

void es1xMath_normalize(GLfloat* x, GLfloat* y, GLfloat* z);

/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _ES1XMATH_H */
