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

 
#ifndef _ES1XDEFS_H
#define _ES1XDEFS_H

#ifndef _DEDEFS_H
#include "deDefs.h"
#endif

#ifdef DE_DEBUG
#       define ES1X_DEBUG
#       define ES1X_DEBUG_CODE         DE_DEBUG_CODE
#endif

#define ES1X_ASSERT             DE_ASSERT
#define ES1X_UNREF              DE_UNREF
#define ES1X_NULL               DE_NULL
#define ES1X_INLINE             DE_INLINE

/*      In some cases it is handy to add void statements to the end of
        a code block because MSVC debugger falls immediately out from
        the block once last statement is executed. Therefore local
        variables are not visible anymore. */

#ifdef ES1X_DEBUG
#       define ES1X_VOID_STATEMENT for(;;) break;
#else
#       define ES1X_VOID_STATEMENT
#endif

/* ES1X API */
#define ES1X_API
#define ES1X_APIENTRY


/* OpenGL ES headers */
#define ES1X_OMIT_FUNCTION_NAME_WRAPPING
#define __GL_EXPORTS

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define ES1X_OMIT_GL_TYPE_DEFINITIONS
#include <GLES/gl.h>
#include <GLES/glext.h>
#undef ES1X_OMIT_GL_TYPE_DEFINITIONS

/* Internal wrapper settings */
#define ES1X_PROJECTION_MATRIX_STACK_SIZE                               16
#define ES1X_MODELVIEW_MATRIX_STACK_SIZE                                16
#define ES1X_TEXTURE_MATRIX_STACK_SIZE                                  4

#define ES1X_NUM_MAX_CACHED_SHADER_PROGRAMS                             16

/*
  #define ES1X_NUM_FACES                                                                        2
  #define ES1X_NDX_FRONT_FACE                                                           0
  #define ES1X_NDX_BACK_FACE                                                            1
*/

#define ES1X_NUM_SUPPORTED_COMPRESSED_TEXTURE_FORMATS   10
#define ES1X_MAX_SUPPORTED_POINT_SIZE                                   2
#define ES1X_MAX_SUPPORTED_TEXTURE_SIZE                                 64
#define ES1X_NUM_SUPPORTED_SAMPLE_BUFFERS                               2
#define ES1X_NUM_SUPPORTED_SAMPLES                                              2

#define ES1X_NDX_RED    0
#define ES1X_NDX_GREEN  1
#define ES1X_NDX_BLUE   2
#define ES1X_NDX_ALPHA  3

// #define ES1X_SUPPORT_STATISTICS // define at make

#include "es1xExternalConfig.h"

#if defined ES1X_DUPLICATE_ES2_STATE
#       define ES20_DEBUG_CODE(STATEMENT)       STATEMENT
#else
#       define ES20_DEBUG_CODE(STATEMENT)
#endif

#define ES1X_NULL_STATEMENT     ;

#ifdef __cplusplus
extern "C"
{
#endif

/* Version information */
extern const char* ES1X_STRING_VENDOR;
extern const char* ES1X_STRING_VERSION;
extern const char* ES1X_STRING_RENDERER;
extern const char* ES1X_STRING_EXTENSIONS;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _ES1XDEFS_H */
