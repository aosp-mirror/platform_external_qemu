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

 
#ifndef _ES1XTEXENV_H
#define _ES1XTEXENV_H

#ifndef _ES1XDEFS_H
#       include "es1xDefs.h"
#endif

#ifndef _ES1XCOLOR_H
#       include "es1xColor.h"
#endif

/* es1xTexEnv */
typedef struct
{
  es1xColor color;
  GLfloat rgbScale;
  GLfloat alphaScale;
} es1xTexEnv;

#ifdef __cplusplus
extern "C"
{
#endif

void es1xTexEnv_init    (es1xTexEnv* env);
void es1xTexEnv_dump    (const es1xTexEnv* env);
GL_API void GL_APIENTRY es1xTexEnvi(void *_context_, GLenum target, GLenum pname, GLint param);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _ES1XENV_H */
