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

#ifndef DUMMY_GL_FUNCS_H
#define DUMMY_GL_FUNCS_H

#include <GLES/gl.h>
#include <GLES2/gl2.h>

#include <GLcommon/gldefs.h>

extern "C" {

    void dummy_glActiveTexture ( GLenum texture );
    void dummy_glBindBuffer (GLenum target, GLuint buffer);
    void dummy_glBindTexture (GLenum target, GLuint texture);
    void dummy_glBlendFunc (GLenum sfactor, GLenum dfactor);
    void dummy_glBlendEquation( GLenum mode );
    void dummy_glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
    void dummy_glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
    void dummy_glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
    void dummy_glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
    void dummy_glClear(GLbitfield mask);
    void dummy_glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    void dummy_glClearStencil(GLint s);
    void dummy_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
    void dummy_glCompressedTexImage2D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data );
    void dummy_glCompressedTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data );
    void dummy_glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
    void dummy_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
    void dummy_glCullFace(GLenum mode);
    void dummy_glDeleteBuffers(GLsizei n, const GLuint *buffers);
    void dummy_glDeleteTextures(GLsizei n, const GLuint *textures);
    void dummy_glDepthFunc(GLenum func);
    void dummy_glDepthMask(GLboolean flag);
    void dummy_glDepthRange(GLclampd zNear, GLclampd zFar);
    void dummy_glDisable(GLenum cap);
    void dummy_glDrawArrays(GLenum mode, GLint first, GLsizei count);
    void dummy_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
    void dummy_glEnable(GLenum cap);
    void dummy_glFinish(void);
    void dummy_glFlush(void);
    void dummy_glFrontFace(GLenum mode);
    void dummy_glGenBuffers(GLsizei n, GLuint *buffers);
    void dummy_glGenTextures(GLsizei n, GLuint *textures);
    void dummy_glGetBooleanv(GLenum pname, GLboolean *params);
    void dummy_glGetBufferParameteriv(GLenum, GLenum, GLint *);
    GLenum dummy_glGetError(void);
    void dummy_glGetFloatv(GLenum pname, GLfloat *params);
    void dummy_glGetIntegerv(GLenum pname, GLint *params);
    const GLubyte * dummy_glGetString(GLenum name);
    void dummy_glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params);
    void dummy_glGetTexParameteriv(GLenum target, GLenum pname, GLint *params);
    void dummy_glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params);
    void dummy_glHint(GLenum target, GLenum mode);
    GLboolean dummy_glIsBuffer(GLuint);
    GLboolean dummy_glIsEnabled(GLenum cap);
    GLboolean dummy_glIsTexture(GLuint texture);
    void dummy_glLineWidth(GLfloat width);
    void dummy_glPolygonOffset(GLfloat factor, GLfloat units);
    void dummy_glPixelStorei(GLenum pname, GLint param);
    void dummy_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
    void dummy_glSampleCoverage(GLclampf value, GLboolean invert );
    void dummy_glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
    void dummy_glStencilFunc(GLenum func, GLint ref, GLuint mask);
    void dummy_glStencilMask(GLuint mask);
    void dummy_glStencilOp(GLenum fail, GLenum zfail, GLenum zpass);
    void dummy_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    void dummy_glTexParameteri(GLenum target, GLenum pname, GLint param);
    void dummy_glTexParameteriv(GLenum target, GLenum pname, const GLint *params);
    void dummy_glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
    void dummy_glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
    void dummy_glPushAttrib( GLbitfield mask );
    void dummy_glPopAttrib( void );
    void dummy_glPushClientAttrib( GLbitfield mask );
    void dummy_glPopClientAttrib( void );

    /* OpenGL functions which are needed ONLY for implementing GLES 1.1*/
    void dummy_glAlphaFunc(GLenum func, GLclampf ref);
    void dummy_glBegin( GLenum mode );
    void dummy_glClearDepth(GLclampd depth);
    void dummy_glClientActiveTexture( GLenum texture );
    void dummy_glClipPlane(GLenum plane, const GLdouble *equation);
    void dummy_glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
    void dummy_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    void dummy_glColor4fv( const GLfloat *v );
    void dummy_glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
    void dummy_glColor4ubv( const GLubyte *v );
    void dummy_glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    void dummy_glDisableClientState(GLenum array);
    void dummy_glEnableClientState(GLenum array);
    void dummy_glEnd(void);
    void dummy_glFogf(GLenum pname, GLfloat param);
    void dummy_glFogfv(GLenum pname, const GLfloat *params);
    void dummy_glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
    void dummy_glGetClipPlane(GLenum plane, GLdouble *equation);
    void dummy_glGetDoublev( GLenum pname, GLdouble *params );
    void dummy_glGetLightfv(GLenum light, GLenum pname, GLfloat *params);
    void dummy_glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params);
    void dummy_glGetPointerv(GLenum pname, GLvoid* *params);
    void dummy_glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params);
    void dummy_glGetTexEnviv(GLenum target, GLenum pname, GLint *params);
    void dummy_glLightf(GLenum light, GLenum pname, GLfloat param);
    void dummy_glLightfv(GLenum light, GLenum pname, const GLfloat *params);
    void dummy_glLightModelf(GLenum pname, GLfloat param);
    void dummy_glLightModelfv(GLenum pname, const GLfloat *params);
    void dummy_glLoadIdentity(void);
    void dummy_glLoadMatrixf(const GLfloat *m);
    void dummy_glLogicOp(GLenum opcode);
    void dummy_glMaterialf(GLenum face, GLenum pname, GLfloat param);
    void dummy_glMaterialfv(GLenum face, GLenum pname, const GLfloat *params);
    void dummy_glMultiTexCoord2fv( GLenum target, const GLfloat *v );
    void dummy_glMultiTexCoord2sv( GLenum target, const GLshort *v );
    void dummy_glMultiTexCoord3fv( GLenum target, const GLfloat *v );
    void dummy_glMultiTexCoord3sv( GLenum target, const GLshort *v );
    void dummy_glMultiTexCoord4f( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q );
    void dummy_glMultiTexCoord4fv( GLenum target, const GLfloat *v );
    void dummy_glMultiTexCoord4sv( GLenum target, const GLshort *v );
    void dummy_glMultMatrixf(const GLfloat *m);
    void dummy_glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
    void dummy_glNormal3fv( const GLfloat *v );
    void dummy_glNormal3sv(const GLshort *v );
    void dummy_glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
    void dummy_glPointParameterf(GLenum, GLfloat);
    void dummy_glPointParameterfv(GLenum, const GLfloat *);
    void dummy_glPointSize(GLfloat size);
    void dummy_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
    void dummy_glScalef(GLfloat x, GLfloat y, GLfloat z);
    void dummy_glTexEnvf(GLenum target, GLenum pname, GLfloat param);
    void dummy_glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params);
    void dummy_glTexParameterf(GLenum target, GLenum pname, GLfloat param);
    void dummy_glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params);
    void dummy_glMatrixMode(GLenum mode);
    void dummy_glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
    void dummy_glPopMatrix(void);
    void dummy_glPushMatrix(void);
    void dummy_glShadeModel(GLenum mode);
    void dummy_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    void dummy_glTexEnvi(GLenum target, GLenum pname, GLint param);
    void dummy_glTexEnviv(GLenum target, GLenum pname, const GLint *params);
    void dummy_glTranslatef(GLfloat x, GLfloat y, GLfloat z);
    void dummy_glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

    /* OpenGL functions which are needed ONLY for implementing GLES 1.1 EXTENSIONS*/
    GLboolean dummy_glIsRenderbufferEXT(GLuint renderbuffer);
    void dummy_glBindRenderbufferEXT(GLenum target, GLuint renderbuffer);
    void dummy_glDeleteRenderbuffersEXT(GLsizei n, const GLuint *renderbuffers);
    void dummy_glGenRenderbuffersEXT(GLsizei n, GLuint *renderbuffers);
    void dummy_glRenderbufferStorageEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    void dummy_glGetRenderbufferParameterivEXT(GLenum target, GLenum pname, GLint *params);
    GLboolean dummy_glIsFramebufferEXT(GLuint framebuffer);
    void dummy_glBindFramebufferEXT(GLenum target, GLuint framebuffer);
    void dummy_glDeleteFramebuffersEXT(GLsizei n, const GLuint *framebuffers);
    void dummy_glGenFramebuffersEXT(GLsizei n, GLuint *framebuffers);
    GLenum dummy_glCheckFramebufferStatusEXT(GLenum target);
    void dummy_glFramebufferTexture1DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    void dummy_glFramebufferTexture2DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    void dummy_glFramebufferTexture3DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
    void dummy_glFramebufferRenderbufferEXT(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    void dummy_glGetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment, GLenum pname, GLint *params);
    void dummy_glGenerateMipmapEXT(GLenum target);
    void dummy_glCurrentPaletteMatrixARB(GLint index);
    void dummy_glMatrixIndexuivARB(GLint size, GLuint * indices);
    void dummy_glMatrixIndexPointerARB(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
    void dummy_glWeightPointerARB(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
    void dummy_glTexGenf(GLenum coord, GLenum pname, GLfloat param );
    void dummy_glTexGeni(GLenum coord, GLenum pname, GLint param );
    void dummy_glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params );
    void dummy_glTexGeniv(GLenum coord, GLenum pname, const GLint *params );
    void dummy_glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params );
    void dummy_glGetTexGeniv(GLenum coord, GLenum pname, GLint *params );

    /* Loading OpenGL functions which are needed ONLY for implementing GLES 2.0*/
    void  dummy_glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    void  dummy_glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask);
    void  dummy_glStencilMaskSeparate(GLenum face, GLuint mask);
    void  dummy_glGenerateMipmap(GLenum target);
    void  dummy_glBindFramebuffer(GLenum target, GLuint framebuffer);
    void  dummy_glBindRenderbuffer(GLenum target, GLuint renderbuffer);
    void  dummy_glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers);
    void  dummy_glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers);
    GLboolean  dummy_glIsProgram(GLuint program);
    GLboolean  dummy_glIsShader(GLuint shader);
    void  dummy_glVertexAttrib1f(GLuint indx, GLfloat x);
    void  dummy_glVertexAttrib1fv(GLuint indx, const GLfloat* values);
    void  dummy_glVertexAttrib2f(GLuint indx, GLfloat x, GLfloat y);
    void  dummy_glVertexAttrib2fv(GLuint indx, const GLfloat* values);
    void  dummy_glVertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z);
    void  dummy_glVertexAttrib3fv(GLuint indx, const GLfloat* values);
    void  dummy_glVertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void  dummy_glVertexAttrib4fv(GLuint indx, const GLfloat* values);
    void  dummy_glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
    void  dummy_glDisableVertexAttribArray(GLuint index);
    void  dummy_glEnableVertexAttribArray(GLuint index);
    void  dummy_glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params);
    void  dummy_glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params);
    void  dummy_glGetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** pointer);
    void  dummy_glUniform1f(GLint location, GLfloat x);
    void  dummy_glUniform1fv(GLint location, GLsizei count, const GLfloat* v);
    void  dummy_glUniform1i(GLint location, GLint x);
    void  dummy_glUniform1iv(GLint location, GLsizei count, const GLint* v);
    void  dummy_glUniform2f(GLint location, GLfloat x, GLfloat y);
    void  dummy_glUniform2fv(GLint location, GLsizei count, const GLfloat* v);
    void  dummy_glUniform2i(GLint location, GLint x, GLint y);
    void  dummy_glUniform2iv(GLint location, GLsizei count, const GLint* v);
    void  dummy_glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z);
    void  dummy_glUniform3fv(GLint location, GLsizei count, const GLfloat* v);
    void  dummy_glUniform3i(GLint location, GLint x, GLint y, GLint z);
    void  dummy_glUniform3iv(GLint location, GLsizei count, const GLint* v);
    void  dummy_glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void  dummy_glUniform4fv(GLint location, GLsizei count, const GLfloat* v);
    void  dummy_glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w);
    void  dummy_glUniform4iv(GLint location, GLsizei count, const GLint* v);
    void  dummy_glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void  dummy_glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void  dummy_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void  dummy_glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params);
    void  dummy_glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params);
    GLboolean  dummy_glIsFramebuffer(GLuint framebuffer);
    GLboolean  dummy_glIsRenderbuffer(GLuint renderbuffer);
    GLenum  dummy_glCheckFramebufferStatus(GLenum target);
    void  dummy_glAttachShader(GLuint program, GLuint shader);
    void  dummy_glBindAttribLocation(GLuint program, GLuint index, const GLchar* name);
    void  dummy_glCompileShader(GLuint shader);
    GLuint  dummy_glCreateProgram(void);
    GLuint  dummy_glCreateShader(GLenum type);
    void  dummy_glDeleteProgram(GLuint program);
    void  dummy_glDeleteShader(GLuint shader);
    void  dummy_glDetachShader(GLuint program, GLuint shader);
    void  dummy_glLinkProgram(GLuint program);
    void  dummy_glUseProgram(GLuint program);
    void  dummy_glValidateProgram(GLuint program);
    void  dummy_glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
    void  dummy_glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
    void  dummy_glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
    int   dummy_glGetAttribLocation(GLuint program, const GLchar* name);
    void  dummy_glGetProgramiv(GLuint program, GLenum pname, GLint* params);
    void  dummy_glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog);
    void  dummy_glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
    void  dummy_glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog);
    void  dummy_glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
    void  dummy_glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source);
    void  dummy_glGetUniformfv(GLuint program, GLint location, GLfloat* params);
    void  dummy_glGetUniformiv(GLuint program, GLint location, GLint* params);
    int   dummy_glGetUniformLocation(GLuint program, const GLchar* name);
    void  dummy_glReleaseShaderCompiler(void);
    void  dummy_glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    void  dummy_glShaderBinary(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length);
    void  dummy_glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
    void  dummy_glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    void  dummy_glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);

}  // extern "C"

#endif
