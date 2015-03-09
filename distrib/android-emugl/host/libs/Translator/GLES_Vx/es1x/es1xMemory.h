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

 
#ifndef _ES1XMEMORY_H
#define _ES1XMEMORY_H

#ifndef _ES1XDEFS_H
#	include "es1xDefs.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

void	es1xMemSet	(void* dst, deInt32 value, size_t size);
void	es1xMemCopy	(void* dst, const void* src, size_t size);
void*	es1xMalloc	(size_t size);
void	es1xFree	(void* ptr);

#include <memory.h>

ES1X_INLINE void es1xMemZero(void* dst, size_t size)
{
	memset(dst, 0, size);
}

#ifdef __cplusplus
}
#endif

#endif /* _ES1XMEMORY_H */
