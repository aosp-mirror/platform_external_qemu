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
#include "GLDecoder.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES/glext.h>

static inline void* SafePointerFromUInt(GLuint value) {
  return (void*)(uintptr_t)value;
}

GLDecoder::GLDecoder()
{
    m_contextData = NULL;
    m_glesDso = NULL;
}

GLDecoder::~GLDecoder()
{
    if (m_glesDso != NULL) {
        delete m_glesDso;
    }
}


int GLDecoder::initGL(get_proc_func_t getProcFunc, void *getProcFuncData)
{
    if (getProcFunc == NULL) {
        const char *libname = GLES_LIBNAME;
        if (getenv(GLES_LIBNAME_VAR) != NULL) {
            libname = getenv(GLES_LIBNAME_VAR);
        }

        m_glesDso = emugl::SharedLibrary::open(libname);
        if (m_glesDso == NULL) {
            fprintf(stderr, "Couldn't find %s \n", GLES_LIBNAME);
            return -1;
        }

        this->initDispatchByName(s_getProc, this);
    } else {
        this->initDispatchByName(getProcFunc, getProcFuncData);
    }

    glGetCompressedTextureFormats = s_glGetCompressedTextureFormats;
    glVertexPointerOffset = s_glVertexPointerOffset;
    glColorPointerOffset = s_glColorPointerOffset;
    glNormalPointerOffset = s_glNormalPointerOffset;
    glTexCoordPointerOffset = s_glTexCoordPointerOffset;
    glPointSizePointerOffset = s_glPointSizePointerOffset;
    glWeightPointerOffset = s_glWeightPointerOffset;
    glMatrixIndexPointerOffset = s_glMatrixIndexPointerOffset;

    glVertexPointerData = s_glVertexPointerData;
    glColorPointerData = s_glColorPointerData;
    glNormalPointerData = s_glNormalPointerData;
    glTexCoordPointerData = s_glTexCoordPointerData;
    glPointSizePointerData = s_glPointSizePointerData;
    glWeightPointerData = s_glWeightPointerData;
    glMatrixIndexPointerData = s_glMatrixIndexPointerData;

    glDrawElementsOffset = s_glDrawElementsOffset;
    glDrawElementsData = s_glDrawElementsData;
    glFinishRoundTrip = s_glFinishRoundTrip;

    return 0;
}

int GLDecoder::s_glFinishRoundTrip(void *self)
{
    GLDecoder *ctx = (GLDecoder *)self;
    ctx->glFinish();
    return 0;
}

void GLDecoder::s_glVertexPointerOffset(void *self, GLint size, GLenum type, GLsizei stride, GLuint offset)
{
    GLDecoder *ctx = (GLDecoder *)self;
    ctx->glVertexPointer(size, type, stride, SafePointerFromUInt(offset));
}

void GLDecoder::s_glColorPointerOffset(void *self, GLint size, GLenum type, GLsizei stride, GLuint offset)
{
    GLDecoder *ctx = (GLDecoder *)self;
    ctx->glColorPointer(size, type, stride, SafePointerFromUInt(offset));
}

void GLDecoder::s_glTexCoordPointerOffset(void *self, GLint size, GLenum type, GLsizei stride, GLuint offset)
{
    GLDecoder *ctx = (GLDecoder *)self;
    ctx->glTexCoordPointer(size, type, stride, SafePointerFromUInt(offset));
}

void GLDecoder::s_glNormalPointerOffset(void *self, GLenum type, GLsizei stride, GLuint offset)
{
    GLDecoder *ctx = (GLDecoder *)self;
    ctx->glNormalPointer(type, stride, SafePointerFromUInt(offset));
}

void GLDecoder::s_glPointSizePointerOffset(void *self, GLenum type, GLsizei stride, GLuint offset)
{
    GLDecoder *ctx = (GLDecoder *)self;
    ctx->glPointSizePointerOES(type, stride, SafePointerFromUInt(offset));
}

void GLDecoder::s_glWeightPointerOffset(void * self, GLint size, GLenum type, GLsizei stride, GLuint offset)
{
    GLDecoder *ctx = (GLDecoder *)self;
    ctx->glWeightPointerOES(size, type, stride, SafePointerFromUInt(offset));
}

void GLDecoder::s_glMatrixIndexPointerOffset(void * self, GLint size, GLenum type, GLsizei stride, GLuint offset)
{
    GLDecoder *ctx = (GLDecoder *)self;
    ctx->glMatrixIndexPointerOES(size, type, stride, SafePointerFromUInt(offset));
}



#define STORE_POINTER_DATA_OR_ABORT(location)    \
    if (ctx->m_contextData != NULL) {   \
        ctx->m_contextData->storePointerData((location), data, datalen); \
    } else { \
        return; \
    }

void GLDecoder::s_glVertexPointerData(void *self, GLint size, GLenum type, GLsizei stride, void *data, GLuint datalen)
{
    GLDecoder *ctx = (GLDecoder *)self;

    STORE_POINTER_DATA_OR_ABORT(GLDecoderContextData::VERTEX_LOCATION);

    ctx->glVertexPointer(size, type, 0, ctx->m_contextData->pointerData(GLDecoderContextData::VERTEX_LOCATION));
}

void GLDecoder::s_glColorPointerData(void *self, GLint size, GLenum type, GLsizei stride, void *data, GLuint datalen)
{
    GLDecoder *ctx = (GLDecoder *)self;

    STORE_POINTER_DATA_OR_ABORT(GLDecoderContextData::COLOR_LOCATION);

    ctx->glColorPointer(size, type, 0, ctx->m_contextData->pointerData(GLDecoderContextData::COLOR_LOCATION));
}

void GLDecoder::s_glTexCoordPointerData(void *self, GLint unit, GLint size, GLenum type, GLsizei stride, void *data, GLuint datalen)
{
    GLDecoder *ctx = (GLDecoder *)self;
    STORE_POINTER_DATA_OR_ABORT((GLDecoderContextData::PointerDataLocation)
                                (GLDecoderContextData::TEXCOORD0_LOCATION + unit));

    ctx->glTexCoordPointer(size, type, 0,
                           ctx->m_contextData->pointerData((GLDecoderContextData::PointerDataLocation)
                                                           (GLDecoderContextData::TEXCOORD0_LOCATION + unit)));
}

void GLDecoder::s_glNormalPointerData(void *self, GLenum type, GLsizei stride, void *data, GLuint datalen)
{
    GLDecoder *ctx = (GLDecoder *)self;

    STORE_POINTER_DATA_OR_ABORT(GLDecoderContextData::NORMAL_LOCATION);

    ctx->glNormalPointer(type, 0, ctx->m_contextData->pointerData(GLDecoderContextData::NORMAL_LOCATION));
}

void GLDecoder::s_glPointSizePointerData(void *self, GLenum type, GLsizei stride, void *data, GLuint datalen)
{
    GLDecoder *ctx = (GLDecoder *)self;

    STORE_POINTER_DATA_OR_ABORT(GLDecoderContextData::POINTSIZE_LOCATION);

    ctx->glPointSizePointerOES(type, 0, ctx->m_contextData->pointerData(GLDecoderContextData::POINTSIZE_LOCATION));
}

void GLDecoder::s_glWeightPointerData(void * self, GLint size, GLenum type, GLsizei stride, void * data, GLuint datalen)
{
    GLDecoder *ctx = (GLDecoder *)self;

    STORE_POINTER_DATA_OR_ABORT(GLDecoderContextData::WEIGHT_LOCATION);

    ctx->glWeightPointerOES(size, type, 0, ctx->m_contextData->pointerData(GLDecoderContextData::WEIGHT_LOCATION));
}

void GLDecoder::s_glMatrixIndexPointerData(void * self, GLint size, GLenum type, GLsizei stride, void * data, GLuint datalen)
{
    GLDecoder *ctx = (GLDecoder *)self;

    STORE_POINTER_DATA_OR_ABORT(GLDecoderContextData::MATRIXINDEX_LOCATION);

    ctx->glMatrixIndexPointerOES(size, type, 0, ctx->m_contextData->pointerData(GLDecoderContextData::MATRIXINDEX_LOCATION));
}

void GLDecoder::s_glDrawElementsOffset(void *self, GLenum mode, GLsizei count, GLenum type, GLuint offset)
{
    GLDecoder *ctx = (GLDecoder *)self;
    ctx->glDrawElements(mode, count, type, SafePointerFromUInt(offset));
}

void GLDecoder::s_glDrawElementsData(void *self, GLenum mode, GLsizei count, GLenum type, void * data, GLuint datalen)
{
    GLDecoder *ctx = (GLDecoder *)self;
    ctx->glDrawElements(mode, count, type, data);
}

void GLDecoder::s_glGetCompressedTextureFormats(void *self, GLint count, GLint *data)
{
    GLDecoder *ctx = (GLDecoder *) self;
    ctx->glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, data);
}

void *GLDecoder::s_getProc(const char *name, void *userData)
{
    GLDecoder *ctx = (GLDecoder *)userData;

    if (ctx == NULL || ctx->m_glesDso == NULL) {
        return NULL;
    }

    void *func = NULL;
#ifdef USE_EGL_GETPROCADDRESS
    func = (void *) eglGetProcAddress(name);
#endif
    if (func == NULL) {
        func = (void *)(ctx->m_glesDso->findSymbol(name));
    }
    return func;
}
