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

 
#ifndef _ES1XSHADERPROGRAMS_H
#define _ES1XSHADERPROGRAMS_H

#ifndef _ES1XDEFS_H
#	include "es1xDefs.h"
#endif

#ifndef _ES1XSHADERCONTEXT_H
#	include "es1xShaderContext.h"
#endif

#ifndef _ES1XSHADERCODEUNIFORMS_H
#	include "es1xShaderCodeUniforms.h"
#endif

#ifndef _ES1XUNIFORMVERSIONS_H
#	include "es1xUniformVersions.h"
#endif

#define ES1X_VERTEX_ARRAY_POS			1
#define	ES1X_NORMAL_ARRAY_POS			2
#define ES1X_COLOR_ARRAY_POS			3
#define ES1X_POINT_SIZE_ARRAY_POS		4
#define ES1X_TEXTURE_COORD_ARRAY0_POS	5

#ifdef __cplusplus
extern "C"
{
#endif

/*---------------------------------------------------------------------------*/

typedef struct
{
	GLuint						program;	
	GLint						uniformLocations[NUM_SHADER_CODE_UNIFORMS];
	es1xUniformVersions			uniformVersions;
	void*						priorityData;
} es1xShaderProgramInfo;

/*---------------------------------------------------------------------------*/

void es1xShaderProgram_create(es1xShaderContext* context, es1xShaderProgramInfo* info, const es1xIndexMapTable* table);
void es1xShaderProgramInfo_init(es1xShaderProgramInfo* info);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _ES1XSHADERPROGRAMS_H */
