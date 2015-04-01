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

 
#ifndef _ES1XMATRIX_H
#define _ES1XMATRIX_H

#ifndef _ES1XDEFS_H
#       include "es1xDefs.h"
#endif

#ifndef _ES1XVECTOR_H
#       include "es1xVector.h"
#endif

/* Matrix4x4 */
typedef struct
{
  GLfloat data[4][4];
} es1xMatrix4x4;

/* Matrix3x3 */
typedef struct
{
  GLfloat data[3][3];
} es1xMatrix3x3;

#ifdef __cplusplus
extern "C"
{
#endif

/* Matrix operations */
void            es1xMatrix4x4_load                              (es1xMatrix4x4* dst, const es1xMatrix4x4* src);
void            es1xMatrix4x4_loadx                             (es1xMatrix4x4* dst, const GLfixed* src);
void            es1xMatrix4x4_loadIdentity              (es1xMatrix4x4* dst);
void            es1xMatrix4x4_multiply                  (es1xMatrix4x4* dst, const es1xMatrix4x4* src);
void            es1xMatrix4x4_multiplyf                 (es1xMatrix4x4* dst, const GLfloat* src);
void            es1xMatrix4x4_createScale               (es1xMatrix4x4* dst, GLfloat x, GLfloat y, GLfloat z);
void            es1xMatrix4x4_createRotation    (es1xMatrix4x4* dst, GLfloat angleDeg, GLfloat x, GLfloat y, GLfloat z);
void            es1xMatrix4x4_createTranslation (es1xMatrix4x4* dst, GLfloat x, GLfloat y, GLfloat z);
void            es1xMatrix4x4_createFrustum             (es1xMatrix4x4* dst, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
void            es1xMatrix4x4_createOrtho               (es1xMatrix4x4* dst, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
void            es1xMatrix4x4_invert3x3                 (es1xMatrix3x3* dst, const es1xMatrix4x4* src);
void            es1xMatrix3x3_transpose                 (es1xMatrix3x3* dst);
void            es1xMatrix4x4_multiplyVec4              (const es1xMatrix4x4* matrix, const es1xVec4* vector, es1xVec4* result);
void            es1xMatrix4x4_multiplyVec3              (const es1xMatrix4x4* matrix, const es1xVec3* vector, es1xVec3* result);

// used in StateSetters
ES1X_API void ES1X_APIENTRY es1xMatrixMode(void *_context_, GLenum mode);

/* Debugging routines */
#ifdef ES1X_DEBUG
void            es1xMatrix4x4_load3x3                   (es1xMatrix4x4* dst, const es1xMatrix3x3* src);
GLboolean       es1xMatrix4x4_isIdentity                (const es1xMatrix4x4* src);
void            es1xMatrix4x4_dump                              (const es1xMatrix4x4* src);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _ES1XMATRIX_H */
