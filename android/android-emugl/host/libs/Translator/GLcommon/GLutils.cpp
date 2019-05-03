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
#include <GLcommon/GLutils.h>

#include "android/base/synchronization/Lock.h"

#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl3ext.h>

bool isPowerOf2(int num) {
    return (num & (num -1)) == 0;
}

android::base::StaticLock sGlobalSettingsLock;

static uint32_t s_gles2Gles = 0;
static uint32_t s_coreProfile = 0;

void setGles2Gles(bool isGles2Gles) {
    __atomic_store_n(&s_gles2Gles, isGles2Gles ? 1 : 0, __ATOMIC_RELAXED);
}

bool isGles2Gles() {
    return __atomic_load_n(&s_gles2Gles, __ATOMIC_RELAXED) != 0;
}

void setCoreProfile(bool isCore) {
    __atomic_store_n(&s_coreProfile, isCore ? 1 : 0, __ATOMIC_RELAXED);
}

bool isCoreProfile() {
    return __atomic_load_n(&s_coreProfile, __ATOMIC_RELAXED) != 0;
}

FramebufferChannelBits glFormatToChannelBits(GLenum colorFormat,
                                             GLenum depthFormat,
                                             GLenum stencilFormat) {
    FramebufferChannelBits res = {};

#define COLOR_BITS_CASE(format, r, g, b, a) \
    case format: res.red = r; res.green = g; res.blue = b; res.alpha = a; break; \

    switch (colorFormat) {
    COLOR_BITS_CASE(GL_ALPHA, 0, 0, 0, 8)
    COLOR_BITS_CASE(GL_LUMINANCE, 0, 0, 0, 0)
    COLOR_BITS_CASE(GL_LUMINANCE_ALPHA, 0, 0, 0, 8)
    COLOR_BITS_CASE(GL_RGB, 8, 8, 8, 0)
    COLOR_BITS_CASE(GL_RGBA, 8, 8, 8, 8)
    COLOR_BITS_CASE(GL_R11F_G11F_B10F, 11, 11, 10, 0)
    COLOR_BITS_CASE(GL_R16F, 16, 0, 0, 0)
    COLOR_BITS_CASE(GL_R16I, 16, 0, 0, 0)
    COLOR_BITS_CASE(GL_R16UI, 16, 0, 0, 0)
    COLOR_BITS_CASE(GL_R32F, 32, 0, 0, 0)
    COLOR_BITS_CASE(GL_R32I, 32, 0, 0, 0)
    COLOR_BITS_CASE(GL_R32UI, 32, 0, 0, 0)
    COLOR_BITS_CASE(GL_R8, 8, 0, 0, 0)
    COLOR_BITS_CASE(GL_R8I, 8, 0, 0, 0)
    COLOR_BITS_CASE(GL_R8_SNORM, 8, 0, 0, 0)
    COLOR_BITS_CASE(GL_R8UI, 8, 0, 0, 0)
    COLOR_BITS_CASE(GL_RG16F, 16, 0, 0, 0)
    COLOR_BITS_CASE(GL_RG16I, 16, 0, 0, 0)
    COLOR_BITS_CASE(GL_RG16UI, 16, 0, 0, 0)
    COLOR_BITS_CASE(GL_RG32F, 32, 0, 0, 0)
    COLOR_BITS_CASE(GL_RG32I, 32, 0, 0, 0)
    COLOR_BITS_CASE(GL_RG32UI, 32, 0, 0, 0)
    COLOR_BITS_CASE(GL_RG8, 8, 8, 0, 0)
    COLOR_BITS_CASE(GL_RG8I, 8, 8, 0, 0)
    COLOR_BITS_CASE(GL_RG8_SNORM, 8, 8, 0, 0)
    COLOR_BITS_CASE(GL_RG8UI, 8, 8, 0, 0)
    COLOR_BITS_CASE(GL_RGB10_A2, 10, 10, 10, 2)
    COLOR_BITS_CASE(GL_RGB10_A2UI, 10, 10, 10, 2)
    COLOR_BITS_CASE(GL_RGB16F, 16, 16, 16, 0)
    COLOR_BITS_CASE(GL_RGB16I, 16, 16, 16, 0)
    COLOR_BITS_CASE(GL_RGB16UI, 16, 16, 16, 0)
    COLOR_BITS_CASE(GL_RGB32F, 32, 32, 32, 0)
    COLOR_BITS_CASE(GL_RGB32I, 32, 32, 32, 0)
    COLOR_BITS_CASE(GL_RGB32UI, 32, 32, 32, 0)
    COLOR_BITS_CASE(GL_RGB565, 5, 6, 5, 0)
    COLOR_BITS_CASE(GL_RGB5_A1, 5, 5, 5, 1)
    COLOR_BITS_CASE(GL_RGB8, 8, 8, 8, 0)
    COLOR_BITS_CASE(GL_RGB8I, 8, 8, 8, 0)
    COLOR_BITS_CASE(GL_RGB8_SNORM, 8, 8, 8, 0)
    COLOR_BITS_CASE(GL_RGB8UI, 8, 8, 8, 0)
    COLOR_BITS_CASE(GL_RGB9_E5, 9, 9, 9, 0)
    COLOR_BITS_CASE(GL_RGBA16F, 16, 16, 16, 16)
    COLOR_BITS_CASE(GL_RGBA16I, 16, 16, 16, 16)
    COLOR_BITS_CASE(GL_RGBA16UI, 16, 16, 16, 16)
    COLOR_BITS_CASE(GL_RGBA32F, 32, 32, 32, 32)
    COLOR_BITS_CASE(GL_RGBA32I, 32, 32, 32, 32)
    COLOR_BITS_CASE(GL_RGBA32UI, 32, 32, 32, 32)
    COLOR_BITS_CASE(GL_RGBA4, 4, 4, 4, 4)
    COLOR_BITS_CASE(GL_RGBA8, 8, 8, 8, 8)
    COLOR_BITS_CASE(GL_RGBA8I, 8, 8, 8, 8)
    COLOR_BITS_CASE(GL_RGBA8_SNORM, 8, 8, 8, 8)
    COLOR_BITS_CASE(GL_RGBA8UI, 8, 8, 8, 8)
    COLOR_BITS_CASE(GL_SRGB8, 8, 8, 8, 0)
    COLOR_BITS_CASE(GL_SRGB8_ALPHA8, 8, 8, 8, 8)
    }

#define DEPTH_BITS_CASE(format, d) \
    case format: res.depth = d; break; \

    switch (depthFormat) {
    DEPTH_BITS_CASE(GL_DEPTH_COMPONENT16, 16)
    DEPTH_BITS_CASE(GL_DEPTH_COMPONENT24, 24)
    DEPTH_BITS_CASE(GL_DEPTH_COMPONENT32F, 32)
    DEPTH_BITS_CASE(GL_DEPTH24_STENCIL8, 24)
    DEPTH_BITS_CASE(GL_DEPTH32F_STENCIL8, 32)
    DEPTH_BITS_CASE(GL_STENCIL_INDEX8, 0)
    }

#define STENCIL_BITS_CASE(format, d) \
    case format: res.stencil = d; break; \

    switch (stencilFormat) {
    STENCIL_BITS_CASE(GL_DEPTH_COMPONENT16, 0)
    STENCIL_BITS_CASE(GL_DEPTH_COMPONENT24, 0)
    STENCIL_BITS_CASE(GL_DEPTH_COMPONENT32F, 0)
    STENCIL_BITS_CASE(GL_DEPTH24_STENCIL8, 8)
    STENCIL_BITS_CASE(GL_DEPTH32F_STENCIL8, 8)
    STENCIL_BITS_CASE(GL_STENCIL_INDEX8, 8)
    }

    return res;
}

GLenum baseFormatOfInternalFormat(GLint internalformat) {

    switch (internalformat) {
    case GL_ALPHA:
    case GL_LUMINANCE:
    case GL_LUMINANCE_ALPHA:
    case GL_RGB:
    case GL_RGBA:
        return internalformat;
    case GL_R8:
    case GL_R8_SNORM:
    case GL_R16F:
    case GL_R32F:
        return GL_RED;
    case GL_R8I:
    case GL_R8UI:
    case GL_R16I:
    case GL_R16UI:
    case GL_R32I:
    case GL_R32UI:
        return GL_RED_INTEGER;
    case GL_RG8:
    case GL_RG8_SNORM:
    case GL_RG16F:
    case GL_RG32F:
        return GL_RG;
    case GL_RG8I:
    case GL_RG8UI:
    case GL_RG16I:
    case GL_RG16UI:
    case GL_RG32I:
    case GL_RG32UI:
        return GL_RG_INTEGER;
    case GL_RGB565:
    case GL_RGB8:
    case GL_SRGB8:
    case GL_RGB8_SNORM:
    case GL_RGB9_E5:
    case GL_R11F_G11F_B10F:
    case GL_RGB16F:
    case GL_RGB32F:
        return GL_RGB;
    case GL_RGB8I:
    case GL_RGB8UI:
    case GL_RGB16I:
    case GL_RGB16UI:
    case GL_RGB32I:
    case GL_RGB32UI:
        return GL_RGB_INTEGER;
    case GL_RGBA4:
    case GL_RGB5_A1:
    case GL_RGBA8:
    case GL_SRGB8_ALPHA8:
    case GL_RGBA8_SNORM:
    case GL_RGB10_A2:
    case GL_RGBA16F:
    case GL_RGBA32F:
        return GL_RGBA;
    case GL_RGBA8I:
    case GL_RGBA8UI:
    case GL_RGB10_A2UI:
    case GL_RGBA16I:
    case GL_RGBA16UI:
    case GL_RGBA32I:
    case GL_RGBA32UI:
        return GL_RGBA_INTEGER;
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32F:
        return GL_DEPTH_COMPONENT;
    case GL_DEPTH24_STENCIL8:
    case GL_DEPTH32F_STENCIL8:
        return GL_DEPTH_STENCIL;
    case GL_STENCIL_INDEX8:
        return GL_STENCIL;
    }

    fprintf(stderr, "%s: warning: unrecognized internal format 0x%x\n", __func__,
            internalformat);
    return internalformat;
}

GLenum accurateTypeOfInternalFormat(GLint internalformat) {
    switch (internalformat) {
    case GL_ALPHA:
    case GL_LUMINANCE:
    case GL_LUMINANCE_ALPHA:
    case GL_RGB:
    case GL_RGBA:
    case GL_R8:
    case GL_R8_SNORM:
    case GL_RG8:
    case GL_RG8_SNORM:
    case GL_RGB8:
    case GL_SRGB8:
    case GL_RGB8_SNORM:
    case GL_RGBA8:
    case GL_SRGB8_ALPHA8:
    case GL_RGBA8_SNORM:
    case GL_R8UI:
    case GL_RG8UI:
    case GL_RGB8UI:
    case GL_RGBA8UI:
        return GL_UNSIGNED_BYTE;
    case GL_R8I:
    case GL_RG8I:
    case GL_RGB8I:
    case GL_RGBA8I:
        return GL_BYTE;
    case GL_R16UI:
    case GL_RG16UI:
    case GL_RGB16UI:
    case GL_RGBA16UI:
    case GL_DEPTH_COMPONENT16:
        return GL_UNSIGNED_SHORT;
    case GL_R16I:
    case GL_RG16I:
    case GL_RGB16I:
    case GL_RGBA16I:
        return GL_SHORT;
    case GL_R32UI:
    case GL_RG32UI:
    case GL_RGB32UI:
    case GL_RGBA32UI:
    case GL_DEPTH_COMPONENT24:
        return GL_UNSIGNED_INT;
    case GL_R32I:
    case GL_RG32I:
    case GL_RGB32I:
    case GL_RGBA32I:
        return GL_INT;
    case GL_R16F:
    case GL_RG16F:
    case GL_RGB16F:
    case GL_RGBA16F:
        return GL_HALF_FLOAT;
    case GL_R32F:
    case GL_RG32F:
    case GL_RGB32F:
    case GL_RGBA32F:
    case GL_DEPTH_COMPONENT32F:
        return GL_FLOAT;
    case GL_DEPTH24_STENCIL8:
        return GL_UNSIGNED_INT_24_8;
    case GL_DEPTH32F_STENCIL8:
        return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
    case GL_RGB565:
        return GL_UNSIGNED_SHORT_5_6_5;
    case GL_RGB9_E5:
        return GL_UNSIGNED_INT_5_9_9_9_REV;
    case GL_R11F_G11F_B10F:
        return GL_UNSIGNED_INT_10F_11F_11F_REV;
    case GL_RGBA4:
        return GL_UNSIGNED_SHORT_4_4_4_4;
    case GL_RGB5_A1:
        return GL_UNSIGNED_SHORT_5_5_5_1;
    case GL_RGB10_A2:
    case GL_RGB10_A2UI:
        return GL_UNSIGNED_INT_2_10_10_10_REV;
    case GL_STENCIL_INDEX8:
        return GL_UNSIGNED_BYTE;
    }

    fprintf(stderr, "%s: warning: unrecognized internal format 0x%x\n", __func__,
            internalformat);
    return GL_UNSIGNED_BYTE;
}
