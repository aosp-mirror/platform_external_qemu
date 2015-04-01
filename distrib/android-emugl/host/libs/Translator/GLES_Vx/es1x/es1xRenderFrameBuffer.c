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

#include "es1xDefs.h"
#include "es1xRouting.h"
#include "es1xContext.h"
#include "es1xDebug.h"
#include "es1xError.h"

/*---------------------------------------------------------------------------*/

ES1X_API GLboolean ES1X_APIENTRY es1xIsRenderbufferOES(void *_context_,
                                                   GLuint renderbuffer)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glIsRenderbufferOES renderbufer=0x%x", renderbuffer));
  ES1X_CHECK_CONTEXT_RET_FALSE(context);
  ES1X_ASSUME_NO_ERROR;
  GLboolean retval = gl2b->glIsRenderbuffer(renderbuffer);
  es1xCheckError(_context_);
  return retval;
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xBindRenderbufferOES(void *_context_,
                                               GLenum target,
                                               GLuint renderbuffer)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glBindRenderbufferOES target=0x%x renderbuffer=0x%x",
                 target, renderbuffer));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;
  gl2b->glBindRenderbuffer(target, renderbuffer);
  es1xCheckError(_context_);
  return;
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xDeleteRenderbuffersOES(void *_context_,
                                                  GLsizei n,
                                                  const GLuint *renderbuffers)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glDeleteRenderbuffersOES n=0x%x renderbufers=%p",
                 n, renderbuffers));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;
  gl2b->glDeleteRenderbuffers(n, renderbuffers);
  es1xCheckError(_context_);
  return;
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xGenRenderbuffersOES(void *_context_,
                                               GLsizei n,
                                               GLuint *renderbuffers)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGenRenderbuffersOES n=0x%x renderbuffers=%p",
                 n, renderbuffers));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;
  gl2b->glGenRenderbuffers(n, renderbuffers);
  es1xCheckError(_context_);
  return;
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xRenderbufferStorageOES(void *_context_,
                                                  GLenum target,
                                                  GLenum internalformat,
                                                  GLsizei width,
                                                  GLsizei height)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glRenderbufferStorageOES target=0x%x, "
                 "internalformat=0x%x width=0x%x weight=0x%x",
                 target, internalformat, width, height));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;
  gl2b->glRenderbufferStorage(target, internalformat, width, height);
  es1xCheckError(_context_);
  return;
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xGetRenderbufferParameterivOES(void *_context_,
                                                         GLenum target,
                                                         GLenum pname,
                                                         GLint* params)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetRenderbufferParameterivOES "
                 "target=0x%x, pname=0x%x params=%p",
                 target, pname, params));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;
  gl2b->glGetRenderbufferParameteriv(target, pname, params);
  es1xCheckError(_context_);
  return;
}

/*---------------------------------------------------------------------------*/

ES1X_API GLboolean ES1X_APIENTRY es1xIsFramebufferOES(void *_context_,
                                                 GLuint framebuffer)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glRenderbufferStorageOES %d", framebuffer));
  ES1X_CHECK_CONTEXT_RET_FALSE(context);
  ES1X_ASSUME_NO_ERROR;
  GLboolean retval = gl2b->glIsFramebuffer(framebuffer);
  es1xCheckError(_context_);
  return retval;
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xBindFramebufferOES(void *_context_,
                                              GLenum target,
                                              GLuint framebuffer)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glBindFramebufferOES target=0x%x framebuffer=0x%x",
                 target, framebuffer));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;
  gl2b->glBindFramebuffer(target, framebuffer);
  es1xCheckError(_context_);
  return;
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xDeleteFramebuffersOES(void *_context_,
                                                 GLsizei n,
                                                 const GLuint *framebuffers)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glDeleteFramebuffersOES n=0x%x framebufers=%p",
                 n, framebuffers));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;
  gl2b->glDeleteFramebuffers(n, framebuffers);
  es1xCheckError(_context_);
  return;
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xGenFramebuffersOES(void *_context_,
                                              GLsizei n,
                                              GLuint *framebuffers)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGenFramebuffersOES n=0x%x framebuffers=%p",
                 n, framebuffers));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;
  gl2b->glGenFramebuffers(n, framebuffers);
  es1xCheckError(_context_);
  return;
}

/*---------------------------------------------------------------------------*/

ES1X_API GLenum ES1X_APIENTRY es1xCheckFramebufferStatusOES(void *_context_,
                                                       GLenum target)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glCheckFramebufferStatusOES target=0x%x", target));
  ES1X_CHECK_CONTEXT_RETVAL(context, 0);
  ES1X_ASSUME_NO_ERROR;
  GLenum retval = gl2b->glCheckFramebufferStatus(target);
  es1xCheckError(_context_);
  return retval;
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xFramebufferTexture2DOES(void *_context_,
                                                   GLenum target,
                                                   GLenum attachment,
                                                   GLenum textarget,
                                                   GLuint texture,
                                                   GLint level)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glFramebufferTexture2DOES target=0x%x attachment=0x%x"
                 "textarget=0x%x texture=0x%x level=0x%x",
                 target, attachment, textarget, texture, level));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;
  gl2b->glFramebufferTexture2D(target, attachment,
                                 textarget, texture, level);
  es1xCheckError(_context_);
  return;
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xFramebufferRenderbufferOES(void *_context_,
                                                      GLenum target,
                                                      GLenum attachment,
                                                      GLenum renderbuffertarget,
                                                      GLuint renderbuffer)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glFramebufferRenderbufferOES target=0x%x attachment=0x%x"
                 "renderbuffertarget=0x%x renderbuffer=0x%x",
                 target, attachment, renderbuffertarget, renderbuffer));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;
  gl2b->glFramebufferRenderbuffer(target, attachment,
                                  renderbuffertarget, renderbuffer);
  es1xCheckError(_context_);
  return;
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xGetFramebufferAttachmentParameterivOES(void *_context_,
                                                                  GLenum target,
                                                                  GLenum attachment,
                                                                  GLenum pname,
                                                                  GLint *params)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glFramebufferRenderbufferOES target=0x%x attachment=0x%x"
                 "pname=0x%x params=%p", target, attachment, pname, params));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;
  gl2b->glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
  es1xCheckError(_context_);
  return;
}
