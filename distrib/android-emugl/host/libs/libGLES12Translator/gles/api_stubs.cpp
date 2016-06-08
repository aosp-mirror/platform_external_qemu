/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stddef.h>

// #include <GLES3/gl3.h>
// 
// #include "gles/macros.h"
// 
// STUB_APIENTRY(void, BeginQuery, GLenum target, GLuint id);
// STUB_APIENTRY(void, BeginQueryEXT, GLenum target, GLuint id);
// STUB_APIENTRY(void, BeginTransformFeedback, GLenum primitiveMode);
// STUB_APIENTRY(void, BindBufferBase, GLenum target, GLuint index, GLuint buffer);
// STUB_APIENTRY(void, BindBufferRange, GLenum target, GLuint index, GLuint buffer,
// 			  GLintptr offset, GLsizeiptr size);
// STUB_APIENTRY(void, BindSampler, GLuint unit, GLuint sampler);
// STUB_APIENTRY(void, BindTransformFeedback, GLenum target, GLuint id);
// STUB_APIENTRY(void, BindVertexArray, GLuint array);
// STUB_APIENTRY(void, BindVertexArrayOES, GLuint array);
// STUB_APIENTRY(void, BlitFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1,
// 			  GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
// 			  GLbitfield mask, GLenum filter);
// STUB_APIENTRY(void, ClearBufferfi, GLenum buffer, GLint drawbuffer,
// 			  GLfloat depth, GLint stencil);
// STUB_APIENTRY(void, ClearBufferfv, GLenum buffer, GLint drawbuffer,
// 			  const GLfloat* value);
// STUB_APIENTRY(void, ClearBufferiv, GLenum buffer, GLint drawbuffer,
// 			  const GLint* value);
// STUB_APIENTRY(void, ClearBufferuiv, GLenum buffer, GLint drawbuffer,
// 			  const GLuint* value);
// STUB_APIENTRY(GLenum, ClientWaitSync, GLsync sync, GLbitfield flags,
// 			  GLuint64 timeout);
// STUB_APIENTRY(void, CompressedTexImage3D, GLenum target, GLint level,
// 			  GLenum internalformat, GLsizei width, GLsizei height,
// 			  GLsizei depth, GLint border, GLsizei imageSize,
// 			  const GLvoid* data);
// STUB_APIENTRY(void, CompressedTexSubImage3D, GLenum target, GLint level,
// 			  GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width,
// 			  GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize,
// 			  const GLvoid* data);
// STUB_APIENTRY(void, CopyBufferSubData, GLenum readTarget, GLenum writeTarget,
// 			  GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
// STUB_APIENTRY(void, CopyTexSubImage3D, GLenum target, GLint level,
// 			  GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y,
// 			  GLsizei width, GLsizei height);
// STUB_APIENTRY(void, DeleteQueries, GLsizei n, const GLuint* ids);
// STUB_APIENTRY(void, DeleteSamplers, GLsizei count, const GLuint* samplers);
// STUB_APIENTRY(void, DeleteSync, GLsync sync);
// STUB_APIENTRY(void, DeleteTransformFeedbacks, GLsizei n, const GLuint* ids);
// STUB_APIENTRY(void, DeleteVertexArrays, GLsizei n, const GLuint* arrays);
// STUB_APIENTRY(void, DeleteVertexArraysOES, GLsizei n, const GLuint *arrays);
// STUB_APIENTRY(void, DiscardFramebufferEXT, GLenum target,
// 			  GLsizei numAttachments, const GLenum *attachments);
// STUB_APIENTRY(void, DrawArraysInstanced, GLenum mode, GLint first,
// 			  GLsizei count, GLsizei instanceCount);
// STUB_APIENTRY(void, DrawBuffers, GLsizei n, const GLenum* bufs);
// STUB_APIENTRY(void, DrawElementsInstanced, GLenum mode, GLsizei count,
// 			  GLenum type, const GLvoid* indices, GLsizei instanceCount);
// STUB_APIENTRY(void, DrawRangeElements, GLenum mode, GLuint start, GLuint end,
// 			  GLsizei count, GLenum type, const GLvoid* indices);
// STUB_APIENTRY(void, EndQuery, GLenum target);
// STUB_APIENTRY(void, EndQueryEXT, GLenum target);
// STUB_APIENTRY(void, EndTilingQCOM, GLbitfield preserveMask);
// STUB_APIENTRY(void, EndTransformFeedback);
// STUB_APIENTRY(GLsync, FenceSync, GLenum condition, GLbitfield flags);
// STUB_APIENTRY(void, FlushMappedBufferRange, GLenum target, GLintptr offset,
// 			  GLsizeiptr length);
// STUB_APIENTRY(void, FlushMappedBufferRangeEXT, GLenum target, GLintptr offset,
// 			  GLsizeiptr length);
// STUB_APIENTRY(void, FramebufferTexture2DMultisampleEXT, GLenum target,
// 			  GLenum attachment, GLenum textarget, GLuint texture, GLint level,
// 			  GLsizei samples);
// STUB_APIENTRY(void, FramebufferTexture2DMultisampleIMG, GLenum target,
// 			  GLenum attachment, GLenum textarget, GLuint texture, GLint level,
// 			  GLsizei samples);
// STUB_APIENTRY(void, FramebufferTextureLayer, GLenum target, GLenum attachment,
// 			  GLuint texture, GLint level, GLint layer);
// STUB_APIENTRY(void, GenQueries, GLsizei n, GLuint* ids);
// STUB_APIENTRY(void, GenQueriesEXT, GLsizei n, GLuint* ids);
// STUB_APIENTRY(void, GenSamplers, GLsizei count, GLuint* samplers);
// STUB_APIENTRY(void, GenTransformFeedbacks, GLsizei n, GLuint* ids);
// STUB_APIENTRY(void, GenVertexArrays, GLsizei n, GLuint* arrays);
// STUB_APIENTRY(void, GenVertexArraysOES, GLsizei n, GLuint *arrays);
// STUB_APIENTRY(void, GetActiveUniformBlockName, GLuint program,
// 			  GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length,
// 			  GLchar* uniformBlockName);
// STUB_APIENTRY(void, GetActiveUniformBlockiv, GLuint program,
// 			  GLuint uniformBlockIndex, GLenum pname, GLint* params);
// STUB_APIENTRY(void, GetActiveUniformsiv, GLuint program, GLsizei uniformCount,
// 			  const GLuint* uniformIndices, GLenum pname, GLint* params);
// STUB_APIENTRY(void, GetBufferParameteri64v, GLenum target, GLenum pname,
// 			  GLint64* params);
// STUB_APIENTRY(void, GetBufferPointerv, GLenum target, GLenum pname,
// 			  GLvoid** params);
// STUB_APIENTRY(GLint, GetFragDataLocation, GLuint program, const GLchar *name);
// STUB_APIENTRY(void, GetInteger64i_v, GLenum target, GLuint index,
// 			  GLint64* data);
// STUB_APIENTRY(void, GetInteger64v, GLenum pname, GLint64* params);
// STUB_APIENTRY(void, GetIntegeri_v, GLenum target, GLuint index, GLint* data);
// STUB_APIENTRY(void, GetInternalformativ, GLenum target, GLenum internalformat,
// 			  GLenum pname, GLsizei bufSize, GLint* params);
// STUB_APIENTRY(void, GetObjectLabelEXT, GLenum type, GLuint object,
// 			  GLsizei bufSize, GLsizei *length, GLchar *label);
// STUB_APIENTRY(void, GetProgramBinary, GLuint program, GLsizei bufSize,
// 			  GLsizei* length, GLenum* binaryFormat, GLvoid* binary);
// STUB_APIENTRY(void, GetQueryObjectiv, GLuint id, GLenum pname, GLint* params);
// STUB_APIENTRY(void, GetQueryObjectivEXT, GLuint id, GLenum pname,
// 			  GLint* params);
// STUB_APIENTRY(void, GetQueryObjectuiv, GLuint id, GLenum pname,
// 			  GLuint* params);
// STUB_APIENTRY(void, GetQueryObjectuivEXT, GLuint id, GLenum pname,
// 			  GLuint* params);
// STUB_APIENTRY(void, GetQueryObjecti64v, GLuint id, GLenum pname,
// 			  GLint64* params);
// STUB_APIENTRY(void, GetQueryObjecti64vEXT, GLuint id, GLenum pname,
// 			  GLint64* params);
// STUB_APIENTRY(void, GetQueryObjectui64v, GLuint id, GLenum pname,
// 			  GLuint64* params);
// STUB_APIENTRY(void, GetQueryObjectui64vEXT, GLuint id, GLenum pname,
// 			  GLuint64* params);
// STUB_APIENTRY(void, GetQueryiv, GLenum target, GLenum pname, GLint* params);
// STUB_APIENTRY(void, GetSamplerParameterfv, GLuint sampler, GLenum pname,
// 			  GLfloat* params);
// STUB_APIENTRY(void, GetSamplerParameteriv, GLuint sampler, GLenum pname,
// 			  GLint* params);
// STUB_APIENTRY(const GLubyte*, GetStringi, GLenum name, GLuint index);
// STUB_APIENTRY(void, GetSynciv, GLsync sync, GLenum pname, GLsizei bufSize,
// 			  GLsizei* length, GLint* values);
// STUB_APIENTRY(void, GetTexGenfvOES, GLenum coord, GLenum pname,
// 			  GLfloat *params);
// STUB_APIENTRY(void, GetTexGenivOES, GLenum coord, GLenum pname, GLint *params);
// STUB_APIENTRY(void, GetTexGenxvOES, GLenum coord, GLenum pname,
// 			  GLfixed *params);
// STUB_APIENTRY(void, GetTransformFeedbackVarying, GLuint program, GLuint index,
// 			  GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type,
// 			  GLchar* name);
// STUB_APIENTRY(GLuint, GetUniformBlockIndex, GLuint program,
// 			  const GLchar* uniformBlockName);
// STUB_APIENTRY(void, GetUniformIndices, GLuint program, GLsizei uniformCount,
// 			  const GLchar* const* uniformNames, GLuint* uniformIndices);
// STUB_APIENTRY(void, GetUniformuiv, GLuint program, GLint location,
// 			  GLuint* params);
// STUB_APIENTRY(void, GetVertexAttribIiv, GLuint index, GLenum pname,
// 			  GLint* params);
// STUB_APIENTRY(void, GetVertexAttribIuiv, GLuint index, GLenum pname,
// 			  GLuint* params);
// STUB_APIENTRY(void, InsertEventMarkerEXT, GLsizei length, const GLchar *marker);
// STUB_APIENTRY(void, InvalidateFramebuffer, GLenum target,
// 			  GLsizei numAttachments, const GLenum* attachments);
// STUB_APIENTRY(void, InvalidateSubFramebuffer, GLenum target,
// 			  GLsizei numAttachments, const GLenum* attachments, GLint x,
// 			  GLint y, GLsizei width, GLsizei height);
// STUB_APIENTRY(GLboolean, IsQuery, GLuint id);
// STUB_APIENTRY(GLboolean, IsSampler, GLuint sampler);
// STUB_APIENTRY(GLboolean, IsSync, GLsync sync);
// STUB_APIENTRY(GLboolean, IsTransformFeedback, GLuint id);
// STUB_APIENTRY(GLboolean, IsVertexArray, GLuint array);
// STUB_APIENTRY(void, LabelObjectEXT, GLenum type, GLuint object, GLsizei length,
// 			  const GLchar *label);
// STUB_APIENTRY(void, LogicOp, GLenum opcode);
// STUB_APIENTRY(void*, MapBufferOES, GLenum target, GLenum access);
// STUB_APIENTRY(void*, MapBufferRange, GLenum target, GLintptr offset,
// 			  GLsizeiptr length, GLbitfield access);
// STUB_APIENTRY(void*, MapBufferRangeEXT, GLenum target, GLintptr offset,
// 			  GLsizeiptr length, GLbitfield access);
// STUB_APIENTRY(void, MultiTexCoord4f, GLenum target, GLfloat s, GLfloat t,
// 			  GLfloat r, GLfloat q);
// STUB_APIENTRY(void, MultiTexCoord4x, GLenum target, GLfixed s, GLfixed t,
// 			  GLfixed r, GLfixed q);
// STUB_APIENTRY(void, MultiTexCoord4xOES, GLenum target, GLfixed s, GLfixed t,
// 			  GLfixed r, GLfixed q);
// STUB_APIENTRY(void, PauseTransformFeedback);
// STUB_APIENTRY(void, PopGroupMarkerEXT);
// STUB_APIENTRY(void, ProgramBinary, GLuint program, GLenum binaryFormat,
// 			  const GLvoid* binary, GLsizei length);
// STUB_APIENTRY(void, ProgramParameteri, GLuint program, GLenum pname,
// 			  GLint value);
// STUB_APIENTRY(void, PushGroupMarkerEXT, GLsizei length, const GLchar *marker);
// STUB_APIENTRY(GLbitfield, QueryMatrixxOES, GLfixed mantissa[16],
// 			  GLint exponent[16]);
// STUB_APIENTRY(void, ReadBuffer, GLenum mode);
// STUB_APIENTRY(void, RenderbufferStorageMultisample, GLenum target,
// 			  GLsizei samples, GLenum internalformat, GLsizei width,
// 			  GLsizei height);
// STUB_APIENTRY(void, RenderbufferStorageMultisampleEXT, GLenum target,
// 			  GLsizei samples, GLenum internalformat, GLsizei width,
// 			  GLsizei height);
// STUB_APIENTRY(void, RenderbufferStorageMultisampleIMG, GLenum target,
// 			  GLsizei samples, GLenum internalformat, GLsizei width,
// 			  GLsizei height);
// STUB_APIENTRY(void, ResumeTransformFeedback);
// STUB_APIENTRY(void, SamplerParameterf, GLuint sampler, GLenum pname,
// 			  GLfloat param);
// STUB_APIENTRY(void, SamplerParameterfv, GLuint sampler, GLenum pname,
// 			  const GLfloat* param);
// STUB_APIENTRY(void, SamplerParameteri, GLuint sampler, GLenum pname,
// 			  GLint param);
// STUB_APIENTRY(void, SamplerParameteriv, GLuint sampler, GLenum pname,
// 			  const GLint* param);
// STUB_APIENTRY(void, ShaderBinary, GLsizei n, const GLuint *shaders,
// 			  GLenum binaryformat, const GLvoid *binary, GLsizei length);
// STUB_APIENTRY(void, StartTilingQCOM, GLuint x, GLuint y, GLuint width,
// 			  GLuint height, GLbitfield preserveMask);
// STUB_APIENTRY(void, TexGenfOES, GLenum coord, GLenum pname, GLfloat param);
// STUB_APIENTRY(void, TexGenfvOES, GLenum coord, GLenum pname,
// 			  const GLfloat *params);
// STUB_APIENTRY(void, TexImage3D, GLenum target, GLint level,
// 			  GLint internalformat, GLsizei width, GLsizei height,
// 			  GLsizei depth, GLint border, GLenum format, GLenum type,
// 			  const GLvoid* pixels);
// STUB_APIENTRY(void, TexStorage2D, GLenum target, GLsizei levels,
// 			  GLenum internalformat, GLsizei width, GLsizei height);
// STUB_APIENTRY(void, TexStorage2DEXT, GLenum target, GLsizei levels,
// 			  GLenum internalformat, GLsizei width, GLsizei height);
// STUB_APIENTRY(void, TexStorage3D, GLenum target, GLsizei levels,
// 			  GLenum internalformat, GLsizei width, GLsizei height,
// 			  GLsizei depth);
// STUB_APIENTRY(void, TexSubImage3D, GLenum target, GLint level, GLint xoffset,
// 			  GLint yoffset, GLint zoffset, GLsizei width, GLsizei height,
// 			  GLsizei depth, GLenum format, GLenum type, const GLvoid* pixels);
// STUB_APIENTRY(void, TransformFeedbackVaryings, GLuint program, GLsizei count,
// 			  const GLchar* const* varyings, GLenum bufferMode);
// STUB_APIENTRY(void, Uniform1ui, GLint location, GLuint v0);
// STUB_APIENTRY(void, Uniform1uiv, GLint location, GLsizei count,
// 			  const GLuint* value);
// STUB_APIENTRY(void, Uniform2ui, GLint location, GLuint v0, GLuint v1);
// STUB_APIENTRY(void, Uniform2uiv, GLint location, GLsizei count,
// 			  const GLuint* value);
// STUB_APIENTRY(void, Uniform3ui, GLint location, GLuint v0, GLuint v1,
// 			  GLuint v2);
// STUB_APIENTRY(void, Uniform3uiv, GLint location, GLsizei count,
// 			  const GLuint* value);
// STUB_APIENTRY(void, Uniform4ui, GLint location, GLuint v0, GLuint v1,
// 			  GLuint v2, GLuint v3);
// STUB_APIENTRY(void, Uniform4uiv, GLint location, GLsizei count,
// 			  const GLuint* value);
// STUB_APIENTRY(void, UniformBlockBinding, GLuint program,
// 			  GLuint uniformBlockIndex, GLuint uniformBlockBinding);
// STUB_APIENTRY(void, UniformMatrix2x3fv, GLint location, GLsizei count,
// 			  GLboolean transpose, const GLfloat* value);
// STUB_APIENTRY(void, UniformMatrix2x4fv, GLint location, GLsizei count,
// 			  GLboolean transpose, const GLfloat* value);
// STUB_APIENTRY(void, UniformMatrix3x2fv, GLint location, GLsizei count,
// 			  GLboolean transpose, const GLfloat* value);
// STUB_APIENTRY(void, UniformMatrix3x4fv, GLint location, GLsizei count,
// 			  GLboolean transpose, const GLfloat* value);
// STUB_APIENTRY(void, UniformMatrix4x2fv, GLint location, GLsizei count,
// 			  GLboolean transpose, const GLfloat* value);
// STUB_APIENTRY(void, UniformMatrix4x3fv, GLint location, GLsizei count,
// 			  GLboolean transpose, const GLfloat* value);
// STUB_APIENTRY(GLboolean, UnmapBuffer, GLenum target);
// STUB_APIENTRY(GLboolean, UnmapBufferOES, GLenum target);
// STUB_APIENTRY(void, VertexAttribDivisor, GLuint index, GLuint divisor);
// STUB_APIENTRY(void, VertexAttribI4i, GLuint index, GLint x, GLint y, GLint z,
// 			  GLint w);
// STUB_APIENTRY(void, VertexAttribI4iv, GLuint index, const GLint* v);
// STUB_APIENTRY(void, VertexAttribI4ui, GLuint index, GLuint x, GLuint y,
// 			  GLuint z, GLuint w);
// STUB_APIENTRY(void, VertexAttribI4uiv, GLuint index, const GLuint* v);
// STUB_APIENTRY(void, VertexAttribIPointer, GLuint index, GLint size, GLenum type,
// 			  GLsizei stride, const GLvoid* pointer);
// STUB_APIENTRY(void, WaitSync, GLsync sync, GLbitfield flags, GLuint64 timeout);
