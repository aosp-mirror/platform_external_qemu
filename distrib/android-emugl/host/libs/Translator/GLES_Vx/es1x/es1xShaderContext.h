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

 
#ifndef _ES1XSHADERCONTEXT_H
#define _ES1XSHADERCONTEXT_H

#ifndef _ES1XSHADERCONTEXTKEY_H
#	include "es1xShaderContextKey.h"
#endif

#ifndef _ES1XMATRIX_H
#	include "es1xMatrix.h"
#endif

#ifndef _ES1XLIGHT_H
#	include "es1xLight.h"
#endif

#ifndef _ES1XENUMERATIONS_H
#	include "es1xEnumerations.h"
#endif

typedef struct
{
	deUint8		light[ES1X_NUM_SUPPORTED_LIGHTS];
	deUint8		textureUnit[ES1X_NUM_SUPPORTED_TEXTURE_UNITS];
	int			numEnabledLightsOfType[ES1X_NUM_LIGHT_TYPES];
	int			numEnabledLights;
} es1xIndexMapTable;

typedef struct 
{
	#include "es1xShaderContextFields.inl"
} es1xShaderContext;

#ifdef __cplusplus
extern "C" 
{
#endif 

es1xShaderContext*	es1xShaderContext_create		(void);
void				es1xShaderContext_init			(es1xShaderContext* context, const void* /* const es1xContext* */ source, GLenum drawingPrimitive, const es1xIndexMapTable* mapTable);
void				es1xShaderContext_optimize		(es1xShaderContext* context);
void				es1xShaderContext_constructKey	(const es1xShaderContext* context, es1xShaderContextKey* key);
void				es1xIndexMapTable_init			(es1xIndexMapTable* table, const void* /* const es1xContext* */ source);

ES1X_DEBUG_CODE(void es1xShaderContext_dump(const es1xShaderContext* context));

#ifdef __cplusplus
} /* extern "C" */ 
#endif 

#endif /* _ES1XSHADERCONTEXT_H */

