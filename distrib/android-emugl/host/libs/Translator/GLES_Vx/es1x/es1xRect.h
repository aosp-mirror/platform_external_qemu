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

 
#ifndef _ES1XRECT_H
#define _ES1XRECT_H

#ifndef _ES1XDEFS_H
#	include "es1xDefs.h"
#endif

typedef struct 
{
	GLint	left;
	GLint	top;
	GLint	right;
	GLint	bottom;
} es1xRecti;

typedef struct
{
	GLfloat	left;
	GLfloat	top;
	GLfloat	right;
	GLfloat	bottom;
} es1xRectf;

#ifdef __cplusplus
extern "C"
{
#endif

/* Vector operations */
void es1xRecti_set(es1xRecti* dst, GLint left, GLint top, GLint right, GLint bottom);
void es1xRectf_set(es1xRectf* dst, GLfloat left, GLfloat top, GLfloat right, GLfloat bottom);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* _ES1XRECT_H */
