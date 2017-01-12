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

#ifndef _GL2_DECODER_H_
#define _GL2_DECODER_H_

#include "gles2_dec.h"
#include "GLDecoderContextData.h"
#include "emugl/common/shared_library.h"

class GLESv2Decoder : public gles2_decoder_context_t
{
public:
    typedef void *(*get_proc_func_t)(const char *name, void *userData);
    GLESv2Decoder();
    ~GLESv2Decoder();
    int initGL(get_proc_func_t getProcFunc, void *getProcFuncData);
    void setContextData(GLDecoderContextData *contextData) { m_contextData = contextData; }
private:
    GLDecoderContextData *m_contextData;
    emugl::SharedLibrary* m_GL2library;

    static void *s_getProc(const char *name, void *userData);
    static void gles2_APIENTRY s_glGetCompressedTextureFormats(void *self, int count, GLint *formats);
    static void gles2_APIENTRY s_glVertexAttribPointerData(void *self, GLuint indx, GLint size, GLenum type,
                                      GLboolean normalized, GLsizei stride,  void * data, GLuint datalen);
    static void gles2_APIENTRY s_glVertexAttribPointerOffset(void *self, GLuint indx, GLint size, GLenum type,
                                        GLboolean normalized, GLsizei stride,  GLuint offset);

    static void gles2_APIENTRY s_glDrawElementsOffset(void *self, GLenum mode, GLsizei count, GLenum type, GLuint offset);
    static void gles2_APIENTRY s_glDrawElementsData(void *self, GLenum mode, GLsizei count, GLenum type, void * data, GLuint datalen);
    static void gles2_APIENTRY s_glShaderString(void *self, GLuint shader, const GLchar* string, GLsizei len);
    static int  gles2_APIENTRY s_glFinishRoundTrip(void *self);

    static void gles2_APIENTRY s_glMapBufferRangeAEMU(void* self, GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, void* mapped);
    static void gles2_APIENTRY s_glUnmapBufferAEMU(void* self, GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, void* guest_buffer, GLboolean* out_res);
    static void gles2_APIENTRY s_glFlushMappedBufferRangeAEMU(void* self, GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, void* guest_buffer);

    static void gles2_APIENTRY s_glCompressedTexImage2DOffsetAEMU(void* self, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, GLuint offset);
    static void gles2_APIENTRY s_glCompressedTexSubImage2DOffsetAEMU(void* self, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, GLuint offset);
    static void gles2_APIENTRY s_glTexImage2DOffsetAEMU(void* self, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLuint offset);
    static void gles2_APIENTRY s_glTexSubImage2DOffsetAEMU(void* self, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLuint offset);

    static void gles2_APIENTRY s_glGetUniformIndicesAEMU(void* self, GLuint program, GLsizei uniformCount, const GLchar* packedNames, GLsizei packedLen, GLuint* uniformIndices);
    static void gles2_APIENTRY s_glVertexAttribIPointerDataAEMU(void *self, GLuint indx, GLint size, GLenum type, GLsizei stride, void * data, GLuint datalen);
    static void gles2_APIENTRY s_glVertexAttribIPointerOffsetAEMU(void *self, GLuint indx, GLint size, GLenum type, GLsizei stride,  GLuint offset);

    static void gles2_APIENTRY s_glTransformFeedbackVaryingsAEMU(void* self, GLuint program, GLsizei count, const char* packedVaryings, GLuint packedVaryingsLen, GLenum bufferMode);

    static void gles2_APIENTRY s_glTexImage3DOffsetAEMU(void* self, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, GLuint offset);
    static void gles2_APIENTRY s_glTexSubImage3DOffsetAEMU(void* self, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLuint offset);
    static void gles2_APIENTRY s_glCompressedTexImage3DOffsetAEMU(void* self, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, GLuint offset);
    static void gles2_APIENTRY s_glCompressedTexSubImage3DOffsetAEMU(void* self, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, GLuint offset);
    static void gles2_APIENTRY s_glDrawElementsInstancedOffsetAEMU(void* self, GLenum mode, GLsizei count, GLenum type, GLuint offset, GLsizei primcount);
    static void gles2_APIENTRY s_glDrawElementsInstancedDataAEMU(void* self, GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei primcount, GLsizei datalen);

    static void gles2_APIENTRY s_glReadPixelsOffsetAEMU(void* self, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLuint offset);

    static GLuint gles2_APIENTRY s_glCreateShaderProgramvAEMU(void* self, GLenum type, GLsizei count, const char* packedStrings, GLuint packedLen);

    static void gles2_APIENTRY s_glDrawArraysIndirectDataAEMU(void* self, GLenum mode, const void* indirect, GLuint datalen);
    static void gles2_APIENTRY s_glDrawArraysIndirectOffsetAEMU(void* self, GLenum mode, GLuint offset);

    static void gles2_APIENTRY s_glDrawElementsIndirectDataAEMU(void* self, GLenum mode, GLenum type, const void* indirect, GLuint datalen);
    static void gles2_APIENTRY s_glDrawElementsIndirectOffsetAEMU(void* self, GLenum mode, GLenum type, GLuint offset);

    static uint64_t gles2_APIENTRY s_glFenceSyncAEMU(void* self, GLenum condition, GLbitfield flags);
    static GLenum gles2_APIENTRY s_glClientWaitSyncAEMU(void* self, uint64_t wait_on, GLbitfield flags, GLuint64 timeout);
    static void gles2_APIENTRY s_glWaitSyncAEMU(void* self, uint64_t wait_on, GLbitfield flags, GLuint64 timeout);
    static void gles2_APIENTRY s_glDeleteSyncAEMU(void* self, uint64_t to_delete);
    static GLboolean gles2_APIENTRY s_glIsSyncAEMU(void* self, uint64_t sync);
    static void gles2_APIENTRY s_glGetSyncivAEMU(void* self, uint64_t sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);

};
#endif
