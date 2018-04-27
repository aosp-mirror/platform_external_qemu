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
#ifndef __GL_BASE_TYPES__H
#define __GL_BASE_TYPES__H

#include <KHR/khrplatform.h>

#ifndef gles1_APIENTRY
#define gles1_APIENTRY KHRONOS_APIENTRY
#endif

#ifndef gles2_APIENTRY
#define gles2_APIENTRY KHRONOS_APIENTRY
#endif

using GLvoid = void;
using GLenum = unsigned int;
using GLboolean = unsigned char;
using GLbitfield = unsigned int;
using GLchar = char;
using GLbyte = khronos_int8_t;
using GLshort = short;
using GLint = int;
using GLsizei = int;
using GLubyte = khronos_uint8_t;
using GLushort = unsigned short;
using GLuint = unsigned int;
using GLfloat = khronos_float_t;
using GLclampf = khronos_float_t;
using GLfixed = khronos_int32_t;
using GLclampx = khronos_int32_t;
using GLintptr = khronos_intptr_t;
using GLsizeiptr = khronos_ssize_t;
using GLstr = char*;
/* JR XXX Treating this as an in handle - is this correct? */
using GLeglImageOES = void*;
using GLsync = struct __GLsync*;
using GLint64 = khronos_int64_t;
using GLuint64 = khronos_uint64_t;

/* ErrorCode */
#ifndef GL_INVALID_ENUM
#define GL_NO_ERROR                       0
#define GL_INVALID_ENUM                   0x0500
#define GL_INVALID_VALUE                  0x0501
#define GL_INVALID_OPERATION              0x0502
#define GL_STACK_OVERFLOW                 0x0503
#define GL_STACK_UNDERFLOW                0x0504
#define GL_OUT_OF_MEMORY                  0x0505
#endif

#endif
