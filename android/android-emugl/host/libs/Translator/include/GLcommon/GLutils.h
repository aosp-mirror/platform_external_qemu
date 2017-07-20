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
#ifndef GL_UTILS_H
#define GL_UTILS_H

#include <GLES/gl.h>

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

// setGles2Gles(true) when mounting GLES translator on top of another
// GLES library;
void setGles2Gles(bool isGles2gles);
bool isGles2Gles();

// is/setCoreProfile sets the global variable for
// when core profile is in use.
void setCoreProfile(bool isCore);
bool isCoreProfile();

typedef enum {
    GLES_1_1 = 1,
    GLES_2_0 = 2,
    GLES_3_0 = 3,
    GLES_3_1 = 4,
    MAX_GLES_VERSION //Must be last
} GLESVersion;

bool isPowerOf2(int num);

// <EGL/egl.h> defines many types as 'void*' while they're really
// implemented as unsigned integers. These convenience template functions
// help casting between them safely without generating compiler warnings.
inline void* SafePointerFromUInt(unsigned int handle) {
    return (void*)(uintptr_t)(handle);
}

inline unsigned int SafeUIntFromPointerFileLine(const void* ptr,
                                                const char* file, int line) {
#if 1
    // Ignore the assert below to avoid crashing when running older
    // system images, which might have buggy encoder libraries. Print
    // an error message though.
    if ((uintptr_t)(ptr) != (unsigned int)(uintptr_t)(ptr)) {
        fprintf(stderr, "(%s:%d) EmuGL:WARNING: bad generic pointer %p\n",
                file, line, ptr);
    }
#else
    // Assertion error if the pointer contains a value that does not fit
    // in an unsigned integer!
    assert((uintptr_t)(ptr) == (unsigned int)(uintptr_t)(ptr));
#endif
    return (unsigned int)(uintptr_t)(ptr);
}

#define SafeUIntFromPointer(ptr) \
    SafeUIntFromPointerFileLine((ptr), __FILE__, __LINE__)

struct FramebufferChannelBits {
    int red;
    int green;
    int blue;
    int alpha;
    int depth;
    int stencil;
};

FramebufferChannelBits glFormatToChannelBits(GLenum colorFormat,
                                             GLenum depthFormat,
                                             GLenum stencilFormat);

GLenum baseFormatOfInternalFormat(GLint internalformat);
GLenum accurateTypeOfInternalFormat(GLint internalformat);

#endif
