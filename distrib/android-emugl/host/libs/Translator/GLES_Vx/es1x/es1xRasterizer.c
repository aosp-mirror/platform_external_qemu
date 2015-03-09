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
#include "es1xColor.h"
#include "es1xShaderCache.h"
#include "es1xContext.h"
#include "es1xShaderContextKey.h"
#include "es1xShaderPrograms.h"
#include "es1xMatrix.h"
#include "es1xMath.h"
#include "es1xMath.h"
#include "es1xDebug.h"
#include "es1xShaderCodeUniforms.h"
#include "es1xRouting.h"


#define ES1X_LOG_UNIFORM_UPDATE(P) printf P
// #define ES1X_LOG_UNIFORM_UPDATE(P)

ES1X_DEBUG_CODE(extern const char* ES1X_SHADER_CODE_UNIFORM_NAME_LOOKUP[]);

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xAssignIntegerUniform(es1xShaderProgramInfo* info, es1xShaderCodeUniform uniformID, GLint param)
{
  int uniformLocation = info->uniformLocations[uniformID];

  if (uniformLocation >= 0)
  {
    ES1X_DEBUG_CODE(GLenum error);
    gl2b->glUniform1i(uniformLocation, param);
    ES1X_DEBUG_CODE(error = gl2b->glGetError());
    ES1X_DEBUG_CODE(ES1X_ASSERT(gl2b->glGetError() == GL_NO_ERROR));
    ES1X_DEBUG_CODE(ES1X_LOG_UNIFORM_UPDATE(("Updating uniform: %s = %d\n", ES1X_SHADER_CODE_UNIFORM_NAME_LOOKUP[uniformID], param)));
  }
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xAssignFloatUniform(es1xShaderProgramInfo* info, es1xShaderCodeUniform uniformID, GLfloat param)
{
  int uniformLocation = info->uniformLocations[uniformID];

  if (uniformLocation >= 0)
  {
    ES1X_DEBUG_CODE(GLenum error);
    gl2b->glUniform1f(uniformLocation, param);
    ES1X_DEBUG_CODE(error = gl2b->glGetError());
    ES1X_DEBUG_CODE(ES1X_ASSERT(gl2b->glGetError() == GL_NO_ERROR));
    ES1X_DEBUG_CODE(ES1X_LOG_UNIFORM_UPDATE(("Updating uniform: %s = %.8f\n", ES1X_SHADER_CODE_UNIFORM_NAME_LOOKUP[uniformID], param)));
  }
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xAssignVec4Uniform(es1xShaderProgramInfo* info, es1xShaderCodeUniform uniformID, const GLfloat* params)
{
  int uniformLocation = info->uniformLocations[uniformID];

  if (uniformLocation >= 0)
  {
    ES1X_DEBUG_CODE(GLenum error);
    gl2b->glUniform4fv(uniformLocation, 1, params);
    ES1X_DEBUG_CODE(error = gl2b->glGetError());
    ES1X_DEBUG_CODE(ES1X_ASSERT(gl2b->glGetError() == GL_NO_ERROR));
    ES1X_DEBUG_CODE(ES1X_LOG_UNIFORM_UPDATE(("Updating uniform: %s = [%.8f, %.8f, %.8f, %.8f]\n", ES1X_SHADER_CODE_UNIFORM_NAME_LOOKUP[uniformID], params[0], params[1], params[2], params[3])));
  }
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xAssignVec3Uniform(es1xShaderProgramInfo* info, es1xShaderCodeUniform uniformID, const GLfloat* params)
{
  int uniformLocation = info->uniformLocations[uniformID];

  if (uniformLocation >= 0)
  {
    ES1X_DEBUG_CODE(GLenum error);
    gl2b->glUniform3fv(uniformLocation, 1, params);
    ES1X_DEBUG_CODE(error = gl2b->glGetError());
    ES1X_DEBUG_CODE(ES1X_ASSERT(gl2b->glGetError() == GL_NO_ERROR));
    ES1X_DEBUG_CODE(ES1X_LOG_UNIFORM_UPDATE(("Updating uniform: %s = [%.8f, %.8f, %.8f]\n", ES1X_SHADER_CODE_UNIFORM_NAME_LOOKUP[uniformID], params[0], params[1], params[2])));
  }
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xAssignMatrix3x3Uniform(es1xShaderProgramInfo* info, es1xShaderCodeUniform uniformID, const es1xMatrix3x3* matrix)
{
  int uniformLocation = info->uniformLocations[uniformID];

  if (uniformLocation >= 0)
  {
    ES1X_DEBUG_CODE(GLenum error);
    gl2b->glUniformMatrix3fv( uniformLocation, 1, GL_FALSE, (const GLfloat*)matrix->data);
    ES1X_DEBUG_CODE(error = gl2b->glGetError());
    ES1X_DEBUG_CODE(ES1X_ASSERT(gl2b->glGetError() == GL_NO_ERROR));
    ES1X_DEBUG_CODE(ES1X_LOG_UNIFORM_UPDATE(("Updating uniform: %s = \n                       [ %.8f, %.8f, %.8f ]\n                       [ %.8f, %.8f, %.8f ]\n                       [ %.8f, %.8f, %.8f ]\n", ES1X_SHADER_CODE_UNIFORM_NAME_LOOKUP[uniformID], matrix->data[0][0], matrix->data[0][1], matrix->data[0][2], matrix->data[1][0], matrix->data[1][1], matrix->data[1][2], matrix->data[2][0], matrix->data[2][1], matrix->data[2][2])));
  }
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xAssignMatrix4x4Uniform(es1xShaderProgramInfo* info, es1xShaderCodeUniform uniformID, const es1xMatrix4x4* matrix)
{
  int uniformLocation = info->uniformLocations[uniformID];

  if (uniformLocation >= 0)
  {
    ES1X_DEBUG_CODE(GLenum error);
    gl2b->glUniformMatrix4fv( uniformLocation, 1, GL_FALSE, (const GLfloat*)matrix->data);
    ES1X_DEBUG_CODE(error = gl2b->glGetError());
    ES1X_DEBUG_CODE(ES1X_ASSERT(gl2b->glGetError() == GL_NO_ERROR));
    ES1X_DEBUG_CODE(ES1X_LOG_UNIFORM_UPDATE(("Updating uniform: %s = \n                       [ %.8f, %.8f, %.8f, %.8f ]\n                       [ %.8f, %.8f, %.8f, %.8f ]\n                       [ %.8f, %.8f, %.8f, %.8f ]\n                       [ %.8f, %.8f, %.8f, %.8f ]\n", ES1X_SHADER_CODE_UNIFORM_NAME_LOOKUP[uniformID], matrix->data[0][0], matrix->data[0][1], matrix->data[0][2], matrix->data[0][3], matrix->data[1][0], matrix->data[1][1], matrix->data[1][2], matrix->data[1][3], matrix->data[2][0], matrix->data[2][1], matrix->data[2][2], matrix->data[2][3], matrix->data[3][0], matrix->data[3][1], matrix->data[3][2], matrix->data[3][3])));
  }
}

/*---------------------------------------------------------------------------*/

static void es1xAssignUniforms(const void* _context_, es1xShaderProgramInfo* info, const es1xIndexMapTable* table)
{
  const es1xContext *context = (const es1xContext *) _context_;

  ES1X_ASSUME_NO_ERROR;

  /* Assign matrix uniforms */
  {
    int pmvMatrixLocation               = info->uniformLocations[UNIFORM_PMV_MATRIX];
    int mvMatrixLocation                = info->uniformLocations[UNIFORM_MV_MATRIX];
    int mvMatrixInversedLocation        = info->uniformLocations[UNIFORM_MV_MATRIX_INV];

    const es1xMatrix4x4* pMatrix        = es1xMatrixStack_peekMatrix(context->projectionMatrixStack);
    const es1xMatrix4x4* mvMatrix       = es1xMatrixStack_peekMatrix(context->modelViewMatrixStack);

    if (pmvMatrixLocation >= 0)
    {
      if (es1xUniformVersions_differs(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_PMV_MATRIX))
      {
        es1xMatrix4x4 pmvMatrix;

        es1xMatrix4x4_load(&pmvMatrix, pMatrix);
        es1xMatrix4x4_multiply(&pmvMatrix, mvMatrix);
        es1xAssignMatrix4x4Uniform(info, UNIFORM_PMV_MATRIX, &pmvMatrix);
        es1xUniformVersions_update(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_PMV_MATRIX);
      }
    }

    if (mvMatrixLocation >= 0)
    {
      if (es1xUniformVersions_differs(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_MODELVIEW_MATRIX))
      {
        es1xAssignMatrix4x4Uniform(info, UNIFORM_MV_MATRIX, mvMatrix);
        es1xUniformVersions_update(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_MODELVIEW_MATRIX);
      }
    }

    if (mvMatrixInversedLocation >= 0)
    {
      if (es1xUniformVersions_differs(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_INVERSION_MODELVIEW_MATRIX))
      {
        es1xMatrix3x3 mvMatrixInversed;
        es1xMatrix4x4_invert3x3(&mvMatrixInversed, mvMatrix);

        /* \todo [090405:christian] temporarily disabled as there is an additional
           transpose somewhere or matrix is being read in wrong way */
        /* es1xMatrix3x3_transpose(&mvMatrixInversed); */

        es1xAssignMatrix3x3Uniform(info, UNIFORM_MV_MATRIX_INV, &mvMatrixInversed);

        /* Compute and assign rescale normal factor */
        if (context->rescaleNormalEnabled)
        {
          const GLfloat* src = (const GLfloat*) mvMatrix->data;

          GLfloat factor = 1.f / es1xMath_sqrtf(src[2]  * src[2] +
                                                src[6]  * src[6] +
                                                src[10] * src[10]);

          es1xAssignFloatUniform(info, UNIFORM_RESCALE_NORMAL_FACTOR, factor);
        }

        es1xUniformVersions_update(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_INVERSION_MODELVIEW_MATRIX);
      }
    }

    /* Assign texture matrices */
    if (context->texturingEnabled)
    {
      int unitNdx;

      for(unitNdx=0;unitNdx<ES1X_NUM_SUPPORTED_TEXTURE_UNITS;unitNdx++)
      {
        if (context->textureUnitEnabled[unitNdx])
        {
          /* \todo Do only once */
          es1xAssignIntegerUniform(info, UNIFORM_SAMPLER0 + unitNdx, unitNdx);

          if (es1xUniformVersions_differs(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_TEXTURE_MATRIX + unitNdx))
          {
            const es1xMatrix4x4* matrix = es1xMatrixStack_peekMatrix(context->textureMatrixStack[unitNdx]);
            es1xAssignMatrix4x4Uniform(info, UNIFORM_TEXTURE_MATRIX0 + unitNdx, matrix);
            ES1X_ASSUME_NO_ERROR;

            es1xUniformVersions_update(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_TEXTURE_MATRIX + unitNdx);
          }
        }
      }
    }
  }

  /* Assign fog uniforms */
  if (context->fogEnabled)
  {
    if (es1xUniformVersions_differs(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_FOG))
    {
      es1xAssignFloatUniform    (info, UNIFORM_FOG_DENSITY,                     context->fogDensity);
      es1xAssignFloatUniform    (info, UNIFORM_FOG_RCPENDSTARTDIFF,     1.f / (context->fogEnd - context->fogStart));
      es1xAssignFloatUniform    (info, UNIFORM_FOG_END,                         context->fogEnd);
      es1xAssignVec4Uniform     (info, UNIFORM_FOG_COLOR,                       (const GLfloat*)&context->fogColor);

      es1xUniformVersions_update(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_FOG);
    }
  }

  /* Assign light uniforms */
  if (context->lightingEnabled)
  {
    int slotIndex;

    if (es1xUniformVersions_differs(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_LIGHT_MODEL))
    {
      es1xAssignVec4Uniform(info, UNIFORM_LIGHT_MODEL_AMBIENT, (const GLfloat*)&context->lightModelAmbient);
      es1xUniformVersions_update(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_LIGHT_MODEL);
    }

    for(slotIndex=0;slotIndex<ES1X_NUM_SUPPORTED_LIGHTS;slotIndex++)
    {
      int lightIndex = table->light[slotIndex];
      ES1X_ASSERT(lightIndex >= 0 && lightIndex < ES1X_NUM_SUPPORTED_LIGHTS);

      if (context->lightEnabled[lightIndex] == GL_FALSE)
        continue;

      if (es1xUniformVersions_differs(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_LIGHT + lightIndex))
      {
        /* Generic light parameters */
        es1xAssignVec4Uniform   (info, UNIFORM_LIGHT0_AMBIENT + slotIndex,      (const GLfloat*)&context->light[lightIndex].ambient);
        es1xAssignVec4Uniform   (info, UNIFORM_LIGHT0_DIFFUSE + slotIndex,      (const GLfloat*)&context->light[lightIndex].diffuse);
        es1xAssignVec4Uniform   (info, UNIFORM_LIGHT0_SPECULAR + slotIndex,     (const GLfloat*)&context->light[lightIndex].specular);

        if (context->lightType[lightIndex] == ES1X_LIGHT_TYPE_DIRECTIONAL)
        {
          /*    Normalize eye position for directional lights in order to
                reduce the need for computations in shaders per vertex */
          es1xVec4 temp;
          es1xVec4_setVec4(&temp, &context->light[lightIndex].eyePosition);
          es1xVec4_normalize(&temp);

          es1xAssignVec4Uniform(info, UNIFORM_LIGHT0_EYEPOSITION + slotIndex, (const GLfloat*)&temp);
        }
        else
        {
          es1xAssignVec4Uniform(info, UNIFORM_LIGHT0_EYEPOSITION + slotIndex, (const GLfloat*)&context->light[lightIndex].eyePosition);
        }

        /* Omni light parameters */
        es1xAssignFloatUniform  (info, UNIFORM_LIGHT0_CONSTANT_ATTENUATION      + slotIndex,    context->light[lightIndex].constantAttenuation);
        es1xAssignFloatUniform  (info, UNIFORM_LIGHT0_LINEAR_ATTENUATION        + slotIndex,    context->light[lightIndex].linearAttenuation);
        es1xAssignFloatUniform  (info, UNIFORM_LIGHT0_QUADRATIC_ATTENUATION + slotIndex,        context->light[lightIndex].quadraticAttenuation);

        /* Spot light parameters */
        es1xAssignFloatUniform  (info, UNIFORM_LIGHT0_SPOT_EXPONENT                     + slotIndex,    context->light[lightIndex].spotExponent);
        es1xAssignFloatUniform  (info, UNIFORM_LIGHT0_COS_SPOT_CUTOFF           + slotIndex,    es1xMath_cosf(es1xMath_radians(context->light[lightIndex].spotCutoff)));

        /*      Normalize eye direction for spot lights so that it
                is not needed to be done in shaders per vertex */
        {
          es1xVec3 temp;
          es1xVec3_setVec3(&temp, &context->light[lightIndex].spotEyeDirection);
          es1xVec3_normalize(&temp);

          es1xAssignVec3Uniform(info, UNIFORM_LIGHT0_SPOT_EYEDIRECTION + slotIndex, (const GLfloat*)&temp);
        }

        es1xUniformVersions_update(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_LIGHT + lightIndex);
      }
    }

    /* Material uniforms */
    {
      if (es1xUniformVersions_differs(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_MATERIALS))
      {
        es1xAssignVec4Uniform   (info, UNIFORM_MATERIAL_AMBIENT,                        (const GLfloat*)&context->materialAmbient);
        es1xAssignVec4Uniform   (info, UNIFORM_MATERIAL_DIFFUSE,                        (const GLfloat*)&context->materialDiffuse);
        es1xAssignVec4Uniform   (info, UNIFORM_MATERIAL_SPECULAR,                       (const GLfloat*)&context->materialSpecular);
        es1xAssignVec4Uniform   (info, UNIFORM_MATERIAL_EMISSIVE,                       (const GLfloat*)&context->materialEmission);
        es1xAssignFloatUniform  (info, UNIFORM_MATERIAL_SPECULAR_EXPONENT,      context->materialShininess);

        es1xUniformVersions_update(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_MATERIALS);
      }
    }
  }

  /* Assign texturing uniforms */
  if (es1xUniformVersions_differs(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_TEXTURE_ENV))
  {
    int unitNdx;

    for(unitNdx=0;unitNdx<ES1X_NUM_SUPPORTED_TEXTURE_UNITS;unitNdx++)
      if (context->textureUnitEnabled[unitNdx])
        es1xAssignVec4Uniform(info, UNIFORM_TEXTURE_ENV_COLOR0 + unitNdx, (const GLfloat*)&context->texEnv[unitNdx].color);

    es1xUniformVersions_update(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_TEXTURE_ENV);
    /* \todo [090326:christian] Missing alphaScale and rgbScale */
  }

  /* Assign vertex parameters */
  if (es1xUniformVersions_differs(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_VERTEX_PARAMETER))
  {
    es1xAssignFloatUniform      (info, UNIFORM_CURRENT_VERTEX_POINT_SIZE,       context->pointSize);
    es1xAssignVec4Uniform       (info, UNIFORM_CURRENT_VERTEX_COLOR,            (const GLfloat*)&context->currentColor);
    es1xAssignVec3Uniform       (info, UNIFORM_CURRENT_VERTEX_NORMAL,           (const GLfloat*)&context->currentNormal);

    es1xUniformVersions_update(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_VERTEX_PARAMETER);
  }

  /* Assign alpha test parameters */
  if (es1xUniformVersions_differs(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_ALPHA_TEST))
  {
    es1xAssignFloatUniform      (info, UNIFORM_ALPHA_TEST_REFERENCE_VALUE, context->alphaTestRef);
    es1xUniformVersions_update(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_ALPHA_TEST);
  }

  /* Assign point parameters */
  if (es1xUniformVersions_differs(&context->uniformVersions, &info->uniformVersions, UNIFORM_GROUP_POINT_SIZE))
  {
    es1xAssignFloatUniform      (info, UNIFORM_MIN_DERIVED_POINT_WIDTH,                 context->pointSizeMin);
    es1xAssignFloatUniform      (info, UNIFORM_MAX_DERIVED_POINT_WIDTH,                 context->pointSizeMin);
    es1xAssignVec3Uniform       (info, UNIFORM_POINT_SIZE_DISTANCE_ATTENUATION, (const GLfloat*)&context->pointDistanceAttenuation);
  }
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE GLboolean es1xIsFunctionalContext(es1xShaderContext* context)
{
  if (context->vertexArrayEnabled == GL_FALSE)
    return GL_FALSE;

  return GL_TRUE;
}

/*---------------------------------------------------------------------------*/

static GLboolean es1xPrepareShaders(void *_context_, GLenum primitive)
{
  /*    Check if there has been changes in the context that would require shaders to be changed.
        If no changes were detected, previous shaders will do. */
  es1xContext *context = (es1xContext *) _context_;

  if (context->dirty)
  {
    /* Allocate shader context structures for temporary usage. */
    es1xShaderContext*          shaderContext           = es1xShaderCache_allocateContext(context->shaderCache);
    es1xShaderProgramInfo*      info                    = 0;
    GLboolean                   isFunctionalContext     = GL_FALSE;

    ES1X_INCREASE_STATISTICS(_context_, STATISTIC_NUM_RENDER_CALLS_WITH_DIRTY_CONTEXT);

    /* Construct shader context and corresponding key */
    es1xIndexMapTable_init(&context->currentMapTable, context);
    es1xShaderContext_init(shaderContext, context, primitive, &context->currentMapTable);
    es1xShaderContext_optimize(shaderContext);
    isFunctionalContext = es1xIsFunctionalContext(shaderContext);

    if (isFunctionalContext)
    {
      /*        Try to allocate context key. If allocation fails, we need to release
                one of the programs in the cache */

      es1xShaderContextKey* key = es1xShaderCache_allocateContextKey(context->shaderCache);

      if (key == 0)
      {
        key = es1xShaderCache_unregisterMostRarelyUsedProgram(context->shaderCache, &info);

        ES1X_ASSERT(key);
        ES1X_ASSERT(info);

        /* Release program */
        gl2b->glDeleteProgram(info->program);
        ES1X_ASSUME_NO_ERROR;

        /* Nullify last used program if it is the released one */
        if (context->lastUsedShaderProgramInfo == info)
          context->lastUsedShaderProgramInfo = 0;

        es1xShaderCache_releaseProgramInfo(context->shaderCache, info);
      }

      es1xShaderContext_constructKey(shaderContext, key);

      /* Check if cache already contains compiled shader programs with this key */
      info = es1xShaderCache_useProgram(context->shaderCache, key);

      if (info == 0)
      {
        /* Generate GLSL shaders */
        info = es1xShaderCache_allocateProgramInfo(context->shaderCache);
        ES1X_DEBUG_CODE(es1xShaderContext_dump(shaderContext));
        es1xShaderProgram_create(shaderContext, info, &context->currentMapTable);
        es1xShaderCache_registerProgram(context->shaderCache, key, info);
        ES1X_INCREASE_STATISTICS(_context_, STATISTIC_NUM_CREATED_PROGRAMS);
      }
      else
      {
        /* Not needed anymore */
        es1xShaderCache_releaseContextKey(context->shaderCache, key);
      }

      /* Change program only if it not already is in use */
      if (context->lastUsedShaderProgramInfo != info)
      {
        /* Use program */
        gl2b->glUseProgram(info->program);
        ES1X_ASSUME_NO_ERROR;
        context->lastUsedShaderProgramInfo = info;
        es1xUniformVersions_invalidateAll(&info->uniformVersions);
        ES1X_INCREASE_STATISTICS(_context_, STATISTIC_NUM_CHANGED_PROGRAMS);
      }

      /* Clean up */
      es1xShaderCache_releaseContext(context->shaderCache, shaderContext);
      context->dirty = GL_FALSE;
    }
    else
    {
      es1xShaderCache_releaseContext(context->shaderCache, shaderContext);
      return GL_FALSE;
    }
  }

  /* Assign required uniforms */
  ES1X_ASSERT(context->lastUsedShaderProgramInfo != 0);
  es1xAssignUniforms(_context_, context->lastUsedShaderProgramInfo, &context->currentMapTable);

  return GL_TRUE;
}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xDrawArrays(void *_context_, GLenum mode, GLint first, GLsizei count)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glDrawArrays(%s, %d, %d)\n", ES1X_ENUM_TO_STRING(mode), first, count));
  es1xStatistics_dump(_context_);
  ES1X_CHECK_CONTEXT(context);
  ES1X_INCREASE_STATISTICS(_context_, STATISTIC_NUM_RENDER_CALLS);

  if (es1xPrepareShaders(context, mode))
    gl2b->glDrawArrays(mode, first, count);
  else
  {
    ES1X_INCREASE_STATISTICS(_context_, STATISTIC_NUM_INVALIDATED_RENDER_CALLS);
  }

}

/*---------------------------------------------------------------------------*/

GL_API void GL_APIENTRY es1xDrawElements(void *_context_, GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
  es1xContext *context = (es1xContext *) _context_;
  ES1X_LOG_CALL(("glDrawElements(mode %s, count %d, type %d, indixes %p)\n", \
                 ES1X_ENUM_TO_STRING(mode), type, count, (void *) indices));
  es1xStatistics_dump(_context_);
  ES1X_CHECK_CONTEXT(context);
  ES1X_INCREASE_STATISTICS(_context_, STATISTIC_NUM_RENDER_CALLS);

  if (es1xPrepareShaders(context, mode)) {
    gl2b->glDrawElements(mode, count, type, indices);
    ES1X_LOG_CALL(("\n\n______es1x->gl2DrawElements(%s, %d, %d, 0x%p)\n\n", ES1X_ENUM_TO_STRING(mode), count, type, indices));
  }
  else
  {
    ES1X_INCREASE_STATISTICS(_context_, STATISTIC_NUM_INVALIDATED_RENDER_CALLS);
  }
}

/*---------------------------------------------------------------------------*/
