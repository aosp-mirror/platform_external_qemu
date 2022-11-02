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
#include "GLESv1Decoder.h"

#include "aemu/base/memory/LazyInstance.h"
#include "aemu/base/synchronization/Lock.h"

#include "OpenGLESDispatch/GLESv1Dispatch.h"

#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES/glext.h>

#include <atomic>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using android::base::AutoLock;
using android::base::StaticLock;

static inline void* SafePointerFromUInt(GLuint value) {
  return (void*)(uintptr_t)value;
}

int gles1_decoder_extended_context::initDispatch(
    void *(*getProc)(const char *name, void *userData), void *userData) {
    gles1_server_context_t::initDispatchByName(getProc, userData);
    // The following could be "not-implemented" in
    // -gpu swiftshader and -gpu angle.
    // But we should always have them in
    // -gpu host and -gpu [swiftshader|angle]_indirect.
    glColorPointerWithDataSize =
            (glColorPointerWithDataSize_server_proc_t)
            getProc("glColorPointerWithDataSize", userData);
    glNormalPointerWithDataSize =
            (glNormalPointerWithDataSize_server_proc_t)
            getProc("glNormalPointerWithDataSize", userData);
    glTexCoordPointerWithDataSize =
            (glTexCoordPointerWithDataSize_server_proc_t)
            getProc("glTexCoordPointerWithDataSize", userData);
    glVertexPointerWithDataSize =
            (glVertexPointerWithDataSize_server_proc_t)
            getProc("glVertexPointerWithDataSize", userData);
    return 0;
}

static StaticLock sLock;
static GLESv1Decoder::get_proc_func_t sGetProcFunc;
static void* sGetProcFuncData;

namespace {

struct ContextTemplateLoader {
    ContextTemplateLoader() {
        context.initDispatch(sGetProcFunc, sGetProcFuncData);
    }
    gles1_decoder_extended_context context;
};

}  // namespace

static android::base::LazyInstance<ContextTemplateLoader> sContextTemplate = {};

GLESv1Decoder::GLESv1Decoder()
{
    m_contextData = NULL;
    m_glesDso = NULL;
}

GLESv1Decoder::~GLESv1Decoder()
{
}

int GLESv1Decoder::initGL(get_proc_func_t getProcFunc, void *getProcFuncData)
{
    AutoLock lock(sLock);
    sGetProcFunc = getProcFunc;
    sGetProcFuncData = getProcFuncData;
    static_cast<gles1_decoder_extended_context&>(*this) = sContextTemplate->context;

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

    glGenBuffers_dec = s_glGenBuffers;
    glGenTextures_dec = s_glGenTextures;

    glGenFramebuffersOES_dec = s_glGenFramebuffersOES;
    glGenRenderbuffersOES_dec = s_glGenRenderbuffersOES;

    glGenVertexArraysOES_dec = s_glGenVertexArraysOES;

    glDeleteBuffers_dec = s_glDeleteBuffers;
    glDeleteTextures_dec = s_glDeleteTextures;
    glDeleteRenderbuffersOES_dec = s_glDeleteRenderbuffersOES;
    glDeleteFramebuffersOES_dec = s_glDeleteFramebuffersOES;
    glDeleteVertexArraysOES_dec = s_glDeleteVertexArraysOES;

    return 0;
}

int GLESv1Decoder::s_glFinishRoundTrip(void *self)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glFinish();
    return 0;
}

void GLESv1Decoder::s_glVertexPointerOffset(void *self, GLint size, GLenum type, GLsizei stride, GLuint offset)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glVertexPointer(size, type, stride, SafePointerFromUInt(offset));
}

void GLESv1Decoder::s_glColorPointerOffset(void *self, GLint size, GLenum type, GLsizei stride, GLuint offset)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glColorPointer(size, type, stride, SafePointerFromUInt(offset));
}

void GLESv1Decoder::s_glTexCoordPointerOffset(void *self, GLint size, GLenum type, GLsizei stride, GLuint offset)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glTexCoordPointer(size, type, stride, SafePointerFromUInt(offset));
}

void GLESv1Decoder::s_glNormalPointerOffset(void *self, GLenum type, GLsizei stride, GLuint offset)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glNormalPointer(type, stride, SafePointerFromUInt(offset));
}

void GLESv1Decoder::s_glPointSizePointerOffset(void *self, GLenum type, GLsizei stride, GLuint offset)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glPointSizePointerOES(type, stride, SafePointerFromUInt(offset));
}

void GLESv1Decoder::s_glWeightPointerOffset(void * self, GLint size, GLenum type, GLsizei stride, GLuint offset)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glWeightPointerOES(size, type, stride, SafePointerFromUInt(offset));
}

void GLESv1Decoder::s_glMatrixIndexPointerOffset(void * self, GLint size, GLenum type, GLsizei stride, GLuint offset)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glMatrixIndexPointerOES(size, type, stride, SafePointerFromUInt(offset));
}



#define STORE_POINTER_DATA_OR_ABORT(location)    \
    if (ctx->m_contextData != NULL) {   \
        ctx->m_contextData->storePointerData((location), data, datalen); \
    } else { \
        return; \
    }

void GLESv1Decoder::s_glVertexPointerData(void *self, GLint size, GLenum type, GLsizei stride, void *data, GLuint datalen)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;

    STORE_POINTER_DATA_OR_ABORT(GLDecoderContextData::VERTEX_LOCATION);

    if ((void*)ctx->glVertexPointerWithDataSize != gles1_unimplemented) {
        ctx->glVertexPointerWithDataSize(size, type, 0,
                ctx->m_contextData->pointerData(GLDecoderContextData::VERTEX_LOCATION),
                datalen);
    } else {
        assert(0);
        ctx->glVertexPointer(size, type, 0,
                ctx->m_contextData->pointerData(GLDecoderContextData::VERTEX_LOCATION));
    }
}

void GLESv1Decoder::s_glColorPointerData(void *self, GLint size, GLenum type, GLsizei stride, void *data, GLuint datalen)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;

    STORE_POINTER_DATA_OR_ABORT(GLDecoderContextData::COLOR_LOCATION);

    if ((void*)ctx->glColorPointerWithDataSize != gles1_unimplemented) {
        ctx->glColorPointerWithDataSize(size, type, 0,
                ctx->m_contextData->pointerData(GLDecoderContextData::COLOR_LOCATION),
                datalen);
    } else {
        assert(0);
        ctx->glColorPointer(size, type, 0,
                ctx->m_contextData->pointerData(GLDecoderContextData::COLOR_LOCATION));
    }
}

void GLESv1Decoder::s_glTexCoordPointerData(void *self, GLint unit, GLint size, GLenum type, GLsizei stride, void *data, GLuint datalen)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    STORE_POINTER_DATA_OR_ABORT((GLDecoderContextData::PointerDataLocation)
                                (GLDecoderContextData::TEXCOORD0_LOCATION + unit));

    if ((void*)ctx->glTexCoordPointerWithDataSize != gles1_unimplemented) {
        ctx->glTexCoordPointerWithDataSize(size, type, 0,
                ctx->m_contextData->pointerData((GLDecoderContextData::PointerDataLocation)
                                                (GLDecoderContextData::TEXCOORD0_LOCATION + unit)),
                datalen);
    } else {
        assert(0);
        ctx->glTexCoordPointer(size, type, 0,
                ctx->m_contextData->pointerData((GLDecoderContextData::PointerDataLocation)
                                                (GLDecoderContextData::TEXCOORD0_LOCATION + unit)));
    }
}

void GLESv1Decoder::s_glNormalPointerData(void *self, GLenum type, GLsizei stride, void *data, GLuint datalen)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;

    STORE_POINTER_DATA_OR_ABORT(GLDecoderContextData::NORMAL_LOCATION);

    if ((void*)ctx->glNormalPointerWithDataSize != gles1_unimplemented) {
        ctx->glNormalPointerWithDataSize(type, 0,
                ctx->m_contextData->pointerData(GLDecoderContextData::NORMAL_LOCATION),
                datalen);
    } else {
        assert(0);
        ctx->glNormalPointer(type, 0,
                ctx->m_contextData->pointerData(GLDecoderContextData::NORMAL_LOCATION));
    }
}

void GLESv1Decoder::s_glPointSizePointerData(void *self, GLenum type, GLsizei stride, void *data, GLuint datalen)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;

    STORE_POINTER_DATA_OR_ABORT(GLDecoderContextData::POINTSIZE_LOCATION);

    ctx->glPointSizePointerOES(type, 0, ctx->m_contextData->pointerData(GLDecoderContextData::POINTSIZE_LOCATION));
}

void GLESv1Decoder::s_glWeightPointerData(void * self, GLint size, GLenum type, GLsizei stride, void * data, GLuint datalen)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;

    STORE_POINTER_DATA_OR_ABORT(GLDecoderContextData::WEIGHT_LOCATION);

    ctx->glWeightPointerOES(size, type, 0, ctx->m_contextData->pointerData(GLDecoderContextData::WEIGHT_LOCATION));
}

void GLESv1Decoder::s_glMatrixIndexPointerData(void * self, GLint size, GLenum type, GLsizei stride, void * data, GLuint datalen)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;

    STORE_POINTER_DATA_OR_ABORT(GLDecoderContextData::MATRIXINDEX_LOCATION);

    ctx->glMatrixIndexPointerOES(size, type, 0, ctx->m_contextData->pointerData(GLDecoderContextData::MATRIXINDEX_LOCATION));
}

void GLESv1Decoder::s_glDrawElementsOffset(void *self, GLenum mode, GLsizei count, GLenum type, GLuint offset)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glDrawElements(mode, count, type, SafePointerFromUInt(offset));
}

void GLESv1Decoder::s_glDrawElementsData(void *self, GLenum mode, GLsizei count, GLenum type, void * data, GLuint datalen)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glDrawElements(mode, count, type, data);
}

void GLESv1Decoder::s_glGetCompressedTextureFormats(void *self, GLint count, GLint *data)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *) self;
    ctx->glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, data);
}

void GLESv1Decoder::s_glGenBuffers(void* self, GLsizei n, GLuint* buffers) {
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glGenBuffers(n, buffers);
    // TODO: Snapshot names
}

void GLESv1Decoder::s_glGenTextures(void* self, GLsizei n, GLuint* textures) {
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glGenTextures(n, textures);
    // TODO: Snapshot names
}

void GLESv1Decoder::s_glGenRenderbuffersOES(void* self, GLsizei n, GLuint* renderbuffers) {
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glGenRenderbuffersOES(n, renderbuffers);
    // TODO: Snapshot names
}

void GLESv1Decoder::s_glGenFramebuffersOES(void* self, GLsizei n, GLuint* framebuffers) {
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glGenFramebuffersOES(n, framebuffers);
    // TODO: Snapshot names
}

void GLESv1Decoder::s_glGenVertexArraysOES(void* self, GLsizei n, GLuint* arrays) {
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glGenVertexArraysOES(n, arrays);
    // TODO: Snapshot names
}

void GLESv1Decoder::s_glDeleteBuffers(void* self, GLsizei n, const GLuint *buffers) {
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glDeleteBuffers(n, buffers);
    // TODO: Snapshot names
}

void GLESv1Decoder::s_glDeleteTextures(void* self, GLsizei n, const GLuint *textures) {
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glDeleteTextures(n, textures);
    // TODO: Snapshot names
}

void GLESv1Decoder::s_glDeleteRenderbuffersOES(void* self, GLsizei n, const GLuint* renderbuffers) {
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glDeleteRenderbuffersOES(n, renderbuffers);
    // TODO: Snapshot names
}

void GLESv1Decoder::s_glDeleteFramebuffersOES(void* self, GLsizei n, const GLuint* framebuffers) {
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glDeleteFramebuffersOES(n, framebuffers);
    // TODO: Snapshot names
}

void GLESv1Decoder::s_glDeleteVertexArraysOES(void* self, GLsizei n, const GLuint *arrays) {
    GLESv1Decoder *ctx = (GLESv1Decoder *)self;
    ctx->glDeleteVertexArraysOES(n, arrays);
    // TODO: Snapshot names
}

void *GLESv1Decoder::s_getProc(const char *name, void *userData)
{
    GLESv1Decoder *ctx = (GLESv1Decoder *)userData;

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
