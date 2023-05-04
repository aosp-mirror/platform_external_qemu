// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "OpenGLESDispatch/gles_functions.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

using namespace gfxstream::gl;

static const GLESv2Dispatch* s_gles2;
static int s_gles2_version;

static void fatal(const char * message, const char * func) {
    fprintf(stderr, "Fatal: %s func %s\n", message, func);
    abort();
}

#define EPOXY_FUNC_POINTER(return_type,func_name,signature,callargs) \
static return_type GL_APIENTRY my_ ## func_name signature; \
func_name ## _t  epoxy_ ## func_name = &my_ ## func_name ; \
static return_type GL_APIENTRY my_ ## func_name signature { \
    if (!s_gles2) fatal("null gles dispatcher", #func_name); \
    if (!s_gles2->func_name) fatal("null function pointer", #func_name); \
    epoxy_ ## func_name = s_gles2->func_name; \
    return s_gles2-> func_name callargs; \
}

LIST_GLES2_FUNCTIONS(EPOXY_FUNC_POINTER, EPOXY_FUNC_POINTER)

typedef void(* VOID_FUNC) ();

#define ALIAS(alias, orig) \
    void (* epoxy_ ## alias ) () = (VOID_FUNC) & my_ ## orig ;

ALIAS(glBindBufferARB, glBindBuffer)
ALIAS(glGenBuffersARB, glGenBuffers)
ALIAS(glVertexAttribDivisorARB, glVertexAttribDivisor)
ALIAS(glTexParameterIuiv, glTexParameteriv)
ALIAS(glSamplerParameterIuiv, glSamplerParameteriv);

#define STUB(func) \
static void stub_ ## func (void) { \
    fatal("unimplemented", #func); \
} \
VOID_FUNC epoxy_ ## func = & stub_ ## func;

STUB(glAlphaFunc)
STUB(glBeginConditionalRender)
STUB(glBindFragDataLocationIndexed)
STUB(glBeginQueryIndexed)
STUB(glBlendEquationSeparateiARB)
STUB(glBlendFuncSeparateiARB)
STUB(glClampColor)
STUB(glClipPlane)
STUB(glColorMaskIndexedEXT)
STUB(glCompressedTexSubImage1D)
STUB(glDepthRangeIndexed)
STUB(glDisableClientState)
STUB(glDisableIndexedEXT)
STUB(glDrawArraysInstancedARB)
STUB(glDrawArraysInstancedBaseInstance)
STUB(glDrawElementsInstancedARB)
STUB(glDrawElementsInstancedBaseVertex)
STUB(glDrawPixels)
STUB(glDrawRangeElementsBaseVertex)
STUB(glEnableClientState)
STUB(glEnableIndexedEXT)
STUB(glEndConditionalRenderNV)
STUB(glEndQueryIndexed)
STUB(glFramebufferTexture)
STUB(glGetCompressedTexImage)
STUB(glGetQueryObjectui64v)
STUB(glGetnCompressedTexImageARB)
STUB(glGetnTexImageARB)
STUB(glLineStipple)
STUB(glLogicOp)
STUB(glPixelTransferf)
STUB(glPixelZoom)
STUB(glPointParameteri)
STUB(glPointSize)
STUB(glPolygonMode)
STUB(glPolygonStipple)
STUB(glPrimitiveRestartIndexNV)
STUB(glProvokingVertexEXT)
STUB(glQueryCounter)
STUB(glReadnPixelsARB)
STUB(glReadnPixelsKHR)
STUB(glScissorIndexed)
STUB(glShadeModel)
STUB(glTexBuffer)
STUB(glTexEnvi)
STUB(glTexImage1D)
STUB(glTexImage2DMultisample)
STUB(glTexImage3DMultisample)
STUB(glTexStorage3DMultisample)
STUB(glTexSubImage1D)
STUB(glViewportIndexedf)
STUB(glWindowPos2i)

void tinyepoxy_init(const GLESv2Dispatch* gles, int version) {
    s_gles2 = gles;
    s_gles2_version = version;
    epoxy_glBindFramebufferEXT = gles->glBindFramebuffer;
    epoxy_glFramebufferTexture2DEXT = gles->glFramebufferTexture2D;
}

template<typename T> static void set_index_type(unsigned int *dst, void *src,
                                                int index, int base) {
    dst[index] = ((T *)src)[index] + base;
}

static void GL_APIENTRY glDrawElementsBaseVertex(GLenum mode,
                                                 GLsizei count,
                                                 GLenum type,
                                                 GLintptr indices,
                                                 GLint base) {
    size_t el_size;
    void (*set_index)(unsigned int*, void*, int, int);
    switch (type) {
        case GL_UNSIGNED_BYTE:
            el_size = 1;
            set_index = &set_index_type<unsigned char>;
            break;
        case GL_UNSIGNED_SHORT:
            el_size = 2;
            set_index = &set_index_type<unsigned short>;
            break;
        case GL_UNSIGNED_INT:
            el_size = 4;
            set_index = &set_index_type<unsigned int>;
            break;
        default:
            fprintf(stderr, "Unsupported type: %x\n", type);
            return;
    }
    size_t isize = el_size * count;
    size_t osize = sizeof(unsigned int) * count;
    GLint old_bid = 0;
    s_gles2->glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &old_bid);
    void* orig;
    if (old_bid) {
        orig = s_gles2->glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, indices,
                                         isize, GL_MAP_READ_BIT);
    } else {
        orig = (void *)indices;
    }
    if (orig == NULL) {
        fprintf(stderr, "Can't map\n");
        return;
    }

    unsigned int *tmp = (unsigned int *)malloc(osize);
    if (tmp == NULL) fatal("can't malloc", __func__);
    for (int i = 0; i < count; ++i) {
        set_index(tmp, orig, i, base);
    }
    if (old_bid) {
        s_gles2->glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        s_gles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    s_gles2->glDrawElements(mode, count, GL_UNSIGNED_INT, tmp);
    free(tmp);
    if (old_bid) {
        s_gles2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, old_bid);
    }
}

void GL_APIENTRY (* epoxy_glDrawElementsBaseVertex)(GLenum, GLsizei, GLenum,
                                                    GLintptr, GLint)
        = &glDrawElementsBaseVertex;

extern "C" {

bool epoxy_is_desktop_gl() {
    return false;
}

int epoxy_gl_version() {
    return s_gles2_version;
}

bool epoxy_has_gl_extension(const char* ext) {
    const char* all_ext = (const char *)epoxy_glGetString(GL_EXTENSIONS);
    const char* match = strstr(all_ext, ext);
    size_t len = strlen(ext);
    return (match && (match[len] == ' ' || match[len] == 0));
}

}
