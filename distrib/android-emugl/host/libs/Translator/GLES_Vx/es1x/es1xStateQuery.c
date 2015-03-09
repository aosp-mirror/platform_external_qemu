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

 
#include "es1xContext.h"
#include "es1xMemory.h"
#include "es1xStateQueryLookup.h"
#include "es1xError.h"
#include "es1xMath.h"
#include "es1xDebug.h"
#include "deInt32.h"
#include "es1xTypeConverter.h"
#include "es1xRouting.h"

/*---------------------------------------------------------------------------*/

static const deInt8* es1xGetContextParamSource(es1xContext* context,
                                               const es1xStateQueryEntry* entry,
                                               int elementIndex)
{
  return ((const deInt8*)context) + entry->sourceOffset + elementIndex * entry->elementOffset;
}

/*---------------------------------------------------------------------------*/

static void queryStateValue(void *_context_,
                            const es1xStateQueryEntry* entry,
                            void *dst,
                            es1xStateQueryParamType dstType,
                            int elementIndex)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_ASSERT(entry);

  /* Fetch source attributes and perform conversion */
  {
    es1xStateQueryParamType srcType             = entry->paramType;
    int                                         numElements = entry->numParams;

    const deInt8* src = es1xGetContextParamSource(context, entry, elementIndex);

    /* ensure that pointers are properly aligned */
    ES1X_ASSERT(srcType == PARAM_TYPE_BOOLEAN || deIsAlignedPtr(src, 4));
    ES1X_ASSERT(dstType == PARAM_TYPE_BOOLEAN || deIsAlignedPtr(dst, 4));

    es1xTypeConverter_convertEntry(entry, dst, dstType, src, srcType, numElements);
  }
}

/*---------------------------------------------------------------------------*/

static void handleSpecialQuery(void *_context_,
                               const es1xStateQueryEntry* entry,
                               void *dst,
                               es1xStateQueryParamType dstType)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_ASSERT(context);
  ES1X_ASSERT((entry->flags & FLG_IS_SPECIAL_QUERY) != 0);

  switch(entry->name)
  {
    case GL_MAX_LIGHTS:
      {
        GLint src = ES1X_NUM_SUPPORTED_LIGHTS;
        es1xTypeConverter_convert(dst, dstType, &src, PARAM_TYPE_INTEGER, 1);
        break;
      }
    case GL_MAX_CLIP_PLANES:
      {
        GLint src = ES1X_NUM_SUPPORTED_CLIP_PLANES;
        es1xTypeConverter_convert(dst, dstType, &src, PARAM_TYPE_INTEGER, 1);
        break;
      }
    case GL_MAX_MODELVIEW_STACK_DEPTH:
      {
        GLint src = ES1X_MODELVIEW_MATRIX_STACK_SIZE;
        es1xTypeConverter_convert(dst, dstType, &src, PARAM_TYPE_INTEGER, 1);
        break;
      }
    case GL_MAX_PROJECTION_STACK_DEPTH:
      {
        GLint src = ES1X_PROJECTION_MATRIX_STACK_SIZE;
        es1xTypeConverter_convert(dst, dstType, &src, PARAM_TYPE_INTEGER, 1);
        break;
      }
    case GL_MAX_TEXTURE_STACK_DEPTH:
      {
        GLint src = ES1X_TEXTURE_MATRIX_STACK_SIZE;
        es1xTypeConverter_convert(dst, dstType, &src, PARAM_TYPE_INTEGER, 1);
        break;
      }
    case GL_MODELVIEW_STACK_DEPTH:
      {
        GLint src = context->modelViewMatrixStack->top + 1;
        es1xTypeConverter_convert(dst, dstType, &src, PARAM_TYPE_INTEGER, 1);
        break;
      }
    case GL_PROJECTION_STACK_DEPTH:
      {
        GLint src = context->projectionMatrixStack->top + 1;
        es1xTypeConverter_convert(dst, dstType, &src, PARAM_TYPE_INTEGER, 1);
        break;
      }
    case GL_TEXTURE_STACK_DEPTH:
      {
        GLint src = context->textureMatrixStack[context->activeTextureUnitIndex]->top + 1;
        es1xTypeConverter_convert(dst, dstType, &src, PARAM_TYPE_INTEGER, 1);
        break;
      }

    case GL_MAX_TEXTURE_UNITS:
      {
        GLint src = ES1X_NUM_SUPPORTED_TEXTURE_UNITS;
        es1xTypeConverter_convert(dst, dstType, &src, PARAM_TYPE_INTEGER, 1);
        break;
      }
    case GL_MODELVIEW_MATRIX:
      {
        const GLfloat* src = (const GLfloat*) es1xMatrixStack_peekMatrix(context->modelViewMatrixStack)->data;
        es1xTypeConverter_convert(dst, dstType, src, PARAM_TYPE_FLOAT, 16);
        break;
      }
    case GL_PROJECTION_MATRIX:
      {
        const GLfloat* src = (const GLfloat*) es1xMatrixStack_peekMatrix(context->projectionMatrixStack)->data;
        es1xTypeConverter_convert(dst, dstType, src, PARAM_TYPE_FLOAT, 16);
        break;
      }
    case GL_TEXTURE_MATRIX:
      {
        const GLfloat* src = (const GLfloat*) es1xMatrixStack_peekMatrix(context->textureMatrixStack[context->activeTextureUnitIndex])->data;
        es1xTypeConverter_convert(dst, dstType, src, PARAM_TYPE_FLOAT, 16);
        break;
      }
    case GL_MODELVIEW_MATRIX_FLOAT_AS_INT_BITS_OES:
      {
        es1xMemCopy(dst, es1xMatrixStack_peekMatrix(context->modelViewMatrixStack)->data, sizeof(GLfloat) * 16);
        break;
      }
    case GL_PROJECTION_MATRIX_FLOAT_AS_INT_BITS_OES:
      {
        es1xMemCopy(dst, es1xMatrixStack_peekMatrix(context->projectionMatrixStack)->data, sizeof(GLfloat) * 16);
        break;
      }
    case GL_TEXTURE_MATRIX_FLOAT_AS_INT_BITS_OES:
      {
        es1xMemCopy(dst, es1xMatrixStack_peekMatrix(context->textureMatrixStack[context->activeTextureUnitIndex])->data, sizeof(GLfloat) * 16);
        break;
      }

    case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
      {
        /* \note [090512:christian] reroute this query to underlying implementation? */
        GLenum type = GL_UNSIGNED_BYTE;
        es1xTypeConverter_convert(dst, dstType, &type, PARAM_TYPE_INTEGER, 1);
        break;
      }

    case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
      {
        /* \note [090512:christian] reroute this query to underlying implementation? */
        GLenum format = GL_RGBA;
        es1xTypeConverter_convert(dst, dstType, &format, PARAM_TYPE_INTEGER, 1);
        break;
      }

    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
      {
        GLenum value = ES1X_NUM_SUPPORTED_COMPRESSED_TEXTURE_FORMATS;
        es1xTypeConverter_convert(dst, dstType, &value, PARAM_TYPE_INTEGER, 1);
        break;
      }

    case GL_COMPRESSED_TEXTURE_FORMATS:
      {
        es1xTypeConverter_convert(dst, dstType, s_supportedCompressedTextureFormats, PARAM_TYPE_INTEGER, ES1X_NUM_SUPPORTED_COMPRESSED_TEXTURE_FORMATS);
        break;
      }

    case GL_TEXTURE_ENV_COLOR:
      {
        es1xTypeConverter_convert(dst, dstType, &context->texEnv[context->activeTextureUnitIndex].color, PARAM_TYPE_FLOAT, 4);
        break;
      }
    case GL_RGB_SCALE:
      {
        es1xTypeConverter_convert(dst, dstType, &context->texEnv[context->activeTextureUnitIndex].rgbScale, PARAM_TYPE_FLOAT, 1);
        break;
      }
    case GL_ALPHA_SCALE:
      {
        es1xTypeConverter_convert(dst, dstType, &context->texEnv[context->activeTextureUnitIndex].alphaScale, PARAM_TYPE_FLOAT, 1);
        break;
      }

    case GL_CURRENT_NORMAL:
      {
        const deInt8* src = es1xGetContextParamSource(context, entry, 0);
        ES1X_ASSERT(entry->paramType == PARAM_TYPE_FLOAT);
        es1xTypeConverter_convert(dst, dstType, src, PARAM_TYPE_CLAMPF, entry->numParams);
        break;
      }

    case GL_CURRENT_TEXTURE_COORDS:
      {
        const deInt8* src = es1xGetContextParamSource(context, entry, context->activeTextureUnitIndex);
        es1xTypeConverter_convert(dst, dstType, src, entry->paramType, entry->numParams);
        break;
      }

    case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING:
    case GL_TEXTURE_COORD_ARRAY:
    case GL_TEXTURE_COORD_ARRAY_SIZE:
    case GL_TEXTURE_COORD_ARRAY_STRIDE:
    case GL_TEXTURE_COORD_ARRAY_TYPE:
    case GL_TEXTURE_COORD_ARRAY_POINTER:
      {
        const deInt8* src = es1xGetContextParamSource(context, entry, context->clientActiveTexture);
        es1xTypeConverter_convert(dst, dstType, src, entry->paramType, entry->numParams);
        break;
      }

    case GL_CLIENT_ACTIVE_TEXTURE:
      {
        const GLenum src = GL_TEXTURE0 + context->clientActiveTexture;
        es1xTypeConverter_convert(dst, dstType, &src, PARAM_TYPE_GL_ENUM, 1);
        break;
      }

    case GL_ACTIVE_TEXTURE:
      {
        const GLenum src = GL_TEXTURE0 + context->activeTextureUnitIndex;
        es1xTypeConverter_convert(dst, dstType, &src, PARAM_TYPE_GL_ENUM, 1);
        break;
      }

    case GL_BLEND_SRC:
    case GL_BLEND_DST:

    default:
      ES1X_ASSERT(!"Unhandled case!");
      break;
  }
}

/*---------------------------------------------------------------------------*/

static const es1xStateQueryEntry* getStateQueryEntry(GLenum pname, deUint32 queryMethod)
{
  /* perform a simple binary search for given enum,
     return null if not found */

  int leftBoundary      = -1;
  int rightBoundary     = STATE_QUERY_ENTRY_LOOKUP_SIZE;
  int searchIndex               = STATE_QUERY_ENTRY_LOOKUP_SIZE / 2;
  int prevSearchIndex = -1;

  while(searchIndex != prevSearchIndex)
  {
    const es1xStateQueryEntry* entry = 0;

    ES1X_ASSERT(searchIndex >= 0 && searchIndex < STATE_QUERY_ENTRY_LOOKUP_SIZE);

    entry = &STATE_QUERY_ENTRY_LOOKUP[searchIndex];

    if (entry->name == pname)
    {
      if ((entry->flags & queryMethod) != 0)
        return entry;

      /*        Switch binary search to two directional linear search as there might
                be multiple entries with same enumeration, but they mean different things
                (e.g. material and light diffuses) */
      prevSearchIndex = searchIndex;

      /* Search backwards */
      for(searchIndex=prevSearchIndex-1; searchIndex>=0; searchIndex--)
      {
        entry = &STATE_QUERY_ENTRY_LOOKUP[searchIndex];
        if (entry->name != pname)
          break;
        if ((entry->flags & queryMethod) != 0)
          return entry;
      };

      /* Search forward */
      for(searchIndex=prevSearchIndex+1; searchIndex<STATE_QUERY_ENTRY_LOOKUP_SIZE; searchIndex++)
      {
        entry = &STATE_QUERY_ENTRY_LOOKUP[searchIndex];
        if (entry->name != pname)
          break;
        if ((entry->flags & queryMethod) != 0)
          return entry;
      };

      /* No luck */
      return 0;
    }

    /* find out which direction to continue the search
       and update boundaries */
    prevSearchIndex = searchIndex;

    if (entry->name < pname)
    {
      /* Greater */
      leftBoundary      = searchIndex;
      searchIndex               = searchIndex + (rightBoundary - leftBoundary) / 2;
    }
    else
    {
      /* Less */
      rightBoundary     = searchIndex;
      searchIndex               = searchIndex - (rightBoundary - leftBoundary) / 2;
    }
  }

  return 0;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLenum convertEnumToForwardCompatible(GLenum param)
{
  switch(param)
  {
    case GL_BLEND_SRC:          return GL_BLEND_SRC_RGB;
    case GL_BLEND_DST:          return GL_BLEND_DST_RGB;

    default:
      /* Nothing to do */
      return param;
  }
}

/*---------------------------------------------------------------------------*/

static void es1xGetGeneric(void *_context_,
                           GLenum pname,
                           void * dst,
                           es1xStateQueryParamType dstType,
                           deUint32 queryMethod,
                           int elementIndex)
{
  es1xContext *context = (es1xContext *) _context_;
  const es1xStateQueryEntry* entry = ES1X_NULL;

  ES1X_CHECK_CONTEXT(context);
  ES1X_ASSUME_NO_ERROR;

  /* Fetch query info from generated lookup */
  entry = getStateQueryEntry(pname, queryMethod);
  ES1X_SET_ERROR_AND_RETURN_IF_NULL(_context_, entry, GL_INVALID_ENUM);

  if (!!(entry->flags & FLG_IS_SPECIAL_QUERY))
  {
    handleSpecialQuery(_context_, entry, dst, dstType);

    /* Error should be handled already */
    ES1X_ASSUME_NO_ERROR;
    return;
  }

  /* Redirect call to underlying implementation? */
  if (!!(entry->flags & FLG_ES2_FUNCTIONALITY))
  {
    pname = convertEnumToForwardCompatible(pname);

    ES1X_ASSERT(dePop32((int)queryMethod & (~FLG_ES2_FUNCTIONALITY)) == 1);
    ES1X_ASSUME_NO_ERROR;

    switch(queryMethod)
    {
      case FLG_SUPPORT_GETPOINTERV:
      case FLG_SUPPORT_GETINTEGERV:     gl2b->glGetIntegerv  (pname, (GLint*)dst);           break;
      case FLG_SUPPORT_GETFLOATV:       gl2b->glGetFloatv    (pname, (GLfloat*)dst);         break;
      case FLG_SUPPORT_GETBOOLEANV:     gl2b->glGetBooleanv  (pname, (GLboolean*)dst);       break;
      case FLG_SUPPORT_GETFIXEDV:
        {
#define NUM_MAX_PARAMETERS 256
          GLint params[NUM_MAX_PARAMETERS];

          /* Query as integer and convert to fixed point */
          ES1X_ASSERT(entry->numParams < NUM_MAX_PARAMETERS);
          gl2b->glGetIntegerv(pname, (GLint*)params);

          /* Enum to fixed point must be handled separately */
          if (entry->paramType == PARAM_TYPE_GL_ENUM)
            es1xTypeConverter_convert(dst, PARAM_TYPE_FIXED, params, PARAM_TYPE_GL_ENUM, entry->numParams);
          else
            es1xTypeConverter_convert(dst, PARAM_TYPE_FIXED, params, PARAM_TYPE_INTEGER, entry->numParams);

#undef NUM_MAX_PARAMETERS /* Keep preprocessor clean */
          break;
        }
      case FLG_SUPPORT_ISENABLED:
        switch(pname)
        {
          case GL_COLOR_LOGIC_OP:
            pname = GL_COLOR_LOGIC_OP; // GL_COLOR_LOGIC_OP_DE;
            break;
        }
        *((GLboolean*)dst) = gl2b->glIsEnabled(pname);
        break;
      default:
        ES1X_ASSERT(!"Unhandled case!");
        break;
    }

    /* Should be handled with no error by underlying implementation */
    ES1X_ASSUME_NO_ERROR;
    return;
  }

  /* Call generic query and conversion routines */
  queryStateValue(_context_, entry, dst, dstType, elementIndex);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetIntegerv(void *_context_, GLenum pname, GLint* params)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetIntegerv(%s, %p)\n", ES1X_ENUM_TO_STRING(pname), params));
  es1xGetGeneric(_context_, pname, params, PARAM_TYPE_INTEGER, FLG_SUPPORT_GETINTEGERV, 0);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetBooleanv(void *_context_, GLenum pname, GLboolean* params)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetBooleanv(%s, %p)\n", ES1X_ENUM_TO_STRING(pname), params));
  es1xGetGeneric(_context_, pname, params, PARAM_TYPE_BOOLEAN, FLG_SUPPORT_GETBOOLEANV, 0);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetFloatv(void *_context_, GLenum pname, GLfloat* params)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetFloatv(%s, %p)\n", ES1X_ENUM_TO_STRING(pname), params));
  es1xGetGeneric(_context_, pname, params, PARAM_TYPE_FLOAT, FLG_SUPPORT_GETFLOATV, 0);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetFixedv(void *_context_, GLenum pname, GLfixed* params)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetFixedv(%s, %p)\n", ES1X_ENUM_TO_STRING(pname), params));
  es1xGetGeneric(_context_, pname, params, PARAM_TYPE_FIXED, FLG_SUPPORT_GETFIXEDV, 0);
}

/*---------------------------------------------------------------------------*/

GL_API GLboolean GL_APIENTRY es1xIsEnabled(void *_context_, GLenum cap)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glIsEnabled(%s)\n", ES1X_ENUM_TO_STRING(cap)));
  ES1X_CHECK_CONTEXT_RET_FALSE(context);

  /* Handle texture coord array query */
  if (cap == GL_TEXTURE_COORD_ARRAY)
    return context->textureCoordArrayEnabled[context->clientActiveTexture];

  /* handle texture unit enabled query */
  if (cap == GL_TEXTURE_2D)
    return context->textureUnitEnabled[context->activeTextureUnitIndex];

  /* Handle clip plane query */
  if (cap >= GL_CLIP_PLANE0 && cap < GL_CLIP_PLANE0 + ES1X_NUM_SUPPORTED_CLIP_PLANES)
  {
    int ndx = cap - GL_CLIP_PLANE0;
    return context->clipPlaneEnabled[ndx];
  }

  /* Light query */
  if (cap >= GL_LIGHT0 && cap < GL_LIGHT0 + ES1X_NUM_SUPPORTED_LIGHTS)
  {
    int ndx = cap - GL_LIGHT0;
    return context->lightEnabled[ndx];
  }

  /* Perform generic query */
  {
    GLboolean retVal = GL_FALSE;
    es1xGetGeneric(_context_, cap, &retVal, PARAM_TYPE_BOOLEAN, FLG_SUPPORT_ISENABLED, 0);
    return retVal;
  }
}

/*---------------------------------------------------------------------------*/

static GLboolean GL_APIENTRY es1xGetClipPlane(void *_context_, GLenum pname, GLvoid* params, es1xStateQueryParamType paramType)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_CHECK_CONTEXT_RET_FALSE(context);

  ES1X_ASSERT(paramType >= 0 && paramType < NUM_PARAM_TYPES);

  if (pname >= GL_CLIP_PLANE0 && pname < GL_CLIP_PLANE0 + ES1X_NUM_SUPPORTED_CLIP_PLANES)
  {
    int ndx = pname - GL_CLIP_PLANE0;
    ES1X_ASSERT(ndx >= 0 && ndx < ES1X_NUM_SUPPORTED_CLIP_PLANES);

    es1xTypeConverter_convert(params, paramType, &context->clipPlane[ndx], PARAM_TYPE_FLOAT, 4);
    return GL_TRUE;
  }

  es1xSetError(_context_, GL_INVALID_ENUM);
  return GL_FALSE;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetClipPlanef(void *_context_, GLenum pname, GLfloat eqn[4])
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetClipPlanef(%s, 0x%p)\n", ES1X_ENUM_TO_STRING(pname), eqn));
  es1xGetClipPlane(_context_, pname, eqn, PARAM_TYPE_FLOAT);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetClipPlanex(void *_context_, GLenum pname, GLfixed eqn[4])
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetClipPlanex(%s, 0x%p)\n", ES1X_ENUM_TO_STRING(pname), eqn));
  es1xGetClipPlane(_context_, pname, eqn, PARAM_TYPE_FIXED);
}

/*---------------------------------------------------------------------------*/

static void es1xGetMaterialv(es1xContext *_context_,
                             GLenum face,
                             GLenum pname,
                             GLvoid* params,
                             es1xStateQueryParamType paramType)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_CHECK_CONTEXT(context);

  /* Check for invalid face */
  if (face != GL_FRONT &&
      face != GL_BACK)
  {
    es1xSetError(_context_, GL_INVALID_ENUM);
    return;
  }

  /* Color material query? */
  if (context->colorMaterialEnabled)
  {
    if (pname == GL_AMBIENT || pname == GL_DIFFUSE)
    {
      es1xGetGeneric(_context_, GL_CURRENT_COLOR, params, paramType, FLG_SUPPORT_GETINTEGERV, 0);
      return;
    }
  }

  es1xGetGeneric(_context_, pname, params, paramType, FLG_SUPPORT_GETMATERIALV, 0);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetMaterialfv(void *_context_, GLenum face, GLenum pname, GLfloat* params)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetMaterialfv"));
  es1xGetMaterialv(_context_, face, pname, params, PARAM_TYPE_FLOAT);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetMaterialxv(void *_context_, GLenum face, GLenum pname, GLfixed* params)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetMaterialxv"));
  es1xGetMaterialv(_context_, face, pname, params, PARAM_TYPE_FIXED);
}

/*---------------------------------------------------------------------------*/

static void GL_APIENTRY es1xGetLightv(void *_context_, GLenum light, GLenum pname, GLvoid* params, es1xStateQueryParamType paramType)
{
  es1xContext *context = (es1xContext *) _context_;
  int ndx = light - GL_LIGHT0;

  ES1X_LOG_CALL(("glGetLightv"));
  ES1X_CHECK_CONTEXT(context);

  /* Check invalid input */
  if (ndx < 0 || ndx >= ES1X_NUM_SUPPORTED_LIGHTS)
  {
    es1xSetError(_context_, GL_INVALID_ENUM);
    return;
  }

  es1xGetGeneric(_context_, pname, params, paramType, FLG_SUPPORT_GETLIGHTV, ndx);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetLightfv(void *_context_, GLenum light, GLenum pname, GLfloat* params)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetLightfv"));
  es1xGetLightv(_context_, light, pname, params, PARAM_TYPE_FLOAT);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetLightxv(void *_context_, GLenum light, GLenum pname, GLfixed* params)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetLightxv"));
  es1xGetLightv(_context_, light, pname, params, PARAM_TYPE_FIXED);
}

/*---------------------------------------------------------------------------*/

static void es1xGetTexEnv(void *_context_, GLenum env, GLenum pname, GLvoid* params, es1xStateQueryParamType paramType, GLuint flags)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetTexEnv"));
  ES1X_CHECK_CONTEXT(context);

  if (env != GL_TEXTURE_ENV && env != GL_POINT_SPRITE_OES)
  {
    es1xSetError(_context_, GL_INVALID_ENUM);
    return;
  }

  es1xGetGeneric(_context_, pname, params, paramType, flags, context->activeTextureUnitIndex);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetTexEnviv(void *_context_, GLenum env, GLenum pname, GLint* params)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetTexEnviv"));
  es1xGetTexEnv(_context_, env, pname, params, PARAM_TYPE_INTEGER, FLG_SUPPORT_GETTEXENVV);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetTexEnvfv(void *_context_, GLenum env, GLenum pname, GLfloat* params)
{
  //   es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetTexEnvfv"));
  es1xGetTexEnv(_context_, env, pname, params, PARAM_TYPE_FLOAT, FLG_SUPPORT_GETTEXENVV);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetTexEnvxv(void *_context_, GLenum env, GLenum pname, GLfixed* params)
{
  //  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetTexEnvxv"));
  es1xGetTexEnv(_context_, env, pname, params, PARAM_TYPE_FIXED, FLG_SUPPORT_GETTEXENVV);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetTexParameterfv(void *_context_, GLenum target, GLenum pname, GLfloat* params)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetTexParameterfv"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_, target == GL_TEXTURE_2D, GL_INVALID_ENUM);
  ES1X_SET_ERROR_AND_RETURN_IF_NULL(_context_, params, GL_INVALID_VALUE);

  /* Is mipmap query? */
  if (target == GL_TEXTURE_2D && pname == GL_GENERATE_MIPMAP)
  {
    es1xGetGeneric(_context_, pname, params, PARAM_TYPE_FLOAT, FLG_SUPPORT_GETTEXPARAMETER, context->activeTextureUnitIndex);
    return;
  }

  gl2b->glGetTexParameterfv(target, pname, params);
  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetTexParameteriv(void *_context_, GLenum target, GLenum pname, GLint* params)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetTexParameteriv"));
  ES1X_CHECK_CONTEXT(context);
  ES1X_SET_ERROR_AND_RETURN_IF_FALSE(_context_, target == GL_TEXTURE_2D, GL_INVALID_ENUM);
  ES1X_SET_ERROR_AND_RETURN_IF_NULL(_context_, params, GL_INVALID_VALUE);

  /* Is mipmap query? */
  if (target == GL_TEXTURE_2D && pname == GL_GENERATE_MIPMAP)
  {
    es1xGetGeneric(_context_, pname, params, PARAM_TYPE_INTEGER, FLG_SUPPORT_GETTEXPARAMETER, context->activeTextureUnitIndex);
    return;
  }

  gl2b->glGetTexParameteriv(target, pname, params);
  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetTexParameterxv(void *_context_, GLenum target, GLenum pname, GLfixed* params)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetTexParameterxv"));
  ES1X_CHECK_CONTEXT(context);

  /* Query as float and convert */
  {
    GLfloat temp;
    es1xGetTexParameterfv(_context_, target, pname, &temp);
    params[0] = es1xMath_ftox(temp);
  }
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetBufferParameteriv(void *_context_, GLenum target, GLenum pname, GLint* params)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetBufferParameteriv"));
  ES1X_CHECK_CONTEXT(context);

  gl2b->glGetBufferParameteriv(target, pname, params);
  es1xCheckError(_context_);
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xGetPointerv(void *_context_, GLenum pname, void** params)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetPointerv"));
  ES1X_CHECK_CONTEXT(context);

  if (pname != GL_TEXTURE_COORD_ARRAY_POINTER)
    es1xGetGeneric(_context_, pname, params, PARAM_TYPE_INTEGER, FLG_SUPPORT_GETPOINTERV, 0);
  else
    es1xGetGeneric(_context_, pname, params, PARAM_TYPE_INTEGER, FLG_SUPPORT_GETPOINTERV, context->clientActiveTexture);

  return;
}

/*---------------------------------------------------------------------------*/

GL_API const GLubyte* GL_APIENTRY es1xGetString(void *_context_, GLenum pname)
{
  //   es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glGetString"));
  switch(pname)
  {
    case GL_VENDOR:             return (const GLubyte*)ES1X_STRING_VENDOR;
    case GL_VERSION:            return (const GLubyte*)ES1X_STRING_VERSION;
    case GL_RENDERER:           return (const GLubyte*)ES1X_STRING_RENDERER;
    case GL_EXTENSIONS:         return (const GLubyte*)ES1X_STRING_EXTENSIONS;
    default:
      es1xSetError(_context_, GL_INVALID_ENUM);
      return (const GLubyte*) "";
  }
}
