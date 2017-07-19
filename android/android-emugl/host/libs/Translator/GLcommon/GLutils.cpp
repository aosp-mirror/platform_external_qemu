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

#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

bool isPowerOf2(int num) {
    return (num & (num -1)) == 0;
}

static bool s_gles2Gles = false;
static bool s_coreProfile = false;

void setGles2Gles(bool isGles2Gles) {
    s_gles2Gles = isGles2Gles;
}

bool isGles2Gles() {
    return s_gles2Gles;
}

void setCoreProfile(bool isCore) {
    s_coreProfile = isCore;
}

bool isCoreProfile() {
    return s_coreProfile;
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
