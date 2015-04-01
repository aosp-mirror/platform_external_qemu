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

 
#include "es1xMatrix.h"
#include "es1xMemory.h"
#include "es1xMath.h"
#include "es1xContext.h"
#include "es1xDebug.h"

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xMarkCurrentMatrixUniformDirty(void *_context_)
{
  es1xContext *context = (es1xContext *) _context_;
  switch(context->matrixMode)
  {
    case GL_MODELVIEW:
      ES1X_UPDATE_CONTEXT_MODELVIEW_MATRIX(ES1X_NULL_STATEMENT)
          /* Fallthrough */
    case GL_PROJECTION:
      ES1X_UPDATE_CONTEXT_PROJECTION_MODELVIEW_MATRIX(ES1X_NULL_STATEMENT);
      break;
    case GL_TEXTURE:
      ES1X_UPDATE_CONTEXT_TEXTURE_MATRIX(context->activeTextureUnitIndex, ES1X_NULL_STATEMENT);
      break;
    default:
      ES1X_ASSERT(!"Unknown matrix mode");
      break;
  }
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xMatrix4x4_copy(es1xMatrix4x4* dst, const es1xMatrix4x4* src)
{
  es1xMemCopy(dst, src, sizeof(es1xMatrix4x4));
}

/*---------------------------------------------------------------------------*/

void es1xMatrix4x4_load(es1xMatrix4x4* dst, const es1xMatrix4x4* src)
{
  es1xMatrix4x4_copy(dst, src);
}

/*---------------------------------------------------------------------------*/

/* Identity Matrix */
static const es1xMatrix4x4 s_identity =
{{
    {1.f, 0.f, 0.f, 0.f},
    {0.f, 1.f, 0.f, 0.f},
    {0.f, 0.f, 1.f, 0.f},
    {0.f, 0.f, 0.f, 1.f}
  }};

void es1xMatrix4x4_loadIdentity(es1xMatrix4x4* dst)
{
  ES1X_ASSERT(dst);

  es1xMatrix4x4_copy(dst, &s_identity);
}

/*---------------------------------------------------------------------------*/

void es1xMatrix4x4_loadx(es1xMatrix4x4* dst, const GLfixed* src)
{
  dst->data[0][0]       = es1xMath_xtof(src[0]);
  dst->data[0][1]       = es1xMath_xtof(src[1]);
  dst->data[0][2]       = es1xMath_xtof(src[2]);
  dst->data[0][3]       = es1xMath_xtof(src[3]);
  dst->data[1][0]       = es1xMath_xtof(src[4]);
  dst->data[1][1]       = es1xMath_xtof(src[5]);
  dst->data[1][2]       = es1xMath_xtof(src[6]);
  dst->data[1][3]       = es1xMath_xtof(src[7]);
  dst->data[2][0]       = es1xMath_xtof(src[8]);
  dst->data[2][1]       = es1xMath_xtof(src[9]);
  dst->data[2][2]       = es1xMath_xtof(src[10]);
  dst->data[2][3]       = es1xMath_xtof(src[11]);
  dst->data[3][0]       = es1xMath_xtof(src[12]);
  dst->data[3][1]       = es1xMath_xtof(src[13]);
  dst->data[3][2]       = es1xMath_xtof(src[14]);
  dst->data[3][3]       = es1xMath_xtof(src[15]);
}

/*---------------------------------------------------------------------------*/

void es1xMatrix4x4_multiplyf(es1xMatrix4x4* dst, const GLfloat* src)
{
  es1xMatrix4x4 temp;

  /**/

  GLfloat* dstPtr = (GLfloat*)dst->data;
  GLfloat* tmpPtr = (GLfloat*)temp.data;

  ES1X_ASSERT(dst);
  ES1X_ASSERT(src);

  tmpPtr[0]     = src[0]  * dstPtr[0] + src[1]  * dstPtr[4] + src[2]  * dstPtr[8]  + src[3]  * dstPtr[12];
  tmpPtr[1]     = src[0]  * dstPtr[1] + src[1]  * dstPtr[5] + src[2]  * dstPtr[9]  + src[3]  * dstPtr[13];
  tmpPtr[2]     = src[0]  * dstPtr[2] + src[1]  * dstPtr[6] + src[2]  * dstPtr[10] + src[3]  * dstPtr[14];
  tmpPtr[3]     = src[0]  * dstPtr[3] + src[1]  * dstPtr[7] + src[2]  * dstPtr[11] + src[3]  * dstPtr[15];
  tmpPtr[4]     = src[4]  * dstPtr[0] + src[5]  * dstPtr[4] + src[6]  * dstPtr[8]  + src[7]  * dstPtr[12];
  tmpPtr[5]     = src[4]  * dstPtr[1] + src[5]  * dstPtr[5] + src[6]  * dstPtr[9]  + src[7]  * dstPtr[13];
  tmpPtr[6]     = src[4]  * dstPtr[2] + src[5]  * dstPtr[6] + src[6]  * dstPtr[10] + src[7]  * dstPtr[14];
  tmpPtr[7]     = src[4]  * dstPtr[3] + src[5]  * dstPtr[7] + src[6]  * dstPtr[11] + src[7]  * dstPtr[15];
  tmpPtr[8]     = src[8]  * dstPtr[0] + src[9]  * dstPtr[4] + src[10] * dstPtr[8]  + src[11] * dstPtr[12];
  tmpPtr[9]     = src[8]  * dstPtr[1] + src[9]  * dstPtr[5] + src[10] * dstPtr[9]  + src[11] * dstPtr[13];
  tmpPtr[10]    = src[8]  * dstPtr[2] + src[9]  * dstPtr[6] + src[10] * dstPtr[10] + src[11] * dstPtr[14];
  tmpPtr[11]    = src[8]  * dstPtr[3] + src[9]  * dstPtr[7] + src[10] * dstPtr[11] + src[11] * dstPtr[15];
  tmpPtr[12]    = src[12] * dstPtr[0] + src[13] * dstPtr[4] + src[14] * dstPtr[8]  + src[15] * dstPtr[12];
  tmpPtr[13]    = src[12] * dstPtr[1] + src[13] * dstPtr[5] + src[14] * dstPtr[9]  + src[15] * dstPtr[13];
  tmpPtr[14]    = src[12] * dstPtr[2] + src[13] * dstPtr[6] + src[14] * dstPtr[10] + src[15] * dstPtr[14];
  tmpPtr[15]    = src[12] * dstPtr[3] + src[13] * dstPtr[7] + src[14] * dstPtr[11] + src[15] * dstPtr[15];

  /*

    int i = 0;
    int j = 0;
    int k = 0;

    ES1X_ASSERT(dst);
    ES1X_ASSERT(src);

    for(i=0;i<4;i++)
    {
    for(j=0;j<4;j++)
    {
    GLfloat sum = 0.f;

    for (k=0;k<4;k++)
    sum += src[i*4+k] * dst->data[k][j];

    temp.data[i][j] = sum;
    }
    }

    */

  es1xMatrix4x4_copy(dst, &temp);
}

void es1xMatrix4x4_multiply(es1xMatrix4x4* dst, const es1xMatrix4x4* src)
{
  es1xMatrix4x4_multiplyf(dst, (const GLfloat*)src->data);
}

/*---------------------------------------------------------------------------*/

void es1xMatrix4x4_createScale(es1xMatrix4x4* dst, GLfloat x, GLfloat y, GLfloat z)
{
  es1xMatrix4x4_loadIdentity(dst);

  dst->data[0][0] = x;
  dst->data[1][1] = y;
  dst->data[2][2] = z;
}

/*---------------------------------------------------------------------------*/

void es1xMatrix4x4_createRotation(es1xMatrix4x4* dst, GLfloat angleDeg, GLfloat x, GLfloat y, GLfloat z)
{
  GLfloat angleRad              = angleDeg * ES1X_PI / 180;
  GLfloat sinAngle              = es1xMath_sinf(angleRad);
  GLfloat cosAngle              = es1xMath_cosf(angleRad);
  GLfloat oneMinusCosine        = 1.f - cosAngle;

  es1xMath_normalize(&x, &y, &z);

  ES1X_ASSERT(dst);

  /* Create matrix */
  {
    /**/
    GLfloat oneMinusCosineMultX                 = oneMinusCosine * x;
    GLfloat oneMinusCosineMultZ                 = oneMinusCosine * z;
    GLfloat oneMinusCosineMultXMultY    = oneMinusCosineMultX * y;
    GLfloat oneMinusCosineMultZMultY    = oneMinusCosineMultZ * y;
    GLfloat sinAngleMultZ                               = sinAngle * z;
    GLfloat sinAngleMultX                               = sinAngle * x;
    GLfloat sinAngleMultY                               = sinAngle * y;

    dst->data[0][0] = (oneMinusCosineMultX * x)  + cosAngle;
    dst->data[1][0] = (oneMinusCosineMultXMultY) - (sinAngleMultZ);
    dst->data[2][0] = (oneMinusCosineMultX * z)  + (sinAngleMultY);
    dst->data[3][0] = 0;

    dst->data[0][1] = (oneMinusCosineMultXMultY) + (sinAngleMultZ);
    dst->data[1][1] = (oneMinusCosine * y * y)   + cosAngle;
    dst->data[2][1] = (oneMinusCosineMultZMultY) - (sinAngleMultX);
    dst->data[3][1] = 0;

    dst->data[0][2] = (oneMinusCosineMultZ * x)  - (sinAngleMultY);
    dst->data[1][2] = (oneMinusCosineMultZMultY) + (sinAngleMultX);
    dst->data[2][2] = (oneMinusCosineMultZ * z)  + cosAngle;
    dst->data[3][2] = 0;

    dst->data[0][3] = 0;
    dst->data[1][3] = 0;
    dst->data[2][3] = 0;
    dst->data[3][3] = 1;

    /*
      dst->data[0][0] = (oneMinusCosine * x * x) + cosAngle;
      dst->data[1][0] = (oneMinusCosine * x * y) - (z * sinAngle);
      dst->data[2][0] = (oneMinusCosine * x * z) + (y * sinAngle);
      dst->data[3][0] = 0;

      dst->data[0][1] = (oneMinusCosine * x * y) + (z * sinAngle);
      dst->data[1][1] = (oneMinusCosine * y * y) + cosAngle;
      dst->data[2][1] = (oneMinusCosine * z * y) - (x * sinAngle);
      dst->data[3][1] = 0;

      dst->data[0][2] = (oneMinusCosine * z * x) - (y * sinAngle);
      dst->data[1][2] = (oneMinusCosine * z * y) + (x * sinAngle);
      dst->data[2][2] = (oneMinusCosine * z * z) + cosAngle;
      dst->data[3][2] = 0;

      dst->data[0][3] = 0;
      dst->data[1][3] = 0;
      dst->data[2][3] = 0;
      dst->data[3][3] = 1;
      */
  }
}

/*---------------------------------------------------------------------------*/

void es1xMatrix4x4_createTranslation(es1xMatrix4x4* dst, GLfloat x, GLfloat y, GLfloat z)
{
  es1xMatrix4x4_loadIdentity(dst);

  dst->data[3][0] = x;
  dst->data[3][1] = y;
  dst->data[3][2] = z;
}

/*---------------------------------------------------------------------------*/

void es1xMatrix4x4_createFrustum(es1xMatrix4x4* dst, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
  GLfloat zNear2                        = zNear + zNear;
  GLfloat zFar2                 = zFar  + zFar;
  GLfloat rightPlusLeft = right + left;
  GLfloat rightMinusLeft        = right - left;
  GLfloat topPlusBottom = top   + bottom;
  GLfloat topMinusBottom        = top   - bottom;
  GLfloat zFarPluszNear = zFar  + zNear;
  GLfloat zFarMinuszNear        = zFar  - zNear;

  ES1X_ASSERT(dst);

  dst->data[0][0] = zNear2 / rightMinusLeft;
  dst->data[0][1] = 0.f;
  dst->data[0][2] = 0.f;
  dst->data[0][3] = 0.f;

  dst->data[1][0] = 0.f;
  dst->data[1][1] = zNear2 / topMinusBottom;
  dst->data[1][2] = 0.f;
  dst->data[1][3] = 0.f;

  dst->data[2][0] = rightPlusLeft / rightMinusLeft;
  dst->data[2][1] = topPlusBottom / topMinusBottom;
  dst->data[2][2] = -(zFarPluszNear / zFarMinuszNear);
  dst->data[2][3] = -1.f;

  dst->data[3][0] = 0.f;
  dst->data[3][1] = 0.f;
  dst->data[3][2] = -(zFar2 * zNear / zFarMinuszNear);
  dst->data[3][3] = 0.f;
}

/*---------------------------------------------------------------------------*/

void es1xMatrix4x4_createOrtho(es1xMatrix4x4* dst, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
  GLfloat rightPlusLeft = right + left;
  GLfloat rightMinusLeft        = right - left;
  GLfloat topPlusBottom = top + bottom;
  GLfloat topMinusBottom        = top - bottom;
  GLfloat zFarPluszNear = zFar + zNear;
  GLfloat zFarMinuszNear        = zFar - zNear;

  ES1X_ASSERT(dst);

  dst->data[0][0] = 2.f / rightMinusLeft;
  dst->data[0][1] = 0.f;
  dst->data[0][2] = 0.f;
  dst->data[0][3] = 0.f;

  dst->data[1][0] = 0.f;
  dst->data[1][1] = 2.f / topMinusBottom;
  dst->data[1][2] = 0.f;
  dst->data[1][3] = 0.f;

  dst->data[2][0] = 0.f;
  dst->data[2][1] = 0.f;
  dst->data[2][2] = -(2.f / zFarMinuszNear);
  dst->data[2][3] = 0.f;

  dst->data[3][0] = -(rightPlusLeft / rightMinusLeft);
  dst->data[3][1] = -(topPlusBottom / topMinusBottom);
  dst->data[3][2] = -(zFarPluszNear / zFarMinuszNear);
  dst->data[3][3] = 1.f;
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xMultMatrixf(void *_context_, const GLfloat* matrix)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glMultMatrixf(0x%p)\n", matrix));
  ES1X_CHECK_CONTEXT(context);

  es1xMatrix4x4_multiplyf(es1xMatrixStack_peekMatrix(context->currentMatrixStack), matrix);
  es1xMarkCurrentMatrixUniformDirty(context);
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xMultMatrixx(void *_context_, const GLfixed* matrix)
{
  //  es1xContext *context = (es1xContext *) _context_;
  es1xMatrix4x4 dst;
  es1xMatrix4x4_loadx(&dst, matrix);
  ES1X_LOG_CALL(("glMultMatrixx(0x%p)\n", matrix));
  es1xMultMatrixf(_context_, (const GLfloat*) dst.data);
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xLoadMatrixf(void *_context_, const GLfloat* matrix)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glLoadMatrixf(0x%p)\n", matrix));
  ES1X_CHECK_CONTEXT(context);

  es1xMatrixStack_loadMatrix(context->currentMatrixStack, matrix);
  es1xMarkCurrentMatrixUniformDirty(context);

}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xLoadMatrixx(void *_context_, const GLfixed* matrix)
{
  //   es1xContext *context = (es1xContext *) _context_;
  es1xMatrix4x4 dst;
  es1xMatrix4x4_loadx(&dst, matrix);
  ES1X_LOG_CALL(("glLoadMatrixx(0x%p)\n", matrix));
  es1xLoadMatrixf(_context_, (const GLfloat*)dst.data);
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xMatrixMode(void *_context_, GLenum mode)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glMatrixMode(%s)\n", ES1X_ENUM_TO_STRING(mode)));
  ES1X_CHECK_CONTEXT(context);

  switch(mode)
  {
    /* First set matrix stack pointer */
    case GL_MODELVIEW:  context->currentMatrixStack = context->modelViewMatrixStack;                                                            break;
    case GL_PROJECTION: context->currentMatrixStack = context->projectionMatrixStack;                                                           break;
    case GL_TEXTURE:    context->currentMatrixStack = context->textureMatrixStack[context->activeTextureUnitIndex];     break;
    default:
      es1xSetError(_context_, GL_INVALID_ENUM);
      return;
  }

  context->matrixMode = mode;
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xLoadIdentity (void *_context_)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glLoadIdentity also calls --> "));
  es1xLoadMatrixf(_context_, &(s_identity.data[0][0]));
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xRotatef(void *_context_, GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glRotatef(%.8f, %.8f, %.8f, %.8f)\n", angle, x, y, z));
  ES1X_CHECK_CONTEXT(context);

  /* create and apply rotation matrix */
  {
    es1xMatrix4x4       rotation;
    es1xMatrix4x4*      top     = es1xMatrixStack_peekMatrix(context->currentMatrixStack);

    es1xMatrix4x4_createRotation(&rotation, angle, x, y, z);
    es1xMatrix4x4_multiply(top, &rotation);

    es1xMarkCurrentMatrixUniformDirty(context);
  }
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xRotatex(void *_context_, GLfixed angle, GLfixed x, GLfixed y, GLfixed z)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glRotatex(0x%x, 0x%x, 0x%x, 0x%x)\n", angle, x, y, z));
  es1xRotatef(_context_, es1xMath_xtof(angle), es1xMath_xtof(x), es1xMath_xtof(y), es1xMath_xtof(z));
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xScalef(void *_context_, GLfloat x, GLfloat y, GLfloat z)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glScalef(%.8f, %.8f, %.8f)\n", x, y, z));
  ES1X_CHECK_CONTEXT(context);

  /* Create and apply scale matrix */
  {
    es1xMatrix4x4       scale;
    es1xMatrix4x4*      top     = es1xMatrixStack_peekMatrix(context->currentMatrixStack);

    es1xMatrix4x4_createScale(&scale, x, y, z);
    es1xMatrix4x4_multiply(top, &scale);

    es1xMarkCurrentMatrixUniformDirty(context);
  }
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xScalex(void *_context_, GLfixed x, GLfixed y, GLfixed z)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glScalex(%.8x, %.8x, %.8x)\n", x, y, z));
  es1xScalef(_context_, es1xMath_xtof(x), es1xMath_xtof(y), es1xMath_xtof(z));
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xFrustumf(void *_context_, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near, GLfloat far)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glFrustumf(%.8f, %.8f, %.8f, %.8f, %.8f, %.8f)\n", left, right, bottom, top, near, far));
  ES1X_CHECK_CONTEXT(context);

  /* Check invalid input */
  if (near      <= 0.f          ||
      far               <= 0.f          ||
      left      == right        ||
      bottom    == top          ||
      near      == far)
  {
    es1xSetError(_context_, GL_INVALID_VALUE);
    return;
  }
  else
  {
    es1xMatrix4x4       frustum;
    es1xMatrix4x4*      current = es1xMatrixStack_peekMatrix(context->currentMatrixStack);

    es1xMatrix4x4_createFrustum(&frustum, left, right, bottom, top, near, far);
    es1xMatrix4x4_multiply(current, &frustum);
    es1xMarkCurrentMatrixUniformDirty(context);
  }
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xFrustumx(void *_context_, GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glFrustumx(%.8x, %.8x, %.8x, %.8x, %.8x, %.8x)\n", left, right, bottom, top, zNear, zFar));
  es1xFrustumf(_context_,
               es1xMath_xtof(left),
               es1xMath_xtof(right),
               es1xMath_xtof(bottom),
               es1xMath_xtof(top),
               es1xMath_xtof(zNear),
               es1xMath_xtof(zFar));
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xOrthof(void *_context_, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near, GLfloat far)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glOrthof(%.8f, %.8f, %.8f, %.8f, %.8f, %.8f)\n", left, right, bottom, top, near, far));
  ES1X_CHECK_CONTEXT(context);

  /* Check invalid input */
  if (left      == right        ||
      bottom    == top          ||
      near      == far)
  {
    es1xSetError(_context_, GL_INVALID_VALUE);
    return;
  }
  else
  {
    es1xMatrix4x4       ortho;
    es1xMatrix4x4*      current = es1xMatrixStack_peekMatrix(context->currentMatrixStack);

    es1xMatrix4x4_createOrtho(&ortho, left, right, bottom, top, near, far);
    es1xMatrix4x4_multiply(current, &ortho);
    es1xMarkCurrentMatrixUniformDirty(context);
  }
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xOrthox(void *_context_, GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed near, GLfixed far)
{
  //   es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glOrthox(%.8x, %.8x, %.8x, %.8x, %.8x, %.8x)\n", left, right, bottom, top, near, far));

  es1xOrthof(_context_,
             es1xMath_xtof(left),
             es1xMath_xtof(right),
             es1xMath_xtof(bottom),
             es1xMath_xtof(top),
             es1xMath_xtof(near),
             es1xMath_xtof(far));
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xTranslatef(void *_context_, GLfloat x, GLfloat y, GLfloat z)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glTranslatef(%.8f, %.8f, %.8f)\n", x, y, z));
  ES1X_CHECK_CONTEXT(context);

  /* Create and apply translation matrix */
  {
    es1xMatrix4x4       translation;
    es1xMatrix4x4*      current = es1xMatrixStack_peekMatrix(context->currentMatrixStack);

    es1xMatrix4x4_createTranslation(&translation, x, y, z);
    es1xMatrix4x4_multiply(current, &translation);
    es1xMarkCurrentMatrixUniformDirty(context);
  }
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xTranslatex(void *_context_, GLfixed x, GLfixed y, GLfixed z)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glTranslatex(%.8x, %.8x, %.8x)\n", x, y, z));
  es1xTranslatef(_context_, es1xMath_xtof(x), es1xMath_xtof(y), es1xMath_xtof(z));
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xPopMatrix(void *_context_)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glPopMatrix()\n"));
  ES1X_CHECK_CONTEXT(context);

  /* Pop matrix */
  {
    es1xMatrixStack* stack = context->currentMatrixStack;

    if (stack->top > 0)
    {
      es1xMatrixStack_popMatrix(stack);
      es1xMarkCurrentMatrixUniformDirty(context);
    }
    else
      es1xSetError(_context_, GL_STACK_UNDERFLOW);
  }
}

/*---------------------------------------------------------------------------*/

ES1X_API void ES1X_APIENTRY es1xPushMatrix(void *_context_)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glPushMatrix()\n"));
  ES1X_CHECK_CONTEXT(context);

  /* Push matrix */
  {
    es1xMatrixStack* stack = context->currentMatrixStack;

    if (stack->top < stack->size-1)
    {
      es1xMatrixStack_pushMatrix(stack, es1xMatrixStack_peekMatrix(stack));
      es1xMarkCurrentMatrixUniformDirty(context);
    }
    else
      es1xSetError(_context_, GL_STACK_OVERFLOW);
  }
}

/*---------------------------------------------------------------------------*/

void es1xMatrix3x3_transpose(es1xMatrix3x3* dst)
{
#define SWAP(a, b) { float temp = a; a = b; b = temp; }

  SWAP((dst->data[0][1]), (dst->data[1][0]));
  SWAP((dst->data[0][2]), (dst->data[2][0]));
  SWAP((dst->data[1][2]), (dst->data[2][1]));

#undef SWAP
}

/*---------------------------------------------------------------------------*/

void es1xMatrix4x4_invert3x3(es1xMatrix3x3* dst, const es1xMatrix4x4* src)
{
  ES1X_ASSERT(dst);
  ES1X_ASSERT(src);

  /* Compute inversion: A * A^-1 = I */
  {
    float determinant;

    /* Compute determinant */

    /**/
    GLfloat*            dstData = (GLfloat*)dst->data;
    const GLfloat*      srcData = (const GLfloat*)src->data;

    determinant = srcData[0] * srcData[5] * srcData[10]
                  + srcData[4] * srcData[9] * srcData[2]
                  + srcData[8] * srcData[1] * srcData[6]
                  - srcData[0] * srcData[9] * srcData[6]
                  - srcData[4] * srcData[1] * srcData[10]
                  - srcData[8] * srcData[5] * srcData[2];

    dstData[0] = srcData[5] * srcData[10] - srcData[9] * srcData[6];
    dstData[3] = srcData[8] * srcData[6]  - srcData[4] * srcData[10];
    dstData[6] = srcData[4] * srcData[9]  - srcData[8] * srcData[5];
    dstData[1] = srcData[9] * srcData[2]  - srcData[1] * srcData[10];
    dstData[4] = srcData[0] * srcData[10] - srcData[8] * srcData[2];
    dstData[7] = srcData[8] * srcData[1]  - srcData[0] * srcData[9];
    dstData[2] = srcData[1] * srcData[6]  - srcData[5] * srcData[2];
    dstData[5] = srcData[4] * srcData[2]  - srcData[0] * srcData[6];
    dstData[8] = srcData[0] * srcData[5]  - srcData[4] * srcData[1];

    /* Check if the matrix is singular */
    if (determinant == 0.f)
      return;

    /* Apply determinant */
    if (determinant != 1.f)
    {
      float d = 1 / determinant;

      dstData[0] *= d;  dstData[3] *= d; dstData[6] *= d;
      dstData[1] *= d;  dstData[4] *= d; dstData[7] *= d;
      dstData[2] *= d;  dstData[5] *= d; dstData[8] *= d;
    }
    /*
      determinant       = src->data[0][0] * src->data[1][1] * src->data[2][2]
      + src->data[1][0] * src->data[2][1] * src->data[0][2]
      + src->data[2][0] * src->data[0][1] * src->data[1][2]
      - src->data[0][0] * src->data[2][1] * src->data[1][2]
      - src->data[1][0] * src->data[0][1] * src->data[2][2]
      - src->data[2][0] * src->data[1][1] * src->data[0][2];

      dst->data[0][0] = src->data[1][1] * src->data[2][2] - src->data[2][1] * src->data[1][2];
      dst->data[1][0] = src->data[2][0] * src->data[1][2] - src->data[1][0] * src->data[2][2];
      dst->data[2][0] = src->data[1][0] * src->data[2][1] - src->data[2][0] * src->data[1][1];
      dst->data[0][1] = src->data[2][1] * src->data[0][2] - src->data[0][1] * src->data[2][2];
      dst->data[1][1] = src->data[0][0] * src->data[2][2] - src->data[2][0] * src->data[0][2];
      dst->data[2][1] = src->data[2][0] * src->data[0][1] - src->data[0][0] * src->data[2][1];
      dst->data[0][2] = src->data[0][1] * src->data[1][2] - src->data[1][1] * src->data[0][2];
      dst->data[1][2] = src->data[1][0] * src->data[0][2] - src->data[0][0] * src->data[1][2];
      dst->data[2][2] = src->data[0][0] * src->data[1][1] - src->data[1][0] * src->data[0][1];

      if (determinant == 0.f)
      return;

      if (determinant != 1.f)
      {
      float d = 1 / determinant;

      dst->data[0][0] *= d;     dst->data[1][0] *= d; dst->data[2][0] *= d;
      dst->data[0][1] *= d;     dst->data[1][1] *= d; dst->data[2][1] *= d;
      dst->data[0][2] *= d;     dst->data[1][2] *= d; dst->data[2][2] *= d;
      }
      */
  }
}

/*---------------------------------------------------------------------------*/

void es1xMatrix4x4_multiplyVec4(const es1xMatrix4x4* matrix, const es1xVec4* vector, es1xVec4* result)
{
  ES1X_ASSERT(matrix);
  ES1X_ASSERT(vector);
  ES1X_ASSERT(result);

  result->x =   vector->x * matrix->data[0][0] +
                vector->y * matrix->data[1][0] +
                vector->z * matrix->data[2][0] +
                vector->w * matrix->data[3][0];

  result->y =   vector->x * matrix->data[0][1] +
                vector->y * matrix->data[1][1] +
                vector->z * matrix->data[2][1] +
                vector->w * matrix->data[3][1];

  result->z =   vector->x * matrix->data[0][2] +
                vector->y * matrix->data[1][2] +
                vector->z * matrix->data[2][2] +
                vector->w * matrix->data[3][2];

  result->w =   vector->x * matrix->data[0][3] +
                vector->y * matrix->data[1][3] +
                vector->z * matrix->data[2][3] +
                vector->w * matrix->data[3][3];
}


/*---------------------------------------------------------------------------*/

void es1xMatrix4x4_multiplyVec3(const es1xMatrix4x4* matrix, const es1xVec3* vector, es1xVec3* result)
{
  ES1X_ASSERT(matrix);
  ES1X_ASSERT(vector);
  ES1X_ASSERT(result);

  result->x =   vector->x * matrix->data[0][0] +
                vector->y * matrix->data[1][0] +
                vector->z * matrix->data[2][0];

  result->y =   vector->x * matrix->data[0][1] +
                vector->y * matrix->data[1][1] +
                vector->z * matrix->data[2][1];

  result->z =   vector->x * matrix->data[0][2] +
                vector->y * matrix->data[1][2] +
                vector->z * matrix->data[2][2];
}

/*---------------------------------------------------------------------------*/

#ifdef ES1X_DEBUG
#       include <stdio.h>

void es1xMatrix4x4_load3x3(es1xMatrix4x4* dst, const es1xMatrix3x3* src)
{
  int i, j;
  es1xMatrix4x4_loadIdentity(dst);

  for(i=0;i<3;i++)
    for(j=0;j<3;j++)
      dst->data[j][i] = src->data[j][i];
}

void es1xMatrix4x4_dump(const es1xMatrix4x4* src)
{
  int i=0;

  ES1X_ASSERT(src);

  for(i=0;i<4;i++)
    printf("[ %.8f, %.8f, %.8f, %.8f ]\n", src->data[0][i], src->data[1][i], src->data[2][i], src->data[3][i]);
}

GLboolean es1xMatrix4x4_isIdentity(const es1xMatrix4x4* src)
{
  int i=0;
  int j=0;

  ES1X_ASSERT(src);

  for(i=0;i<4;i++)
    for(j=0;j<4;j++)
      if (src->data[i][j] != s_identity.data[i][j])
        return GL_FALSE;

  return GL_TRUE;
}

#endif /* ES1X_DEBUG */
