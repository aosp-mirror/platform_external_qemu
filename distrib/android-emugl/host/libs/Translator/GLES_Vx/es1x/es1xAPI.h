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

 
#ifndef _ES1XAPI_H
#define _ES1XAPI_H

#ifndef _ES1XDEFS_H
#	include "es1xDefs.h"
#endif

#ifndef _ES1XROUTING_H
#	include "es1xRouting.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

ES1X_API GLenum ES1X_APIENTRY es1xGetError(void *_context_);
ES1X_API void* ES1X_APIENTRY es1xCreate(void *_context_);
ES1X_API void ES1X_APIENTRY es1xDestroy(void *_context_);
ES1X_API void ES1X_APIENTRY es1xMakeCurrent(void *_context_);
ES1X_API void ES1X_APIENTRY es1xMultMatrixf(void *_context_, const GLfloat* matrix);
ES1X_API void ES1X_APIENTRY es1xMultMatrixx(void *_context_, const GLfixed* matrix);
ES1X_API void ES1X_APIENTRY es1xLoadMatrixf(void *_context_, const GLfloat* matrix);
ES1X_API void ES1X_APIENTRY es1xLoadMatrixx(void *_context_, const GLfixed* matrix);
ES1X_API void ES1X_APIENTRY es1xMatrixMode(void *_context_, GLenum mode);
ES1X_API void ES1X_APIENTRY es1xLoadIdentity (void *_context_);
ES1X_API void ES1X_APIENTRY es1xRotatef(void *_context_, GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
ES1X_API void ES1X_APIENTRY es1xRotatex(void *_context_, GLfixed angle, GLfixed x, GLfixed y, GLfixed z);
ES1X_API void ES1X_APIENTRY es1xScalef(void *_context_, GLfloat x, GLfloat y, GLfloat z);
ES1X_API void ES1X_APIENTRY es1xScalex(void *_context_, GLfixed x, GLfixed y, GLfixed z);
ES1X_API void ES1X_APIENTRY es1xFrustumf(void *_context_, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near, GLfloat far);
ES1X_API void ES1X_APIENTRY es1xFrustumx(void *_context_, GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar);
ES1X_API void ES1X_APIENTRY es1xOrthof(void *_context_, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near, GLfloat far);
ES1X_API void ES1X_APIENTRY es1xOrthox(void *_context_, GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed near, GLfixed far);
ES1X_API void ES1X_APIENTRY es1xTranslatef(void *_context_, GLfloat x, GLfloat y, GLfloat z);
ES1X_API void ES1X_APIENTRY es1xTranslatex(void *_context_, GLfixed x, GLfixed y, GLfixed z);
ES1X_API void ES1X_APIENTRY es1xPopMatrix(void *_context_);
ES1X_API void ES1X_APIENTRY es1xPushMatrix(void *_context_);
ES1X_API void ES1X_APIENTRY es1xDrawArrays(void *_context_, GLenum mode, GLint first, GLsizei count);
ES1X_API void ES1X_APIENTRY es1xDrawElements(void *_context_, GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
ES1X_API void ES1X_APIENTRY es1xGetIntegerv(void *_context_, GLenum pname, GLint* params);
ES1X_API void ES1X_APIENTRY es1xGetBooleanv(void *_context_, GLenum pname, GLboolean* params);
ES1X_API void ES1X_APIENTRY es1xGetFloatv(void *_context_, GLenum pname, GLfloat* params);
ES1X_API void ES1X_APIENTRY es1xGetFixedv(void *_context_, GLenum pname, GLfixed* params);
ES1X_API GLboolean ES1X_APIENTRY es1xIsEnabled(void *_context_, GLenum cap);
// static GLboolean ES1X_APIENTRY es1xGetClipPlane(void *_context_, GLenum pname, GLvoid* params, es1xStateQueryParamType paramType);
ES1X_API void ES1X_APIENTRY es1xGetClipPlanef(void *_context_, GLenum pname, GLfloat eqn[4]);
ES1X_API void ES1X_APIENTRY es1xGetClipPlanex(void *_context_, GLenum pname, GLfixed eqn[4]);
ES1X_API void ES1X_APIENTRY es1xGetMaterialfv(void *_context_, GLenum face, GLenum pname, GLfloat* params);
ES1X_API void ES1X_APIENTRY es1xGetMaterialxv(void *_context_, GLenum face, GLenum pname, GLfixed* params);
// static void ES1X_APIENTRY es1xGetLightv(void *_context_, GLenum light, GLenum pname, GLvoid* params, es1xStateQueryParamType paramType);
ES1X_API void ES1X_APIENTRY es1xGetLightfv(void *_context_, GLenum light, GLenum pname, GLfloat* params);
ES1X_API void ES1X_APIENTRY es1xGetLightxv(void *_context_, GLenum light, GLenum pname, GLfixed* params);
ES1X_API void ES1X_APIENTRY es1xGetTexEnviv(void *_context_, GLenum env, GLenum pname, GLint* params);
ES1X_API void ES1X_APIENTRY es1xGetTexEnvfv(void *_context_, GLenum env, GLenum pname, GLfloat* params);
ES1X_API void ES1X_APIENTRY es1xGetTexEnvxv(void *_context_, GLenum env, GLenum pname, GLfixed* params);
ES1X_API void ES1X_APIENTRY es1xGetTexParameterfv(void *_context_, GLenum target, GLenum pname, GLfloat* params);
ES1X_API void ES1X_APIENTRY es1xGetTexParameteriv(void *_context_, GLenum target, GLenum pname, GLint* params);
ES1X_API void ES1X_APIENTRY es1xGetTexParameterxv(void *_context_, GLenum target, GLenum pname, GLfixed* params);
ES1X_API void ES1X_APIENTRY es1xGetBufferParameteriv(void *_context_, GLenum target, GLenum pname, GLint* params);
ES1X_API void ES1X_APIENTRY es1xGetPointerv(void *_context_, GLenum pname, void** params);
ES1X_API const GLubyte* ES1X_APIENTRY es1xGetString(void *_context_, GLenum pname);
ES1X_API void ES1X_APIENTRY es1xActiveTexture(void *_context_, GLenum pname);
ES1X_API void ES1X_APIENTRY es1xAlphaFunc(void *_context_, GLenum func, GLclampf ref);
ES1X_API void ES1X_APIENTRY es1xAlphaFuncx(void *_context_, GLenum func, GLclampx ref);
ES1X_API void ES1X_APIENTRY es1xBindBuffer(void *_context_, GLenum target, GLuint buffer);
ES1X_API void ES1X_APIENTRY es1xBindTexture(void *_context_, GLenum target, GLuint textureName);
ES1X_API void ES1X_APIENTRY es1xBlendFunc(void *_context_, GLenum src, GLenum dst);

ES1X_API void ES1X_APIENTRY es1xBlendEquation(void *_context_, GLenum mode);
ES1X_API void ES1X_APIENTRY es1xBlendEquationSeparate(void *_context_, GLenum modeRGB, GLenum modeAlpha);
ES1X_API void ES1X_APIENTRY es1xBlendFuncSeparate(void *_context_, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);

ES1X_API void ES1X_APIENTRY es1xBufferData(void *_context_, GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
ES1X_API void ES1X_APIENTRY es1xBufferSubData(void *_context_, GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);
ES1X_API void ES1X_APIENTRY es1xClear(void *_context_, GLbitfield mask);
ES1X_API void ES1X_APIENTRY es1xClearColor(void *_context_, GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
ES1X_API void ES1X_APIENTRY es1xClearColorx(void *_context_, GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha);
ES1X_API void ES1X_APIENTRY es1xClearDepthf(void *_context_, GLclampf depth);
ES1X_API void ES1X_APIENTRY es1xClearDepthx(void *_context_, GLclampx depth);
ES1X_API void ES1X_APIENTRY es1xClearStencil(void *_context_, GLint s);
ES1X_API void ES1X_APIENTRY es1xClientActiveTexture(void *_context_, GLenum texture);
ES1X_API void ES1X_APIENTRY es1xClipPlanef(void *_context_, GLenum plane, const GLfloat* eqn);
ES1X_API void ES1X_APIENTRY es1xClipPlanex(void *_context_, GLenum plane, const GLfixed* eqn);
ES1X_API void ES1X_APIENTRY es1xColor4f(void *_context_, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
ES1X_API void ES1X_APIENTRY es1xColor4ub(void *_context_, GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
ES1X_API void ES1X_APIENTRY es1xColor4x(void *_context_, GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha);
ES1X_API void ES1X_APIENTRY es1xColorMask(void *_context_, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
ES1X_API void ES1X_APIENTRY es1xColorPointer(void *_context_, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
ES1X_API void ES1X_APIENTRY es1xCompressedTexImage2D(void *_context_, GLenum target, GLint level, GLenum internalFormat,
                                                 GLsizei width, GLsizei height, GLint border,
                                                 GLsizei imageSize, const GLvoid* data);
ES1X_API void ES1X_APIENTRY es1xCompressedTexSubImage2D(void *_context_, GLenum target, GLint level, GLint xoffset,
                                                    GLint yoffset, GLsizei width, GLsizei height,
                                                    GLenum format, GLsizei imageSize, const GLvoid* data);;
ES1X_API void ES1X_APIENTRY es1xCopyTexImage2D(void *_context_, GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y,
                                           GLsizei width, GLsizei height, GLint border);;
ES1X_API void ES1X_APIENTRY es1xCopyTexSubImage2D(void *_context_, GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                              GLint x, GLint y, GLsizei width, GLsizei height);;
ES1X_API void ES1X_APIENTRY es1xCullFace(void *_context_, GLenum mode);
ES1X_API void ES1X_APIENTRY es1xDeleteBuffers(void *_context_, GLsizei n, const GLuint* buffers);
ES1X_API void ES1X_APIENTRY es1xDeleteTextures(void *_context_, GLsizei n, const GLuint* textures);
ES1X_API void ES1X_APIENTRY es1xDepthFunc(void *_context_, GLenum func);
ES1X_API void ES1X_APIENTRY es1xDepthMask(void *_context_, GLboolean mask);
ES1X_API void ES1X_APIENTRY es1xDepthRangef(void *_context_, GLclampf zNear, GLclampf zFar);
ES1X_API void ES1X_APIENTRY es1xDepthRangex(void *_context_, GLclampx zNear, GLclampx zFar);
ES1X_API void ES1X_APIENTRY es1xDisable(void *_context_, GLenum cap);
ES1X_API void ES1X_APIENTRY es1xDisableClientState(void *_context_, GLenum array);
ES1X_API void ES1X_APIENTRY es1xEnable(void *_context_, GLenum cap);
ES1X_API void ES1X_APIENTRY es1xEnableClientState(void *_context_, GLenum array);
ES1X_API void ES1X_APIENTRY es1xFinish(void *_context_);
ES1X_API void ES1X_APIENTRY es1xFlush(void *_context_);
ES1X_API void ES1X_APIENTRY es1xFogf(void *_context_, GLenum pname, GLfloat param);
ES1X_API void ES1X_APIENTRY es1xFogfv(void *_context_, GLenum pname, const GLfloat* param);
ES1X_API void ES1X_APIENTRY es1xFogx(void *_context_, GLenum pname, GLfixed param);
ES1X_API void ES1X_APIENTRY es1xFogxv(void *_context_, GLenum pname, const GLfixed* param);
ES1X_API void ES1X_APIENTRY es1xFrontFace(void *_context_, GLenum dir);
ES1X_API void ES1X_APIENTRY es1xGenBuffers(void *_context_, GLsizei n, GLuint* buffers);
ES1X_API void ES1X_APIENTRY es1xGenTextures(void *_context_, GLsizei n, GLuint* textures);
ES1X_API void ES1X_APIENTRY es1xHint(void *_context_, GLenum target, GLenum hint);
ES1X_API GLboolean ES1X_APIENTRY es1xIsBuffer(void *_context_, GLuint buffer);
ES1X_API GLboolean ES1X_APIENTRY es1xIsTexture(void *_context_, GLuint texture);
ES1X_API void ES1X_APIENTRY es1xLightModelf(void *_context_, GLenum pname, GLfloat param);
ES1X_API void ES1X_APIENTRY es1xLightModelfv(void *_context_, GLenum pname, const GLfloat* param);
ES1X_API void ES1X_APIENTRY es1xLightModelx(void *_context_, GLenum pname, GLfixed param);
ES1X_API void ES1X_APIENTRY es1xLightModelxv(void *_context_, GLenum pname, const GLfixed* param);
ES1X_API void ES1X_APIENTRY es1xLightf(void *_context_, GLenum lightEnum, GLenum pname, GLfloat param);
ES1X_API void ES1X_APIENTRY es1xLightfv(void *_context_, GLenum lightEnum, GLenum pname, const GLfloat* params);
ES1X_API void ES1X_APIENTRY es1xLightx(void *_context_, GLenum light, GLenum pname, GLfixed param);
ES1X_API void ES1X_APIENTRY es1xLightxv(void *_context_, GLenum light, GLenum pname, const GLfixed* params);
ES1X_API void ES1X_APIENTRY es1xLineWidth(void *_context_, GLfloat width);
ES1X_API void ES1X_APIENTRY es1xLineWidthx(void *_context_, GLfixed width);
ES1X_API void ES1X_APIENTRY es1xLogicOp(void *_context_, GLenum mode);
ES1X_API void ES1X_APIENTRY es1xMaterialf(void *_context_, GLenum face, GLenum pname, GLfloat param);
ES1X_API void ES1X_APIENTRY es1xMaterialfv(void *_context_, GLenum face, GLenum pname, const GLfloat* params);
ES1X_API void ES1X_APIENTRY es1xMaterialx(void *_context_, GLenum face, GLenum pname, GLfixed param);
ES1X_API void ES1X_APIENTRY es1xMaterialxv(void *_context_, GLenum face, GLenum pname, const GLfixed* params);
ES1X_API void ES1X_APIENTRY es1xMultiTexCoord4f(void *_context_, GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
ES1X_API void ES1X_APIENTRY es1xMultiTexCoord4x(void *_context_, GLenum target, GLfixed s, GLfixed t, GLfixed r, GLfixed q);
ES1X_API void ES1X_APIENTRY es1xNormal3f(void *_context_, GLfloat x, GLfloat y, GLfloat z);
ES1X_API void ES1X_APIENTRY es1xNormal3x(void *_context_, GLfixed x, GLfixed y, GLfixed z);
ES1X_API void ES1X_APIENTRY es1xPixelStorei(void *_context_, GLenum pname, GLint param);
ES1X_API void ES1X_APIENTRY es1xPointParameterf(void *_context_, GLenum pname, GLfloat param);
ES1X_API void ES1X_APIENTRY es1xPointParameterfv(void *_context_, GLenum pname, const GLfloat *params);
ES1X_API void ES1X_APIENTRY es1xPointParameterx(void *_context_, GLenum pname, GLfixed param);
ES1X_API void ES1X_APIENTRY es1xPointParameterxv(void *_context_, GLenum pname, const GLfixed* params);
ES1X_API void ES1X_APIENTRY es1xPointSize(void *_context_, GLfloat size);
ES1X_API void ES1X_APIENTRY es1xPointSizex(void *_context_, GLfixed size);
ES1X_API void ES1X_APIENTRY es1xPointSizePointerOES(void *_context_, GLenum type, GLsizei stride, const GLvoid* pointer);
ES1X_API void ES1X_APIENTRY es1xPolygonOffset(void *_context_, GLfloat factor, GLfloat units);
ES1X_API void ES1X_APIENTRY es1xPolygonOffsetx(void *_context_, GLfixed factor, GLfixed units);
ES1X_API void ES1X_APIENTRY es1xReadPixels(void *_context_, GLint x, GLint y, GLsizei width, GLsizei height,
                                       GLenum format, GLenum type, GLvoid* pixels);
ES1X_API void ES1X_APIENTRY es1xSampleCoverage(void *_context_, GLclampf value, GLboolean invert);
ES1X_API void ES1X_APIENTRY es1xSampleCoveragex(void *_context_, GLclampx value, GLboolean invert);
ES1X_API void ES1X_APIENTRY es1xScissor(void *_context_, GLint x, GLint y, GLsizei width, GLsizei height);
ES1X_API void ES1X_APIENTRY es1xShadeModel(void *_context_, GLenum model);
ES1X_API void ES1X_APIENTRY es1xStencilFunc(void *_context_, GLenum func, GLint ref, GLuint mask);
ES1X_API void ES1X_APIENTRY es1xStencilMask(void *_context_, GLuint mask);
ES1X_API void ES1X_APIENTRY es1xStencilOp(void *_context_, GLenum fail, GLenum zFail, GLenum zPass);
ES1X_API void ES1X_APIENTRY es1xTexCoordPointer(void *_context_, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
ES1X_API void ES1X_APIENTRY es1xTexEnvf(void *_context_, GLenum target, GLenum pname, GLfloat param);
ES1X_API void ES1X_APIENTRY es1xTexEnvfv(void *_context_, GLenum target, GLenum pname, const GLfloat* params);
ES1X_API void ES1X_APIENTRY es1xTexEnvi(void *_context_, GLenum target, GLenum pname, GLint param);
ES1X_API void ES1X_APIENTRY es1xTexEnvx(void *_context_, GLenum target, GLenum pname, GLfixed param);
ES1X_API void ES1X_APIENTRY es1xTexEnviv(void *_context_, GLenum target, GLenum pname, const GLint* params);
ES1X_API void ES1X_APIENTRY es1xTexEnvxv(void *_context_, GLenum target, GLenum pname, const GLfixed* params);
ES1X_API void ES1X_APIENTRY es1xTexImage2D(void *_context_, GLenum target, GLint level, GLint internalFormat,
                                       GLsizei width, GLsizei height, GLint border,
                                       GLenum format, GLenum type, const GLvoid* pixels);
ES1X_API void ES1X_APIENTRY es1xTexParameterf(void *_context_, GLenum target, GLenum pname, GLfloat param);
ES1X_API void ES1X_APIENTRY es1xTexParameterfv(void *_context_, GLenum target, GLenum pname, const GLfloat* params);
ES1X_API void ES1X_APIENTRY es1xTexParameteri(void *_context_, GLenum target, GLenum pname, GLint param);
ES1X_API void ES1X_APIENTRY es1xTexParameterx(void *_context_, GLenum target, GLenum pname, GLfixed param);
ES1X_API void ES1X_APIENTRY es1xTexParameteriv(void *_context_, GLenum target, GLenum pname, const GLint* params);
ES1X_API void ES1X_APIENTRY es1xTexParameterxv(void *_context_, GLenum target, GLenum pname, const GLfixed* params);
ES1X_API void ES1X_APIENTRY es1xTexSubImage2D(void *_context_, GLenum target, GLint level, GLint xoffset,
                                          GLint yoffset, GLsizei width, GLsizei height,
                                          GLenum format, GLenum type, const GLvoid* pixels);
ES1X_API void ES1X_APIENTRY es1xViewport(void *_context_, GLint x, GLint y, GLsizei width, GLsizei height);
ES1X_API void ES1X_APIENTRY es1xNormalPointer(void *_context_, GLenum type, GLsizei stride, const GLvoid* pointer);
ES1X_API void ES1X_APIENTRY es1xVertexPointer(void *_context_, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
ES1X_API void ES1X_APIENTRY es1xCurrentPaletteMatrixOES(void *_context_, GLuint matrixpaletteindex);
ES1X_API void ES1X_APIENTRY es1xLoadPaletteFromModelViewMatrixOES(void *_context_);
ES1X_API void ES1X_APIENTRY es1xMatrixIndexPointerOES(void *_context_, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
ES1X_API void ES1X_APIENTRY es1xWeightPointerOES(void *_context_, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
ES1X_API void ES1X_APIENTRY es1xGenerateMipmapOES(void *_context_, GLenum target);
ES1X_API GLboolean ES1X_APIENTRY es1xIsRenderbufferOES(void *_context_, GLuint renderbuffer);
ES1X_API void ES1X_APIENTRY es1xBindRenderbufferOES(void *_context_, GLenum target, GLuint renderbuffer);
ES1X_API void ES1X_APIENTRY es1xDeleteRenderbuffersOES(void *_context_, GLsizei n, const GLuint *renderbuffers);
ES1X_API void ES1X_APIENTRY es1xGenRenderbuffersOES(void *_context_, GLsizei n, GLuint *renderbuffers);
ES1X_API void ES1X_APIENTRY es1xRenderbufferStorageOES(void *_context_, GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
ES1X_API void ES1X_APIENTRY es1xGetRenderbufferParameterivOES(void *_context_, GLenum target, GLenum pname, GLint* params);
ES1X_API GLboolean ES1X_APIENTRY es1xIsFramebufferOES(void *_context_, GLuint framebuffer);
ES1X_API void ES1X_APIENTRY es1xBindFramebufferOES(void *_context_, GLenum target, GLuint framebuffer);
ES1X_API void ES1X_APIENTRY es1xDeleteFramebuffersOES(void *_context_, GLsizei n, const GLuint *framebuffers);
ES1X_API void ES1X_APIENTRY es1xGenFramebuffersOES(void *_context_, GLsizei n, GLuint *framebuffers);
ES1X_API GLenum ES1X_APIENTRY es1xCheckFramebufferStatusOES(void *_context_, GLenum target);
ES1X_API void ES1X_APIENTRY es1xFramebufferTexture2DOES(void *_context_, GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
ES1X_API void ES1X_APIENTRY es1xFramebufferRenderbufferOES(void *_context_, GLenum target, GLenum attachment,GLenum renderbuffertarget, GLuint renderbuffer);
ES1X_API void ES1X_APIENTRY es1xGetFramebufferAttachmentParameterivOES(void *_context_, GLenum target, GLenum attachment, GLenum pname, GLint *params);
ES1X_API void ES1X_APIENTRY es1xEGLImageTargetTexture2DOES(void *_context_, GLenum target, GLeglImageOES image);
ES1X_API void ES1X_APIENTRY es1xEGLImageTargetRenderbufferStorageOES(void *_context_, GLenum target, GLeglImageOES image);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _ES1XAPI_H
