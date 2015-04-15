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
#ifndef _OPENGL_RENDERER_RENDER_API_H
#define _OPENGL_RENDERER_RENDER_API_H

/* This header and its declarations must be usable from C code.
 *
 * If RENDER_API_NO_PROTOTYPES is #defined before including this header, only
 * the interface function pointer types will be declared, not the prototypes.
 * This allows the client to use those names for its function pointer variables.
 *
 * All interfaces which can fail return an int, with zero indicating failure
 * and anything else indicating success.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "render_api_functions.h"

#include <KHR/khrplatform.h>

// Use KHRONOS_APICALL to control visibility, but do not use KHRONOS_APIENTRY
// because we don't need the functions to be __stdcall on Win32.
#define RENDER_APICALL  KHRONOS_APICALL
#define RENDER_APIENTRY

/* list of constants to be passed to setStreamMode */
#define STREAM_MODE_DEFAULT   0
#define STREAM_MODE_TCP       1
#define STREAM_MODE_UNIX      2
#define STREAM_MODE_PIPE      3


#define RENDER_API_DECLARE(return_type, func_name, signature) \
    typedef return_type (RENDER_APIENTRY *func_name ## Fn) signature; \
    RENDER_APICALL return_type RENDER_APIENTRY func_name signature;

LIST_RENDER_API_FUNCTIONS(RENDER_API_DECLARE)

#ifdef __cplusplus
}
#endif

#endif
