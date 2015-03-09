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

 
#ifndef _ES1XSHADERCACHE_H
#define _ES1XSHADERCACHE_H

#ifndef _ES1XDEFS_H
#	include "es1xDefs.h"
#endif

#ifndef _ES1XSHADERCONTEXTKEY_H
#	include "es1xShaderContextKey.h"
#endif

#ifndef _ES1XSHADERCONTEXT_H
#	include "es1xShaderContext.h"
#endif

#ifndef _ES1XSHADERCONTEXTKEY_H
#	include "es1xShaderContextKey.h"
#endif

#ifndef _DEINT32_H
#	include "deInt32.h"
#endif

#ifndef _ES1XSHADERPROGRAMHASHTABLE_H
#	include "es1xShaderProgramInfoHashTable.h"
#endif

#ifndef _ES1XSHADERCONTEXTKEYALLOCATOR_H
#	include "es1xShaderContextKeyAllocator.h"
#endif

#ifndef _ES1XSHADERPROGRAMINFOALLOCATOR_H
#	include "es1xShaderProgramInfoAllocator.h"
#endif

#ifndef _ES1XSHADERPRIORITYENTRYALLOCATOR_H
#	include "es1xShaderPriorityEntryAllocator.h"
#endif

#ifndef _ES1XSHADERPROGRAMS_H
#	include "es1xShaderPrograms.h"
#endif

DE_STATIC_ASSERT(ES1X_NUM_MAX_CACHED_SHADER_PROGRAMS >= 1);

typedef struct 
{
	/* Recycled shader context */
	es1xShaderContext					shaderContext;

	es1xShaderContextKeyAllocator*		keyAllocator;
	es1xShaderProgramInfoAllocator*		programInfoAllocator;
	es1xShaderPriorityEntryAllocator*	priorityEntryAllocator;
	es1xShaderPriorityEntry*			highestPriorityEntry;
	es1xShaderPriorityEntry*			lowestPriorityEntry;
	es1xShaderProgramHashTable			programs;
} es1xShaderCache;

#ifdef __cplusplus
extern "C"
{
#endif

/* Shader cache API */
es1xShaderCache*		es1xShaderCache_create();
void					es1xShaderCache_destroy();
es1xShaderContext*		es1xShaderCache_allocateContext					(es1xShaderCache* cache);
void					es1xShaderCache_releaseContext					(es1xShaderCache* cache, es1xShaderContext* context);
es1xShaderContextKey*	es1xShaderCache_allocateContextKey				(es1xShaderCache* cache);
void					es1xShaderCache_releaseContextKey				(es1xShaderCache* cache, es1xShaderContextKey* array);
es1xShaderProgramInfo*	es1xShaderCache_allocateProgramInfo				(es1xShaderCache* cache);
void					es1xShaderCache_releaseProgramInfo				(es1xShaderCache* cache, es1xShaderProgramInfo* info);

void					es1xShaderCache_registerProgram					(es1xShaderCache* cache, es1xShaderContextKey* key, es1xShaderProgramInfo* value);
es1xShaderProgramInfo*	es1xShaderCache_useProgram						(es1xShaderCache* cache, es1xShaderContextKey* key);
es1xShaderContextKey*	es1xShaderCache_unregisterMostRarelyUsedProgram	(es1xShaderCache* cache, es1xShaderProgramInfo** program);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* _ES1XSHADERCACHE_H */
