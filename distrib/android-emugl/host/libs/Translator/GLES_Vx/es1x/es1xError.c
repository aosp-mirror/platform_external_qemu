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

 
#include "es1xError.h"
#include "es1xDebug.h"
#include "es1xContext.h"


/*---------------------------------------------------------------------------*/

ES1X_API GLenum ES1X_APIENTRY es1xGetError(void *_context_)
{
  struct _es1xContext_* context = (struct _es1xContext_*) _context_;
  ES1X_LOG_CALL(("glGetError\n"));

  if (context != 0)
  {
    /* clear last error and return it */
    GLenum errorCode = context->lastError;
    context->lastError = GL_NO_ERROR;
    return errorCode;
  }

  /* return error in case that wrapper has not been initialized yet */
  return GL_INVALID_OPERATION;
}

/*---------------------------------------------------------------------------*/

void es1xSetError(void* _context_, GLenum errorCode)
{
  struct _es1xContext_* context = (struct _es1xContext_*) _context_;
  ES1X_ASSERT(context);

  /* Do not allow setting error codes returned by OpenGL ES 2.0.
     The following exist in OpenGL ES 1.x */
  ES1X_ASSERT(errorCode == GL_INVALID_ENUM              ||
              errorCode == GL_INVALID_VALUE             ||
              errorCode == GL_INVALID_OPERATION ||
              errorCode == GL_STACK_OVERFLOW            ||
              errorCode == GL_STACK_UNDERFLOW           ||
              errorCode == GL_OUT_OF_MEMORY);

  /* Ignore this error in case that user has not yet retrieved
     previous error */
  if (context->lastError != GL_NO_ERROR)
    return;

  context->lastError = errorCode;
}

/*---------------------------------------------------------------------------*/
