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

 
#ifndef _ES1XCOLOR_H
#define _ES1XCOLOR_H

#ifndef _ES1XDEFS_H
#       include "es1xDefs.h"
#endif

typedef struct
{
  GLfloat r;
  GLfloat g;
  GLfloat b;
  GLfloat a;
} es1xColor;

#ifdef __cplusplus
extern "C"
{
#endif

/* Color operations */
ES1X_INLINE void es1xColor_set(es1xColor* color, GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
  ES1X_ASSERT(color);

  ES1X_ASSERT(r >= 0.f && r <= 1.f);
  ES1X_ASSERT(g >= 0.f && g <= 1.f);
  ES1X_ASSERT(b >= 0.f && b <= 1.f);
  ES1X_ASSERT(a >= 0.f && a <= 1.f);

  color->r = r;
  color->g = g;
  color->b = b;
  color->a = a;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _ES1XCOLOR_H */
