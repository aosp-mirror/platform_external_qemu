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

 
#include "es1xRect.h"

void es1xRecti_set(es1xRecti* dst, GLint left, GLint top, GLint right, GLint bottom)
{
	ES1X_ASSERT(dst);
	
	dst->left		= left;
	dst->right		= right;
	dst->top		= top;
	dst->bottom		= bottom;
}

void es1xRectf_set(es1xRectf* dst, GLfloat left, GLfloat top, GLfloat right, GLfloat bottom)
{
	ES1X_ASSERT(dst);
	
	dst->left		= left;
	dst->right		= right;
	dst->top		= top;
	dst->bottom		= bottom;
}
