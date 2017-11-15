#ifndef ASYNC_GL_FUNCTIONS_H
#define ASYNC_GL_FUNCTIONS_H

#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

typedef void* voidptr;

  // X(void, glBindAttribLocation, (GLuint program, GLuint index, const GLchar* name), (program, index, name))
  // X(void, glVertexAttribPointerWithDataSize, (GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr, GLsizei dataSize), (indx, size, type, normalized, stride, ptr, dataSize))
  
// needs custom handling.
#define LIST_CUSTOM_ASYNC_GL_FUNCTIONS(X) \
  X(void, glBufferData, (GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage), (target, size, data, usage)) \
  X(void, glBufferSubData, (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data), (target, offset, size, data)) \
  X(void, glVertexAttrib1fv, (GLuint indx, const GLfloat* values), (indx, values)) \
  X(void, glVertexAttrib2fv, (GLuint indx, const GLfloat* values), (indx, values)) \
  X(void, glVertexAttrib3fv, (GLuint indx, const GLfloat* values), (indx, values)) \
  X(void, glVertexAttrib4fv, (GLuint indx, const GLfloat* values), (indx, values)) \
  X(void, glUniform1fv, (GLint location, GLsizei count, const GLfloat* v), (location, count, v)) \
  X(void, glUniform1iv, (GLint location, GLsizei count, const GLint* v), (location, count, v)) \
  X(void, glUniform2fv, (GLint location, GLsizei count, const GLfloat* v), (location, count, v)) \
  X(void, glUniform2iv, (GLint location, GLsizei count, const GLint* v), (location, count, v)) \
  X(void, glUniform3fv, (GLint location, GLsizei count, const GLfloat* v), (location, count, v)) \
  X(void, glUniform3iv, (GLint location, GLsizei count, const GLint* v), (location, count, v)) \
  X(void, glUniform4fv, (GLint location, GLsizei count, const GLfloat* v), (location, count, v)) \
  X(void, glUniform4iv, (GLint location, GLsizei count, const GLint* v), (location, count, v)) \
  X(void, glUniformMatrix2fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (location, count, transpose, value)) \
  X(void, glUniformMatrix3fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (location, count, transpose, value)) \
  X(void, glUniformMatrix4fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (location, count, transpose, value)) \

// Only plain values, non pointers passed.
#define LIST_ASYNC_GL_FUNCTIONS(X) \
  X(void, glDrawElements, (GLenum mode, GLsizei count, GLenum type, const GLvoid * indices), (mode, count, type, indices)) \
  X(void, glActiveTexture, (GLenum texture), (texture)) \
  X(void, glBindBuffer, (GLenum target, GLuint buffer), (target, buffer)) \
  X(void, glBindTexture, (GLenum target, GLuint texture), (target, texture)) \
  X(void, glBlendFunc, (GLenum sfactor, GLenum dfactor), (sfactor, dfactor)) \
  X(void, glBlendEquation, (GLenum mode), (mode)) \
  X(void, glBlendEquationSeparate, (GLenum modeRGB, GLenum modeAlpha), (modeRGB, modeAlpha)) \
  X(void, glBlendFuncSeparate, (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha), (srcRGB, dstRGB, srcAlpha, dstAlpha)) \
  X(void, glClear, (GLbitfield mask), (mask)) \
  X(void, glClearColor, (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha), (red, green, blue, alpha)) \
  X(void, glClearDepth, (GLclampd depth), (depth)) \
  X(void, glClearDepthf, (GLclampf depth), (depth)) \
  X(void, glClearStencil, (GLint s), (s)) \
  X(void, glColorMask, (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha), (red, green, blue, alpha)) \
  X(void, glCopyTexImage2D, (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border), (target, level, internalFormat, x, y, width, height, border)) \
  X(void, glCopyTexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height), (target, level, xoffset, yoffset, x, y, width, height)) \
  X(void, glCullFace, (GLenum mode), (mode)) \
  X(void, glDepthFunc, (GLenum func), (func)) \
  X(void, glDepthMask, (GLboolean flag), (flag)) \
  X(void, glDepthRange, (GLclampd zNear, GLclampd zFar), (zNear, zFar)) \
  X(void, glDepthRangef, (GLclampf zNear, GLclampf zFar), (zNear, zFar)) \
  X(void, glDisable, (GLenum cap), (cap)) \
  X(void, glDrawArrays, (GLenum mode, GLint first, GLsizei count), (mode, first, count)) \
  X(void, glEnable, (GLenum cap), (cap)) \
  X(void, glFinish, (), ()) \
  X(void, glFlush, (), ()) \
  X(void, glFrontFace, (GLenum mode), (mode)) \
  X(void, glTexParameterf, (GLenum target, GLenum pname, GLfloat param), (target, pname, param)) \
  X(void, glHint, (GLenum target, GLenum mode), (target, mode)) \
  X(void, glLineWidth, (GLfloat width), (width)) \
  X(void, glPolygonOffset, (GLfloat factor, GLfloat units), (factor, units)) \
  X(void, glPixelStorei, (GLenum pname, GLint param), (pname, param)) \
  X(void, glRenderbufferStorageMultisample, (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height), (target, samples, internalformat, width, height)) \
  X(void, glSampleCoverage, (GLclampf value, GLboolean invert), (value, invert)) \
  X(void, glScissor, (GLint x, GLint y, GLsizei width, GLsizei height), (x, y, width, height)) \
  X(void, glStencilFunc, (GLenum func, GLint ref, GLuint mask), (func, ref, mask)) \
  X(void, glStencilMask, (GLuint mask), (mask)) \
  X(void, glStencilOp, (GLenum fail, GLenum zfail, GLenum zpass), (fail, zfail, zpass)) \
  X(void, glTexParameteri, (GLenum target, GLenum pname, GLint param), (target, pname, param)) \
  X(void, glViewport, (GLint x, GLint y, GLsizei width, GLsizei height), (x, y, width, height)) \
  X(void, glPushAttrib, (GLbitfield mask), (mask)) \
  X(void, glPushClientAttrib, (GLbitfield mask), (mask)) \
  X(void, glPopAttrib, (), ()) \
  X(void, glPopClientAttrib, (), ()) \
  X(void, glAlphaFunc, (GLenum func, GLclampf ref), (func, ref)) \
  X(void, glBegin, (GLenum mode), (mode)) \
  X(void, glClientActiveTexture, (GLenum texture), (texture)) \
  X(void, glColor4d, (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha), (red, green, blue, alpha)) \
  X(void, glColor4f, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha), (red, green, blue, alpha)) \
  X(void, glColor4ub, (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha), (red, green, blue, alpha)) \
  X(void, glDisableClientState, (GLenum array), (array)) \
  X(void, glEnableClientState, (GLenum array), (array)) \
  X(void, glEnd, (), ()) \
  X(void, glFogf, (GLenum pname, GLfloat param), (pname, param)) \
  X(void, glFrustum, (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar), (left, right, bottom, top, zNear, zFar)) \
  X(void, glLightf, (GLenum light, GLenum pname, GLfloat param), (light, pname, param)) \
  X(void, glLightModelf, (GLenum pname, GLfloat param), (pname, param)) \
  X(void, glLoadIdentity, (), ()) \
  X(void, glLogicOp, (GLenum opcode), (opcode)) \
  X(void, glMaterialf, (GLenum face, GLenum pname, GLfloat param), (face, pname, param)) \
  X(void, glMultiTexCoord4f, (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q), (target, s, t, r, q)) \
  X(void, glNormal3f, (GLfloat nx, GLfloat ny, GLfloat nz), (nx, ny, nz)) \
  X(void, glOrtho, (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar), (left, right, bottom, top, zNear, zFar)) \
  X(void, glPointParameterf, (GLenum param, GLfloat value), (param, value)) \
  X(void, glPointSize, (GLfloat size), (size)) \
  X(void, glRotatef, (GLfloat angle, GLfloat x, GLfloat y, GLfloat z), (angle, x, y, z)) \
  X(void, glScalef, (GLfloat x, GLfloat y, GLfloat z), (x, y, z)) \
  X(void, glTexEnvf, (GLenum target, GLenum pname, GLfloat param), (target, pname, param)) \
  X(void, glMatrixMode, (GLenum mode), (mode)) \
  X(void, glPopMatrix, (), ()) \
  X(void, glPushMatrix, (), ()) \
  X(void, glShadeModel, (GLenum mode), (mode)) \
  X(void, glTexEnvi, (GLenum target, GLenum pname, GLint param), (target, pname, param)) \
  X(void, glTranslatef, (GLfloat x, GLfloat y, GLfloat z), (x, y, z)) \
  X(void, glCurrentPaletteMatrixARB, (GLint index), (index)) \
  X(void, glTexGenf, (GLenum coord, GLenum pname, GLfloat param), (coord, pname, param)) \
  X(void, glTexGeni, (GLenum coord, GLenum pname, GLint param), (coord, pname, param)) \
  X(void, glBlendColor, (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha), (red, green, blue, alpha)) \
  X(void, glStencilFuncSeparate, (GLenum face, GLenum func, GLint ref, GLuint mask), (face, func, ref, mask)) \
  X(void, glStencilMaskSeparate, (GLenum face, GLuint mask), (face, mask)) \
  X(void, glStencilOpSeparate, (GLenum face, GLenum fail, GLenum zfail, GLenum zpass), (face, fail, zfail, zpass)) \
  X(void, glVertexAttrib1f, (GLuint indx, GLfloat x), (indx, x)) \
  X(void, glVertexAttrib2f, (GLuint indx, GLfloat x, GLfloat y), (indx, x, y)) \
  X(void, glVertexAttrib3f, (GLuint indx, GLfloat x, GLfloat y, GLfloat z), (indx, x, y, z)) \
  X(void, glVertexAttrib4f, (GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w), (indx, x, y, z, w)) \
  X(void, glDisableVertexAttribArray, (GLuint index), (index)) \
  X(void, glEnableVertexAttribArray, (GLuint index), (index)) \
  X(void, glUniform1f, (GLint location, GLfloat x), (location, x)) \
  X(void, glUniform1i, (GLint location, GLint x), (location, x)) \
  X(void, glUniform2f, (GLint location, GLfloat x, GLfloat y), (location, x, y)) \
  X(void, glUniform2i, (GLint location, GLint x, GLint y), (location, x, y)) \
  X(void, glUniform3f, (GLint location, GLfloat x, GLfloat y, GLfloat z), (location, x, y, z)) \
  X(void, glUniform3i, (GLint location, GLint x, GLint y, GLint z), (location, x, y, z)) \
  X(void, glUniform4f, (GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w), (location, x, y, z, w)) \
  X(void, glUniform4i, (GLint location, GLint x, GLint y, GLint z, GLint w), (location, x, y, z, w)) \
  X(void, glAttachShader, (GLuint program, GLuint shader), (program, shader)) \
  X(void, glCompileShader, (GLuint shader), (shader)) \
  X(void, glDetachShader, (GLuint program, GLuint shader), (program, shader)) \
  X(void, glLinkProgram, (GLuint program), (program)) \
  X(void, glUseProgram, (GLuint program), (program)) \
  X(void, glValidateProgram, (GLuint program), (program)) \
  X(void, glBindFramebuffer, (GLenum target, GLuint framebuffer), (target, framebuffer)) \
  X(void, glFramebufferTexture2D, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level), (target, attachment, textarget, texture, level)) \
  X(void, glBindRenderbuffer, (GLenum target, GLuint renderbuffer), (target, renderbuffer)) \
  X(void, glRenderbufferStorage, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height), (target, internalformat, width, height)) \
  X(void, glFramebufferRenderbuffer, (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer), (target, attachment, renderbuffertarget, renderbuffer)) \
  X(void, glReleaseShaderCompiler, (), ()) \
  X(void, glBindVertexArray, (GLuint array), (array)) \
  X(void, glFlushMappedBufferRange, (GLenum target, GLintptr offset, GLsizeiptr length), (target, offset, length)) \
  X(void, glBindBufferRange, (GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size), (target, index, buffer, offset, size)) \
  X(void, glBindBufferBase, (GLenum target, GLuint index, GLuint buffer), (target, index, buffer)) \
  X(void, glCopyBufferSubData, (GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size), (readtarget, writetarget, readoffset, writeoffset, size)) \
  X(void, glClearBufferfi, (GLenum buffer, GLint drawBuffer, GLfloat depth, GLint stencil), (buffer, drawBuffer, depth, stencil)) \
  X(void, glUniformBlockBinding, (GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding), (program, uniformBlockIndex, uniformBlockBinding)) \
  X(void, glUniform1ui, (GLint location, GLuint v0), (location, v0)) \
  X(void, glUniform2ui, (GLint location, GLuint v0, GLuint v1), (location, v0, v1)) \
  X(void, glUniform3ui, (GLint location, GLuint v0, GLuint v1, GLuint v2), (location, v0, v1, v2)) \
  X(void, glUniform4ui, (GLint location, GLint v0, GLuint v1, GLuint v2, GLuint v3), (location, v0, v1, v2, v3)) \
  X(void, glVertexAttribI4i, (GLuint index, GLint v0, GLint v1, GLint v2, GLint v3), (index, v0, v1, v2, v3)) \
  X(void, glVertexAttribI4ui, (GLuint index, GLuint v0, GLuint v1, GLuint v2, GLuint v3), (index, v0, v1, v2, v3)) \
  X(void, glVertexAttribDivisor, (GLuint index, GLuint divisor), (index, divisor)) \
  X(void, glDrawArraysInstanced, (GLenum mode, GLint first, GLsizei count, GLsizei primcount), (mode, first, count, primcount)) \
  X(void, glWaitSync, (GLsync wait_on, GLbitfield flags, GLuint64 timeout), (wait_on, flags, timeout)) \
  X(void, glReadBuffer, (GLenum src), (src)) \
  X(void, glBlitFramebuffer, (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter), (srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter)) \
  X(void, glFramebufferTextureLayer, (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer), (target, attachment, texture, level, layer)) \
  X(void, glTexStorage2D, (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height), (target, levels, internalformat, width, height)) \
  X(void, glBeginTransformFeedback, (GLenum primitiveMode), (primitiveMode)) \
  X(void, glEndTransformFeedback, (), ()) \
  X(void, glBindTransformFeedback, (GLenum target, GLuint id), (target, id)) \
  X(void, glPauseTransformFeedback, (), ()) \
  X(void, glResumeTransformFeedback, (), ()) \
  X(void, glBindSampler, (GLuint unit, GLuint sampler), (unit, sampler)) \
  X(void, glSamplerParameterf, (GLuint sampler, GLenum pname, GLfloat param), (sampler, pname, param)) \
  X(void, glSamplerParameteri, (GLuint sampler, GLenum pname, GLint param), (sampler, pname, param)) \
  X(void, glBeginQuery, (GLenum target, GLuint query), (target, query)) \
  X(void, glEndQuery, (GLenum target), (target)) \
  X(void, glProgramParameteri, (GLuint program, GLenum pname, GLint value), (program, pname, value)) \
  X(void, glTexStorage3D, (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth), (target, levels, internalformat, width, height, depth)) \
  X(void, glCopyTexSubImage3D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height), (target, level, xoffset, yoffset, zoffset, x, y, width, height)) \
  X(void, glPrimitiveRestartIndex, (GLuint index), (index)) \
  X(void, glMemoryBarrier, (GLbitfield barriers), (barriers)) \
  X(void, glMemoryBarrierByRegion, (GLbitfield barriers), (barriers)) \
  X(void, glBindProgramPipeline, (GLuint pipeline), (pipeline)) \
  X(void, glValidateProgramPipeline, (GLuint pipeline), (pipeline)) \
  X(void, glUseProgramStages, (GLuint pipeline, GLbitfield stages, GLuint program), (pipeline, stages, program)) \
  X(void, glActiveShaderProgram, (GLuint pipeline, GLuint program), (pipeline, program)) \
  X(void, glProgramUniform1f, (GLuint program, GLint location, GLfloat v0), (program, location, v0)) \
  X(void, glProgramUniform2f, (GLuint program, GLint location, GLfloat v0, GLfloat v1), (program, location, v0, v1)) \
  X(void, glProgramUniform3f, (GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2), (program, location, v0, v1, v2)) \
  X(void, glProgramUniform4f, (GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3), (program, location, v0, v1, v2, v3)) \
  X(void, glProgramUniform1i, (GLuint program, GLint location, GLint v0), (program, location, v0)) \
  X(void, glProgramUniform2i, (GLuint program, GLint location, GLint v0, GLint v1), (program, location, v0, v1)) \
  X(void, glProgramUniform3i, (GLuint program, GLint location, GLint v0, GLint v1, GLint v2), (program, location, v0, v1, v2)) \
  X(void, glProgramUniform4i, (GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3), (program, location, v0, v1, v2, v3)) \
  X(void, glProgramUniform1ui, (GLuint program, GLint location, GLuint v0), (program, location, v0)) \
  X(void, glProgramUniform2ui, (GLuint program, GLint location, GLint v0, GLuint v1), (program, location, v0, v1)) \
  X(void, glProgramUniform3ui, (GLuint program, GLint location, GLint v0, GLint v1, GLuint v2), (program, location, v0, v1, v2)) \
  X(void, glProgramUniform4ui, (GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLuint v3), (program, location, v0, v1, v2, v3)) \
  X(void, glBindImageTexture, (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format), (unit, texture, level, layered, layer, access, format)) \
  X(void, glDispatchCompute, (GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z), (num_groups_x, num_groups_y, num_groups_z)) \
  X(void, glDispatchComputeIndirect, (GLintptr indirect), (indirect)) \
  X(void, glBindVertexBuffer, (GLuint bindingindex, GLuint buffer, GLintptr offset, GLintptr stride), (bindingindex, buffer, offset, stride)) \
  X(void, glVertexAttribBinding, (GLuint attribindex, GLuint bindingindex), (attribindex, bindingindex)) \
  X(void, glVertexAttribFormat, (GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset), (attribindex, size, type, normalized, relativeoffset)) \
  X(void, glVertexAttribIFormat, (GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset), (attribindex, size, type, relativeoffset)) \
  X(void, glVertexBindingDivisor, (GLuint bindingindex, GLuint divisor), (bindingindex, divisor)) \
  X(void, glTexStorage2DMultisample, (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations), (target, samples, internalformat, width, height, fixedsamplelocations)) \
  X(void, glSampleMaski, (GLuint maskNumber, GLbitfield mask), (maskNumber, mask)) \
  X(void, glFramebufferParameteri, (GLenum target, GLenum pname, GLint param), (target, pname, param)) \


#endif  // GLES3_ONLY_FUNCTIONS_H
