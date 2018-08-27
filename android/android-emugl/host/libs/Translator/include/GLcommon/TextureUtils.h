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
#ifndef _TEXTURE_UTILS_H
#define _TEXTURE_UTILS_H

#include "GLEScontext.h"
#include "PaletteTexture.h"
#include "etc.h"

#include <functional>
#include <GLES/gl.h>
#include <GLES/glext.h>

typedef std::function<void(GLenum target, GLint level,
    GLint internalformat, GLsizei width, GLsizei height,
    GLint border, GLenum format, GLenum type, const GLvoid * data)>
        glTexImage2D_t;

ETC2ImageFormat getEtcFormat(GLenum internalformat);
void getAstcFormats(const GLint** formats, size_t* formatsCount);
bool isAstcFormat(GLenum internalformat);
bool isEtcFormat(GLenum internalformat);
bool isPaletteFormat(GLenum internalformat);
int getCompressedFormats(int* formats);
void doCompressedTexImage2D(GLEScontext* ctx, GLenum target, GLint level,
                            GLenum internalformat, GLsizei width,
                            GLsizei height, GLint border, GLsizei imageSize,
                            const GLvoid* data, glTexImage2D_t glTexImage2DPtr);
void deleteRenderbufferGlobal(GLuint rbo);
GLenum decompressedInternalFormat(GLEScontext* ctx, GLenum compressedFormat);

bool isCubeMapFaceTarget(GLenum target);
bool isCoreProfileEmulatedFormat(GLenum format);
GLenum getCoreProfileEmulatedFormat(GLenum format);
GLint getCoreProfileEmulatedInternalFormat(GLint internalformat, GLenum type);

// TextureSwizzle: Structure to represent texture swizzling and help wth
// emulation of swizzle state.
//
// |to(Red|Green|Blue|Alpha)| represent the components
// (GL_(RED|GREEN|BLUE|ALPHA|ZERO|ONE)) that should be mapped
// from a source to destination image.
//
// For example, i.e., if |toRed| = GL_GREEN, the red component of the
// destination is read from the green component of the source.
//
// The functions below encode the swizzles to emulate deprecated formats
// such as GL_ALPHA/LUMINANCE plus utility functions to obtain the
// result of concatenating two swizzles in a row.
struct TextureSwizzle {
    GLenum toRed = GL_RED;
    GLenum toGreen = GL_GREEN;
    GLenum toBlue = GL_BLUE;
    GLenum toAlpha = GL_ALPHA;
};

TextureSwizzle getSwizzleForEmulatedFormat(GLenum format);
TextureSwizzle getInverseSwizzleForEmulatedFormat(GLenum format);

GLenum swizzleComponentOf(const TextureSwizzle& s, GLenum component);
TextureSwizzle concatSwizzles(const TextureSwizzle& first,
                              const TextureSwizzle& next);

bool isSwizzleParam(GLenum pname);

bool isIntegerInternalFormat(GLint internalFormat);

#endif
