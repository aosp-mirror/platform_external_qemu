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
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>

#include <string>
#include <vector>

#include <string.h>

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
    glMapBufferRangeAEMU = s_glMapBufferRangeAEMU;
    glUnmapBufferAEMU = s_glUnmapBufferAEMU;
    glFlushMappedBufferRangeAEMU = s_glFlushMappedBufferRangeAEMU;
    glCompressedTexImage2DOffsetAEMU = s_glCompressedTexImage2DOffsetAEMU;
    glCompressedTexSubImage2DOffsetAEMU = s_glCompressedTexSubImage2DOffsetAEMU;
    glTexImage2DOffsetAEMU = s_glTexImage2DOffsetAEMU;
    glTexSubImage2DOffsetAEMU = s_glTexSubImage2DOffsetAEMU;
    glGetUniformIndicesAEMU = s_glGetUniformIndicesAEMU;
    glVertexAttribIPointerDataAEMU = s_glVertexAttribIPointerDataAEMU;
    glVertexAttribIPointerOffsetAEMU = s_glVertexAttribIPointerOffsetAEMU;
    glTransformFeedbackVaryingsAEMU = s_glTransformFeedbackVaryingsAEMU;
    glTexImage3DOffsetAEMU = s_glTexImage3DOffsetAEMU;
    glTexSubImage3DOffsetAEMU = s_glTexSubImage3DOffsetAEMU;
    glCompressedTexImage3DOffsetAEMU = s_glCompressedTexImage3DOffsetAEMU;
    glCompressedTexSubImage3DOffsetAEMU = s_glCompressedTexSubImage3DOffsetAEMU;
    glDrawElementsInstancedOffsetAEMU = s_glDrawElementsInstancedOffsetAEMU;
    glDrawElementsInstancedDataAEMU = s_glDrawElementsInstancedDataAEMU;
    glReadPixelsOffsetAEMU = s_glReadPixelsOffsetAEMU;

    glCreateShaderProgramvAEMU = s_glCreateShaderProgramvAEMU;

    glDrawArraysIndirectDataAEMU = s_glDrawArraysIndirectDataAEMU;
    glDrawArraysIndirectOffsetAEMU = s_glDrawArraysIndirectOffsetAEMU;

    glDrawElementsIndirectDataAEMU = s_glDrawElementsIndirectDataAEMU;
    glDrawElementsIndirectOffsetAEMU = s_glDrawElementsIndirectOffsetAEMU;

    glFenceSyncAEMU = s_glFenceSyncAEMU;
    glClientWaitSyncAEMU = s_glClientWaitSyncAEMU;
    glWaitSyncAEMU = s_glWaitSyncAEMU;
    glIsSyncAEMU = s_glIsSyncAEMU;
    glGetSyncivAEMU = s_glGetSyncivAEMU;

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

void GLESv2Decoder::s_glMapBufferRangeAEMU(void* self, GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, void* mapped)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    if ((access & GL_MAP_READ_BIT) ||
        ((access & GL_MAP_WRITE_BIT) &&
         (!(access & GL_MAP_INVALIDATE_RANGE_BIT) &&
          !(access & GL_MAP_INVALIDATE_BUFFER_BIT)))) {
        void* gpu_ptr = ctx->glMapBufferRange(target, offset, length, access);
        memcpy(mapped, gpu_ptr, length);
        ctx->glUnmapBuffer(target);
    } else {
        // if writing while not wanting to preserve previous contents,
        // let |mapped| stay as garbage.
    }
}

void GLESv2Decoder::s_glUnmapBufferAEMU(void* self, GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, void* guest_buffer, GLboolean* out_res)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    if (access & GL_MAP_WRITE_BIT) {
        if (!guest_buffer) fprintf(stderr, "%s: error: wanted to write to a mapped buffer with NULL!\n", __FUNCTION__);
        void* gpu_ptr = ctx->glMapBufferRange(target, offset, length, access);
        if (!gpu_ptr) fprintf(stderr, "%s: could not get host gpu pointer!\n", __FUNCTION__);
        memcpy(gpu_ptr, guest_buffer, length);
        ctx->glUnmapBuffer(target);
    }
}

void GLESv2Decoder::s_glFlushMappedBufferRangeAEMU(void* self, GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, void* guest_buffer) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    if (!guest_buffer) fprintf(stderr, "%s: error: wanted to write to a mapped buffer with NULL!\n", __FUNCTION__);
    void* gpu_ptr = ctx->glMapBufferRange(target, offset, length, access);
    memcpy(gpu_ptr, guest_buffer, length);
    if (!gpu_ptr) fprintf(stderr, "%s: could not get host gpu pointer!\n", __FUNCTION__);
    // |offset| was the absolute offset into the mapping, so just flush offset 0.
    ctx->glFlushMappedBufferRange(target, 0, length);
    ctx->glUnmapBuffer(target);
}

void GLESv2Decoder::s_glCompressedTexImage2DOffsetAEMU(void* self, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
	ctx->glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, SafePointerFromUInt(offset));
}
void GLESv2Decoder::s_glCompressedTexSubImage2DOffsetAEMU(void* self, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
	ctx->glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, SafePointerFromUInt(offset));
}
void GLESv2Decoder::s_glTexImage2DOffsetAEMU(void* self, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
	ctx->glTexImage2D(target, level, internalformat, width, height, border, format, type, SafePointerFromUInt(offset));
}
void GLESv2Decoder::s_glTexSubImage2DOffsetAEMU(void* self, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
	ctx->glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, SafePointerFromUInt(offset));
}

static const char* const kNameDelimiter = ";";

static std::vector<std::string> sUnpackVarNames(GLsizei count, const char* packedNames) {
    std::vector<std::string> unpacked;
    GLsizei current = 0;

    while (current < count) {
        const char* delimPos = strstr(packedNames, kNameDelimiter);
        size_t nameLen = delimPos - packedNames;
        std::string next;
        next.resize(nameLen);
        memcpy(&next[0], packedNames, nameLen);
        unpacked.push_back(next);
        packedNames = delimPos + 1;
        current++;
    }

    return unpacked;
}

void GLESv2Decoder::s_glGetUniformIndicesAEMU(void* self, GLuint program, GLsizei uniformCount, const GLchar* packedNames, GLsizei packedLen, GLuint* uniformIndices) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;

    std::vector<std::string> unpacked = sUnpackVarNames(uniformCount, packedNames);

    GLchar** unpackedArray = new GLchar*[unpacked.size()];
    GLsizei i = 0;
    for (auto& elt : unpacked) {
        unpackedArray[i] = (GLchar*)&elt[0];
        i++;
    }

    ctx->glGetUniformIndices(program, uniformCount, (const GLchar**)unpackedArray, uniformIndices);

    delete [] unpackedArray;
}

void GLESv2Decoder::s_glVertexAttribIPointerDataAEMU(void *self, GLuint indx, GLint size, GLenum type, GLsizei stride, void * data, GLuint datalen)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;
    if (ctx->m_contextData != NULL) {
        ctx->m_contextData->storePointerData(indx, data, datalen);
        // note - the stride of the data is always zero when it comes out of the codec.
        // See gl2.attrib for the packing function call.
        ctx->glVertexAttribIPointer(indx, size, type, 0, ctx->m_contextData->pointerData(indx));
    }
}

void GLESv2Decoder::s_glVertexAttribIPointerOffsetAEMU(void *self, GLuint indx, GLint size, GLenum type, GLsizei stride, GLuint data)
{
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;
    ctx->glVertexAttribIPointer(indx, size, type, stride, SafePointerFromUInt(data));
}

void GLESv2Decoder::s_glTransformFeedbackVaryingsAEMU(void* self, GLuint program, GLsizei count, const char* packedVaryings, GLuint packedVaryingsLen, GLenum bufferMode) {

    GLESv2Decoder *ctx = (GLESv2Decoder *) self;

    std::vector<std::string> unpacked = sUnpackVarNames(count, packedVaryings);

    char** unpackedArray = new char*[unpacked.size()];
    GLsizei i = 0;
    for (auto& elt : unpacked) {
        unpackedArray[i] = &elt[0];
        i++;
    }

    ctx->glTransformFeedbackVaryings(program, count, (const char**)unpackedArray, bufferMode);

    delete [] unpackedArray;
}

void GLESv2Decoder::s_glTexImage3DOffsetAEMU(void* self, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;
	ctx->glTexImage3D(target, level, internalFormat, width, height, depth, border, format, type, SafePointerFromUInt(offset));
}
void GLESv2Decoder::s_glTexSubImage3DOffsetAEMU(void* self, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;
	ctx->glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, SafePointerFromUInt(offset));
}
void GLESv2Decoder::s_glCompressedTexImage3DOffsetAEMU(void* self, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;
	ctx->glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, SafePointerFromUInt(offset));
}
void GLESv2Decoder::s_glCompressedTexSubImage3DOffsetAEMU(void* self, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *) self;
	ctx->glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, SafePointerFromUInt(offset));
}

void GLESv2Decoder::s_glDrawElementsInstancedOffsetAEMU(void* self, GLenum mode, GLsizei count, GLenum type, GLuint offset, GLsizei primcount) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDrawElementsInstanced(mode, count, type, SafePointerFromUInt(offset), primcount);
}

void GLESv2Decoder::s_glDrawElementsInstancedDataAEMU(void* self, GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei primcount, GLsizei datalen) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDrawElementsInstanced(mode, count, type, indices, primcount);
}

void GLESv2Decoder::s_glReadPixelsOffsetAEMU(void* self, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glReadPixels(x, y, width, height, format, type, SafePointerFromUInt(offset));
}

GLuint GLESv2Decoder::s_glCreateShaderProgramvAEMU(void* self, GLenum type, GLsizei count, const char* packedStrings, GLuint packedLen) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    return ctx->glCreateShaderProgramv(type, 1, &packedStrings);
}

void GLESv2Decoder::s_glDrawArraysIndirectDataAEMU(void* self, GLenum mode, const void* indirect, GLuint datalen) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDrawArraysIndirect(mode, indirect);
}

void GLESv2Decoder::s_glDrawArraysIndirectOffsetAEMU(void* self, GLenum mode, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDrawArraysIndirect(mode, SafePointerFromUInt(offset));
}

void GLESv2Decoder::s_glDrawElementsIndirectDataAEMU(void* self, GLenum mode, GLenum type, const void* indirect, GLuint datalen) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDrawElementsIndirect(mode, type, indirect);
}

void GLESv2Decoder::s_glDrawElementsIndirectOffsetAEMU(void* self, GLenum mode, GLenum type, GLuint offset) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDrawElementsIndirect(mode, type, SafePointerFromUInt(offset));
}

uint64_t GLESv2Decoder::s_glFenceSyncAEMU(void* self, GLenum condition, GLbitfield flags) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    return (uint64_t)(uintptr_t)ctx->glFenceSync(condition, flags);
}

GLenum GLESv2Decoder::s_glClientWaitSyncAEMU(void* self, uint64_t wait_on, GLbitfield flags, GLuint64 timeout) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    return ctx->glClientWaitSync((GLsync)(uintptr_t)wait_on, flags, timeout);
}

void GLESv2Decoder::s_glWaitSyncAEMU(void* self, uint64_t wait_on, GLbitfield flags, GLuint64 timeout) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glWaitSync((GLsync)(uintptr_t)wait_on, flags, timeout);
}

void GLESv2Decoder::s_glDeleteSyncAEMU(void* self, uint64_t to_delete) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glDeleteSync((GLsync)(uintptr_t)to_delete);
}

GLboolean GLESv2Decoder::s_glIsSyncAEMU(void* self, uint64_t sync) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    return ctx->glIsSync((GLsync)(uintptr_t)sync);
}

void GLESv2Decoder::s_glGetSyncivAEMU(void* self, uint64_t sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values) {
    GLESv2Decoder *ctx = (GLESv2Decoder *)self;
    ctx->glGetSynciv((GLsync)(uintptr_t)sync, pname, bufSize, length, values);
}
