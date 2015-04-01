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

ES1X_API void ES1X_APIENTRY es1xEGLImageTargetTexture2DOES(void *_context_,
                                                       GLenum target,
                                                       GLeglImageOES image)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glEGLImageTargetRenderbufferStorageOES renderbufer=0x%x GLeglImageOES=%p",
                 target, (void *) image));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;
  gl2b->glEGLImageTargetTexture2DOES(target, image);
  es1xCheckError(_context_);
  return;
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xEGLImageTargetRenderbufferStorageOES(void *_context_,
                                                                 GLenum target,
                                                                 GLeglImageOES image)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glEGLImageTargetRenderbufferStorageOES renderbufer=0x%x GLeglImageOES=%p",
                 target, (void *) image));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;
  gl2b->glEGLImageTargetRenderbufferStorageOES(target, image);
  es1xCheckError(_context_);
  return;
}
