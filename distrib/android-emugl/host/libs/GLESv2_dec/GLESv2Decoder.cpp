/*
* Copyright 2011 The Android Open Source Project
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

#include "GLESv2Decoder.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

static inline void* SafePointerFromUInt(GLuint value) {
  return (void*)(uintptr_t)value;
}

GLESv2Decoder::GLESv2Decoder()
{
    m_contextData = NULL;
    m_GL2library = NULL;
}

GLESv2Decoder::~GLESv2Decoder()
{
    delete m_GL2library;
}

void *GLESv2Decoder::s_getProc(const char *name, void *userData)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *) userData;

    if (ctx == NULL || ctx->m_GL2library == NULL) {
        return NULL;
    }

    void *func = NULL;
#ifdef USE_EGL_GETPROCADDRESS
    func = (void *) eglGetProcAddress(name);
#endif
    if (func == NULL) {
        func = (void *) ctx->m_GL2library->findSymbol(name);
    }
    return func;
}

int GLESv2Decoder::initGL(get_proc_func_t getProcFunc, void *getProcFuncData)
{
    this->initDispatchByName(getProcFunc, getProcFuncData);

    glGetCompressedTextureFormats = s_glGetCompressedTextureFormats;
    glVertexAttribPointerData = s_glVertexAttribPointerData;
    glVertexAttribPointerOffset = s_glVertexAttribPointerOffset;

    glDrawElementsOffset = s_glDrawElementsOffset;
    glDrawElementsData = s_glDrawElementsData;
    glShaderString = s_glShaderString;
    glFinishRoundTrip = s_glFinishRoundTrip;
    glVertexAttributes = s_glVertexAttributes;
    return 0;

}

int GLESv2Decoder::s_glFinishRoundTrip(void *self)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glFinish();
    return 0;
}

void GLESv2Decoder::s_glGetCompressedTextureFormats(void *self, int count, GLint *formats)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;

    int nFormats;
    ctx->glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &nFormats);
    if (nFormats > count) {
        fprintf(stderr, "%s: GetCompressedTextureFormats: The requested number of formats does not match the number that is reported by OpenGL\n", __FUNCTION__);
    } else {
        ctx->glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, formats);
    }
}

void GLESv2Decoder::s_glVertexAttribPointerData(void *self, GLuint indx, GLint size, GLenum type,
                                             GLboolean normalized, GLsizei stride,  void * data, GLuint datalen)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;
    if (ctx->m_contextData != NULL) {
        ctx->m_contextData->storePointerData(indx, data, datalen);
        // note - the stride of the data is always zero when it comes out of the codec.
        // See gl2.attrib for the packing function call.
        ctx->glVertexAttribPointer(indx, size, type, normalized, 0, ctx->m_contextData->pointerData(indx));
    }
}

void GLESv2Decoder::s_glVertexAttribPointerOffset(void *self, GLuint indx, GLint size, GLenum type,
                                               GLboolean normalized, GLsizei stride,  GLuint data)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;
    ctx->glVertexAttribPointer(indx, size, type, normalized, stride, SafePointerFromUInt(data));
}


void GLESv2Decoder::s_glDrawElementsData(void *self, GLenum mode, GLsizei count, GLenum type, void * data, GLuint datalen)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDrawElements(mode, count, type, data);
}


void GLESv2Decoder::s_glDrawElementsOffset(void *self, GLenum mode, GLsizei count, GLenum type, GLuint offset)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDrawElements(mode, count, type, SafePointerFromUInt(offset));
}

void GLESv2Decoder::s_glShaderString(void *self, GLuint shader, const GLchar* string, GLsizei len)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glShaderSource(shader, 1, &string, NULL);
}

#define DEBUG 0

#if DEBUG
#define DPRINT(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define DPRINT(...)
#endif

#define DUMPBUF(name, typ) do { \
    DPRINT("%s: %s {\n", __FUNCTION__, #name); \
    for (uint32_t i = 0; i < num_##name / sizeof(typ); i++) { DPRINT("%s:    %s[%u]=%lld\n", __FUNCTION__, #name, i, name[i]); } \
    DPRINT("%s: }\n", __FUNCTION__); } while(0)

void  GLESv2Decoder::s_glVertexAttributes(void* self,
        GLuint* attribarrays, uint32_t num_attribarrays,
        GLint* firsts, uint32_t num_firsts,
        GLsizei* counts, uint32_t num_counts,
        GLuint* bufs, uint32_t num_bufs,
        GLint* sizes, uint32_t num_sizes,
        GLenum* types, uint32_t num_types,
        GLboolean* normalizeds, uint32_t num_normalizeds,
        GLsizei* strides, uint32_t num_strides,
        void* _ptrs, uint32_t num_ptrs,
        uint32_t* offsets, uint32_t num_offsets,
        uint32_t* datalens, uint32_t num_datalens,
        GLboolean* disables, uint32_t num_disables,
        GLuint lastBoundVbo)
{
    void** ptrs = (void**)_ptrs;

    DPRINT("%s: call. nums: attribarrays %u fsts %u counts %u bufs %u sizes %u types %u norms %u strides %u ptrdata %u offsets %u datalens %u disables %u\n", __FUNCTION__,
            num_attribarrays,
            num_firsts,
            num_counts,
            num_bufs,
            num_sizes,

            num_types,
            num_normalizeds,
            num_strides,
            num_ptrs,

            num_offsets,

            num_datalens,
            num_disables
            );
    DUMPBUF(attribarrays, GLuint);
    DUMPBUF(firsts, GLint);
    DUMPBUF(counts, GLsizei);
    DUMPBUF(bufs, GLuint);
    DUMPBUF(sizes, GLint);
    DUMPBUF(types, GLenum);
    DUMPBUF(normalizeds, GLboolean);
    DUMPBUF(strides, GLsizei);
    DUMPBUF(ptrs, uint32_t);
    DUMPBUF(offsets, GLuint);
    DUMPBUF(datalens, uint32_t);
    DUMPBUF(disables, GLboolean);

    DPRINT("%s: lastBoundVbo=%u\n", __FUNCTION__, lastBoundVbo);

    GLESv2Decoder *ctx = (GLESv2Decoder *)self;

   GLint first = firsts[0];
   GLsizei count = counts[0];
   GLuint lastBoundVbo_working = lastBoundVbo;

   GLuint attribArray;
   GLuint bufferObject;
   GLint size;
   GLenum type;
   GLboolean normalized;

   int stride;
   unsigned char* curr_data = (unsigned char*)_ptrs;
   GLuint offset;
   unsigned int datalen;
   GLboolean disable;

   for(uint32_t i = 0; i < num_attribarrays / sizeof(GLuint); i++) {
       attribArray = attribarrays[i];
       bufferObject = bufs[i];
       size = sizes[i];
       type = types[i];
       normalized = normalizeds[i];

       stride = strides[i];
       offset = offsets[i];
       datalen = datalens[i];
       disable = disables[i];

       if (!disable) {
           if (lastBoundVbo_working != bufferObject) {
               ctx->glBindBuffer(GL_ARRAY_BUFFER, bufferObject);
               DPRINT("%s: would glBindBuffer(GL_ARRAY_BUFFER, %d)\n", __FUNCTION__, bufferObject);
               lastBoundVbo_working = bufferObject;
           }
           DPRINT("%s: would glEnableVertexAttribArray(%d)\n", __FUNCTION__, attribArray);
           ctx->glEnableVertexAttribArray(attribArray);

           if (bufferObject == 0) {
               DPRINT("%s: would glVertexAttribPointerData(%p, %u, %d, %u, %d, %d, %p, %u)\n",
                       __FUNCTION__, ctx,
                       attribArray, size, type, normalized, stride,
                       curr_data, datalen);
               ctx->glVertexAttribPointerData(ctx, 
                       attribArray, size, type, normalized, stride,
                       curr_data, datalen);
               curr_data += datalen;
               DPRINT("%s: done with glVertexAttribPointerData\n", __FUNCTION__);
           } else {
               DPRINT("%s: would glVertexAttribPointerOffset(%p, %u, %d, %u, %d, %d, %u)\n",
                       __FUNCTION__, ctx,
                       attribArray, size, type, normalized, stride,
                       offset);
               ctx->glVertexAttribPointerOffset(ctx, 
                       attribArray, size, type, normalized, stride,
                       offset);
               DPRINT("%s: done with glVertexAttribPointerOffset\n", __FUNCTION__);
           }
       } else {
           DPRINT("%s: would glDisableVertexAttribArray(%d)\n", __FUNCTION__, attribArray);
           ctx->glDisableVertexAttribArray(attribArray);
       }
   }

   if (lastBoundVbo_working != lastBoundVbo) {
       DPRINT("%s: would glBindBuffer(GL_ARRAY_BUFFER, %d)\n", __FUNCTION__, lastBoundVbo);
       ctx->glBindBuffer(GL_ARRAY_BUFFER, lastBoundVbo);
   }

   DPRINT("%s: done\n", __FUNCTION__);


}
