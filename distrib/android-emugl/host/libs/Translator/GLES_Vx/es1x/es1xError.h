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

 
#ifndef _ES1XERROR_H
#define _ES1XERROR_H

#ifndef _ES1XDEFS_H
#       include "es1xDefs.h"
#endif

#ifndef _ES1XROUTING_H
#       include "es1xRouting.h"
#endif

#ifndef _ES1XDEBUG_H
#       include "es1xDebug.h"
#endif

struct _es1xContext_;

#ifdef __cplusplus
extern "C"
{
#endif

/* For internal use only */
void es1xSetError(void *_context_, GLenum errorCode);

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Macros for convenient error handling */
#define ES1X_SET_ERROR_AND_RETURN_IF_TRUE(CONTEXT, EXPRESSION, ERROR)   \
  {                                                                     \
    if (EXPRESSION)                                                     \
    {                                                                   \
      es1xSetError(CONTEXT, ERROR);                                     \
      return;                                                           \
    }                                                                   \
  }

//    ES1X_ASSERT(errorCode == GL_NO_ERROR);
#define ES1X_ASSUME_NO_ERROR                                            \
  {                                                                     \
    ES1X_DEBUG_CODE(GLenum errorCode = gl2b->glGetError());            \
    if(errorCode != GL_NO_ERROR)                                        \
      ES1X_LOG(("!!! es1x glERROR 0x%x", errorCode));                   \
  } while(deGetFalse())

#define ES1X_SET_ERROR_AND_RETURN_IF_FALSE(CONTEXT, EXPRESSION, ERROR)  \
  ES1X_SET_ERROR_AND_RETURN_IF_TRUE(CONTEXT, !(EXPRESSION), ERROR)

#define ES1X_SET_ERROR_AND_RETURN_IF_NULL(CONTEXT, EXPRESSION, ERROR)   \
  ES1X_SET_ERROR_AND_RETURN_IF_TRUE(CONTEXT, (EXPRESSION) == ES1X_NULL, ERROR)

ES1X_INLINE void es1xCheckError(void *_context_)
{
  GLenum error = gl2b->glGetError();

  if (error != GL_NO_ERROR)
    es1xSetError(_context_, error);
}


#endif /* _ES1XERROR_H */
