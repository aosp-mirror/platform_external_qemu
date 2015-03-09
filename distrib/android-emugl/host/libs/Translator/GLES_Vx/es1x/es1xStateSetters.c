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
#include "es1xMath.h"
#include "es1xShaderPrograms.h"
#include "es1xShaderContext.h"
#include "es1xContext.h"
#include "es1xDebug.h"
#include "es1xTypeConverter.h"
#include "es1xRouting.h"

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLboolean es1xIsValidVertexPositionDataType(GLenum type)
{
  if (type == GL_BYTE || type == GL_UNSIGNED_BYTE || type == GL_SHORT || type == GL_FIXED || type == GL_FLOAT)
    return GL_TRUE;

  return GL_FALSE;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLboolean es1xIsValidVertexNormalDataType(GLenum type)
{
  if (type == GL_BYTE || type == GL_UNSIGNED_BYTE || type == GL_SHORT || type == GL_FIXED || type == GL_FLOAT)
    return GL_TRUE;

  return GL_FALSE;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLboolean es1xIsValidVertexColorDataType(GLenum type)
{
  if (type == GL_UNSIGNED_BYTE || type == GL_FIXED || type == GL_FLOAT)
    return GL_TRUE;

  return GL_FALSE;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLboolean es1xIsValidVertexPointSizeDataType(GLenum type)
{
  if (type == GL_FIXED || type == GL_FLOAT)
    return GL_TRUE;

  return GL_FALSE;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLboolean es1xIsValidVertexTextureCoordinateDataType(GLenum type)
{
  if (type == GL_BYTE || type == GL_SHORT || type == GL_FIXED || type == GL_FLOAT)
    return GL_TRUE;

  return GL_FALSE;
}

/*---------------------------------------------------------------------------*/

#if defined ES1X_DUPLICATE_ES2_STATE

static GLboolean es1xIsAllowedBlendSrcFunc(GLenum src)
{
  if (src == GL_ZERO                    ||
      src == GL_ONE     >               ||
      src == GL_DST_COLOR               ||
      src == GL_ONE_MINUS_DST_COLOR     ||
      src == GL_SRC_ALPHA               ||
      src == GL_ONE_MINUS_SRC_ALPHA     ||
      src == GL_DST_ALPHA               ||
      src == GL_ONE_MINUS_DST_ALPHA     ||
      src == GL_SRC_ALPHA_SATURATE)
    return GL_TRUE;

  return GL_FALSE;
}

/*---------------------------------------------------------------------------*/

static GLboolean es1xIsAllowedBlendDstFunc(GLenum dst)
{
  if (dst == GL_ZERO                                    ||
      dst == GL_ONE                                     ||
      dst == GL_SRC_COLOR                               ||
      dst == GL_ONE_MINUS_SRC_COLOR                     ||
      dst == GL_SRC_ALPHA                               ||
      dst == GL_ONE_MINUS_SRC_ALPHA                     ||
      dst == GL_DST_ALPHA                               ||
      dst == GL_ONE_MINUS_DST_ALPHA)
    return GL_TRUE;

  return GL_FALSE;
}

/*---------------------------------------------------------------------------*/

static GLboolean es1xIsAllowedDepthFunc(GLenum func)
{
  if (func == GL_NEVER          ||
      func == GL_ALWAYS         ||
      func == GL_LESS           ||
      func == GL_LEQUAL         ||
      func == GL_EQUAL          ||
      func == GL_GREATER        ||
      func == GL_GEQUAL         ||
      func == GL_NOTEQUAL)
    return GL_TRUE;

  return GL_FALSE;
}

#endif /* ES1X_DUPLICATE_ES2_STATE */

/*---------------------------------------------------------------------------*/

static void es1xSetClientState(void *_context_,
                               GLenum array,
                               GLboolean value)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;
  value = es1xMath_clampb(value);

  switch(array)
  {
    case GL_VERTEX_ARRAY:
      {
        if (value)
          gl2b->glEnableVertexAttribArray(ES1X_VERTEX_ARRAY_POS);
        else
          gl2b->glDisableVertexAttribArray(ES1X_VERTEX_ARRAY_POS);

        ES1X_ASSUME_NO_ERROR;
        ES1X_UPDATE_CONTEXT_STATE(context->vertexArrayEnabled = value);
        break;
      }

    case GL_NORMAL_ARRAY:
      {
        if (value)
          gl2b->glEnableVertexAttribArray(ES1X_NORMAL_ARRAY_POS);
        else
          gl2b->glDisableVertexAttribArray(ES1X_NORMAL_ARRAY_POS);

        ES1X_ASSUME_NO_ERROR;

        ES1X_UPDATE_CONTEXT_STATE(context->normalArrayEnabled = value);
        break;
      }

    case GL_COLOR_ARRAY:
      {
        if (value)
          gl2b->glEnableVertexAttribArray(ES1X_COLOR_ARRAY_POS);
        else
          gl2b->glDisableVertexAttribArray(ES1X_COLOR_ARRAY_POS);

        ES1X_ASSUME_NO_ERROR;
        ES1X_UPDATE_CONTEXT_STATE(context->colorArrayEnabled = value);
        break;
      }

    case GL_POINT_SIZE_ARRAY_OES:
      {
        if (value)
          gl2b->glEnableVertexAttribArray(ES1X_POINT_SIZE_ARRAY_POS);
        else
          gl2b->glDisableVertexAttribArray(ES1X_POINT_SIZE_ARRAY_POS);

        ES1X_ASSUME_NO_ERROR;
        ES1X_UPDATE_CONTEXT_STATE(context->pointSizeArrayOESEnabled = value);
        break;
      }

    case GL_TEXTURE_COORD_ARRAY:
      {
        int textureUnitNdx = context->clientActiveTexture;
        ES1X_ASSERT(textureUnitNdx >= 0 && textureUnitNdx < ES1X_NUM_SUPPORTED_TEXTURE_UNITS);

        if (value)
          gl2b->glEnableVertexAttribArray(ES1X_TEXTURE_COORD_ARRAY0_POS + textureUnitNdx);
        else
          gl2b->glDisableVertexAttribArray(ES1X_TEXTURE_COORD_ARRAY0_POS + textureUnitNdx);

        ES1X_ASSUME_NO_ERROR;
        ES1X_UPDATE_CONTEXT_STATE(context->textureCoordArrayEnabled[textureUnitNdx] = value);
        break;
      }
    default:
      es1xSetError(_context_, GL_INVALID_ENUM);
      break;
  }
}

/*---------------------------------------------------------------------------*/

static void es1xSet(void* _context_,
                    GLenum cap,
                    GLboolean value)
{
  es1xContext *context = (es1xContext *) _context_;

  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;
  value = es1xMath_clampb(value);

  switch(cap)
  {
    case GL_TEXTURE_2D:
      {
        ES1X_UPDATE_CONTEXT_STATE(context->textureUnitEnabled[context->activeTextureUnitIndex] = value);

        /* Check if texturing should be enabled */
        {
          int unitNdx;

          context->texturingEnabled = GL_FALSE;

          for(unitNdx =0;unitNdx<ES1X_NUM_SUPPORTED_TEXTURE_UNITS;unitNdx++)
            if (context->textureUnitEnabled[unitNdx])
              context->texturingEnabled = GL_TRUE;
        }
        break;
      }

    case GL_CULL_FACE:
    case GL_POLYGON_OFFSET_FILL:
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
    case GL_SAMPLE_COVERAGE:
    case GL_SAMPLE_COVERAGE_INVERT:
    case GL_SAMPLE_COVERAGE_VALUE:
    case GL_SCISSOR_TEST:
    case GL_STENCIL_TEST:
    case GL_DEPTH_TEST:
    case GL_BLEND:
    case GL_DITHER:
      {
        if (value == GL_FALSE)
          gl2b->glDisable(cap);
        else
          gl2b->glEnable(cap);

        /* glEnable/glDisable should not cause errors because valid values
           are already checked in the wrapper */
        ES1X_ASSUME_NO_ERROR;

#if defined ES1X_DUPLICATE_ES2_STATE
        switch(cap)
        {
          case GL_CULL_FACE:
            context->cullFaceEnabled = value;
            break;
          case GL_POLYGON_OFFSET_FILL:
            context->polygonOffsetFillEnabled = value;
            break;
          case GL_SAMPLE_ALPHA_TO_COVERAGE:
            context->sampleAlphaToCoverageEnabled = value;
            break;
          case GL_SAMPLE_COVERAGE:
            context->sampleCoverageEnabled = value;
            break;
          case GL_SAMPLE_COVERAGE_INVERT:
            context->sampleCoverageInvertEnabled = value;
            break;
          case GL_SAMPLE_COVERAGE_VALUE:
            context->sampleCoverageValueEnabled = value;
            break;
          case GL_SCISSOR_TEST:
            context->scissorTestEnabled = value;
            break;
          case GL_STENCIL_TEST:
            context->stencilTestEnabled = value;
            break;
          case GL_DEPTH_TEST:
            context->depthTestEnabled = value;
            break;
          case GL_BLEND:
            context->blendEnabled = value;
            break;
          case GL_DITHER:
            context->ditherEnabled = value;
            break;

          default:
            ES1X_ASSERT(!"Unhandled capability");
            break;
        }
#endif /* ES1X_DUPLICATE_ES2_STATE */

        break;
      }

    case GL_NORMALIZE:
      ES1X_UPDATE_CONTEXT_STATE(context->normalizeEnabled = value);
      break;
    case GL_RESCALE_NORMAL:
      ES1X_UPDATE_CONTEXT_STATE(context->rescaleNormalEnabled = value);
      break;
    case GL_FOG:
      ES1X_UPDATE_CONTEXT_STATE(context->fogEnabled = value);
      break;
    case GL_LIGHTING:
      ES1X_UPDATE_CONTEXT_STATE(context->lightingEnabled = value);
      break;
    case GL_COLOR_MATERIAL:
      ES1X_UPDATE_CONTEXT_STATE(context->colorMaterialEnabled = value);
      break;

    case GL_SAMPLE_ALPHA_TO_ONE:
      ES1X_UPDATE_CONTEXT_STATE(context->sampleAlphaToOneEnabled = value);
      break;
    case GL_POINT_SMOOTH:
      ES1X_UPDATE_CONTEXT_STATE(context->pointAntialiasingEnabled = value);
      break;
    case GL_MULTISAMPLE:
      ES1X_UPDATE_CONTEXT_STATE(context->multiSamplingEnabled = value);
      break;
    case GL_POINT_SPRITE_OES:
      ES1X_UPDATE_CONTEXT_STATE(context->pointSpriteOESEnabled = value);
      break;
    case GL_LINE_SMOOTH:
      ES1X_UPDATE_CONTEXT_STATE(context->smoothLinesEnabled = value);
      break;
    case GL_ALPHA_TEST:
      ES1X_UPDATE_CONTEXT_STATE(context->alphaTestEnabled = value);
      break;

      /* \todo this must be rerouted to underlying implementation similar to if glLogicOp is available */
    case GL_COLOR_LOGIC_OP:
      ES20_DEBUG_CODE(context->colorLogicOpEnabled = value);
      break;

    default:
      {
        /* Handle clip planes */
        if (cap >= GL_CLIP_PLANE0 && cap < GL_CLIP_PLANE0 + ES1X_NUM_SUPPORTED_CLIP_PLANES)
        {
          int ndx = cap - GL_CLIP_PLANE0;
          ES1X_UPDATE_CONTEXT_STATE(context->clipPlaneEnabled[ndx] = value);
          break;
        }

        /* Handle lights */
        if (cap >= GL_LIGHT0 && cap < GL_LIGHT0 + ES1X_NUM_SUPPORTED_LIGHTS)
        {
          int ndx = cap - GL_LIGHT0;
          ES1X_UPDATE_CONTEXT_STATE(context->lightEnabled[ndx] = value);
          break;
        }

        es1xSetError(_context_, GL_INVALID_ENUM);
        break;
      }
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xActiveTexture(void *_context_,
                                          GLenum pname)
{
  es1xContext *context = (es1xContext *) _context_;
  int textureUnit = pname - GL_TEXTURE0;

  ES1X_LOG_CALL(("glActiveTexture(%d)\n",
                 pname));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  if (textureUnit >= 0 && textureUnit < ES1X_NUM_SUPPORTED_TEXTURE_UNITS)
  {
    gl2b->glActiveTexture(pname);
    es1xCheckError(_context_);

    context->activeTextureUnitIndex = textureUnit;

    /* Ensure that correct texture matrix gets selected */
    if (context->matrixMode == GL_TEXTURE)
      es1xMatrixMode(_context_, GL_TEXTURE);
  }
  else
  {
    es1xSetError(_context_, GL_INVALID_ENUM);
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xAlphaFunc(void *_context_,
                                      GLenum func,
                                      GLclampf ref)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glAlphaFunc(%s, %.8f)\n",
                 ES1X_ENUM_TO_STRING(func),
                 ref));
  ES1X_CHECK_CONTEXT(context);

  switch(func)
  {
    case GL_NEVER:
      ES1X_UPDATE_CONTEXT_STATE(context->alphaTestFunc = ES1X_COMPAREFUNC_NEVER);
      break;
    case GL_ALWAYS:
      ES1X_UPDATE_CONTEXT_STATE(context->alphaTestFunc = ES1X_COMPAREFUNC_ALWAYS);
      break;
    case GL_LESS:
      ES1X_UPDATE_CONTEXT_STATE(context->alphaTestFunc = ES1X_COMPAREFUNC_LESS);
      break;
    case GL_LEQUAL:
      ES1X_UPDATE_CONTEXT_STATE(context->alphaTestFunc = ES1X_COMPAREFUNC_LEQUAL);
      break;
    case GL_EQUAL:
      ES1X_UPDATE_CONTEXT_STATE(context->alphaTestFunc = ES1X_COMPAREFUNC_EQUAL);
      break;
    case GL_GEQUAL:
      ES1X_UPDATE_CONTEXT_STATE(context->alphaTestFunc = ES1X_COMPAREFUNC_GEQUAL);
      break;
    case GL_GREATER:
      ES1X_UPDATE_CONTEXT_STATE(context->alphaTestFunc = ES1X_COMPAREFUNC_GREATER);
      break;
    case GL_NOTEQUAL:
      ES1X_UPDATE_CONTEXT_STATE(context->alphaTestFunc = ES1X_COMPAREFUNC_NOTEQUAL);
      break;
      break;

    default:
      es1xSetError(_context_, GL_INVALID_ENUM);
      return;
  }

  ES1X_UPDATE_CONTEXT_ALPHA_TEST_PARAMETER(context->alphaTestRef = es1xMath_clampf(ref));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xAlphaFuncx(void *_context_,
                                       GLenum func,
                                       GLclampx ref)
{
  // es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glAlphaFuncx"));
  es1xAlphaFunc(_context_, func, es1xMath_xtof(ref));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xBindBuffer(void *_context_,
                                       GLenum target,
                                       GLuint buffer)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glBindBuffer(%s, %d)\n",
                 ES1X_ENUM_TO_STRING(target),
                 buffer));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glBindBuffer(target, buffer);

  es1xCheckError(_context_);
  context->activeBufferBinding = buffer;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xBindTexture(void *_context_,
                                        GLenum target,
                                        GLuint textureName)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glBindTexture(%s, 0x%x)\n", ES1X_ENUM_TO_STRING(target), textureName));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glBindTexture(target, textureName);

  /*    Check for errors and store texture name
        and format only in case of success */
  {
    GLenum error = gl2b->glGetError();

    if (error == GL_NO_ERROR)
    {
      es1xTexture2D* texture = 0;

      if (textureName == 0) /* Assign default texture */
        texture = &context->texture[context->activeTextureUnitIndex];
      else
      {
        texture = es1xContext_getTexture(context, textureName);
        /* Find texture */
        if (texture == 0)
          texture = es1xContext_allocateTexture(context, textureName);
        ES1X_ASSERT(texture);
      }

      ES1X_UPDATE_CONTEXT_STATE(context->boundTexture[context->activeTextureUnitIndex] = texture);

      if (context->generateMipMap[context->activeTextureUnitIndex] == GL_TRUE)
      {
        gl2b->glGenerateMipmap(target);
        /* \todo Is this okay to discard error if calling with texture name zero? */
        error = gl2b->glGetError();
        ES1X_ASSERT(error == GL_INVALID_VALUE || error == GL_NO_ERROR);
      }
    }
    else
    {
      es1xSetError(_context_, error);
      ES1X_LOG_CALL(("GL ERROR @ glBindTexture ( %s ) \n", __func__));
    }
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xBlendFunc(void *_context_,
                                      GLenum src,
                                      GLenum dst)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glBlendFunc(%s, %s)\n",
                 ES1X_ENUM_TO_STRING(src),
                 ES1X_ENUM_TO_STRING(dst)));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glBlendFunc(src,
                     dst);
  es1xCheckError(_context_);

  ES20_DEBUG_CODE(ES1X_SET_ERROR_AND_RETURN_IF_FALSE(es1xIsAllowedBlendSrcFunc(src),
                                                     GL_INVALID_ENUM));
  ES20_DEBUG_CODE(ES1X_SET_ERROR_AND_RETURN_IF_FALSE(es1xIsAllowedBlendDstFunc(dst),
                                                     GL_INVALID_ENUM));
  ES20_DEBUG_CODE(context->blendSrc = src);
  ES20_DEBUG_CODE(context->blendDst = dst);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xBufferData(void *_context_,
                                       GLenum target,
                                       GLsizeiptr size,
                                       const GLvoid* data,
                                       GLenum usage)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glBufferData(%s, %ld,0x%p, %s)\n",
                 ES1X_ENUM_TO_STRING(target),
                 (long) size,
                 data,
                 ES1X_ENUM_TO_STRING(usage)));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glBufferData(target,
                      size,
                      data,
                      usage);
  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xBufferSubData(void *_context_,
                                          GLenum target,
                                          GLintptr offset,
                                          GLsizeiptr size,
                                          const GLvoid* data)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glBufferSubData(%s, %ld, %ld, %p)\n",
                 ES1X_ENUM_TO_STRING(target),
                 (long) offset,
                 (long) size,
                 data));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glBufferSubData(target,
                         offset,
                         (GLsizeiptr)size,
                         data);
  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xClear(void *_context_,
                                  GLbitfield mask)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glClear"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glClear(mask);

  /* glClear should never cause errors */
  ES1X_ASSUME_NO_ERROR;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xClearColor(void *_context_,
                                       GLclampf red,
                                       GLclampf green,
                                       GLclampf blue,
                                       GLclampf alpha)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glClearColor"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glClearColor(red,
                      green,
                      blue,
                      alpha);

  ES20_DEBUG_CODE(es1xColor_set(&context->colorClearValue,
                                es1xMath_clampf(red),
                                es1xMath_clampf(green),
                                es1xMath_clampf(blue),
                                es1xMath_clampf(alpha)));

  /* glClearColor should never cause errors */
  ES1X_ASSUME_NO_ERROR;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xClearColorx(void *_context_,
                                        GLclampx red,
                                        GLclampx green,
                                        GLclampx blue,
                                        GLclampx alpha)
{
  //   es1xContext *context = (es1xContext *) _context_;
  es1xClearColor(_context_,
                 es1xMath_xtof(red),
                 es1xMath_xtof(green),
                 es1xMath_xtof(blue),
                 es1xMath_xtof(alpha) );
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xClearDepthf(void *_context_,
                                        GLclampf depth)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glClearDepthf"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glClearDepthf(depth);

  ES20_DEBUG_CODE(context->depthClearValue = es1xMath_clampf(depth));

  /* glClearDepthf should never cause errors */
  ES1X_ASSUME_NO_ERROR;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xClearDepthx(void *_context_,
                                        GLclampx depth)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glClearDepthx"));
  es1xClearDepthf(_context_, es1xMath_xtof(depth));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xClearStencil(void *_context_,
                                         GLint s)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glClearStencil"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glClearStencil(s);

  ES20_DEBUG_CODE(context->stencilClearValue = s);

  /* glClearStencil should never cause errors */
  ES1X_ASSUME_NO_ERROR;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xClientActiveTexture(void *_context_,
                                                GLenum texture)
{
  es1xContext *context = (es1xContext *) _context_;
  GLint textureUnitIndex = texture - GL_TEXTURE0;

  ES1X_LOG_CALL(("glClientActiveTexture"));
  ES1X_CHECK_CONTEXT(context);

  if (textureUnitIndex >= 0 && textureUnitIndex < ES1X_NUM_SUPPORTED_TEXTURE_UNITS)
    context->clientActiveTexture = textureUnitIndex;
  else
    es1xSetError(_context_, GL_INVALID_ENUM);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xClipPlanef(void *_context_,
                                       GLenum plane,
                                       const GLfloat* eqn)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glClipPlanef"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     plane >= GL_CLIP_PLANE0 && plane <= GL_CLIP_PLANE0 + ES1X_NUM_SUPPORTED_CLIP_PLANES,
                                     GL_INVALID_ENUM);

  /* Store clip plane */
  {
    int ndx = plane - GL_CLIP_PLANE0;
    ES1X_ASSERT(ndx >= 0 && ndx < ES1X_NUM_SUPPORTED_CLIP_PLANES);
    ES1X_UDPATE_CONTEXT_CLIP_PLANE_PARAMETER(ndx,
                                             es1xVec4_set(&context->clipPlane[ndx],
                                                          eqn[0],
                                                          eqn[1],
                                                          eqn[2],
                                                          eqn[3]));
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xClipPlanex(void *_context_,
                                       GLenum plane,
                                       const GLfixed* eqn)
{
  //  es1xContext *context = (es1xContext *) _context_;
  /* Convert input and call glClipPlanef */
  GLfloat temp[4];

  temp[0] = es1xMath_xtof(eqn[0]);
  temp[1] = es1xMath_xtof(eqn[1]);
  temp[2] = es1xMath_xtof(eqn[2]);
  temp[3] = es1xMath_xtof(eqn[3]);

  ES1X_LOG_CALL(("glClipPlanex"));
  es1xClipPlanef(_context_, plane, (const GLfloat*) temp);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xColor4f(void *_context_,
                                    GLfloat red,
                                    GLfloat green,
                                    GLfloat blue,
                                    GLfloat alpha)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glColor4f"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_UPDATE_CONTEXT_VERTEX_PARAMETER(es1xColor_set(&context->currentColor,
                                                     es1xMath_clampf(red),
                                                     es1xMath_clampf(green),
                                                     es1xMath_clampf(blue),
                                                     es1xMath_clampf(alpha)));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xColor4ub(void *_context_,
                                     GLubyte red,
                                     GLubyte green,
                                     GLubyte blue,
                                     GLubyte alpha)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glColor4ub"));

  es1xColor4f(_context_,
              red       / 255.f,
              green     / 255.f,
              blue      / 255.f,
              alpha     / 255.f);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xColor4x(void *_context_,
                                    GLfixed red,
                                    GLfixed green,
                                    GLfixed blue,
                                    GLfixed alpha)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glColor4x"));
  es1xColor4f(_context_,
              es1xMath_xtof(red),
              es1xMath_xtof(green),
              es1xMath_xtof(blue),
              es1xMath_xtof(alpha));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xColorMask(void *_context_,
                                      GLboolean red,
                                      GLboolean green,
                                      GLboolean blue,
                                      GLboolean alpha)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glColorMask"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  red = es1xMath_clampb(red);
  green = es1xMath_clampb(green);
  blue = es1xMath_clampb(blue);
  alpha = es1xMath_clampb(alpha);

  gl2b->glColorMask(red,
                     green,
                     blue,
                     alpha);

  ES20_DEBUG_CODE(context->colorWriteMask[ES1X_NDX_RED] = red);
  ES20_DEBUG_CODE(context->colorWriteMask[ES1X_NDX_GREEN] = green);
  ES20_DEBUG_CODE(context->colorWriteMask[ES1X_NDX_BLUE] = blue);
  ES20_DEBUG_CODE(context->colorWriteMask[ES1X_NDX_ALPHA] = alpha);

  /* glColorMask should never cause errors */
  ES1X_ASSUME_NO_ERROR;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xColorPointer(void *_context_,
                                         GLint size,
                                         GLenum type,
                                         GLsizei stride,
                                         const GLvoid* pointer)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glColorPointer(%d, %s, %d, 0x%p)\n",
                 size,
                 ES1X_ENUM_TO_STRING(type),
                 stride,
                 pointer));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     es1xIsValidVertexColorDataType(type),
                                     GL_INVALID_ENUM);
  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     size == 4,
                                     GL_INVALID_VALUE);
  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     stride >= 0,
                                     GL_INVALID_VALUE);

  gl2b->glVertexAttribPointer(ES1X_COLOR_ARRAY_POS,
                               size,
                               type,
                               GL_TRUE,
                               stride,
                               pointer);
  ES1X_ASSUME_NO_ERROR;

  /* Pointer must be stored here too as it might be queried later */
  context->colorArrayPointer = pointer;
  context->colorArraySize = size;
  context->colorArrayType = type;
  context->colorArrayStride = stride;
  context->colorArrayBufferBinding = context->activeBufferBinding;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLubyte es1xUnpackInternal(GLubyte data,
                                       GLubyte numBitsLeft,
                                       GLubyte numBitsToRead)
{
  //  es1xContext *context = (es1xContext *) _context_;
  /*
    GLubyte shift = numBitsLeft - numBitsToRead;
    GLubyte mask = ((1 << numBitsToRead) - 1) << shift;

    return (data & mask) >> shift;
  */
  return data >> (8 - numBitsLeft) & ((1 << numBitsToRead)-1);

}

/*---------------------------------------------------------------------------*/

ES1X_INLINE int es1xUnpackColorComponent(GLubyte** packedStream,
                                         GLubyte* bitsLeftInByte,
                                         GLubyte numBitsToRead)
{
  GLubyte retVal;
  GLubyte byte = **packedStream;

  ES1X_ASSERT(packedStream);
  ES1X_ASSERT(*bitsLeftInByte > 0 && *bitsLeftInByte <= 8);
  ES1X_ASSERT(numBitsToRead > 0 && numBitsToRead <= 8);

  if (*bitsLeftInByte >= numBitsToRead)
  {
    retVal = es1xUnpackInternal(byte,
                                *bitsLeftInByte,
                                numBitsToRead);
    *bitsLeftInByte -= numBitsToRead;

    if (*bitsLeftInByte == 0)
    {
      (*packedStream)++;
      *bitsLeftInByte = 8;
    }
  }
  else
  {
    GLubyte bitsToReadFirst = *bitsLeftInByte;
    GLubyte bitsToReadAfter = numBitsToRead - bitsToReadFirst;

    retVal = es1xUnpackInternal(byte,
                                bitsToReadFirst,
                                bitsToReadFirst) << (bitsToReadAfter);
    (*packedStream)++;
    byte = **packedStream;
    retVal |= es1xUnpackInternal(byte,
                                 8,
                                 bitsToReadAfter);
    *bitsLeftInByte = 8 - bitsToReadAfter;
    /*
      retVal = **packedStream >> (8 - *bitsLeftInByte) & ((1 << numBitsToRead)-1);
      (*packedStream)++;
      retVal | = **packedStream & (1 << ((numBitsToRead - *bitsLeftInByte)-1));
      *bitsLeftInByte = 8 - (*bitsLeftInByte);
      */
  }

  ES1X_ASSERT(*bitsLeftInByte > 0 && *bitsLeftInByte <= 8);
  return retVal;
}

/*---------------------------------------------------------------------------*/

typedef enum
{
  ES1X_PALETTE_FORMAT_RGB8 = 0,
  ES1X_PALETTE_FORMAT_RGBA8,
  ES1X_PALETTE_FORMAT_R5_G6_B5,
  ES1X_PALETTE_FORMAT_RGBA4,
  ES1X_PALETTE_FORMAT_RGB5_A1,
} es1xPaletteFormat;

/*---------------------------------------------------------------------------*/

static GLuint* es1xUnpackPalette(const GLvoid* data,
                                 GLint paletteSize,
                                 es1xPaletteFormat format,
                                 GLint* bitsPerPackedPixel,
                                 GLint* numBytesRead)
{
  GLuint*        unpackedPalette = 0;
  const GLubyte* src = (const GLubyte*)data;
  int i;

  ES1X_ASSERT(data);
  ES1X_ASSERT(bitsPerPackedPixel);
  ES1X_ASSERT(numBytesRead);

  switch(paletteSize)
  {
    case 16:
      *bitsPerPackedPixel = 4;
      break;
    case 256:
      *bitsPerPackedPixel = 8;
      break;
    default:
      ES1X_ASSERT("Unsupported palette size");
      return 0;
  }

  unpackedPalette = (GLuint*) es1xMalloc(paletteSize * sizeof(GLuint));

  switch(format)
  {
    case ES1X_PALETTE_FORMAT_RGB8:
      {
        for(i =0;i<paletteSize;i++)
        {
          GLuint red = *src++;
          GLuint green = *src++;
          GLuint blue = *src++;

          unpackedPalette[i] = 0xFF000000u | (blue << 16) | (green << 8) | red;
        }

        *numBytesRead = paletteSize * 3;
        break;
      }

    case ES1X_PALETTE_FORMAT_RGBA8:
      {
        for(i =0;i<paletteSize;i++)
        {
          GLuint red = *src++;
          GLuint green = *src++;
          GLuint blue = *src++;
          GLuint alpha = *src++;

          unpackedPalette[i] = (alpha << 24) | (blue << 16) | (green << 8) | red;
        }

        *numBytesRead = paletteSize * 4;
        break;
      }

    case ES1X_PALETTE_FORMAT_R5_G6_B5:
      {
        int rbScaleFactor = 8;
        int gScaleFactor = 4;

        for(i =0;i<paletteSize;i++)
        {
          GLuint input0 = *src++;
          GLuint input1 = *src++;

          int red = rbScaleFactor * ((input1 & 0xF8) >> 3);
          int green = gScaleFactor  * (((input0 & 0xE0) >> 5) | ((input1 & 0x07) << 3));
          int blue = rbScaleFactor * (input0 & 0x1F);

          ES1X_ASSERT(red >= 0 && red < 256);
          ES1X_ASSERT(green >= 0 && green < 256);
          ES1X_ASSERT(blue >= 0 && blue < 256);

          unpackedPalette[i] = (0xFF000000u) | (blue << 16) | (green << 8) | red;
        }

        *numBytesRead = paletteSize * 2;
        break;
      }

    case ES1X_PALETTE_FORMAT_RGBA4:
      {
        GLuint scaleFactor = 16;

        for(i =0;i<paletteSize;i++)
        {
          GLuint input0 = *src++;
          GLuint input1 = *src++;

          int red = ((input1 & 0xF0) >> 4) * scaleFactor;
          int green = ((input1 & 0x0F) >> 0) * scaleFactor;
          int blue = ((input0 & 0xF0) >> 4) * scaleFactor;
          int alpha = ((input0 & 0x0F) >> 0) * scaleFactor;

          ES1X_ASSERT(red >= 0 && red < 256);
          ES1X_ASSERT(green >= 0 && green < 256);
          ES1X_ASSERT(blue >= 0 && blue < 256);
          ES1X_ASSERT(alpha >= 0 && alpha < 256);

          unpackedPalette[i] = (alpha << 24) | (blue << 16) | (green << 8) | red;
        }

        *numBytesRead = paletteSize * 2;
        break;
      }

    case ES1X_PALETTE_FORMAT_RGB5_A1:
      {
        GLuint scaleFactor = 8;

        for(i =0;i<paletteSize;i++)
        {
          GLuint input0 = *src++;
          GLuint input1 = *src++;

          int red = scaleFactor * ((input1 & 0xF8) >> 3);
          int green = scaleFactor * (((input0 & 0xC0) >> 6) | ((input1 & 0x07) << 2));
          int blue = scaleFactor * ((input0 & 0x3E) >> 1);
          int alpha = (input0 & 1) ? 255 :
                      0;

          /*
            GLuint red = rbScaleFactor * ((input1 & 0xF8) >> 3);
            GLuint green = gScaleFactor  * (((input0 & 0xE0) >> 5) | ((input1 & 0x07) << 3));
            GLuint blue = rbScaleFactor * (input0 & 0x1F);
          */

          ES1X_ASSERT(red >= 0 && red < 256);
          ES1X_ASSERT(green >= 0 && green < 256);
          ES1X_ASSERT(blue >= 0 && blue < 256);
          ES1X_ASSERT(alpha >= 0 && alpha < 256);

          unpackedPalette[i] = (alpha << 24) | (blue << 16) | (green << 8) | red;
        }

        *numBytesRead = paletteSize * 2;
        break;
      }

    default:
      ES1X_ASSERT(!"Unsupported format");
      es1xFree(unpackedPalette);
      return 0;
  }

  return unpackedPalette;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xTexImage2D(void *_context_,
                                       GLenum target,
                                       GLint level,
                                       GLint internalFormat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLint border,
                                       GLenum format,
                                       GLenum type,
                                       const GLvoid* pixels)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glTexImage2D"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  {
    GLenum error;
    gl2b->glTexImage2D(target,
                        level,
                        internalFormat,
                        width,
                        height,
                        border,
                        format,
                        type,
                        pixels);

    error = gl2b->glGetError();

    if (error == GL_NO_ERROR)
    {
      es1xTexture2D* texture = context->boundTexture[context->activeTextureUnitIndex];
      ES1X_ASSERT(texture);

      switch(internalFormat)
      {
        case GL_ALPHA:
          ES1X_UPDATE_CONTEXT_STATE(texture->format = ES1X_TEXTUREFORMAT_ALPHA);
          break;
        case GL_LUMINANCE:
          ES1X_UPDATE_CONTEXT_STATE(texture->format = ES1X_TEXTUREFORMAT_LUMINANCE);
          break;
        case GL_LUMINANCE_ALPHA:
          ES1X_UPDATE_CONTEXT_STATE(texture->format = ES1X_TEXTUREFORMAT_LUMINANCE_ALPHA);
          break;
        case GL_RGB:
          ES1X_UPDATE_CONTEXT_STATE(texture->format = ES1X_TEXTUREFORMAT_RGB);
          break;
        case GL_RGBA:
          ES1X_UPDATE_CONTEXT_STATE(texture->format = ES1X_TEXTUREFORMAT_RGBA);
          break;
        default:
          ES1X_ASSERT(!"Unknown internal format");
          es1xSetError(_context_, GL_INVALID_ENUM);
          return;
      }

      /* Generate mipmaps */
      if (context->generateMipMap[context->activeTextureUnitIndex] == GL_TRUE && level == 0)
      {
        gl2b->glGenerateMipmap(GL_TEXTURE_2D);
        es1xCheckError(_context_);
      }
    }
    else
    {
      es1xSetError(_context_, error);
    }
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xCompressedTexImage2D(void *_context_,
                                                 GLenum target,
                                                 GLint level,
                                                 GLenum internalFormat,
                                                 GLsizei width,
                                                 GLsizei height,
                                                 GLint border,
                                                 GLsizei imageSize,
                                                 const GLvoid* data)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glCompressedTexImage2D"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     border == 0,
                                     GL_INVALID_VALUE);
  ES1X_SET_ERROR_AND_RETURN_IF_TRUE(_context_,
                                    data == 0,
                                    GL_INVALID_VALUE);
  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     width > 0 && height > 0,
                                     GL_INVALID_VALUE);
  ES1X_ASSUME_NO_ERROR;

  {
    const GLubyte*      packedPalette = (const GLubyte*)data;
    const GLubyte*      packedImageData = 0;
    GLuint*                     unpackedPalette = 0;
    GLuint*                     unpackedImageData = 0;
    GLint                       bitsPerPackedPixel = 0;
    GLint                       numBytesRead = 0;

    switch(internalFormat)
    {
      case GL_PALETTE4_RGB8_OES:
        unpackedPalette = es1xUnpackPalette(data,
                                            16,
                                            ES1X_PALETTE_FORMAT_RGB8,
                                            &bitsPerPackedPixel,
                                            &numBytesRead);
        break;
      case GL_PALETTE4_RGBA8_OES:
        unpackedPalette = es1xUnpackPalette(data,
                                            16,
                                            ES1X_PALETTE_FORMAT_RGBA8,
                                            &bitsPerPackedPixel,
                                            &numBytesRead);
        break;
      case GL_PALETTE4_R5_G6_B5_OES:
        unpackedPalette = es1xUnpackPalette(data,
                                            16,
                                            ES1X_PALETTE_FORMAT_R5_G6_B5,
                                            &bitsPerPackedPixel,
                                            &numBytesRead);
        break;
      case GL_PALETTE4_RGBA4_OES:
        unpackedPalette = es1xUnpackPalette(data,
                                            16,
                                            ES1X_PALETTE_FORMAT_RGBA4,
                                            &bitsPerPackedPixel,
                                            &numBytesRead);
        break;
      case GL_PALETTE4_RGB5_A1_OES:
        unpackedPalette = es1xUnpackPalette(data,
                                            16,
                                            ES1X_PALETTE_FORMAT_RGB5_A1,
                                            &bitsPerPackedPixel,
                                            &numBytesRead);
        break;
      case GL_PALETTE8_RGB8_OES:
        unpackedPalette = es1xUnpackPalette(data,
                                            256,
                                            ES1X_PALETTE_FORMAT_RGB8,
                                            &bitsPerPackedPixel,
                                            &numBytesRead);
        break;
      case GL_PALETTE8_RGBA8_OES:
        unpackedPalette = es1xUnpackPalette(data,
                                            256,
                                            ES1X_PALETTE_FORMAT_RGBA8,
                                            &bitsPerPackedPixel,
                                            &numBytesRead);
        break;
      case GL_PALETTE8_R5_G6_B5_OES:
        unpackedPalette = es1xUnpackPalette(data,
                                            256,
                                            ES1X_PALETTE_FORMAT_R5_G6_B5,
                                            &bitsPerPackedPixel,
                                            &numBytesRead);
        break;
      case GL_PALETTE8_RGBA4_OES:
        unpackedPalette = es1xUnpackPalette(data,
                                            256,
                                            ES1X_PALETTE_FORMAT_RGBA4,
                                            &bitsPerPackedPixel,
                                            &numBytesRead);
        break;
      case GL_PALETTE8_RGB5_A1_OES:
        unpackedPalette = es1xUnpackPalette(data,
                                            256,
                                            ES1X_PALETTE_FORMAT_RGB5_A1,
                                            &bitsPerPackedPixel,
                                            &numBytesRead);
        break;

      default:
        /* Route call to ES2 anyway as it might support other formats through extensions */
        gl2b->glCompressedTexImage2D(target,
                                      level,
                                      internalFormat,
                                      width,
                                      height,
                                      border,
                                      imageSize,
                                      data);
        es1xCheckError(_context_);
        break;
    }

    packedImageData = packedPalette + numBytesRead;

    /* Test errors */
    if (bitsPerPackedPixel != 4 &&
        bitsPerPackedPixel != 8)
    {
      es1xSetError(_context_, GL_INVALID_OPERATION);
      return;
    }

    /* Unpack image data */
    {
      int i = 0;
      int numPixels = width * height;

      unpackedImageData = es1xMalloc(width * height * sizeof(GLuint));

      if (bitsPerPackedPixel == 4)
      {
        const GLubyte*  src = packedImageData;
        GLuint*                 dst = unpackedImageData;
        GLubyte mask = 0xF0;

        for(i =0;i<numPixels;i++)
        {
          int paletteIndex = (*src & mask) >> (4 * ((i+1) & 1));
          mask = ~mask;
          packedImageData                       += (i & 1);
          src                                           += (i & 1);

          ES1X_ASSERT(paletteIndex >= 0 && paletteIndex < 16);
          *dst++= unpackedPalette[paletteIndex];
        }
      }
      else
      {
        GLuint* dst = unpackedImageData;
        ES1X_ASSERT(bitsPerPackedPixel == 8);

        for(i =0;i<numPixels;i++)
          *dst++= unpackedPalette[*packedImageData++];
      }

      /* Serve unpacked texture data */
      es1xTexImage2D(_context_,
                     target,
                     level,
                     GL_RGBA,
                     width,
                     height,
                     border,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     unpackedImageData);

      /* Clean up */
      es1xFree(unpackedImageData);
      es1xFree(unpackedPalette);
    }
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xCompressedTexSubImage2D(void *_context_,
                                                    GLenum target,
                                                    GLint level,
                                                    GLint xoffset,
                                                    GLint yoffset,
                                                    GLsizei width,
                                                    GLsizei height,
                                                    GLenum format,
                                                    GLsizei imageSize,
                                                    const GLvoid* data)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glCompressedTexSubImage2D"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glCompressedTexSubImage2D(target,
                                   level,
                                   xoffset,
                                   yoffset,
                                   width,
                                   height,
                                   format,
                                   imageSize,
                                   data);
  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xCopyTexImage2D(void *_context_,
                                           GLenum target,
                                           GLint level,
                                           GLenum internalFormat,
                                           GLint x,
                                           GLint y,
                                           GLsizei width,
                                           GLsizei height,
                                           GLint border)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glCopyTexImage2D"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  /* Copy texture data from framebuffer */
  {
    GLenum error;
    gl2b->glCopyTexImage2D(target,
                            level,
                            internalFormat,
                            x,
                            y,
                            width,
                            height,
                            border);

    error = gl2b->glGetError();

    if (error == GL_NO_ERROR)
    {
      es1xTexture2D* texture = context->boundTexture[context->activeTextureUnitIndex];
      ES1X_ASSERT(texture);

      switch(internalFormat)
      {
        case GL_ALPHA:
          ES1X_UPDATE_CONTEXT_STATE(texture->format = ES1X_TEXTUREFORMAT_ALPHA);
          break;
        case GL_LUMINANCE:
          ES1X_UPDATE_CONTEXT_STATE(texture->format = ES1X_TEXTUREFORMAT_LUMINANCE);
          break;
        case GL_LUMINANCE_ALPHA:
          ES1X_UPDATE_CONTEXT_STATE(texture->format = ES1X_TEXTUREFORMAT_LUMINANCE_ALPHA);
          break;
        case GL_RGB:
          ES1X_UPDATE_CONTEXT_STATE(texture->format = ES1X_TEXTUREFORMAT_RGB);
          break;
        case GL_RGBA:
          ES1X_UPDATE_CONTEXT_STATE(texture->format = ES1X_TEXTUREFORMAT_RGBA);
          break;
        default:
          ES1X_ASSERT(!"Unknown internal format");
          es1xSetError(_context_, GL_INVALID_ENUM);
          return;
      }
    }
    else
    {
      es1xSetError(_context_, error);
    }
  }

  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xCopyTexSubImage2D(void *_context_,
                                              GLenum target,
                                              GLint level,
                                              GLint xoffset,
                                              GLint yoffset,
                                              GLint x,
                                              GLint y,
                                              GLsizei width,
                                              GLsizei height)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glCopyTexSubImage2D"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glCopyTexSubImage2D(target,
                             level,
                             xoffset,
                             yoffset,
                             x,
                             y,
                             width,
                             height);
  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xCullFace(void *_context_,
                                     GLenum mode)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glCullFace"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glCullFace(mode);
  es1xCheckError(_context_);

  ES1X_DEBUG_CODE(ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                                     mode == GL_FRONT || mode == GL_BACK || mode == GL_FRONT_AND_BACK,
                                                     GL_INVALID_ENUM));
  ES20_DEBUG_CODE(context->cullFaceMode = mode);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xDeleteBuffers(void *_context_,
                                          GLsizei n,
                                          const GLuint* buffers)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glDeleteBuffers"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glDeleteBuffers(n,
                         buffers);
  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xDeleteTextures(void *_context_,
                                           GLsizei n,
                                           const GLuint* textures)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glDeleteTextures"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glDeleteTextures(n, textures);
  es1xCheckError(_context_);

  /* Unbind deleted textures if they are in use */
  {
    int textureNdx;
    int textureUnitNdx;

    for (textureNdx =0;textureNdx<n;textureNdx++)
    {
      es1xTexture2D* texture = es1xContext_getTexture(context,
                                                      textures[textureNdx]);
      if (texture == 0)
        continue; /* Not bound,
                     therefore not allocated internally either */

      for(textureUnitNdx =0;textureUnitNdx<ES1X_NUM_SUPPORTED_TEXTURE_UNITS;textureUnitNdx++)
        if (context->boundTexture[textureUnitNdx] == texture)
          context->boundTexture[textureUnitNdx] = &context->texture[textureUnitNdx];

      es1xContext_releaseTexture(context,
                                 texture);
    }
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xDepthFunc(void *_context_,
                                      GLenum func)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glDepthFunc"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glDepthFunc(func);
  es1xCheckError(_context_);

  ES20_DEBUG_CODE(ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                                     es1xIsAllowedDepthFunc(func),
                                                     GL_INVALID_ENUM));
  ES20_DEBUG_CODE(context->depthFunc = func);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xDepthMask(void *_context_,
                                      GLboolean mask)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glDepthMask"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  mask = es1xMath_clampb(mask);
  gl2b->glDepthMask(mask);

  /* glDepthMask should never cause errors */
  ES1X_ASSUME_NO_ERROR;
  ES20_DEBUG_CODE(context->depthWriteMask = !!mask);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xDepthRangef(void *_context_,
                                        GLclampf zNear,
                                        GLclampf zFar)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glDepthRangef"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glDepthRangef(zNear,
                       zFar);

  /* glDepthMask should never cause errors */
  ES1X_ASSUME_NO_ERROR;
  ES20_DEBUG_CODE(es1xVec2_set(&context->depthRange,
                               es1xMath_clampf(zNear),
                               es1xMath_clampf(zFar)));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xDepthRangex(void *_context_,
                                        GLclampx zNear,
                                        GLclampx zFar)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glDepthRangex"));
  es1xDepthRangef(_context_,
                  es1xMath_xtof(zNear),
                  es1xMath_xtof(zFar));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xDisable(void *_context_,
                                    GLenum cap)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glDisable(%s)\n", ES1X_ENUM_TO_STRING(cap)));
  es1xSet(_context_, cap, GL_FALSE);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xDisableClientState(void *_context_,
                                               GLenum array)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glDisableClientState(%s)\n", ES1X_ENUM_TO_STRING(array)));
  es1xSetClientState(_context_, array, GL_FALSE);

  return;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xEnable(void *_context_,
                                   GLenum cap)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glEnable(%s)\n", ES1X_ENUM_TO_STRING(cap)));
  es1xSet(_context_, cap, GL_TRUE);

  return;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xEnableClientState(void *_context_,
                                              GLenum array)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glEnableClientState(%s)\n", ES1X_ENUM_TO_STRING(array)));
  es1xSetClientState(_context_, array, GL_TRUE);

  return;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xFinish(void *_context_)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glFinish"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glFinish();

  /* glFinish should not cause errors */
  ES1X_ASSUME_NO_ERROR;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xFlush(void *_context_)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glFlush"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glFlush();

  /* glFlush should not cause errors */
  ES1X_ASSUME_NO_ERROR;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xFogf(void *_context_,
                                 GLenum pname,
                                 GLfloat param)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glFogf"));
  ES1X_CHECK_CONTEXT(context);

  switch(pname)
  {
    case GL_FOG_DENSITY:
      {
        /* Check value validity */
        if (param < 0.f)
        {
          es1xSetError(_context_, GL_INVALID_VALUE);
          return;
        }

        ES1X_UPDATE_CONTEXT_FOG_PARAMETER(context->fogDensity = param)
            break;
      }

    case GL_FOG_START:
      {
        ES1X_UPDATE_CONTEXT_FOG_PARAMETER(context->fogStart = param);
        break;
      }

    case GL_FOG_END:
      {
        ES1X_UPDATE_CONTEXT_FOG_PARAMETER(context->fogEnd = param);
        break;
      }

    case GL_FOG_MODE:
      {
        switch((GLenum)param)
        {
          case GL_EXP:
            context->fogMode = ES1X_FOGMODE_EXP;
            break;
          case GL_EXP2:
            context->fogMode = ES1X_FOGMODE_EXP2;
            break;
          case GL_LINEAR:
            context->fogMode = ES1X_FOGMODE_LINEAR;
            break;

          default:
            es1xSetError(_context_, GL_INVALID_VALUE);
            break;
        }
        break;
      }
    default:
      es1xSetError(_context_, GL_INVALID_ENUM);
      return;
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xFogfv(void *_context_,
                                  GLenum pname,
                                  const GLfloat* param)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glFogfv"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_NULL(_context_,
                                    param,
                                    GL_INVALID_VALUE);

  switch(pname)
  {
    case GL_FOG_COLOR:
      {
        ES1X_UPDATE_CONTEXT_FOG_PARAMETER(es1xColor_set(&context->fogColor,
                                                        es1xMath_clampf(param[0]),
                                                        es1xMath_clampf(param[1]),
                                                        es1xMath_clampf(param[2]),
                                                        es1xMath_clampf(param[3])));
        break;
      }

    default:
      /* Call glFogf with first parameter */
      es1xFogf(_context_, pname, *param);
      break;
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xFogx(void *_context_,
                                 GLenum pname,
                                 GLfixed param)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glFogx"));
  ES1X_CHECK_CONTEXT(context);

  if (pname == GL_FOG_MODE)
    es1xFogf(_context_, pname, (GLfloat)param);
  else
    es1xFogf(_context_, pname, es1xMath_xtof(param));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xFogxv(void *_context_,
                                  GLenum pname,
                                  const GLfixed* param)
{
  //   es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glFogxv"));

  switch(pname)
  {
    case GL_FOG_COLOR:
      {
        GLfloat temp[4];

        /* Convert to float and call glFogfv */
        temp[0] = es1xMath_xtof(param[0]);
        temp[1] = es1xMath_xtof(param[1]);
        temp[2] = es1xMath_xtof(param[2]);
        temp[3] = es1xMath_xtof(param[3]);

        es1xFogfv(_context_, pname, (const GLfloat*)temp);
        break;
      }
    default:
      {
        /* Call glFogx with first parameter */
        es1xFogx(_context_, pname, *param);
        break;
      }
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xFrontFace(void *_context_,
                                      GLenum dir)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glFrontFace"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glFrontFace(dir);
  es1xCheckError(_context_);

  ES20_DEBUG_CODE(ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                                     dir == GL_CW || dir == GL_CCW,
                                                     GL_INVALID_ENUM));
  ES20_DEBUG_CODE(context->frontFace = dir);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGenBuffers(void *_context_,
                                       GLsizei n,
                                       GLuint* buffers)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGenBuffers [%p->%p]  (%d, 0x%p)\n", gl2b, gl2b->glGenBuffers, n, buffers));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glGenBuffers(n, buffers);
  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGenTextures(void *_context_,
                                        GLsizei n,
                                        GLuint* textures)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGenTextures"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glGenTextures(n,
                       textures);
  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xHint(void *_context_,
                                 GLenum target,
                                 GLenum hint)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glHint"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     hint == GL_NICEST  ||
                                     hint == GL_FASTEST ||
                                     hint == GL_DONT_CARE,
                                     GL_INVALID_ENUM);

  switch(target)
  {
    case GL_PERSPECTIVE_CORRECTION_HINT:
      context->perspectiveCorrectionHint = hint;
      break;
    case GL_POINT_SMOOTH_HINT:
      context->pointSmoothHint = hint;
      break;
    case GL_LINE_SMOOTH_HINT:
      context->lineSmoothHint = hint;
      break;
    case GL_FOG_HINT:
      context->fogHint = hint;
      break;

    case GL_GENERATE_MIPMAP_HINT:
      {
        gl2b->glHint(target,
                      hint);

        /* glHint should not cause errors */
        ES1X_ASSUME_NO_ERROR;
        ES20_DEBUG_CODE(context->generateMipMapHint = hint);
        break;
      }

    default:
      es1xSetError(_context_, GL_INVALID_ENUM);
      break;
  }
}

/*---------------------------------------------------------------------------*/

GL_API GLboolean GL_APIENTRY es1xIsBuffer(void *_context_,
                                          GLuint buffer)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glIsBuffer"));
  if (buffer == 0)
    return GL_FALSE;
  /* \todo PowerVR returns GL_TRUE for buffer =0,
     that is incorrect */
  return gl2b->glIsBuffer(buffer);
}

/*---------------------------------------------------------------------------*/

GL_API GLboolean GL_APIENTRY es1xIsTexture(void *_context_,
                                           GLuint texture)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glIsTexture"));

  if (context == 0 || texture == 0)
    return GL_FALSE;

  return !!es1xContext_getTexture(context,
                                  texture);
  /*
    return gl2b->glIsTexture(texture);
  */
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xLightModelf(void *_context_,
                                        GLenum pname,
                                        GLfloat param)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glLightModelf"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     pname == GL_LIGHT_MODEL_TWO_SIDE,
                                     GL_INVALID_ENUM);

  ES1X_UPDATE_CONTEXT_STATE(context->lightModelTwoSide = (GLboolean)(param != 0.f));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xLightModelfv(void *_context_,
                                         GLenum pname,
                                         const GLfloat* param)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glLightModelfv"));
  ES1X_CHECK_CONTEXT(context);

  switch(pname)
  {
    case GL_LIGHT_MODEL_AMBIENT:
      ES1X_UPDATE_CONTEXT_LIGHT_MODEL_PARAMETER(es1xColor_set(&context->lightModelAmbient,
                                                              es1xMath_clampf(param[0]),
                                                              es1xMath_clampf(param[1]),
                                                              es1xMath_clampf(param[2]),
                                                              es1xMath_clampf(param[3])));
      break;

    default:
      es1xLightModelf(_context_, pname, *param);
      break;
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xLightModelx(void *_context_,
                                        GLenum pname,
                                        GLfixed param)
{
  //  es1xContext *context = (es1xContext *) _context_;
  es1xLightModelf(_context_, pname, es1xMath_xtof(param));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xLightModelxv(void *_context_,
                                         GLenum pname,
                                         const GLfixed* param)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glLightModelxv"));

  switch(pname)
  {
    case GL_LIGHT_MODEL_AMBIENT:
      {
        /* convert and call float variant */
        GLfloat converted[4];

        converted[0] = es1xMath_xtof(param[0]);
        converted[1] = es1xMath_xtof(param[1]);
        converted[2] = es1xMath_xtof(param[2]);
        converted[3] = es1xMath_xtof(param[3]);

        es1xLightModelfv(_context_, pname, (const GLfloat*)converted);
        break;
      }
    default:
      es1xLightModelf(_context_, pname, es1xMath_xtof(*param));
      break;
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xLightf(void *_context_,
                                   GLenum lightEnum,
                                   GLenum pname,
                                   GLfloat param)
{
  es1xContext *context = (es1xContext *) _context_;
  int                           lightNdx = lightEnum - GL_LIGHT0;
  es1xLight*            light;

  ES1X_LOG_CALL(("glLightf"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     lightNdx >= 0 && lightNdx < ES1X_NUM_SUPPORTED_LIGHTS,
                                     GL_INVALID_ENUM);

  light = &context->light[lightNdx];

  switch(pname)
  {
    case GL_SPOT_EXPONENT:
      ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                         param >= 0.f && param <= 128.f,
                                         GL_INVALID_VALUE);
      ES1X_UPDATE_CONTEXT_LIGHT_PARAMETER(lightNdx,
                                          light->spotExponent = param);
      break;

    case GL_SPOT_CUTOFF:
      ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                         (param >= 0.f && param <= 90.f) || param == 180.f,
                                         GL_INVALID_VALUE);
      ES1X_UPDATE_CONTEXT_LIGHT_PARAMETER(lightNdx,
                                          light->spotCutoff = param);
      context->lightType[lightNdx] = es1xLight_determineType(light);
      break;

    case GL_CONSTANT_ATTENUATION:
      ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                         param >= 0.f,
                                         GL_INVALID_VALUE);
      ES1X_UPDATE_CONTEXT_LIGHT_PARAMETER(lightNdx,
                                          light->constantAttenuation = param);
      break;

    case GL_LINEAR_ATTENUATION:
      ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                         param >= 0.f,
                                         GL_INVALID_VALUE);
      ES1X_UPDATE_CONTEXT_LIGHT_PARAMETER(lightNdx,
                                          light->linearAttenuation = param);
      break;

    case GL_QUADRATIC_ATTENUATION:
      ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                         param >= 0.f,
                                         GL_INVALID_VALUE);
      ES1X_UPDATE_CONTEXT_LIGHT_PARAMETER(lightNdx,
                                          light->quadraticAttenuation = param);
      break;

      /* Not supported with glLightf */
    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_POSITION:
    case GL_SPOT_DIRECTION:
      es1xSetError(_context_, GL_INVALID_ENUM);
      break;
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xLightfv(void *_context_,
                                    GLenum lightEnum,
                                    GLenum pname,
                                    const GLfloat* params)
{
  es1xContext *context = (es1xContext *) _context_;
  int          lightNdx = lightEnum - GL_LIGHT0;
  es1xLight*   light;

  ES1X_LOG_CALL(("glLightfv"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     lightNdx >= 0 && lightNdx < ES1X_NUM_SUPPORTED_LIGHTS,
                                     GL_INVALID_ENUM);

  light = &context->light[lightNdx];

  switch(pname)
  {
    case GL_AMBIENT:
      /* \todo ensure that clamping is correct */
      ES1X_UPDATE_CONTEXT_LIGHT_PARAMETER(lightNdx,
                                          es1xColor_set(&light->ambient,
                                                        es1xMath_clampf(params[0]),
                                                        es1xMath_clampf(params[1]),
                                                        es1xMath_clampf(params[2]),
                                                        es1xMath_clampf(params[3])));
      break;
    case GL_DIFFUSE:
      ES1X_UPDATE_CONTEXT_LIGHT_PARAMETER(lightNdx,
                                          es1xColor_set(&light->diffuse,
                                                        es1xMath_clampf(params[0]),
                                                        es1xMath_clampf(params[1]),
                                                        es1xMath_clampf(params[2]),
                                                        es1xMath_clampf(params[3])));
      break;
    case GL_SPECULAR:
      ES1X_UPDATE_CONTEXT_LIGHT_PARAMETER(lightNdx,
                                          es1xColor_set(&light->specular,
                                                        es1xMath_clampf(params[0]),
                                                        es1xMath_clampf(params[1]),
                                                        es1xMath_clampf(params[2]),
                                                        es1xMath_clampf(params[3])));
      break;
    case GL_POSITION:
      {
        es1xVec4 temp;
        es1xVec4_set(&temp,
                     params[0],
                     params[1],
                     params[2],
                     params[3]);

        /* Transform position to eye space */
        {
          es1xMatrix4x4* mvMatrix = es1xMatrixStack_peekMatrix(context->modelViewMatrixStack);
          ES1X_UPDATE_CONTEXT_LIGHT_PARAMETER(lightNdx,
                                              es1xMatrix4x4_multiplyVec4(mvMatrix,
                                                                         &temp,
                                                                         &light->eyePosition));
          context->lightType[lightNdx] = es1xLight_determineType(light);
        }
        break;
      }
    case GL_SPOT_DIRECTION:
      {
        es1xVec3 temp;
        es1xVec3_set(&temp,
                     params[0],
                     params[1],
                     params[2]);

        /* Transform direction to eye space */
        {
          es1xMatrix4x4*        mvMatrix = es1xMatrixStack_peekMatrix(context->modelViewMatrixStack);
          ES1X_UPDATE_CONTEXT_LIGHT_PARAMETER(lightNdx,
                                              es1xMatrix4x4_multiplyVec3(mvMatrix,
                                                                         &temp,
                                                                         &light->spotEyeDirection));
        }
        break;
      }
    default:
      /* call getLightf */
      es1xLightf(_context_, lightEnum, pname, *params);
      break;
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xLightx(void *_context_,
                                   GLenum light,
                                   GLenum pname,
                                   GLfixed param)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glLightf"));
  es1xLightf(_context_, light, pname, es1xMath_xtof(param));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xLightxv(void *_context_,
                                    GLenum light,
                                    GLenum pname,
                                    const GLfixed* params)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glLightxv"));

  switch(pname)
  {
    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_POSITION:
      {
        /* Convert and call glLightfv */
        GLfloat converted[4];

        converted[0] = es1xMath_xtof(params[0]);
        converted[1] = es1xMath_xtof(params[1]);
        converted[2] = es1xMath_xtof(params[2]);
        converted[3] = es1xMath_xtof(params[3]);

        es1xLightfv(_context_, light, pname, (const GLfloat*)converted);
        break;
      }
    case GL_SPOT_DIRECTION:
      {
        /* Convert and call glLightfv */
        GLfloat converted[3];

        converted[0] = es1xMath_xtof(params[0]);
        converted[1] = es1xMath_xtof(params[1]);
        converted[2] = es1xMath_xtof(params[2]);

        es1xLightfv(_context_, light, pname, (const GLfloat*)converted);
        break;
      }

    default:
      /* Convert and call glLightf */
      es1xLightf(_context_, light, pname, es1xMath_xtof(*params));
      break;
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xLineWidth(void *_context_,
                                      GLfloat width)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glLineWidth"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glLineWidth(width);
  es1xCheckError(_context_);

  ES20_DEBUG_CODE(ES1X_SET_ERROR_AND_RETURN_IF_TRUE(_context_,
                                                    width <= 0.f,
                                                    GL_INVALID_VALUE));
  ES20_DEBUG_CODE(context->lineWidth = width);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xLineWidthx(void *_context_,
                                       GLfixed width)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glLineWidthx"));
  es1xLineWidth(_context_, es1xMath_xtof(width));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xLogicOp(void *_context_,
                                    GLenum mode)
{
  es1xContext *context = (es1xContext *) _context_;
  /* \todo [2009-03-09 petri] Will be implemented as an extension (essentially the glLogicOp() will just be present in the ES2 driver). */
  ES1X_LOG_CALL(("glLogicOp"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     mode == GL_CLEAR                  ||
                                     mode == GL_AND                    ||
                                     mode == GL_AND_REVERSE            ||
                                     mode == GL_COPY                   ||
                                     mode == GL_AND_INVERTED           ||
                                     mode == GL_NOOP                   ||
                                     mode == GL_XOR                    ||
                                     mode == GL_OR                     ||
                                     mode == GL_NOR                    ||
                                     mode == GL_EQUIV                  ||
                                     mode == GL_INVERT                 ||
                                     mode == GL_OR_REVERSE             ||
                                     mode == GL_COPY_INVERTED          ||
                                     mode == GL_OR_INVERTED            ||
                                     mode == GL_NAND                   ||
                                     mode == GL_SET,
                                     GL_INVALID_ENUM);

  /*    ES1X_UPDATE_CONTEXT_LOGIC_OP_PARAMETER(context->colorLogicOpMode = mode);       */
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xMaterialf(void *_context_,
                                      GLenum face,
                                      GLenum pname,
                                      GLfloat param)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glMaterialf"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     face == GL_FRONT_AND_BACK,
                                     GL_INVALID_ENUM);
  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     pname == GL_SHININESS,
                                     GL_INVALID_ENUM);
  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     param >= 0.f && param <= 180.f,
                                     GL_INVALID_VALUE);

  ES1X_UPDATE_CONTEXT_MATERIAL_PARAMETER(context->materialShininess = param);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xMaterialfv(void *_context_,
                                       GLenum face,
                                       GLenum pname,
                                       const GLfloat* params)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glMaterialfv"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     face == GL_FRONT_AND_BACK,
                                     GL_INVALID_ENUM);

  switch(pname)
  {
    case GL_AMBIENT:
      ES1X_UPDATE_CONTEXT_MATERIAL_PARAMETER(es1xColor_set(&context->materialAmbient,
                                                           params[0],
                                                           params[1],
                                                           params[2],
                                                           params[3]));
      break;
    case GL_DIFFUSE:
      ES1X_UPDATE_CONTEXT_MATERIAL_PARAMETER(es1xColor_set(&context->materialDiffuse,
                                                           params[0],
                                                           params[1],
                                                           params[2],
                                                           params[3]));
      break;
    case GL_SPECULAR:
      ES1X_UPDATE_CONTEXT_MATERIAL_PARAMETER(es1xColor_set(&context->materialSpecular,
                                                           params[0],
                                                           params[1],
                                                           params[2],
                                                           params[3]));
      break;
    case GL_EMISSION:
      ES1X_UPDATE_CONTEXT_MATERIAL_PARAMETER(es1xColor_set(&context->materialEmission,
                                                           params[0],
                                                           params[1],
                                                           params[2],
                                                           params[3]));
      break;
    case GL_AMBIENT_AND_DIFFUSE:
      ES1X_UPDATE_CONTEXT_MATERIAL_PARAMETER(es1xColor_set(&context->materialAmbient,
                                                           params[0],
                                                           params[1],
                                                           params[2],
                                                           params[3]));

      ES1X_UPDATE_CONTEXT_MATERIAL_PARAMETER(es1xColor_set(&context->materialDiffuse,
                                                           params[0],
                                                           params[1],
                                                           params[2],
                                                           params[3]));
      break;
    default:
      es1xMaterialf(_context_, face, pname, *params);
      break;
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xMaterialx(void *_context_,
                                      GLenum face,
                                      GLenum pname,
                                      GLfixed param)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glMaterialf"));
  es1xMaterialf(_context_, face, pname, es1xMath_xtof(param));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xMaterialxv(void *_context_,
                                       GLenum face,
                                       GLenum pname,
                                       const GLfixed* params)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glMaterialxv"));
  ES1X_CHECK_CONTEXT(context);

  switch(pname)
  {
    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_EMISSION:
    case GL_AMBIENT_AND_DIFFUSE:
      {
        /* Convert and call float variant */
        GLfloat converted[4];

        converted[0] = es1xMath_xtof(params[0]);
        converted[1] = es1xMath_xtof(params[1]);
        converted[2] = es1xMath_xtof(params[2]);
        converted[3] = es1xMath_xtof(params[3]);

        es1xMaterialfv(_context_, face, pname, (const GLfloat*)converted);
        break;
      }

    default:
      es1xMaterialf(_context_, face, pname, es1xMath_xtof(*params));

      break;
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xMultiTexCoord4f(void *_context_,
                                            GLenum target,
                                            GLfloat s,
                                            GLfloat t,
                                            GLfloat r,
                                            GLfloat q)
{
  es1xContext *context = (es1xContext *) _context_;
  int textureUnit = target - GL_TEXTURE0;

  ES1X_LOG_CALL(("glMultiTexCoord4f"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     textureUnit >= 0 && textureUnit < ES1X_NUM_SUPPORTED_TEXTURE_UNITS,
                                     GL_INVALID_ENUM);
  es1xVec4_set(&context->currentTextureCoords[textureUnit],
               s,
               t,
               r,
               q);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xMultiTexCoord4x(void *_context_,
                                            GLenum target,
                                            GLfixed s,
                                            GLfixed t,
                                            GLfixed r,
                                            GLfixed q)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glMultiTexCoord4x"));
  es1xMultiTexCoord4f(_context_,
                      target,
                      es1xMath_xtof(s),
                      es1xMath_xtof(t),
                      es1xMath_xtof(r),
                      es1xMath_xtof(q));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xNormal3f(void *_context_,
                                     GLfloat x,
                                     GLfloat y,
                                     GLfloat z)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glNormal3f"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_UPDATE_CONTEXT_VERTEX_PARAMETER(es1xVec3_set(&context->currentNormal,
                                                    x,
                                                    y,
                                                    z));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xNormal3x(void *_context_,
                                     GLfixed x,
                                     GLfixed y,
                                     GLfixed z)
{
  //  es1xContext *context = (es1xContext *) _context_;
  es1xNormal3f(_context_,
               es1xMath_xtof(x),
               es1xMath_xtof(y),
               es1xMath_xtof(z));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xPixelStorei(void *_context_,
                                        GLenum pname,
                                        GLint param)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glPixelStorei"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     pname == GL_UNPACK_ALIGNMENT || pname == GL_PACK_ALIGNMENT,
                                     GL_INVALID_ENUM);
  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     param == 1 || param == 2 || param == 4 || param == 8,
                                     GL_INVALID_VALUE);

  gl2b->glPixelStorei(pname,
                       param);
  ES1X_ASSUME_NO_ERROR;

#ifdef ES1X_DUPLICATE_ES2_STATE

  if (pname == GL_UNPACK_ALIGNMENT)
    context->unpackAlignment = param;
  else
    context->packAlignment = param;

#endif
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xPointParameterf(void *_context_,
                                            GLenum pname,
                                            GLfloat param)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glPointParameterf"));
  ES1X_CHECK_CONTEXT(context);

  switch(pname)
  {
    /* \todo check these parameters and usage out */
    case GL_POINT_SIZE_MIN:
      ES1X_SET_ERROR_AND_RETURN_IF_TRUE(_context_,
                                        param <= 0.f,
                                        GL_INVALID_VALUE);
      ES1X_UPDATE_CONTEXT_POINT_SIZE_PARAMETER(context->pointSizeMin = param);
      break;
    case GL_POINT_SIZE_MAX:
      ES1X_SET_ERROR_AND_RETURN_IF_TRUE(_context_,
                                        param <= 0.f,
                                        GL_INVALID_VALUE);
      ES1X_UPDATE_CONTEXT_POINT_SIZE_PARAMETER(context->pointSizeMax = param);
      break;
    case GL_POINT_FADE_THRESHOLD_SIZE:
      ES1X_SET_ERROR_AND_RETURN_IF_TRUE(_context_,
                                        param <= 0.f,
                                        GL_INVALID_VALUE);
      ES1X_UPDATE_CONTEXT_POINT_SIZE_PARAMETER(context->pointFadeThresholdSize = param);
      break;

      /* Point distance attenuation cannot be set using glPointParameterf */
    case GL_POINT_DISTANCE_ATTENUATION:
    default:
      es1xSetError(_context_, GL_INVALID_ENUM);
      break;
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xPointParameterfv(void *_context_,
                                             GLenum pname,
                                             const GLfloat *params)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glPointParameterfv"));
  ES1X_CHECK_CONTEXT(context);

  switch(pname)
  {
    case GL_POINT_DISTANCE_ATTENUATION:
      {
        ES1X_UPDATE_CONTEXT_POINT_SIZE_PARAMETER(es1xVec3_set(&context->pointDistanceAttenuation,
                                                              params[0],
                                                              params[1],
                                                              params[2]));
        break;
      }
    default:
      es1xPointParameterf(_context_, pname, *params);
      break;
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xPointParameterx(void *_context_,
                                            GLenum pname,
                                            GLfixed param)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glPointParameterx"));
  es1xPointParameterf(_context_, pname, es1xMath_xtof(param));

  return;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xPointParameterxv(void *_context_,
                                             GLenum pname,
                                             const GLfixed* params)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glPointParameterxv"));
  ES1X_CHECK_CONTEXT(context);

  /* Convert input and call float variants */
  switch(pname)
  {
    case GL_POINT_DISTANCE_ATTENUATION:
      {
        GLfloat converted[3];
        converted[0] = es1xMath_xtof(params[0]);
        converted[1] = es1xMath_xtof(params[1]);
        converted[2] = es1xMath_xtof(params[2]);
        es1xPointParameterfv(_context_, pname, (const GLfloat*)converted);
        break;
      }
    default:
      es1xPointParameterf(_context_, pname, es1xMath_xtof(*params));
      break;
  }

}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xPointSize(void *_context_,
                                      GLfloat size)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glPointSize"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_TRUE(_context_,
                                    size <= 0.f,
                                    GL_INVALID_VALUE);

  ES1X_UPDATE_CONTEXT_POINT_SIZE_PARAMETER(context->pointSize = size);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xPointSizex(void *_context_,
                                       GLfixed size)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glPointSizex"));
  es1xPointSize(_context_, es1xMath_xtof(size));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xPointSizePointerOES(void *_context_,
                                                GLenum type,
                                                GLsizei stride,
                                                const GLvoid* pointer)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glPointSizePointerOES"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     es1xIsValidVertexPointSizeDataType(type),
                                     GL_INVALID_ENUM);
  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     stride >= 0,
                                     GL_INVALID_VALUE);

  gl2b->glVertexAttribPointer(ES1X_POINT_SIZE_ARRAY_POS,
                               1,
                               type,
                               GL_FALSE,
                               stride,
                               pointer);

  context->pointSizeArrayPointerOES = pointer;
  context->pointSizeArrayTypeOES = type;
  context->pointSizeArrayStrideOES = stride;
  context->pointSizeArrayBufferBindingOES = context->activeBufferBinding;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xPolygonOffset(void *_context_,
                                          GLfloat factor,
                                          GLfloat units)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glPolygonOffset"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glPolygonOffset(factor,
                         units);

  /* glPolygonOffset should not produce errors */
  ES1X_ASSUME_NO_ERROR;

  ES20_DEBUG_CODE(context->polygonOffsetFactor = factor);
  ES20_DEBUG_CODE(context->polygonOffsetUnits = units);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xPolygonOffsetx(void *_context_,
                                           GLfixed factor,
                                           GLfixed units)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glPolygonOffsetx"));
  es1xPolygonOffset(_context_, es1xMath_xtof(factor), es1xMath_xtof(units));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xReadPixels(void *_context_,
                                       GLint x,
                                       GLint y,
                                       GLsizei width,
                                       GLsizei height,
                                       GLenum format,
                                       GLenum type,
                                       GLvoid* pixels)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_CHECK_CONTEXT(context);

  ES1X_LOG_CALL(("glReadPixels"));
  gl2b->glReadPixels(x,
                      y,
                      width,
                      height,
                      format,
                      type,
                      pixels);

  /* \note [090309:
     christian] No errors? */
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xSampleCoverage(void *_context_,
                                           GLclampf value,
                                           GLboolean invert)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glSampleCoverage"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  invert = es1xMath_clampb(invert);
  gl2b->glSampleCoverage(value,
                          invert);

  /* glSampleCoverage should not produce errors */
  ES1X_ASSUME_NO_ERROR;

  ES20_DEBUG_CODE(context->sampleCoverageValue = es1xMath_clampf(value));
  ES20_DEBUG_CODE(context->sampleCoverageInvertEnabled = invert);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xSampleCoveragex(void *_context_,
                                            GLclampx value,
                                            GLboolean invert)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glSampleCoveragex"));
  es1xSampleCoverage(_context_, es1xMath_xtof(value), invert);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xScissor(void *_context_,
                                    GLint x,
                                    GLint y,
                                    GLsizei width,
                                    GLsizei height)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glScissor"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glScissor(x,
                   y,
                   width,
                   height);
  es1xCheckError(_context_);

  ES20_DEBUG_CODE(ES1X_SET_ERROR_AND_RETURN_IF_TRUE(_context_,
                                                    width < 0           || height < 0,
                                                    GL_INVALID_VALUE));
  ES20_DEBUG_CODE(ES1X_SET_ERROR_AND_RETURN_IF_TRUE(_context_,
                                                    x + width < x || y + width < y,
                                                    GL_INVALID_VALUE));
  ES20_DEBUG_CODE(es1xRecti_set(&context->scissorBox,
                                x,
                                y,
                                x + width,
                                y + height));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xShadeModel(void *_context_,
                                       GLenum model)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glShadeModel"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     model == GL_SMOOTH || model == GL_FLAT,
                                     GL_INVALID_ENUM);

  /* \todo [2009-05-20 christian] This should be handled by underlying implementation? */
  ES1X_UPDATE_CONTEXT_UNSORTED_PARAMETER(context->shadeModelType = model);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xStencilFunc(void *_context_,
                                        GLenum func,
                                        GLint ref,
                                        GLuint mask)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glStencilFunc"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glStencilFunc(func,
                       ref,
                       mask);
  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xStencilMask(void *_context_,
                                        GLuint mask)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glStencilMask"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glStencilMask(mask);

  /* glStencilMask cannot produce an error */
  ES1X_ASSUME_NO_ERROR;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xStencilOp(void *_context_,
                                      GLenum fail,
                                      GLenum zFail,
                                      GLenum zPass)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glStencilOp"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glStencilOp(fail,
                     zFail,
                     zPass);
  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xTexCoordPointer(void *_context_,
                                            GLint size,
                                            GLenum type,
                                            GLsizei stride,
                                            const GLvoid* pointer)
{
  es1xContext *context = (es1xContext *) _context_;
  int textureUnitIndex;

  ES1X_LOG_CALL(("glTexCoordPointer(%d, %s, %d, 0x%p)\n",
                 size,
                 ES1X_ENUM_TO_STRING(type),
                 stride,
                 pointer));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     es1xIsValidVertexTextureCoordinateDataType(type),
                                     GL_INVALID_ENUM);
  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     size == 2 || size == 3 || size == 4,
                                     GL_INVALID_VALUE);
  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     stride >= 0,
                                     GL_INVALID_VALUE);

  textureUnitIndex = context->clientActiveTexture;
  ES1X_ASSERT(textureUnitIndex >= 0 && textureUnitIndex < ES1X_NUM_SUPPORTED_TEXTURE_UNITS);
  gl2b->glVertexAttribPointer(ES1X_TEXTURE_COORD_ARRAY0_POS + textureUnitIndex,
                               size,
                               type,
                               GL_FALSE,
                               stride,
                               pointer);
  ES1X_ASSUME_NO_ERROR;

  /* Pointer must be stored here too as it might be queried later */
  context->textureCoordArrayPointer[textureUnitIndex] = pointer;
  context->textureCoordArraySize[textureUnitIndex] = size;
  context->textureCoordArrayType[textureUnitIndex] = type;
  context->textureCoordArrayStride[textureUnitIndex] = stride;
  context->textureCoordArrayBufferBinding[textureUnitIndex] = context->activeBufferBinding;

}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xTexEnvf(void *_context_,
                                    GLenum target,
                                    GLenum pname,
                                    GLfloat param)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glTexEnvf"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     target == GL_TEXTURE_ENV || target == GL_POINT_SPRITE_OES,
                                     GL_INVALID_ENUM);

  switch(target)
  {
    case GL_POINT_SPRITE_OES:
      {
        switch(pname)
        {
          case GL_COORD_REPLACE_OES:
            {
              /* Clamp to boolean */
              GLboolean b = param != 0.f ? GL_TRUE :
                            GL_FALSE;
              ES1X_UPDATE_CONTEXT_TEXTURE_ENV_PARAMETER(context->activeTextureUnitIndex,
                                                        context->texEnvCoordReplaceOES[context->activeTextureUnitIndex] = b);
              break;
            }
          default:
            es1xSetError(_context_, GL_INVALID_ENUM);
            break;
        }
        break;
      }
    case GL_TEXTURE_ENV:
      {
        switch(pname)
        {
          case GL_COMBINE_RGB:
          case GL_COMBINE_ALPHA:
            es1xTexEnvi(_context_, target, pname, (GLint)param);
            break;
          case GL_RGB_SCALE:
            {
              ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                                 param == 1.0 || param == 2.0 || param == 4.0,
                                                 GL_INVALID_VALUE);
              ES1X_UPDATE_CONTEXT_TEXTURE_ENV_PARAMETER(context->activeTextureUnitIndex,
                                                        context->texEnv[context->activeTextureUnitIndex].rgbScale = param);
              break;
            }
          case GL_ALPHA_SCALE:
            {
              ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                                 param == 1.0 || param == 2.0 || param == 4.0,
                                                 GL_INVALID_VALUE);
              ES1X_UPDATE_CONTEXT_TEXTURE_ENV_PARAMETER(context->activeTextureUnitIndex,
                                                        context->texEnv[context->activeTextureUnitIndex].alphaScale = param);
              break;
            }

          case GL_TEXTURE_ENV_COLOR:
            es1xSetError(_context_, GL_INVALID_ENUM);
            break;
          default:
            {
              es1xTexEnvi(_context_, target, pname, (GLint)param);
              break;
            }
        }

        break;
      }
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xTexEnvfv(void *_context_,
                                     GLenum target,
                                     GLenum pname,
                                     const GLfloat* params)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glTexEnvfv"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     target == GL_TEXTURE_ENV || target == GL_POINT_SPRITE_OES,
                                     GL_INVALID_ENUM);

  switch(target)
  {
    case GL_POINT_SPRITE_OES:
      {
        es1xTexEnvf(_context_, target, pname, params[0]);
        break;
      }
    case GL_TEXTURE_ENV:
      {
        switch(pname)
        {
          case GL_TEXTURE_ENV_COLOR:
            {
              ES1X_UPDATE_CONTEXT_TEXTURE_ENV_PARAMETER(context->activeTextureUnitIndex,
                                                        (es1xColor_set(&context->texEnv[context->activeTextureUnitIndex].color,
                                                                       es1xMath_clampf(params[0]),
                                                                       es1xMath_clampf(params[1]),
                                                                       es1xMath_clampf(params[2]),
                                                                       es1xMath_clampf(params[3]))));
              break;
            }
          default:
            es1xTexEnvf(_context_, target, pname, params[0]);
            break;
        }
        break;
      }
    default:
      ES1X_ASSERT(!"Unhandled case");
      break;
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xTexEnvi(void *_context_,
                                    GLenum target,
                                    GLenum pname,
                                    GLint param)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glTexEnvi"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     target == GL_TEXTURE_ENV || target == GL_POINT_SPRITE_OES,
                                     GL_INVALID_ENUM);

  switch(target)
  {
    case GL_POINT_SPRITE_OES:
      {
        es1xTexEnvf(_context_, target, pname, (GLfloat) param);
        break;
      }
    case GL_TEXTURE_ENV:
      {
        switch(pname)
        {
          case GL_TEXTURE_ENV_MODE:
            {
              switch(param)
              {
                case GL_REPLACE:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvMode[context->activeTextureUnitIndex] = ES1X_TEXENVMODE_REPLACE);
                  break;
                case GL_MODULATE:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvMode[context->activeTextureUnitIndex] = ES1X_TEXENVMODE_MODULATE);
                  break;
                case GL_DECAL:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvMode[context->activeTextureUnitIndex] = ES1X_TEXENVMODE_DECAL);
                  break;
                case GL_BLEND:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvMode[context->activeTextureUnitIndex] = ES1X_TEXENVMODE_BLEND);
                  break;
                case GL_ADD:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvMode[context->activeTextureUnitIndex] = ES1X_TEXENVMODE_ADD);
                  break;
                case GL_COMBINE:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvMode[context->activeTextureUnitIndex] = ES1X_TEXENVMODE_COMBINE);
                  break;
                default:
                  es1xSetError(_context_, GL_INVALID_VALUE);
                  break;
              }
              break;
            }
          case GL_COMBINE_RGB:
            {
              switch(param)
              {
                case GL_REPLACE:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvCombineRGB[context->activeTextureUnitIndex] = ES1X_TEXENVCOMBINERGB_REPLACE);
                  break;
                case GL_MODULATE:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvCombineRGB[context->activeTextureUnitIndex] = ES1X_TEXENVCOMBINERGB_MODULATE);
                  break;
                case GL_ADD:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvCombineRGB[context->activeTextureUnitIndex] = ES1X_TEXENVCOMBINERGB_ADD);
                  break;
                case GL_ADD_SIGNED:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvCombineRGB[context->activeTextureUnitIndex] = ES1X_TEXENVCOMBINERGB_ADD_SIGNED);
                  break;
                case GL_INTERPOLATE:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvCombineRGB[context->activeTextureUnitIndex] = ES1X_TEXENVCOMBINERGB_INTERPOLATE);
                  break;
                case GL_SUBTRACT:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvCombineRGB[context->activeTextureUnitIndex] = ES1X_TEXENVCOMBINERGB_SUBTRACT);
                  break;
                case GL_DOT3_RGB:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvCombineRGB[context->activeTextureUnitIndex] = ES1X_TEXENVCOMBINERGB_DOT3_RGB);
                  break;
                case GL_DOT3_RGBA:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvCombineRGB[context->activeTextureUnitIndex] = ES1X_TEXENVCOMBINERGB_DOT3_RGBA);
                  break;
                default:
                  es1xSetError(_context_, GL_INVALID_VALUE);
                  break;
              }
              break;
            }
          case GL_COMBINE_ALPHA:
            {
              switch(param)
              {
                case GL_REPLACE:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvCombineAlpha[context->activeTextureUnitIndex] = ES1X_TEXENVCOMBINEALPHA_REPLACE);
                  break;
                case GL_MODULATE:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvCombineAlpha[context->activeTextureUnitIndex] = ES1X_TEXENVCOMBINEALPHA_MODULATE);
                  break;
                case GL_ADD:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvCombineAlpha[context->activeTextureUnitIndex] = ES1X_TEXENVCOMBINEALPHA_ADD);
                  break;
                case GL_ADD_SIGNED:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvCombineAlpha[context->activeTextureUnitIndex] = ES1X_TEXENVCOMBINEALPHA_ADD_SIGNED);
                  break;
                case GL_INTERPOLATE:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvCombineAlpha[context->activeTextureUnitIndex] = ES1X_TEXENVCOMBINEALPHA_INTERPOLATE);
                  break;
                case GL_SUBTRACT:
                  ES1X_UPDATE_CONTEXT_STATE(context->texEnvCombineAlpha[context->activeTextureUnitIndex] = ES1X_TEXENVCOMBINEALPHA_SUBTRACT);
                  break;
                default:
                  es1xSetError(_context_, GL_INVALID_VALUE);
                  break;
              }
              break;
            }
          case GL_RGB_SCALE:
          case GL_ALPHA_SCALE:
            es1xTexEnvf(_context_, target, pname, es1xMath_itof(param));
            break;
          case GL_SRC0_ALPHA:
          case GL_SRC1_ALPHA:
          case GL_SRC2_ALPHA:
          case GL_SRC0_RGB:
          case GL_SRC1_RGB:
          case GL_SRC2_RGB:
            {
              GLenum* dst = 0;
              switch(pname)
              {
                case GL_SRC0_RGB:
                  dst = (GLenum*) context->texEnvSrc0RGB;
                  break;
                case GL_SRC1_RGB:
                  dst = (GLenum*) context->texEnvSrc1RGB;
                  break;
                case GL_SRC2_RGB:
                  dst = (GLenum*) context->texEnvSrc2RGB;
                  break;
                case GL_SRC0_ALPHA:
                  dst = (GLenum*) context->texEnvSrc0Alpha;
                  break;
                case GL_SRC1_ALPHA:
                  dst = (GLenum*) context->texEnvSrc1Alpha;
                  break;
                case GL_SRC2_ALPHA:
                  dst = (GLenum*) context->texEnvSrc2Alpha;
                  break;
                default:
                  break;
              }
              ES1X_ASSERT(dst);

              switch(param)
              {
                case GL_TEXTURE:
                  ES1X_UPDATE_CONTEXT_TEXTURE_ENV_PARAMETER(context->activeTextureUnitIndex,
                                                            dst[context->activeTextureUnitIndex] = ES1X_TEXENVSRC_TEXTURE);
                  break;
                case GL_CONSTANT:
                  ES1X_UPDATE_CONTEXT_TEXTURE_ENV_PARAMETER(context->activeTextureUnitIndex,
                                                            dst[context->activeTextureUnitIndex] = ES1X_TEXENVSRC_CONSTANT);
                  break;
                case GL_PRIMARY_COLOR:
                  ES1X_UPDATE_CONTEXT_TEXTURE_ENV_PARAMETER(context->activeTextureUnitIndex,
                                                            dst[context->activeTextureUnitIndex] = ES1X_TEXENVSRC_PRIMARY_COLOR);
                  break;
                case GL_PREVIOUS:
                  ES1X_UPDATE_CONTEXT_TEXTURE_ENV_PARAMETER(context->activeTextureUnitIndex,
                                                            dst[context->activeTextureUnitIndex] = ES1X_TEXENVSRC_PREVIOUS);
                  break;
                default:
                  es1xSetError(_context_, GL_INVALID_ENUM);
                  break;
              }

              break;
            }

          case GL_OPERAND0_RGB:
          case GL_OPERAND1_RGB:
          case GL_OPERAND2_RGB:
            {
              GLenum* dst = 0;
              switch(pname)
              {
                case GL_OPERAND0_RGB:
                  dst = (GLenum*) context->texEnvOperand0RGB;
                  break;
                case GL_OPERAND1_RGB:
                  dst = (GLenum*) context->texEnvOperand1RGB;
                  break;
                case GL_OPERAND2_RGB:
                  dst = (GLenum*) context->texEnvOperand2RGB;
                  break;
                default:
                  es1xSetError(_context_, GL_INVALID_ENUM);
                  break;
              }
              ES1X_ASSERT(dst);

              switch(param)
              {
                case GL_SRC_COLOR:
                  ES1X_UPDATE_CONTEXT_TEXTURE_ENV_PARAMETER(context->activeTextureUnitIndex,
                                                            dst[context->activeTextureUnitIndex] = ES1X_TEXENVOPERANDRGB_SRC_COLOR);
                  break;
                case GL_ONE_MINUS_SRC_COLOR:
                  ES1X_UPDATE_CONTEXT_TEXTURE_ENV_PARAMETER(context->activeTextureUnitIndex,
                                                            dst[context->activeTextureUnitIndex] = ES1X_TEXENVOPERANDRGB_ONE_MINUS_SRC_COLOR);
                  break;
                case GL_SRC_ALPHA:
                  ES1X_UPDATE_CONTEXT_TEXTURE_ENV_PARAMETER(context->activeTextureUnitIndex,
                                                            dst[context->activeTextureUnitIndex] = ES1X_TEXENVOPERANDRGB_SRC_ALPHA);
                  break;
                case GL_ONE_MINUS_SRC_ALPHA:
                  ES1X_UPDATE_CONTEXT_TEXTURE_ENV_PARAMETER(context->activeTextureUnitIndex,
                                                            dst[context->activeTextureUnitIndex] = ES1X_TEXENVOPERANDRGB_ONE_MINUS_SRC_ALPHA);
                  break;
                default:
                  es1xSetError(_context_, GL_INVALID_ENUM);
                  break;
              }

              break;
            }

          case GL_OPERAND0_ALPHA:
          case GL_OPERAND1_ALPHA:
          case GL_OPERAND2_ALPHA:
            {
              GLenum* dst = 0;
              switch(pname)
              {
                case GL_OPERAND0_ALPHA:
                  dst = (GLenum*) context->texEnvOperand0Alpha;
                  break;
                case GL_OPERAND1_ALPHA:
                  dst = (GLenum*) context->texEnvOperand1Alpha;
                  break;
                case GL_OPERAND2_ALPHA:
                  dst = (GLenum*) context->texEnvOperand2Alpha;
                  break;
                default:
                  es1xSetError(_context_, GL_INVALID_ENUM);
                  break;
              }
              ES1X_ASSERT(dst);
              switch(param)
              {
                case GL_SRC_ALPHA:
                  ES1X_UPDATE_CONTEXT_TEXTURE_ENV_PARAMETER(context->activeTextureUnitIndex,
                                                            dst[context->activeTextureUnitIndex] = ES1X_TEXENVOPERANDALPHA_SRC_ALPHA);
                  break;
                case GL_ONE_MINUS_SRC_ALPHA:
                  ES1X_UPDATE_CONTEXT_TEXTURE_ENV_PARAMETER(context->activeTextureUnitIndex,
                                                            dst[context->activeTextureUnitIndex] = ES1X_TEXENVOPERANDALPHA_ONE_MINUS_SRC_ALPHA);
                  break;
                default:
                  es1xSetError(_context_, GL_INVALID_ENUM);
                  break;
              }
              break;
            }
          case GL_TEXTURE_ENV_COLOR:
          default:
            es1xSetError(_context_, GL_INVALID_ENUM);
            break;
        }

        break;
      }

    default:
      ES1X_ASSERT(!"Unhandled case");
      break;
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xTexEnvx(void *_context_,
                                    GLenum target,
                                    GLenum pname,
                                    GLfixed param)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glTexEnvx"));
  es1xTexEnvf(_context_, target, pname, (GLfloat)(param));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xTexEnviv(void *_context_,
                                     GLenum target,
                                     GLenum pname,
                                     const GLint* params)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glTexEnviv"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     target == GL_TEXTURE_ENV || target == GL_POINT_SPRITE_OES,
                                     GL_INVALID_ENUM);

  switch(target)
  {
    case GL_POINT_SPRITE_OES:
      {
        es1xTexEnvi(_context_,
                    target,
                    pname,
                    params[0]);
        break;
      }
    case GL_TEXTURE_ENV:
      {
        switch(pname)
        {
          case GL_TEXTURE_ENV_COLOR:
            {
              GLfloat convertedParams[4];

              convertedParams[0] = es1xMath_itof(params[0]);
              convertedParams[1] = es1xMath_itof(params[1]);
              convertedParams[2] = es1xMath_itof(params[2]);
              convertedParams[3] = es1xMath_itof(params[3]);
              es1xTexEnvfv(_context_, target, pname, convertedParams);
              break;
            }

          default:
            {
              es1xTexEnvi(_context_,
                          target,
                          pname,
                          params[0]);
              break;
            }
        }
        break;
      }
    default:
      ES1X_ASSERT(!"Unhandled case");
      break;
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xTexEnvxv(void *_context_,
                                     GLenum target,
                                     GLenum pname,
                                     const GLfixed* params)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glTexEnvxv"));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     target == GL_TEXTURE_ENV || target == GL_POINT_SPRITE_OES,
                                     GL_INVALID_ENUM);

  switch(target)
  {
    case GL_POINT_SPRITE_OES:
      {
        es1xTexEnvx(_context_, target, pname, params[0]);
        break;
      }
    case GL_TEXTURE_ENV:
      {
        switch(pname)
        {
          case GL_TEXTURE_ENV_COLOR:
            {
              GLfloat convertedParams[4];
              /* Because parameters are enums,
                 we treat them as them */
              es1xTypeConverter_itof(convertedParams,
                                     params,
                                     4);
              es1xTexEnvfv(_context_, target, pname, convertedParams);
              break;
            }
          default:
            {
              es1xTexEnvf(_context_, target, pname, (GLfloat) params[0]);
              break;
            }
        }
        break;
      }
    default:
      ES1X_ASSERT(!"Unhandled case");
      break;
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xTexParameterf(void *_context_,
                                          GLenum target,
                                          GLenum pname,
                                          GLfloat param)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glTexParameterf(%s, %s, %.8f)\n",
                 ES1X_ENUM_TO_STRING(target),
                 ES1X_ENUM_TO_STRING(pname),
                 param));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  /* Toggle mipmapping */
  if (target == GL_TEXTURE_2D && pname == GL_GENERATE_MIPMAP)
  {
    context->generateMipMap[context->activeTextureUnitIndex] = (param != 0.f);
    return;
  }

  gl2b->glTexParameterf(target,
                         pname,
                         param);
  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xTexParameterfv(void *_context_,
                                           GLenum target,
                                           GLenum pname,
                                           const GLfloat* params)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glTexParameterfv(%s, %s, 0x%p)\n",
                 ES1X_ENUM_TO_STRING(target),
                 ES1X_ENUM_TO_STRING(pname),
                 params));
  ES1X_CHECK_CONTEXT(context);
  ES1X_SET_ERROR_AND_RETURN_IF_NULL(_context_,
                                    params,
                                    GL_INVALID_VALUE);
  ES1X_ASSUME_NO_ERROR;

  /* Toggle mipmapping */
  if (target == GL_TEXTURE_2D && pname == GL_GENERATE_MIPMAP)
  {
    context->generateMipMap[context->activeTextureUnitIndex] = (params[0] != 0.f);
    return;
  }

  gl2b->glTexParameterfv(target,
                          pname,
                          params);
  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xTexParameteri(void *_context_,
                                          GLenum target,
                                          GLenum pname,
                                          GLint param)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glTexParameteri(%s, %s, %d)\n",
                 ES1X_ENUM_TO_STRING(target),
                 ES1X_ENUM_TO_STRING(pname),
                 param));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  /* Toggle mipmapping */
  if (target == GL_TEXTURE_2D && pname == GL_GENERATE_MIPMAP)
  {
    context->generateMipMap[context->activeTextureUnitIndex] = (param != 0);
    return;
  }

  gl2b->glTexParameteri(target,
                         pname,
                         param);
  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xTexParameterx(void *_context_,
                                          GLenum target,
                                          GLenum pname,
                                          GLfixed param)
{
  //   es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glTexParameterx(%s, %s, %.8x)\n",
                 ES1X_ENUM_TO_STRING(target),
                 ES1X_ENUM_TO_STRING(pname),
                 param));
  es1xTexParameterf(_context_, target, pname, (GLfloat)param);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xTexParameteriv(void *_context_,
                                           GLenum target,
                                           GLenum pname,
                                           const GLint* params)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glTexParameteriv(%s, %s, 0x%p)\n",
                 ES1X_ENUM_TO_STRING(target),
                 ES1X_ENUM_TO_STRING(pname),
                 params));
  ES1X_CHECK_CONTEXT(context);
  ES1X_SET_ERROR_AND_RETURN_IF_NULL(_context_,
                                    params,
                                    GL_INVALID_VALUE);
  ES1X_ASSUME_NO_ERROR;

  /* Toggle mipmapping */
  if (target == GL_TEXTURE_2D && pname == GL_GENERATE_MIPMAP)
  {
    context->generateMipMap[context->activeTextureUnitIndex] = (params[0] != 0);
    return;
  }

  gl2b->glTexParameteriv(target,
                          pname,
                          params);
  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xTexParameterxv(void *_context_,
                                           GLenum target,
                                           GLenum pname,
                                           const GLfixed* params)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glTexParameterxv"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  /* Just reroute call */
  es1xTexParameterf(_context_, target, pname, (GLfloat)params[0]);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xTexSubImage2D(void *_context_,
                                          GLenum target,
                                          GLint level,
                                          GLint xoffset,
                                          GLint yoffset,
                                          GLsizei width,
                                          GLsizei height,
                                          GLenum format,
                                          GLenum type,
                                          const GLvoid* pixels)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glTexSubImage2D"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glTexSubImage2D(target,
                         level,
                         xoffset,
                         yoffset,
                         width,
                         height,
                         format,
                         type,
                         pixels);
  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xViewport(void *_context_,
                                     GLint x,
                                     GLint y,
                                     GLsizei width,
                                     GLsizei height)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glViewport"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  gl2b->glViewport(x,
                    y,
                    width,
                    height);
  es1xCheckError(_context_);

  ES20_DEBUG_CODE(ES1X_SET_ERROR_AND_RETURN_IF_TRUE(_context_,
                                                    width < 0.f || height < 0.f,
                                                    GL_INVALID_VALUE));
  ES20_DEBUG_CODE(es1xRecti_set(&context->viewPort,
                                x,
                                y,
                                x + width,
                                x + height));
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xNormalPointer(void *_context_,
                                          GLenum type,
                                          GLsizei stride,
                                          const GLvoid* pointer)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glNormalPointer(%s, %d, 0x%p)\n",
                 ES1X_ENUM_TO_STRING(type),
                 stride,
                 pointer));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     es1xIsValidVertexNormalDataType(type),
                                     GL_INVALID_ENUM);
  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_,
                                     stride >= 0,
                                     GL_INVALID_VALUE);

  /* Assign the data for underlying implementation */
  gl2b->glVertexAttribPointer(ES1X_NORMAL_ARRAY_POS,
                               3,
                               type,
                               GL_FALSE,
                               stride,
                               pointer);
  ES1X_ASSUME_NO_ERROR;

  /* Pointer must be stored here too as it might be queried later */
  context->normalArrayPointer = pointer;
  context->normalArrayType = type;
  context->normalArrayStride = stride;
  context->normalArrayBufferBinding = context->activeBufferBinding;

}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xVertexPointer(void *_context_,
                                          GLint size,
                                          GLenum type,
                                          GLsizei stride,
                                          const GLvoid* pointer)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glVertexPointer(%d, %s, %d, 0x%p)\n",
                 size,
                 ES1X_ENUM_TO_STRING(type),
                 stride,
                 pointer));
  ES1X_CHECK_CONTEXT(context);

  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_, es1xIsValidVertexPositionDataType(type), GL_INVALID_ENUM);
  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_, size == 2 || size == 3 || size == 4, GL_INVALID_VALUE);
  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_, stride >= 0, GL_INVALID_VALUE);

  /* Assign the data for underlying implementation */
  gl2b->glVertexAttribPointer(ES1X_VERTEX_ARRAY_POS,
                               size,
                               type,
                               GL_FALSE,
                               stride,
                               pointer);
  ES1X_ASSUME_NO_ERROR;

  /* Pointer must be stored here too as it might be queried later */
  context->vertexArrayPointer = pointer;
  context->vertexArraySize = size;
  context->vertexArrayType = type;
  context->vertexArrayStride = stride;
  context->vertexArrayBufferBinding = context->activeBufferBinding;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xCurrentPaletteMatrixOES(void *_context_,
                                                    GLuint matrixpaletteindex)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_UNREF(matrixpaletteindex);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xLoadPaletteFromModelViewMatrixOES(void *_context_)
{
  //  es1xContext *context = (es1xContext *) _context_;
  return;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xMatrixIndexPointerOES(void *_context_,
                                                  GLint size,
                                                  GLenum type,
                                                  GLsizei stride,
                                                  const GLvoid *pointer)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_UNREF(size);
  ES1X_UNREF(type);
  ES1X_UNREF(stride);
  ES1X_UNREF(pointer);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xWeightPointerOES(void *_context_,
                                             GLint size,
                                             GLenum type,
                                             GLsizei stride,
                                             const GLvoid *pointer)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_UNREF(size);
  ES1X_UNREF(type);
  ES1X_UNREF(stride);
  ES1X_UNREF(pointer);
}
