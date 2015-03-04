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

#include <GLcommon/GLIssue.h>
#include <stdio.h>
#include "emugl/common/shared_library.h"

#ifdef __linux__
#include <GL/glx.h>
#elif defined(WIN32)
#include <windows.h>
#endif

#include "DummyGLfuncs.h"

typedef void (*GL_FUNC_PTR)();

static GL_FUNC_PTR getGLFuncAddress(const char *funcName) {
    GL_FUNC_PTR ret = NULL;
#ifdef __linux__
    static emugl::SharedLibrary* libGL = emugl::SharedLibrary::open("libGL");
    ret = (GL_FUNC_PTR)glXGetProcAddress((const GLubyte*)funcName);
#elif defined(WIN32)
    static emugl::SharedLibrary* libGL = emugl::SharedLibrary::open("opengl32");
    ret = (GL_FUNC_PTR)wglGetProcAddress(funcName);
#elif defined(__APPLE__)
    static emugl::SharedLibrary* libGL = emugl::SharedLibrary::open("/System/Library/Frameworks/OpenGL.framework/OpenGL");
#endif
    if(!ret && libGL){
        ret = libGL->findSymbol(funcName);
    }
    return ret;
}

#define LOAD_GL_FUNC(name)  do { \
        if (!name) { \
            void* funcAddress = (void *)getGLFuncAddress(#name); \
            if (funcAddress) { \
                name = (__typeof__(name))(funcAddress); \
            } else { \
                fprintf(stderr, "Could not load func %s\n", #name); \
                name = (__typeof__(name))(dummy_##name); \
            } \
        } \
    } while (0)

#define LOAD_GLEXT_FUNC(name) do { \
        if (!name) { \
            void* funcAddress = (void *)getGLFuncAddress(#name); \
            if (funcAddress) { \
                name = (__typeof__(name))(funcAddress); \
            } \
        } \
    } while (0)

/* initializing static GLIssue members*/

emugl::Mutex GLIssue::s_lock;
void (GLAPIENTRY *GLIssue::glActiveTexture)(GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glBindBuffer)(GLenum,GLuint) = NULL;
void (GLAPIENTRY *GLIssue::glBindTexture)(GLenum, GLuint) = NULL;
void (GLAPIENTRY *GLIssue::glBlendFunc)(GLenum,GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glBlendEquation)(GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glBlendEquationSeparate)(GLenum,GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glBlendFuncSeparate)(GLenum,GLenum,GLenum,GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glBufferData)(GLenum,GLsizeiptr,const GLvoid *,GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glBufferSubData)(GLenum,GLintptr,GLsizeiptr,const GLvoid *) = NULL;
void (GLAPIENTRY *GLIssue::glClear)(GLbitfield) = NULL;
void (GLAPIENTRY *GLIssue::glClearColor)(GLclampf,GLclampf,GLclampf,GLclampf) = NULL;
void (GLAPIENTRY *GLIssue::glClearStencil)(GLint) = NULL;
void (GLAPIENTRY *GLIssue::glColorMask)(GLboolean,GLboolean,GLboolean,GLboolean) = NULL;
void (GLAPIENTRY *GLIssue::glCompressedTexImage2D)(GLenum,GLint,GLenum,GLsizei,GLsizei,GLint,GLsizei, const GLvoid *) = NULL;
void (GLAPIENTRY *GLIssue::glCompressedTexSubImage2D)(GLenum,GLint,GLint,GLint,GLsizei,GLsizei,GLenum,GLsizei,const GLvoid *) = NULL;
void (GLAPIENTRY *GLIssue::glCopyTexImage2D)(GLenum,GLint,GLenum,GLint,GLint,GLsizei,GLsizei,GLint) = NULL;
void (GLAPIENTRY *GLIssue::glCopyTexSubImage2D)(GLenum,GLint,GLint,GLint,GLint,GLint,GLsizei,GLsizei) = NULL;
void (GLAPIENTRY *GLIssue::glCullFace)(GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glDeleteBuffers)(GLsizei,const GLuint *) = NULL;
void (GLAPIENTRY *GLIssue::glDeleteTextures)(GLsizei,const GLuint *) = NULL;
void (GLAPIENTRY *GLIssue::glDepthFunc)(GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glDepthMask)(GLboolean) = NULL;
void (GLAPIENTRY *GLIssue::glDepthRange)(GLclampd,GLclampd) = NULL;
void (GLAPIENTRY *GLIssue::glDisable)(GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glDrawArrays)(GLenum,GLint,GLsizei) = NULL;
void (GLAPIENTRY *GLIssue::glDrawElements)(GLenum,GLsizei,GLenum,const GLvoid *) = NULL;
void (GLAPIENTRY *GLIssue::glEnable)(GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glFinish)() = NULL;
void (GLAPIENTRY *GLIssue::glFlush)() = NULL;
void (GLAPIENTRY *GLIssue::glFrontFace)(GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glGenBuffers)(GLsizei,GLuint *) = NULL;
void (GLAPIENTRY *GLIssue::glGenTextures)(GLsizei,GLuint *) = NULL;
void (GLAPIENTRY *GLIssue::glGetBooleanv)(GLenum,GLboolean *) = NULL;
void (GLAPIENTRY *GLIssue::glGetBufferParameteriv)(GLenum, GLenum, GLint *) = NULL;
GLenum (GLAPIENTRY *GLIssue::glGetError)() = NULL;
void (GLAPIENTRY *GLIssue::glGetFloatv)(GLenum,GLfloat *) = NULL;
void (GLAPIENTRY *GLIssue::glGetIntegerv)(GLenum,GLint *) = NULL;
const GLubyte * (GLAPIENTRY *GLIssue::glGetString) (GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glGetTexParameterfv)(GLenum,GLenum,GLfloat *) = NULL;
void (GLAPIENTRY *GLIssue::glGetTexParameteriv)(GLenum,GLenum,GLint *) = NULL;
void (GLAPIENTRY *GLIssue::glGetTexLevelParameteriv) (GLenum target, GLint level, GLenum pname, GLint *params) = NULL;
void (GLAPIENTRY *GLIssue::glHint)(GLenum,GLenum) = NULL;
GLboolean (GLAPIENTRY *GLIssue::glIsBuffer)(GLuint) = NULL;
GLboolean (GLAPIENTRY *GLIssue::glIsEnabled)(GLenum) = NULL;
GLboolean (GLAPIENTRY *GLIssue::glIsTexture)(GLuint) = NULL;
void (GLAPIENTRY *GLIssue::glLineWidth)(GLfloat) = NULL;
void (GLAPIENTRY *GLIssue::glPolygonOffset)(GLfloat, GLfloat) = NULL;
void (GLAPIENTRY *GLIssue::glPixelStorei)(GLenum,GLint) = NULL;
void (GLAPIENTRY *GLIssue::glReadPixels)(GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,GLvoid *) = NULL;
void (GLAPIENTRY *GLIssue::glSampleCoverage)(GLclampf,GLboolean) = NULL;
void (GLAPIENTRY *GLIssue::glScissor)(GLint,GLint,GLsizei,GLsizei) = NULL;
void (GLAPIENTRY *GLIssue::glStencilFunc)(GLenum,GLint,GLuint) = NULL;
void (GLAPIENTRY *GLIssue::glStencilMask)(GLuint) = NULL;
void (GLAPIENTRY *GLIssue::glStencilOp)(GLenum, GLenum,GLenum);
void (GLAPIENTRY *GLIssue::glTexImage2D)(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const GLvoid *) = NULL;
void (GLAPIENTRY *GLIssue::glTexParameterf)(GLenum,GLenum, GLfloat) = NULL;
void (GLAPIENTRY *GLIssue::glTexParameterfv)(GLenum,GLenum,const GLfloat *) = NULL;
void (GLAPIENTRY *GLIssue::glTexParameteri)(GLenum,GLenum,GLint) = NULL;
void (GLAPIENTRY *GLIssue::glTexParameteriv)(GLenum,GLenum,const GLint *) = NULL;
void (GLAPIENTRY *GLIssue::glTexSubImage2D)(GLenum,GLint,GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,const GLvoid *) = NULL;
void (GLAPIENTRY *GLIssue::glViewport)(GLint,GLint,GLsizei,GLsizei) = NULL;
void (GLAPIENTRY *GLIssue::glPushAttrib) ( GLbitfield mask ) = NULL;
void (GLAPIENTRY *GLIssue::glPopAttrib) ( void ) = NULL;
void (GLAPIENTRY *GLIssue::glPushClientAttrib) ( GLbitfield mask ) = NULL;
void (GLAPIENTRY *GLIssue::glPopClientAttrib) ( void ) = NULL;

/*GLES 1.1*/
void (GLAPIENTRY *GLIssue::glAlphaFunc)(GLenum,GLclampf) = NULL;
void (GLAPIENTRY *GLIssue::glBegin)(GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glClearDepth)(GLclampd) = NULL;
void (GLAPIENTRY *GLIssue::glClientActiveTexture)(GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glClipPlane)(GLenum,const GLdouble *) = NULL;
void (GLAPIENTRY *GLIssue::glColor4d)(GLdouble,GLdouble,GLdouble,GLdouble) = NULL;
void (GLAPIENTRY *GLIssue::glColor4f)(GLfloat,GLfloat,GLfloat,GLfloat) = NULL;
void (GLAPIENTRY *GLIssue::glColor4fv)(const GLfloat *) = NULL;
void (GLAPIENTRY *GLIssue::glColor4ub)(GLubyte,GLubyte,GLubyte,GLubyte) = NULL;
void (GLAPIENTRY *GLIssue::glColor4ubv)(const GLubyte *) = NULL;
void (GLAPIENTRY *GLIssue::glColorPointer)(GLint,GLenum,GLsizei,const GLvoid *) = NULL;
void (GLAPIENTRY *GLIssue::glDisableClientState)(GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glEnableClientState)(GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glEnd)() = NULL;
void (GLAPIENTRY *GLIssue::glFogf)(GLenum, GLfloat) = NULL;
void (GLAPIENTRY *GLIssue::glFogfv)(GLenum,const GLfloat *);
void (GLAPIENTRY *GLIssue::glFrustum)(GLdouble,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble) = NULL;
void (GLAPIENTRY *GLIssue::glGetClipPlane)(GLenum,GLdouble *) = NULL;
void (GLAPIENTRY *GLIssue::glGetDoublev)(GLenum,GLdouble *) = NULL;
void (GLAPIENTRY *GLIssue::glGetLightfv)(GLenum,GLenum,GLfloat *) = NULL;
void (GLAPIENTRY *GLIssue::glGetMaterialfv)(GLenum,GLenum,GLfloat *) = NULL;
void (GLAPIENTRY *GLIssue::glGetPointerv)(GLenum,GLvoid**) = NULL;
void (GLAPIENTRY *GLIssue::glGetTexEnvfv)(GLenum,GLenum,GLfloat *) = NULL;
void (GLAPIENTRY *GLIssue::glGetTexEnviv)(GLenum,GLenum,GLint *)= NULL;
void (GLAPIENTRY *GLIssue::glLightf)(GLenum,GLenum,GLfloat) = NULL;
void (GLAPIENTRY *GLIssue::glLightfv)(GLenum,GLenum,const GLfloat *) = NULL;
void (GLAPIENTRY *GLIssue::glLightModelf)(GLenum,GLfloat) = NULL;
void (GLAPIENTRY *GLIssue::glLightModelfv)(GLenum,const GLfloat *) = NULL;
void (GLAPIENTRY *GLIssue::glLoadIdentity)() = NULL;
void (GLAPIENTRY *GLIssue::glLoadMatrixf)(const GLfloat *) = NULL;
void (GLAPIENTRY *GLIssue::glLogicOp)(GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glMaterialf)(GLenum,GLenum,GLfloat) = NULL;
void (GLAPIENTRY *GLIssue::glMaterialfv)(GLenum,GLenum,const GLfloat *);
void (GLAPIENTRY *GLIssue::glMultiTexCoord2fv)(GLenum, const GLfloat *) = NULL;
void (GLAPIENTRY *GLIssue::glMultiTexCoord2sv)(GLenum, const GLshort *) = NULL;
void (GLAPIENTRY *GLIssue::glMultiTexCoord3fv)(GLenum, const GLfloat *) = NULL;
void (GLAPIENTRY *GLIssue::glMultiTexCoord3sv)(GLenum,const GLshort *) = NULL;
void (GLAPIENTRY *GLIssue::glMultiTexCoord4f)(GLenum,GLfloat,GLfloat,GLfloat,GLfloat) = NULL;
void (GLAPIENTRY *GLIssue::glMultiTexCoord4fv)(GLenum,const GLfloat *) = NULL;
void (GLAPIENTRY *GLIssue::glMultiTexCoord4sv)(GLenum,const GLshort *) = NULL;
void (GLAPIENTRY *GLIssue::glMultMatrixf)(const GLfloat *) = NULL;
void (GLAPIENTRY *GLIssue::glNormal3f)(GLfloat,GLfloat,GLfloat) = NULL;
void (GLAPIENTRY *GLIssue::glNormal3fv)(const GLfloat *) = NULL;
void (GLAPIENTRY *GLIssue::glNormal3sv)(const GLshort *) = NULL;
void (GLAPIENTRY *GLIssue::glOrtho)(GLdouble,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble) = NULL;
void (GLAPIENTRY *GLIssue::glPointParameterf)(GLenum, GLfloat) = NULL;
void (GLAPIENTRY *GLIssue::glPointParameterfv)(GLenum, const GLfloat *) = NULL;
void (GLAPIENTRY *GLIssue::glPointSize)(GLfloat) = NULL;
void (GLAPIENTRY *GLIssue::glRotatef)(GLfloat,GLfloat,GLfloat,GLfloat) = NULL;
void (GLAPIENTRY *GLIssue::glScalef)(GLfloat,GLfloat,GLfloat) = NULL;
void (GLAPIENTRY *GLIssue::glTexEnvf)(GLenum,GLenum,GLfloat) = NULL;
void (GLAPIENTRY *GLIssue::glTexEnvfv)(GLenum,GLenum,const GLfloat *) = NULL;
void (GLAPIENTRY *GLIssue::glMatrixMode)(GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glNormalPointer)(GLenum,GLsizei,const GLvoid *) = NULL;
void (GLAPIENTRY *GLIssue::glPopMatrix)() = NULL;
void (GLAPIENTRY *GLIssue::glPushMatrix)() = NULL;
void (GLAPIENTRY *GLIssue::glShadeModel)(GLenum) = NULL;
void (GLAPIENTRY *GLIssue::glTexCoordPointer)(GLint,GLenum, GLsizei, const GLvoid*) = NULL;
void (GLAPIENTRY *GLIssue::glTexEnvi)(GLenum ,GLenum,GLint) = NULL;
void (GLAPIENTRY *GLIssue::glTexEnviv)(GLenum, GLenum, const GLint *) = NULL;
void (GLAPIENTRY *GLIssue::glTranslatef)(GLfloat,GLfloat, GLfloat) = NULL;
void (GLAPIENTRY *GLIssue::glVertexPointer)(GLint,GLenum,GLsizei, const GLvoid *) = NULL;

/* GLES 1.1 EXTENSIONS*/
GLboolean (GLAPIENTRY *GLIssue::glIsRenderbufferEXT) (GLuint renderbuffer) = NULL;
void (GLAPIENTRY *GLIssue::glBindRenderbufferEXT) (GLenum target, GLuint renderbuffer) = NULL;
void (GLAPIENTRY *GLIssue::glDeleteRenderbuffersEXT) (GLsizei n, const GLuint *renderbuffers) = NULL;
void (GLAPIENTRY *GLIssue::glGenRenderbuffersEXT) (GLsizei n, GLuint *renderbuffers) = NULL;
void (GLAPIENTRY *GLIssue::glRenderbufferStorageEXT) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height) = NULL;
void (GLAPIENTRY *GLIssue::glGetRenderbufferParameterivEXT) (GLenum target, GLenum pname, GLint *params) = NULL;
GLboolean (GLAPIENTRY *GLIssue::glIsFramebufferEXT) (GLuint framebuffer) = NULL;
void (GLAPIENTRY *GLIssue::glBindFramebufferEXT) (GLenum target, GLuint framebuffer) = NULL;
void (GLAPIENTRY *GLIssue::glDeleteFramebuffersEXT) (GLsizei n, const GLuint *framebuffers) = NULL;
void (GLAPIENTRY *GLIssue::glGenFramebuffersEXT) (GLsizei n, GLuint *framebuffers) = NULL;
GLenum (GLAPIENTRY *GLIssue::glCheckFramebufferStatusEXT) (GLenum target) = NULL;
void (GLAPIENTRY *GLIssue::glFramebufferTexture1DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = NULL;
void (GLAPIENTRY *GLIssue::glFramebufferTexture2DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = NULL;
void (GLAPIENTRY *GLIssue::glFramebufferTexture3DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) = NULL;
void (GLAPIENTRY *GLIssue::glFramebufferRenderbufferEXT) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) = NULL;
void (GLAPIENTRY *GLIssue::glGetFramebufferAttachmentParameterivEXT) (GLenum target, GLenum attachment, GLenum pname, GLint *params) = NULL;
void (GLAPIENTRY *GLIssue::glGenerateMipmapEXT) (GLenum target) = NULL;
void (GLAPIENTRY *GLIssue::glCurrentPaletteMatrixARB) (GLint index) = NULL;
void (GLAPIENTRY *GLIssue::glMatrixIndexuivARB) (GLint size, GLuint * indices) = NULL;
void (GLAPIENTRY *GLIssue::glMatrixIndexPointerARB) (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer) = NULL;
void (GLAPIENTRY *GLIssue::glWeightPointerARB) (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer) = NULL;
void (GLAPIENTRY *GLIssue::glTexGenf) (GLenum coord, GLenum pname, GLfloat param ) = NULL;
void (GLAPIENTRY *GLIssue::glTexGeni) (GLenum coord, GLenum pname, GLint param ) = NULL;
void (GLAPIENTRY *GLIssue::glTexGenfv) (GLenum coord, GLenum pname, const GLfloat *params ) = NULL;
void (GLAPIENTRY *GLIssue::glTexGeniv) (GLenum coord, GLenum pname, const GLint *params ) = NULL;
void (GLAPIENTRY *GLIssue::glGetTexGenfv) (GLenum coord, GLenum pname, GLfloat *params ) = NULL;
void (GLAPIENTRY *GLIssue::glGetTexGeniv) (GLenum coord, GLenum pname, GLint *params ) = NULL;

/* GLES 2.0*/
void (GL_APIENTRY *GLIssue::glBlendColor)(GLclampf,GLclampf,GLclampf,GLclampf) = NULL;
void (GL_APIENTRY *GLIssue::glStencilFuncSeparate)(GLenum,GLenum,GLint,GLuint) = NULL;
void (GL_APIENTRY *GLIssue::glStencilMaskSeparate)(GLenum,GLuint) = NULL;
GLboolean (GL_APIENTRY *GLIssue::glIsProgram)(GLuint program) = NULL;
GLboolean (GL_APIENTRY *GLIssue::glIsShader)(GLuint shader) = NULL;
void (GL_APIENTRY *GLIssue::glVertexAttrib1f)(GLuint,GLfloat) = NULL;
void (GL_APIENTRY *GLIssue::glVertexAttrib1fv)(GLuint,const GLfloat*) = NULL;
void (GL_APIENTRY *GLIssue::glVertexAttrib2f)(GLuint,GLfloat, GLfloat) = NULL;
void (GL_APIENTRY *GLIssue::glVertexAttrib2fv)(GLuint,const GLfloat*) = NULL;
void (GL_APIENTRY *GLIssue::glVertexAttrib3f)(GLuint,GLfloat, GLfloat,GLfloat) = NULL;
void (GL_APIENTRY *GLIssue::glVertexAttrib3fv)(GLuint,const GLfloat*) = NULL;
void (GL_APIENTRY *GLIssue::glVertexAttrib4f)(GLuint,GLfloat,GLfloat,GLfloat,GLfloat ) = NULL;
void (GL_APIENTRY *GLIssue::glVertexAttrib4fv)(GLuint,const GLfloat*) = NULL;
void (GL_APIENTRY *GLIssue::glVertexAttribPointer)(GLuint,GLint,GLenum,GLboolean,GLsizei,const GLvoid*) = NULL;
void (GL_APIENTRY *GLIssue::glDisableVertexAttribArray)(GLuint) = NULL;
void (GL_APIENTRY *GLIssue::glEnableVertexAttribArray)(GLuint) = NULL;
void (GL_APIENTRY *GLIssue::glGetVertexAttribfv)(GLuint,GLenum,GLfloat*) = NULL;
void (GL_APIENTRY *GLIssue::glGetVertexAttribiv)(GLuint,GLenum,GLint*) = NULL;
void (GL_APIENTRY *GLIssue::glGetVertexAttribPointerv)(GLuint,GLenum,GLvoid**) = NULL;
void (GL_APIENTRY *GLIssue::glUniform1f)(GLint,GLfloat) = NULL;
void (GL_APIENTRY *GLIssue::glUniform1fv)(GLint,GLsizei,const GLfloat*) = NULL;
void (GL_APIENTRY *GLIssue::glUniform1i)(GLint,GLint) = NULL;
void (GL_APIENTRY *GLIssue::glUniform1iv)(GLint,GLsizei,const GLint*) = NULL;
void (GL_APIENTRY *GLIssue::glUniform2f)(GLint,GLfloat,GLfloat) = NULL;
void (GL_APIENTRY *GLIssue::glUniform2fv)(GLint,GLsizei,const GLfloat*) = NULL;
void (GL_APIENTRY *GLIssue::glUniform2i)(GLint,GLint,GLint) = NULL;
void (GL_APIENTRY *GLIssue::glUniform2iv)(GLint ,GLsizei,const GLint*) = NULL;
void (GL_APIENTRY *GLIssue::glUniform3f)(GLint,GLfloat,GLfloat,GLfloat) = NULL;
void (GL_APIENTRY *GLIssue::glUniform3fv)(GLint,GLsizei,const GLfloat*) = NULL;
void (GL_APIENTRY *GLIssue::glUniform3i)(GLint,GLint,GLint,GLint) = NULL;
void (GL_APIENTRY *GLIssue::glUniform3iv)(GLint,GLsizei,const GLint*) = NULL;
void (GL_APIENTRY *GLIssue::glUniform4f)(GLint,GLfloat,GLfloat,GLfloat,GLfloat) = NULL;
void (GL_APIENTRY *GLIssue::glUniform4fv)(GLint,GLsizei,const GLfloat*) = NULL;
void (GL_APIENTRY *GLIssue::glUniform4i)(GLint,GLint,GLint,GLint,GLint) = NULL;
void (GL_APIENTRY *GLIssue::glUniform4iv)(GLint,GLsizei,const GLint*) = NULL;
void (GL_APIENTRY *GLIssue::glUniformMatrix2fv)(GLint,GLsizei,GLboolean,const GLfloat*) = NULL;
void (GL_APIENTRY *GLIssue::glUniformMatrix3fv)(GLint,GLsizei,GLboolean,const GLfloat*) = NULL;
void (GL_APIENTRY *GLIssue::glUniformMatrix4fv)(GLint,GLsizei,GLboolean,const GLfloat*) = NULL;
void (GL_APIENTRY *GLIssue::glAttachShader)(GLuint,GLuint) = NULL;
void (GL_APIENTRY *GLIssue::glBindAttribLocation)(GLuint,GLuint,const GLchar*) = NULL;
void (GL_APIENTRY *GLIssue::glCompileShader)(GLuint) = NULL;
GLuint (GL_APIENTRY *GLIssue::glCreateProgram)() = NULL;
GLuint (GL_APIENTRY *GLIssue::glCreateShader)(GLenum) = NULL;
void (GL_APIENTRY *GLIssue::glDeleteProgram)(GLuint) = NULL;
void (GL_APIENTRY *GLIssue::glDeleteShader)(GLuint) = NULL;
void (GL_APIENTRY *GLIssue::glDetachShader)(GLuint,GLuint) = NULL;
void (GL_APIENTRY *GLIssue::glLinkProgram)(GLuint) = NULL;
void (GL_APIENTRY *GLIssue::glUseProgram)(GLuint) = NULL;
void (GL_APIENTRY *GLIssue::glValidateProgram)(GLuint) = NULL;
void (GL_APIENTRY *GLIssue::glGetActiveAttrib)(GLuint,GLuint,GLsizei,GLsizei*,GLint*,GLenum*,GLchar*) = NULL;
void (GL_APIENTRY *GLIssue::glGetActiveUniform)(GLuint,GLuint,GLsizei,GLsizei*,GLint*,GLenum*,GLchar*) = NULL;
void (GL_APIENTRY *GLIssue::glGetAttachedShaders)(GLuint,GLsizei,GLsizei*,GLuint*) = NULL;
int  (GL_APIENTRY *GLIssue::glGetAttribLocation)(GLuint,const GLchar*) = NULL;
void (GL_APIENTRY *GLIssue::glGetProgramiv)(GLuint,GLenum,GLint*) = NULL;
void (GL_APIENTRY *GLIssue::glGetProgramInfoLog)(GLuint,GLsizei,GLsizei*,GLchar*) = NULL;
void (GL_APIENTRY *GLIssue::glGetShaderiv)(GLuint,GLenum,GLint*) = NULL;
void (GL_APIENTRY *GLIssue::glGetShaderInfoLog)(GLuint,GLsizei,GLsizei*,GLchar*) = NULL;
void (GL_APIENTRY *GLIssue::glGetShaderPrecisionFormat)(GLenum,GLenum,GLint*,GLint*) = NULL;
void (GL_APIENTRY *GLIssue::glGetShaderSource)(GLuint,GLsizei,GLsizei*,GLchar*) = NULL;
void (GL_APIENTRY *GLIssue::glGetUniformfv)(GLuint,GLint,GLfloat*) = NULL;
void (GL_APIENTRY *GLIssue::glGetUniformiv)(GLuint,GLint,GLint*) = NULL;
int  (GL_APIENTRY *GLIssue::glGetUniformLocation)(GLuint,const GLchar*) = NULL;
void (GL_APIENTRY *GLIssue::glReleaseShaderCompiler)() = NULL;
void (GL_APIENTRY *GLIssue::glShaderBinary)(GLsizei,const GLuint*,GLenum,const GLvoid*,GLsizei) = NULL;
void (GL_APIENTRY *GLIssue::glShaderSource)(GLuint,GLsizei,const GLchar* const*,const GLint*) = NULL;

GLIssue::GLIssue():m_isLoaded(false){};


void GLIssue::issueFuncs(GLESVersion version){
    emugl::Mutex::AutoLock mutex(s_lock);
    if(m_isLoaded)
        return;

    /* Loading OpenGL functions which are needed for implementing BOTH GLES 1.1 & GLES 2.0*/
    LOAD_GL_FUNC(glActiveTexture);
    LOAD_GL_FUNC(glBindBuffer);
    LOAD_GL_FUNC(glBindTexture);
    LOAD_GL_FUNC(glBlendFunc);
    LOAD_GL_FUNC(glBlendEquation);
    LOAD_GL_FUNC(glBlendEquationSeparate);
    LOAD_GL_FUNC(glBlendFuncSeparate);
    LOAD_GL_FUNC(glBufferData);
    LOAD_GL_FUNC(glBufferSubData);
    LOAD_GL_FUNC(glClear);
    LOAD_GL_FUNC(glClearColor);
    LOAD_GL_FUNC(glClearDepth);
    LOAD_GL_FUNC(glClearStencil);
    LOAD_GL_FUNC(glColorMask);
    LOAD_GL_FUNC(glCompressedTexImage2D);
    LOAD_GL_FUNC(glCompressedTexSubImage2D);
    LOAD_GL_FUNC(glCopyTexImage2D);
    LOAD_GL_FUNC(glCopyTexSubImage2D);
    LOAD_GL_FUNC(glCullFace);
    LOAD_GL_FUNC(glDeleteBuffers);
    LOAD_GL_FUNC(glDeleteTextures);
    LOAD_GL_FUNC(glDepthFunc);
    LOAD_GL_FUNC(glDepthMask);
    LOAD_GL_FUNC(glDepthRange);
    LOAD_GL_FUNC(glDisable);
    LOAD_GL_FUNC(glDrawArrays);
    LOAD_GL_FUNC(glDrawElements);
    LOAD_GL_FUNC(glEnable);
    LOAD_GL_FUNC(glFinish);
    LOAD_GL_FUNC(glFlush);
    LOAD_GL_FUNC(glFrontFace);
    LOAD_GL_FUNC(glGenBuffers);
    LOAD_GL_FUNC(glGenTextures);
    LOAD_GL_FUNC(glGetBooleanv);
    LOAD_GL_FUNC(glGetBufferParameteriv);
    LOAD_GL_FUNC(glGetError);
    LOAD_GL_FUNC(glGetFloatv);
    LOAD_GL_FUNC(glGetIntegerv);
    LOAD_GL_FUNC(glGetString);
    LOAD_GL_FUNC(glTexParameterf);
    LOAD_GL_FUNC(glTexParameterfv);
    LOAD_GL_FUNC(glGetTexParameterfv);
    LOAD_GL_FUNC(glGetTexParameteriv);
    LOAD_GL_FUNC(glGetTexLevelParameteriv);
    LOAD_GL_FUNC(glHint);
    LOAD_GL_FUNC(glIsBuffer);
    LOAD_GL_FUNC(glIsEnabled);
    LOAD_GL_FUNC(glIsTexture);
    LOAD_GL_FUNC(glLineWidth);
    LOAD_GL_FUNC(glPolygonOffset);
    LOAD_GL_FUNC(glPixelStorei);
    LOAD_GL_FUNC(glReadPixels);
    LOAD_GL_FUNC(glSampleCoverage);
    LOAD_GL_FUNC(glScissor);
    LOAD_GL_FUNC(glStencilFunc);
    LOAD_GL_FUNC(glStencilMask);
    LOAD_GL_FUNC(glStencilOp);
    LOAD_GL_FUNC(glTexImage2D);
    LOAD_GL_FUNC(glTexParameteri);
    LOAD_GL_FUNC(glTexParameteriv);
    LOAD_GL_FUNC(glTexSubImage2D);
    LOAD_GL_FUNC(glViewport);
    LOAD_GL_FUNC(glPushAttrib);
    LOAD_GL_FUNC(glPushClientAttrib);
    LOAD_GL_FUNC(glPopAttrib);
    LOAD_GL_FUNC(glPopClientAttrib);
    LOAD_GLEXT_FUNC(glIsRenderbufferEXT);
    LOAD_GLEXT_FUNC(glBindRenderbufferEXT);
    LOAD_GLEXT_FUNC(glDeleteRenderbuffersEXT);
    LOAD_GLEXT_FUNC(glGenRenderbuffersEXT);
    LOAD_GLEXT_FUNC(glRenderbufferStorageEXT);
    LOAD_GLEXT_FUNC(glGetRenderbufferParameterivEXT);
    LOAD_GLEXT_FUNC(glIsFramebufferEXT);
    LOAD_GLEXT_FUNC(glBindFramebufferEXT);
    LOAD_GLEXT_FUNC(glDeleteFramebuffersEXT);
    LOAD_GLEXT_FUNC(glGenFramebuffersEXT);
    LOAD_GLEXT_FUNC(glCheckFramebufferStatusEXT);
    LOAD_GLEXT_FUNC(glFramebufferTexture1DEXT);
    LOAD_GLEXT_FUNC(glFramebufferTexture2DEXT);
    LOAD_GLEXT_FUNC(glFramebufferTexture3DEXT);
    LOAD_GLEXT_FUNC(glFramebufferRenderbufferEXT);
    LOAD_GLEXT_FUNC(glGetFramebufferAttachmentParameterivEXT);
    LOAD_GLEXT_FUNC(glGenerateMipmapEXT);

    /* Loading OpenGL functions which are needed ONLY for implementing GLES 1.1*/
    switch(version) {
      case GLES_1_1:
        fprintf(stdout, "Loading OpenGL functions which are needed ONLY for implementing GLES 1.1\n");

        LOAD_GL_FUNC(glAlphaFunc);
        LOAD_GL_FUNC(glBegin);
        LOAD_GL_FUNC(glClientActiveTexture);
        LOAD_GL_FUNC(glClipPlane);
        LOAD_GL_FUNC(glColor4d);
        LOAD_GL_FUNC(glColor4f);
        LOAD_GL_FUNC(glColor4fv);
        LOAD_GL_FUNC(glColor4ub);
        LOAD_GL_FUNC(glColor4ubv);
        LOAD_GL_FUNC(glColorPointer);
        LOAD_GL_FUNC(glDisableClientState);
        LOAD_GL_FUNC(glEnableClientState);
        LOAD_GL_FUNC(glEnd);
        LOAD_GL_FUNC(glFogf);
        LOAD_GL_FUNC(glFogfv);
        LOAD_GL_FUNC(glFrustum);
        LOAD_GL_FUNC(glGetClipPlane);
        LOAD_GL_FUNC(glGetDoublev);
        LOAD_GL_FUNC(glGetLightfv);
        LOAD_GL_FUNC(glGetMaterialfv);
        LOAD_GL_FUNC(glGetPointerv);
        LOAD_GL_FUNC(glGetTexEnvfv);
        LOAD_GL_FUNC(glGetTexEnviv);
        LOAD_GL_FUNC(glLightf);
        LOAD_GL_FUNC(glLightfv);
        LOAD_GL_FUNC(glLightModelf);
        LOAD_GL_FUNC(glLightModelfv);
        LOAD_GL_FUNC(glLoadIdentity);
        LOAD_GL_FUNC(glLoadMatrixf);
        LOAD_GL_FUNC(glLogicOp);
        LOAD_GL_FUNC(glMaterialf);
        LOAD_GL_FUNC(glMaterialfv);
        LOAD_GL_FUNC(glMultiTexCoord2fv);
        LOAD_GL_FUNC(glMultiTexCoord2sv);
        LOAD_GL_FUNC(glMultiTexCoord3fv);
        LOAD_GL_FUNC(glMultiTexCoord3sv);
        LOAD_GL_FUNC(glMultiTexCoord4fv);
        LOAD_GL_FUNC(glMultiTexCoord4sv);
        LOAD_GL_FUNC(glMultiTexCoord4f);
        LOAD_GL_FUNC(glMultMatrixf);
        LOAD_GL_FUNC(glNormal3f);
        LOAD_GL_FUNC(glNormal3fv);
        LOAD_GL_FUNC(glNormal3sv);
        LOAD_GL_FUNC(glOrtho);
        LOAD_GL_FUNC(glPointParameterf);
        LOAD_GL_FUNC(glPointParameterfv);
        LOAD_GL_FUNC(glPointSize);
        LOAD_GL_FUNC(glRotatef);
        LOAD_GL_FUNC(glScalef);
        LOAD_GL_FUNC(glTexEnvf);
        LOAD_GL_FUNC(glTexEnvfv);
        LOAD_GL_FUNC(glMatrixMode);
        LOAD_GL_FUNC(glNormalPointer);
        LOAD_GL_FUNC(glPopMatrix);
        LOAD_GL_FUNC(glPushMatrix);
        LOAD_GL_FUNC(glShadeModel);
        LOAD_GL_FUNC(glTexCoordPointer);
        LOAD_GL_FUNC(glTexEnvi);
        LOAD_GL_FUNC(glTexEnviv);
        LOAD_GL_FUNC(glTranslatef);
        LOAD_GL_FUNC(glVertexPointer);

        LOAD_GLEXT_FUNC(glCurrentPaletteMatrixARB);
        LOAD_GLEXT_FUNC(glMatrixIndexuivARB);
        LOAD_GLEXT_FUNC(glMatrixIndexPointerARB);
        LOAD_GLEXT_FUNC(glWeightPointerARB);
        LOAD_GLEXT_FUNC(glTexGenf);
        LOAD_GLEXT_FUNC(glTexGeni);
        LOAD_GLEXT_FUNC(glTexGenfv);
        LOAD_GLEXT_FUNC(glTexGeniv);
        LOAD_GLEXT_FUNC(glGetTexGenfv);
        LOAD_GLEXT_FUNC(glGetTexGeniv);
        break;
      case GLES_2_0:
        /* Loading OpenGL functions which are needed ONLY for implementing GLES 2.0*/
        fprintf(stdout, "Loading OpenGL functions which are needed ONLY for implementing GLES 2.0\n");

        LOAD_GL_FUNC(glBlendColor);
        LOAD_GL_FUNC(glBlendFuncSeparate);
        LOAD_GL_FUNC(glStencilFuncSeparate);
        LOAD_GL_FUNC(glIsProgram);
        LOAD_GL_FUNC(glIsShader);
        LOAD_GL_FUNC(glVertexAttrib1f);
        LOAD_GL_FUNC(glVertexAttrib1fv);
        LOAD_GL_FUNC(glVertexAttrib2f);
        LOAD_GL_FUNC(glVertexAttrib2fv);
        LOAD_GL_FUNC(glVertexAttrib3f);
        LOAD_GL_FUNC(glVertexAttrib3fv);
        LOAD_GL_FUNC(glVertexAttrib4f);
        LOAD_GL_FUNC(glVertexAttrib4fv);
        LOAD_GL_FUNC(glVertexAttribPointer);
        LOAD_GL_FUNC(glDisableVertexAttribArray);
        LOAD_GL_FUNC(glEnableVertexAttribArray);
        LOAD_GL_FUNC(glGetVertexAttribfv);
        LOAD_GL_FUNC(glGetVertexAttribiv);
        LOAD_GL_FUNC(glGetVertexAttribPointerv);
        LOAD_GL_FUNC(glUniform1f);
        LOAD_GL_FUNC(glUniform1fv);
        LOAD_GL_FUNC(glUniform1i);
        LOAD_GL_FUNC(glUniform1iv);
        LOAD_GL_FUNC(glUniform2f);
        LOAD_GL_FUNC(glUniform2fv);
        LOAD_GL_FUNC(glUniform2i);
        LOAD_GL_FUNC(glUniform2iv);
        LOAD_GL_FUNC(glUniform3f);
        LOAD_GL_FUNC(glUniform3fv);
        LOAD_GL_FUNC(glUniform3i);
        LOAD_GL_FUNC(glUniform3iv);
        LOAD_GL_FUNC(glUniform4f);
        LOAD_GL_FUNC(glUniform4fv);
        LOAD_GL_FUNC(glUniform4i);
        LOAD_GL_FUNC(glUniform4iv);
        LOAD_GL_FUNC(glUniformMatrix2fv);
        LOAD_GL_FUNC(glUniformMatrix3fv);
        LOAD_GL_FUNC(glUniformMatrix4fv);
        LOAD_GL_FUNC(glAttachShader);
        LOAD_GL_FUNC(glBindAttribLocation);
        LOAD_GL_FUNC(glCompileShader);
        LOAD_GL_FUNC(glCreateProgram);
        LOAD_GL_FUNC(glCreateShader);
        LOAD_GL_FUNC(glDeleteProgram);
        LOAD_GL_FUNC(glDeleteShader);
        LOAD_GL_FUNC(glDetachShader);
        LOAD_GL_FUNC(glLinkProgram);
        LOAD_GL_FUNC(glUseProgram);
        LOAD_GL_FUNC(glValidateProgram);
        LOAD_GL_FUNC(glGetActiveAttrib);
        LOAD_GL_FUNC(glGetActiveUniform);
        LOAD_GL_FUNC(glGetAttachedShaders);
        LOAD_GL_FUNC(glGetAttribLocation);
        LOAD_GL_FUNC(glGetProgramiv);
        LOAD_GL_FUNC(glGetProgramInfoLog);
        LOAD_GL_FUNC(glGetShaderiv);
        LOAD_GL_FUNC(glGetShaderInfoLog);
        LOAD_GLEXT_FUNC(glGetShaderPrecisionFormat);
        LOAD_GL_FUNC(glGetShaderSource);
        LOAD_GL_FUNC(glGetUniformfv);
        LOAD_GL_FUNC(glGetUniformiv);
        LOAD_GL_FUNC(glGetUniformLocation);
        LOAD_GLEXT_FUNC(glReleaseShaderCompiler);
        LOAD_GLEXT_FUNC(glShaderBinary);
        LOAD_GL_FUNC(glShaderSource);
        LOAD_GL_FUNC(glStencilMaskSeparate);
        break;
      case GLES_1_to_2:
        //
        // Loading OpenGL functions which are needed for implementing
        // GLES 1.1 using libes1x (i.e. GLES 2.0 to 1.1 translator);
        // that is, primarily GLES 1.1 extensions
        //
        LOAD_GL_FUNC(glFrustum);
        break;
      default:
        fprintf(stderr, "Invalid GLES interface code 0x%x \n", version);
        return;
    }
    m_isLoaded = true;
}
