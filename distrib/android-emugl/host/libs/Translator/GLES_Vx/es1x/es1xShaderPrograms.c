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

 
#include "es1xDebug.h"
#include "es1xShaderPrograms.h"
#include "es1xMemory.h"
#include "es1xShaderCodeUniforms.h"
#include "es1xShaderCodeEntries.inl"
#include "es1xError.h"
#include "es1xEnumerations.h"
#include "es1xRouting.h"

#define ES1X_NUM_MAX_SHADER_PROGRAM_ATTRIBUTES                                  64
#define ES1X_NUM_MAX_SHADER_PROGRAM_ROUTINE_ENTRIES                             64
#define ES1X_NUM_MAX_SHADER_PROGRAM_FUNCTIONS                                   8

typedef struct
{
  const char*           declarations[ES1X_NUM_MAX_SHADER_PROGRAM_ATTRIBUTES];
  const char*           routines[ES1X_NUM_MAX_SHADER_PROGRAM_FUNCTIONS][ES1X_NUM_MAX_SHADER_PROGRAM_ROUTINE_ENTRIES];

  int                   numDeclarations;
  int                   numRoutines[ES1X_NUM_MAX_SHADER_PROGRAM_FUNCTIONS];

  int                   numUsedFunctions;

  GLboolean             uniformRequirements[NUM_SHADER_CODE_UNIFORMS];
} es1xShaderProgramCode;

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xShaderProgramCode_init(es1xShaderProgramCode* code)
{
  ES1X_ASSERT(code);

  code->numUsedFunctions        = 0;
  code->numDeclarations = 0;

  es1xMemZero(code->numRoutines,                        sizeof(int)                     * ES1X_NUM_MAX_SHADER_PROGRAM_FUNCTIONS);
  es1xMemZero(code->uniformRequirements,        sizeof(GLboolean)       * NUM_SHADER_CODE_UNIFORMS);
}

ES1X_INLINE int es1xShaderProgramCode_allocateFunction(es1xShaderProgramCode* code)
{
  return code->numUsedFunctions++;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xShaderProgramCode_pushDeclarationInternal(es1xShaderProgramCode* code, const char* declaration)
{
  ES1X_ASSERT(code->numDeclarations < DE_LENGTH_OF_ARRAY(code->declarations));
  code->declarations[code->numDeclarations++] = declaration;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xShaderProgramCode_pushVarying(es1xShaderProgramCode* code, const char* declaration)
{
  es1xShaderProgramCode_pushDeclarationInternal(code, declaration);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xShaderProgramCode_pushAttribute(es1xShaderProgramCode* code, const char* declaration)
{
  es1xShaderProgramCode_pushDeclarationInternal(code, declaration);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xShaderProgramCode_pushStructure(es1xShaderProgramCode* code, const char* declaration)
{
  es1xShaderProgramCode_pushDeclarationInternal(code, declaration);
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xShaderProgramCode_pushUniform(es1xShaderProgramCode* code, const char* declaration, int uniformID)
{
  es1xShaderProgramCode_pushDeclarationInternal(code, declaration);
  code->uniformRequirements[uniformID] = GL_TRUE;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE void es1xShaderProgramCode_pushRoutine(es1xShaderProgramCode* code, int functionIndex, const char* routine)
{
  ES1X_ASSERT(functionIndex >= 0 && functionIndex < DE_LENGTH_OF_ARRAY(code->routines));
  ES1X_ASSERT(code->numRoutines[functionIndex] < DE_LENGTH_OF_ARRAY(code->routines[functionIndex]));
  code->routines[functionIndex][code->numRoutines[functionIndex]++] = routine;
}

/*---------------------------------------------------------------------------*/

ES1X_INLINE int es1xShaderProgramCode_concatenate(es1xShaderProgramCode* code, const char** buffer, int bufferSize)
{
  int functionNdx;
  int size = 0;
  int declarationNdx;

  ES1X_UNREF(bufferSize);

  for(declarationNdx=0;declarationNdx<code->numDeclarations;declarationNdx++)
    buffer[size++] = code->declarations[declarationNdx];

  for(functionNdx=code->numUsedFunctions-1;functionNdx>=0;functionNdx--)
  {
    int lineNdx = 0;
    for(lineNdx=0;lineNdx<code->numRoutines[functionNdx];lineNdx++)
      buffer[size++] = code->routines[functionNdx][lineNdx];
  }

  ES1X_ASSERT(size <= bufferSize);

  return size;
}

/*---------------------------------------------------------------------------*/

static void es1xPushCombineAlphaSource(es1xShaderProgramCode* program, int functionIndex, GLenum* sources, GLenum* operands, int unitNdx)
{
  int operand;
  int src;

  ES1X_ASSERT(sources);
  ES1X_ASSERT(operands);
  ES1X_ASSERT(program);
  ES1X_ASSERT(unitNdx >= 0 && unitNdx < ES1X_NUM_SUPPORTED_TEXTURE_UNITS);

  src                   = sources[unitNdx];
  operand               = operands[unitNdx];

  switch(src)
  {
    case ES1X_TEXENVSRC_TEXTURE:
      {
        switch(operand)
        {
          case ES1X_TEXENVOPERANDALPHA_SRC_ALPHA:                               es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_ALPHA_TEXTURE_SRC_ALPHA);                        return;
          case ES1X_TEXENVOPERANDALPHA_ONE_MINUS_SRC_ALPHA:     es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_ALPHA_TEXTURE_ONE_MINUS_SRC_ALPHA);      return;
          default:
            ES1X_ASSERT(!"Unknown operand");
            break;
        }
        break;
      }
    case ES1X_TEXENVSRC_CONSTANT:
      {
        switch(operand)
        {
          case ES1X_TEXENVOPERANDALPHA_SRC_ALPHA:
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_ALPHA_CONSTANT_SRC_ALPHA_BEGIN);
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_TEXTURE_ENV_COLOR[unitNdx]);
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_ALPHA_CONSTANT_SRC_ALPHA_END);
            return;

          case ES1X_TEXENVOPERANDALPHA_ONE_MINUS_SRC_ALPHA:
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_ALPHA_CONSTANT_ONE_MINUS_SRC_ALPHA_BEGIN);
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_TEXTURE_ENV_COLOR[unitNdx]);
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_ALPHA_CONSTANT_ONE_MINUS_SRC_ALPHA_END);
            return;

          default:
            ES1X_ASSERT(!"Unknown operand");
            break;
        }
        break;
      }
    case ES1X_TEXENVSRC_PRIMARY_COLOR:
      {
        switch(operand)
        {
          case ES1X_TEXENVOPERANDALPHA_SRC_ALPHA:                               es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_ALPHA_PRIMARY_SRC_ALPHA);                        return;
          case ES1X_TEXENVOPERANDALPHA_ONE_MINUS_SRC_ALPHA:     es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_ALPHA_PRIMARY_ONE_MINUS_SRC_ALPHA);      return;
          default:
            ES1X_ASSERT(!"Unknown operand");
            break;
        }
        break;
      }
    case ES1X_TEXENVSRC_PREVIOUS:
      {
        switch(operand)
        {
          case ES1X_TEXENVOPERANDALPHA_SRC_ALPHA:                               es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_ALPHA_PREVIOUS_SRC_ALPHA);                       return;
          case ES1X_TEXENVOPERANDALPHA_ONE_MINUS_SRC_ALPHA:     es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_ALPHA_PREVIOUS_ONE_MINUS_SRC_ALPHA);     return;
          default:
            ES1X_ASSERT(!"Unknown operand");
            break;
        }
        break;
      }
    default:
      ES1X_ASSERT(!"Unknown source");
      break;
  }
}

static void es1xPushCombineRGBSource(es1xShaderProgramCode* program, int functionIndex, GLenum* sources, GLenum* operands, int unitNdx)
{
  int operand;
  int src;

  ES1X_ASSERT(sources);
  ES1X_ASSERT(operands);
  ES1X_ASSERT(program);
  ES1X_ASSERT(unitNdx >= 0 && unitNdx < ES1X_NUM_SUPPORTED_TEXTURE_UNITS);

  src                   = sources[unitNdx];
  operand               = operands[unitNdx];

  switch(src)
  {
    case ES1X_TEXENVSRC_TEXTURE:
      {
        switch(operand)
        {
          case ES1X_TEXENVOPERANDRGB_SRC_COLOR:                         es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_TEXTURE_SRC_COLOR);                          return;
          case ES1X_TEXENVOPERANDRGB_ONE_MINUS_SRC_COLOR:               es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_TEXTURE_ONE_MINUS_SRC_COLOR);        return;
          case ES1X_TEXENVOPERANDRGB_SRC_ALPHA:                         es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_TEXTURE_SRC_ALPHA);                          return;
          case ES1X_TEXENVOPERANDRGB_ONE_MINUS_SRC_ALPHA:               es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_TEXTURE_ONE_MINUS_SRC_ALPHA);        return;
          default:
            ES1X_ASSERT(!"Unknown operand");
            break;
        }
        break;
      }
    case ES1X_TEXENVSRC_CONSTANT:
      {
        switch(operand)
        {
          case ES1X_TEXENVOPERANDRGB_SRC_COLOR:
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_CONSTANT_SRC_COLOR_BEGIN);
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_TEXTURE_ENV_COLOR[unitNdx]);
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_CONSTANT_SRC_COLOR_END);
            return;
          case ES1X_TEXENVOPERANDRGB_ONE_MINUS_SRC_COLOR:
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_CONSTANT_ONE_MINUS_SRC_COLOR_BEGIN);
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_TEXTURE_ENV_COLOR[unitNdx]);
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_CONSTANT_ONE_MINUS_SRC_COLOR_END);
            return;
          case ES1X_TEXENVOPERANDRGB_SRC_ALPHA:
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_CONSTANT_SRC_ALPHA_BEGIN);
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_TEXTURE_ENV_COLOR[unitNdx]);
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_CONSTANT_SRC_ALPHA_END);
            return;
          case ES1X_TEXENVOPERANDRGB_ONE_MINUS_SRC_ALPHA:
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_CONSTANT_ONE_MINUS_SRC_ALPHA_BEGIN);
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_TEXTURE_ENV_COLOR[unitNdx]);
            es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_CONSTANT_ONE_MINUS_SRC_ALPHA_END);
            return;
          default:
            ES1X_ASSERT(!"Unknown operand");
            break;
        }
      }
    case ES1X_TEXENVSRC_PRIMARY_COLOR:
      {
        switch(operand)
        {
          case ES1X_TEXENVOPERANDRGB_SRC_COLOR:                         es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_PRIMARY_SRC_COLOR);                          return;
          case ES1X_TEXENVOPERANDRGB_ONE_MINUS_SRC_COLOR:               es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_PRIMARY_ONE_MINUS_SRC_COLOR);        return;
          case ES1X_TEXENVOPERANDRGB_SRC_ALPHA:                         es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_PRIMARY_SRC_ALPHA);                          return;
          case ES1X_TEXENVOPERANDRGB_ONE_MINUS_SRC_ALPHA:               es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_PRIMARY_ONE_MINUS_SRC_ALPHA);        return;
          default:
            ES1X_ASSERT(!"Unknown operand");
            break;
        }
        break;
      }
    case ES1X_TEXENVSRC_PREVIOUS:
      {
        switch(operand)
        {
          case ES1X_TEXENVOPERANDRGB_SRC_COLOR:                         es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_PREVIOUS_SRC_COLOR);                         return;
          case ES1X_TEXENVOPERANDRGB_ONE_MINUS_SRC_COLOR:               es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_PREVIOUS_ONE_MINUS_SRC_COLOR);       return;
          case ES1X_TEXENVOPERANDRGB_SRC_ALPHA:                         es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_PREVIOUS_SRC_ALPHA);                         return;
          case ES1X_TEXENVOPERANDRGB_ONE_MINUS_SRC_ALPHA:               es1xShaderProgramCode_pushRoutine(program, functionIndex, CODE_FRAGMENT_SHADER_COMBINE_RGB_PREVIOUS_ONE_MINUS_SRC_ALPHA);       return;
          default:
            ES1X_ASSERT(!"Unknown operand");
            break;
        }
        break;
      }
    default:
      ES1X_ASSERT(!"Unknown source");
      break;
  }
}

/*---------------------------------------------------------------------------*/

static void es1xConstructFragmentShader(es1xShaderProgramCode* shader, es1xShaderContext* context)
{
  ES1X_ASSERT(shader);
  ES1X_ASSERT(context);

  es1xShaderProgramCode_init(shader);

  /* Construct shader */
  {
    int mainFunction = es1xShaderProgramCode_allocateFunction(shader);

    es1xShaderProgramCode_pushVarying(shader, DECLARE_VARYING_VERTEX_COLOR);
    es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_BEGIN);

    if (context->texturingEnabled)
    {
      int unitNdx;
      GLboolean combineArgumentsDeclared                                                                = GL_FALSE;
      GLboolean texEnvColorDeclared[ES1X_NUM_SUPPORTED_TEXTURE_UNITS]   = { 0 };

      es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_DECLARE_TEXEL);
      es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_DECLARE_TEXTURE_FUNC_RESULT);

      for(unitNdx=0;unitNdx<ES1X_NUM_SUPPORTED_TEXTURE_UNITS;unitNdx++)
      {
        /* Can be optimize to break after texture units have been sorted */
        if (context->textureUnitEnabled[unitNdx] == GL_FALSE)
          continue;

        /* Comment switch for disabling texture lookups (texture2D(st) in fragment shader */
        /**/
        es1xShaderProgramCode_pushVarying       (shader, DECLARE_VARYING_TEXTURE_COORDINATES[unitNdx]);
        es1xShaderProgramCode_pushUniform       (shader, DECLARE_UNIFORM_SAMPLER[unitNdx], UNIFORM_SAMPLER0 + unitNdx);
        es1xShaderProgramCode_pushRoutine       (shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_LOOKUP[unitNdx]);
        /*
          es1xShaderProgramCode_pushRoutine     (shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_LOOKUP_PLACEHOLDER);
        */

        switch(context->texEnvMode[unitNdx])
        {
          case ES1X_TEXENVMODE_REPLACE:
            {
              switch(context->textureFormat[unitNdx])
              {
                case ES1X_TEXTUREFORMAT_ALPHA:                          es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_REPLACE_ALPHA);                       break;
                case ES1X_TEXTUREFORMAT_LUMINANCE:                      es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_REPLACE_LUMINANCE);           break;
                case ES1X_TEXTUREFORMAT_LUMINANCE_ALPHA:        es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_REPLACE_LUMINANCE_ALPHA);     break;
                case ES1X_TEXTUREFORMAT_RGB:                            es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_REPLACE_RGB);                         break;
                case ES1X_TEXTUREFORMAT_RGBA:                           es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_REPLACE_RGBA);                        break;
                default:
                  ES1X_ASSERT(0);
                  break;
              }
              break;
            }
          case ES1X_TEXENVMODE_MODULATE:
            {
              switch(context->textureFormat[unitNdx])
              {
                case ES1X_TEXTUREFORMAT_ALPHA:                          es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_MODULATE_ALPHA);                      break;
                case ES1X_TEXTUREFORMAT_LUMINANCE:                      es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_MODULATE_LUMINANCE);          break;
                case ES1X_TEXTUREFORMAT_LUMINANCE_ALPHA:        es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_MODULATE_LUMINANCE_ALPHA);break;
                case ES1X_TEXTUREFORMAT_RGB:                            es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_MODULATE_RGB);                        break;
                case ES1X_TEXTUREFORMAT_RGBA:                           es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_MODULATE_RGBA);                       break;
                default:
                  ES1X_ASSERT(0);
                  break;
              }
              break;
            }
          case ES1X_TEXENVMODE_DECAL:
            {
              switch(context->textureFormat[unitNdx])
              {
                case ES1X_TEXTUREFORMAT_RGB:                            es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_DECAL_RGB);                           break;
                case ES1X_TEXTUREFORMAT_RGBA:                           es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_DECAL_RGBA);                          break;
                default:
                  ES1X_ASSERT(0);
                  break;
              }
              break;
            }
          case ES1X_TEXENVMODE_BLEND:
            {
              if (texEnvColorDeclared[unitNdx] == GL_FALSE)
              {
                es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_TEXTURE_ENV_COLOR[unitNdx], UNIFORM_TEXTURE_ENV_COLOR0 + unitNdx);
                texEnvColorDeclared[unitNdx] = GL_TRUE;
              }

              switch(context->textureFormat[unitNdx])
              {
                case ES1X_TEXTUREFORMAT_ALPHA:                          es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_BLEND_ALPHA);                         break;
                case ES1X_TEXTUREFORMAT_LUMINANCE:                      es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_BLEND_LUMINANCE);                     break;
                case ES1X_TEXTUREFORMAT_LUMINANCE_ALPHA:        es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_BLEND_LUMINANCE_ALPHA);       break;
                case ES1X_TEXTUREFORMAT_RGB:                            es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_BLEND_RGB);                           break;
                case ES1X_TEXTUREFORMAT_RGBA:                           es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_BLEND_RGBA);                          break;
                default:
                  ES1X_ASSERT(0);
                  break;
              }
              break;
            }
          case ES1X_TEXENVMODE_ADD:
            {
              switch(context->textureFormat[unitNdx])
              {
                case ES1X_TEXTUREFORMAT_ALPHA:                          es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_ADD_ALPHA);                           break;
                case ES1X_TEXTUREFORMAT_LUMINANCE:                      es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_ADD_LUMINANCE);                       break;
                case ES1X_TEXTUREFORMAT_LUMINANCE_ALPHA:        es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_ADD_LUMINANCE_ALPHA);         break;
                case ES1X_TEXTUREFORMAT_RGB:                            es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_ADD_RGB);                                     break;
                case ES1X_TEXTUREFORMAT_RGBA:                           es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_ADD_RGBA);                            break;
                default:
                  ES1X_ASSERT(0);
                  break;
              }
              break;
            }
          case ES1X_TEXENVMODE_COMBINE:
            {
              if (combineArgumentsDeclared == GL_FALSE)
              {
                es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_DECLARE_COMBINE_VARIABLES);
                es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_DECLARE_RGB_ARG0);
                es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_DECLARE_RGB_ARG1);
                es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_DECLARE_RGB_ARG2);
                es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_DECLARE_ALPHA_ARG0);
                es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_DECLARE_ALPHA_ARG1);
                es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_DECLARE_ALPHA_ARG2);

                combineArgumentsDeclared = GL_TRUE;
              }

              if (texEnvColorDeclared[unitNdx] == GL_FALSE)
              {
                es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_TEXTURE_ENV_COLOR[unitNdx], UNIFORM_TEXTURE_ENV_COLOR0 + unitNdx);
                texEnvColorDeclared[unitNdx] = GL_TRUE;
              }


              switch(context->texEnvCombineRGB[unitNdx])
              {
                case ES1X_TEXENVCOMBINERGB_REPLACE:
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG0_RGB);
                  es1xPushCombineRGBSource(shader, mainFunction, (GLenum*)context->texEnvSrc0RGB, (GLenum*)context->texEnvOperand0RGB, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_COMBINE_RGB_REPLACE);
                  break;

                case ES1X_TEXENVCOMBINERGB_MODULATE:
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG0_RGB);
                  es1xPushCombineRGBSource(shader, mainFunction, (GLenum*)context->texEnvSrc0RGB, (GLenum*)context->texEnvOperand0RGB, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG1_RGB);
                  es1xPushCombineRGBSource(shader, mainFunction, (GLenum*)context->texEnvSrc1RGB, (GLenum*)context->texEnvOperand1RGB, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_COMBINE_RGB_MODULATE);
                  break;

                case ES1X_TEXENVCOMBINERGB_ADD:
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG0_RGB);
                  es1xPushCombineRGBSource(shader, mainFunction, (GLenum*)context->texEnvSrc0RGB, (GLenum*)context->texEnvOperand0RGB, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG1_RGB);
                  es1xPushCombineRGBSource(shader, mainFunction, (GLenum*)context->texEnvSrc1RGB, (GLenum*)context->texEnvOperand1RGB, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_COMBINE_RGB_ADD);
                  break;

                case ES1X_TEXENVCOMBINERGB_ADD_SIGNED:
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG0_RGB);
                  es1xPushCombineRGBSource(shader, mainFunction, (GLenum*)context->texEnvSrc0RGB, (GLenum*)context->texEnvOperand0RGB, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG1_RGB);
                  es1xPushCombineRGBSource(shader, mainFunction, (GLenum*)context->texEnvSrc1RGB, (GLenum*)context->texEnvOperand1RGB, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_COMBINE_RGB_ADD_SIGNED);
                  break;

                case ES1X_TEXENVCOMBINERGB_INTERPOLATE:
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG0_RGB);
                  es1xPushCombineRGBSource(shader, mainFunction, (GLenum*)context->texEnvSrc0RGB, (GLenum*)context->texEnvOperand0RGB, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG1_RGB);
                  es1xPushCombineRGBSource(shader, mainFunction, (GLenum*)context->texEnvSrc1RGB, (GLenum*)context->texEnvOperand1RGB, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG2_RGB);
                  es1xPushCombineRGBSource(shader, mainFunction, (GLenum*)context->texEnvSrc2RGB, (GLenum*)context->texEnvOperand2RGB, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_COMBINE_RGB_INTERPOLATE);
                  break;

                case ES1X_TEXENVCOMBINERGB_SUBTRACT:
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG0_RGB);
                  es1xPushCombineRGBSource(shader, mainFunction, (GLenum*)context->texEnvSrc0RGB, (GLenum*)context->texEnvOperand0RGB, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG1_RGB);
                  es1xPushCombineRGBSource(shader, mainFunction, (GLenum*)context->texEnvSrc1RGB, (GLenum*)context->texEnvOperand1RGB, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_COMBINE_RGB_SUBTRACT);
                  break;

                case ES1X_TEXENVCOMBINERGB_DOT3_RGB:
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG0_RGB);
                  es1xPushCombineRGBSource(shader, mainFunction, (GLenum*)context->texEnvSrc0RGB, (GLenum*)context->texEnvOperand0RGB, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG1_RGB);
                  es1xPushCombineRGBSource(shader, mainFunction, (GLenum*)context->texEnvSrc1RGB, (GLenum*)context->texEnvOperand1RGB, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_COMBINE_RGB_DOT3_RGB);
                  break;

                case ES1X_TEXENVCOMBINERGB_DOT3_RGBA:
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG0_RGB);
                  es1xPushCombineRGBSource(shader, mainFunction, (GLenum*)context->texEnvSrc0RGB, (GLenum*)context->texEnvOperand0RGB, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG1_RGB);
                  es1xPushCombineRGBSource(shader, mainFunction, (GLenum*)context->texEnvSrc1RGB, (GLenum*)context->texEnvOperand1RGB, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_COMBINE_RGB_DOT3_RGBA);
                  break;

                default:
                  ES1X_ASSERT(!"Internal error");
                  break;
              }

              switch(context->texEnvCombineAlpha[unitNdx])
              {
                case ES1X_TEXENVCOMBINEALPHA_REPLACE:
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG0_ALPHA);
                  es1xPushCombineAlphaSource(shader, mainFunction, (GLenum*)context->texEnvSrc0Alpha, (GLenum*)context->texEnvOperand0Alpha, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_COMBINE_ALPHA_REPLACE);
                  break;

                case ES1X_TEXENVCOMBINEALPHA_MODULATE:
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG0_ALPHA);
                  es1xPushCombineAlphaSource(shader, mainFunction, (GLenum*)context->texEnvSrc0Alpha, (GLenum*)context->texEnvOperand0Alpha, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG1_ALPHA);
                  es1xPushCombineAlphaSource(shader, mainFunction, (GLenum*)context->texEnvSrc1Alpha, (GLenum*)context->texEnvOperand1Alpha, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_COMBINE_ALPHA_MODULATE);
                  break;

                case ES1X_TEXENVCOMBINEALPHA_ADD:
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG0_ALPHA);
                  es1xPushCombineAlphaSource(shader, mainFunction, (GLenum*)context->texEnvSrc0Alpha, (GLenum*)context->texEnvOperand0Alpha, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG1_ALPHA);
                  es1xPushCombineAlphaSource(shader, mainFunction, (GLenum*)context->texEnvSrc1Alpha, (GLenum*)context->texEnvOperand1Alpha, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_COMBINE_ALPHA_ADD);
                  break;

                case ES1X_TEXENVCOMBINEALPHA_ADD_SIGNED:
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG0_ALPHA);
                  es1xPushCombineAlphaSource(shader, mainFunction, (GLenum*)context->texEnvSrc0Alpha, (GLenum*)context->texEnvOperand0Alpha, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG1_ALPHA);
                  es1xPushCombineAlphaSource(shader, mainFunction, (GLenum*)context->texEnvSrc1Alpha, (GLenum*)context->texEnvOperand1Alpha, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_COMBINE_ALPHA_ADD_SIGNED);
                  break;

                case ES1X_TEXENVCOMBINEALPHA_INTERPOLATE:
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG0_ALPHA);
                  es1xPushCombineAlphaSource(shader, mainFunction, (GLenum*)context->texEnvSrc0Alpha, (GLenum*)context->texEnvOperand0Alpha, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG1_ALPHA);
                  es1xPushCombineAlphaSource(shader, mainFunction, (GLenum*)context->texEnvSrc1Alpha, (GLenum*)context->texEnvOperand1Alpha, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG2_ALPHA);
                  es1xPushCombineAlphaSource(shader, mainFunction, (GLenum*)context->texEnvSrc2Alpha, (GLenum*)context->texEnvOperand2Alpha, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_COMBINE_ALPHA_INTERPOLATE);
                  break;

                case ES1X_TEXENVCOMBINEALPHA_SUBTRACT:
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG0_ALPHA);
                  es1xPushCombineAlphaSource(shader, mainFunction, (GLenum*)context->texEnvSrc0Alpha, (GLenum*)context->texEnvOperand0Alpha, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_ARG1_ALPHA);
                  es1xPushCombineAlphaSource(shader, mainFunction, (GLenum*)context->texEnvSrc1Alpha, (GLenum*)context->texEnvOperand1Alpha, unitNdx);
                  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_COMBINE_ALPHA_SUBTRACT);
                  break;

                default:
                  ES1X_ASSERT(!"Internal error");
                  break;
              }


              es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_TEXTURE_FUNC_COMBINE_ASSIGN_RESULT);
              break;
            }
        }
      }

      es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_MULTIPLY_TEXTURE_LOOKUP_RESULT);
    }

    /* Alpha test */
    if (context->alphaTestEnabled)
    {
      es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ALPHA_TEST_BEGIN);
      es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_ALPHA_TEST_REFERENCE_VALUE, UNIFORM_ALPHA_TEST_REFERENCE_VALUE);

      switch(context->alphaTestFunc)
      {
        case ES1X_COMPAREFUNC_LESS:                     es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ALPHA_TEST_LESS);          break;
        case ES1X_COMPAREFUNC_LEQUAL:           es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ALPHA_TEST_LEQUAL);        break;
        case ES1X_COMPAREFUNC_EQUAL:            es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ALPHA_TEST_EQUAL);         break;
        case ES1X_COMPAREFUNC_GREATER:          es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ALPHA_TEST_GREATER);       break;
        case ES1X_COMPAREFUNC_GEQUAL:           es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ALPHA_TEST_GEQUAL);        break;
        case ES1X_COMPAREFUNC_NOTEQUAL:         es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ALPHA_TEST_NOTEQUAL);      break;
        case ES1X_COMPAREFUNC_NEVER:
        case ES1X_COMPAREFUNC_ALWAYS:
        default:
          /*    These are the cases that should not be handled here. GL_ALWAYS does actually nothing,
                and GL_NEVER basically makes the whole pipe useless. Both cases will eventually lead
                alphaTestEnabled bit to be unset. */
          ES1X_ASSERT(!"Not allowed");
          break;
      }

      es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ALPHA_TEST_DISCARD);
    }

    if (context->fogEnabled)
    {
      es1xShaderProgramCode_pushVarying         (shader, DECLARE_VARYING_FOG_INTENSITY);
      es1xShaderProgramCode_pushUniform         (shader, DECLARE_UNIFORM_FOG_COLOR, UNIFORM_FOG_COLOR);
      es1xShaderProgramCode_pushRoutine         (shader, mainFunction, CODE_FRAGMENT_SHADER_COMPUTE_FOG);
    }

    es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_ASSIGN_COLOR);
    es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_FRAGMENT_SHADER_END);
  }
}

/*---------------------------------------------------------------------------*/

void es1xConstructVertexShader(es1xShaderProgramCode* shader, const es1xShaderContext* context, const es1xIndexMapTable* mapTable)
{
  ES1X_ASSERT(shader);
  ES1X_ASSERT(context);
  ES1X_ASSERT(mapTable);

  es1xShaderProgramCode_init(shader);

  /* Construct shader */
  {
    int mainFunction = es1xShaderProgramCode_allocateFunction(shader);

    GLboolean eyePositionComputed       = GL_FALSE;
    GLboolean mvMatrixDeclared          = GL_FALSE;

    es1xShaderProgramCode_pushRoutine   (shader, mainFunction, CODE_VERTEX_SHADER_BEGIN);
    /* Nothing to do if there are no vertices to be processed? */
    if (context->vertexArrayEnabled == GL_FALSE)
      return;

    es1xShaderProgramCode_pushUniform   (shader, DECLARE_UNIFORM_PMV_MATRIX, UNIFORM_PMV_MATRIX);

    if (mvMatrixDeclared == GL_FALSE)
    {
      es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_MV_MATRIX, UNIFORM_MV_MATRIX);
      mvMatrixDeclared = GL_TRUE;
    }

    es1xShaderProgramCode_pushAttribute (shader, DECLARE_ATTRIBUTE_VERTEX_POSITION);
    es1xShaderProgramCode_pushRoutine   (shader, mainFunction, CODE_VERTEX_SHADER_TRANSFORM_POSITION);

    /* Discard vertex coloring? */
    if (context->lightingEnabled == GL_FALSE || context->colorMaterialEnabled)
    {
      if (context->colorArrayEnabled)
      {
        es1xShaderProgramCode_pushAttribute     (shader, DECLARE_ATTRIBUTE_VERTEX_COLOR);
        es1xShaderProgramCode_pushRoutine       (shader, mainFunction, CODE_VERTEX_SHADER_INIT_COLOR_FROM_ATTRIBUTE);
      }
      else
      {
        es1xShaderProgramCode_pushUniform       (shader, DECLARE_UNIFORM_CURRENT_VERTEX_COLOR, UNIFORM_CURRENT_VERTEX_COLOR);
        es1xShaderProgramCode_pushRoutine       (shader, mainFunction, CODE_VERTEX_SHADER_INIT_COLOR_FROM_CURRENT);
      }
    }

    /* Texturing */
    if (context->texturingEnabled)
    {
      int unitNdx;
      for(unitNdx=0;unitNdx<ES1X_NUM_SUPPORTED_TEXTURE_UNITS;unitNdx++)
      {
        if (context->textureUnitEnabled[unitNdx])
        {
          es1xShaderProgramCode_pushVarying(shader, DECLARE_VARYING_TEXTURE_COORDINATES[unitNdx]);
          es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_TEXTURE_MATRIX[unitNdx], UNIFORM_TEXTURE_MATRIX0 + unitNdx);

          /* Select where to take texture coordinates */
          if (context->textureCoordArrayEnabled[unitNdx])
          {
            es1xShaderProgramCode_pushAttribute(shader, DECLARE_ATTRIBUTE_TEXCOORD[unitNdx]);
            es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_INIT_TEX_COORD_FROM_ATTRIBUTE[unitNdx]);
          }
          else
          {
            es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_CURRENT_TEXTURE_COORD[unitNdx], UNIFORM_CURRENT_TEXTURE_COORDINATES0 + unitNdx);
            es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_INIT_TEX_COORD_FROM_CURRENT[unitNdx]);
          }

          /* Transform coordinates */
          es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_TRANSFORM_TEXTURE_COORDS[unitNdx]);
        }
      }
    }

    /* Lighting */
    if (context->lightingEnabled)
    {
      /* Declare LightSource and Material structures */
      es1xShaderProgramCode_pushStructure       (shader, STRUCTURE_LIGHT_SOURCE);
      es1xShaderProgramCode_pushStructure       (shader, STRUCTURE_MATERIAL);

      /* Select where to take initial vertex normal */
      if (context->normalArrayEnabled)
      {
        es1xShaderProgramCode_pushAttribute     (shader, DECLARE_ATTRIBUTE_VERTEX_NORMAL);
        es1xShaderProgramCode_pushRoutine       (shader, mainFunction, CODE_VERTEX_SHADER_INIT_NORMAL_FROM_ATTRIBUTE);
      }
      else
      {
        es1xShaderProgramCode_pushUniform       (shader, DECLARE_UNIFORM_CURRENT_VERTEX_NORMAL, UNIFORM_CURRENT_VERTEX_NORMAL);
        es1xShaderProgramCode_pushRoutine       (shader, mainFunction, CODE_VERTEX_SHADER_INIT_NORMAL_FROM_CURRENT);
      }

      /* Normal transformation */
      es1xShaderProgramCode_pushUniform (shader, DECLARE_UNIFORM_MV_MATRIX_INV, UNIFORM_MV_MATRIX_INV);
      es1xShaderProgramCode_pushRoutine (shader, mainFunction, CODE_VERTEX_SHADER_TRANSFORM_NORMAL);

      if (context->rescaleNormalEnabled)
      {
        es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_RESCALE_NORMAL_FACTOR, UNIFORM_RESCALE_NORMAL_FACTOR);
        es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_RESCALE_NORMAL);
      }

      if (context->normalizeEnabled)
        es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_NORMALIZE);

      if (eyePositionComputed == GL_FALSE)
      {
        if (mvMatrixDeclared == GL_FALSE)
        {
          es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_MV_MATRIX, UNIFORM_MV_MATRIX);
          mvMatrixDeclared = GL_TRUE;
        }

        es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_COMPUTE_EYE_POSITION);
        eyePositionComputed = GL_TRUE;
      }

      /* Lighting */
      {
        es1xLightType   lightType       = 0;
        int                             slotIndex       = 0;

        es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_DECLARE_AND_ASSIGN_LIGHT_COLOR);
        es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_COLOR_BLACK);

        /* es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_PREPARE_LIGHTS_LOOP); */

        if (mapTable->numEnabledLights > 0)
        {
          ES1X_ASSERT(mapTable->numEnabledLights - 1 >= 0 && mapTable->numEnabledLights - 1 < ES1X_NUM_SUPPORTED_LIGHTS);
          es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_LIGHT_ARRAY[mapTable->numEnabledLights - 1], UNIFORM_LIGHT);

          /* Light computation in loops */
          /**
             for(lightType=0;lightType<ES1X_NUM_LIGHT_TYPES;lightType++)
             {
             int numLights = mapTable->numEnabledLightsOfType[lightType];

             switch(numLights)
             {
             case 0:            continue;
             case 1:            break;
             default:   es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_COMPUTE_LIGHTS_LOOP[slotIndex + numLights - 1]);     break;
             }

             es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_ADD_LIGHT_COLOR);

             switch(lightType)
             {
             case ES1X_LIGHT_TYPE_SPOT:                 es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_COMPUTE_SPOT_LIGHT);                 break;
             case ES1X_LIGHT_TYPE_DIRECTIONAL:  es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_COMPUTE_DIRECTIONAL_LIGHT);  break;
             case ES1X_LIGHT_TYPE_OMNI:                 es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_COMPUTE_OMNI_LIGHT);                 break;
             default:
             ES1X_ASSERT(!"Unknown light type");
             break;
             }

             slotIndex = slotIndex + numLights;
             }
             /*/

          ES1X_UNREF(lightType);

          for(slotIndex=0;slotIndex<mapTable->numEnabledLights;slotIndex++)
          {
            int lightIndex = mapTable->light[slotIndex];

            es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_ADD_LIGHT_COLOR);

            switch(context->lightType[lightIndex])
            {
              case ES1X_LIGHT_TYPE_SPOT:                        es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_COMPUTE_SPOT_LIGHT_ORIGINAL[slotIndex]);                     break;
              case ES1X_LIGHT_TYPE_DIRECTIONAL: es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_COMPUTE_DIRECTIONAL_LIGHT_ORIGINAL[slotIndex]);      break;
              case ES1X_LIGHT_TYPE_OMNI:                        es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_COMPUTE_OMNI_LIGHT_ORIGINAL[slotIndex]);                     break;
              default:
                ES1X_ASSERT(!"Unknown light type");
                break;
            }
          }

          /**/
        }
#if 0
        else /* Init lightcolor with black in case no lights were enabled */
          es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_COLOR_BLACK);
#endif

        /* Apply material */
        es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_POST_COMPUTE_LIGHTING);

        /* Omni light computation method */
        if (mapTable->numEnabledLightsOfType[ES1X_LIGHT_TYPE_OMNI] > 0)
        {
          int function = es1xShaderProgramCode_allocateFunction(shader);
          es1xShaderProgramCode_pushRoutine(shader, function, CODE_VERTEX_SHADER_FUNCTION_COMPUTE_OMNI_LIGHT);
        }

        /* Directional light computation method */
        if (mapTable->numEnabledLightsOfType[ES1X_LIGHT_TYPE_DIRECTIONAL] > 0)
        {
          int function = es1xShaderProgramCode_allocateFunction(shader);
          es1xShaderProgramCode_pushRoutine(shader, function, CODE_VERTEX_SHADER_FUNCTION_COMPUTE_DIRECTIONAL_LIGHT);
        }

        /* Spot light computation method */
        if (mapTable->numEnabledLightsOfType[ES1X_LIGHT_TYPE_SPOT] > 0)
        {
          int function = es1xShaderProgramCode_allocateFunction(shader);
          es1xShaderProgramCode_pushRoutine(shader, function, CODE_VERTEX_SHADER_FUNCTION_COMPUTE_SPOT_LIGHT);
        }

        /* Add generic light computation function */
        {
          int function = es1xShaderProgramCode_allocateFunction(shader);
          es1xShaderProgramCode_pushRoutine     (shader, function, CODE_VERTEX_SHADER_COMPUTE_LIGHTING_BEGIN);

          if (context->colorMaterialEnabled)
          {
            if (context->colorArrayEnabled)
              es1xShaderProgramCode_pushRoutine(shader, function, CODE_VERTEX_SHADER_COMPUTE_CM_LIGHTING_COLOR_A);
            else
              es1xShaderProgramCode_pushRoutine(shader, function, CODE_VERTEX_SHADER_COMPUTE_CM_LIGHTING_COLOR_U);
          }
          else
            es1xShaderProgramCode_pushRoutine(shader, function, CODE_VERTEX_SHADER_COMPUTE_LIGHTING_COLOR);

          es1xShaderProgramCode_pushRoutine     (shader, function, CODE_VERTEX_SHADER_COMPUTE_LIGHTING_END);
          es1xShaderProgramCode_pushUniform     (shader, DECLARE_UNIFORM_MATERIAL,                              UNIFORM_MATERIAL);
          es1xShaderProgramCode_pushUniform     (shader, DECLARE_UNIFORM_LIGHT_MODEL_AMBIENT,   UNIFORM_LIGHT_MODEL_AMBIENT);
        }

        /* Add direction helper method */
        {
          int directionFunction = es1xShaderProgramCode_allocateFunction(shader);
          es1xShaderProgramCode_pushRoutine(shader, directionFunction, CODE_DIRECTION_FUNCTION);
        }
      }
    }

    /* Fog */
    if (context->fogEnabled)
    {
      if (mvMatrixDeclared == GL_FALSE)
      {
        es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_MV_MATRIX, UNIFORM_MV_MATRIX);
        mvMatrixDeclared = GL_TRUE;
      }

      if (eyePositionComputed == GL_FALSE)
      {
        es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_COMPUTE_EYE_POSITION);
        eyePositionComputed = GL_TRUE;
      }

      es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_COMPUTE_EYE_DISTANCE);
      es1xShaderProgramCode_pushVarying(shader, DECLARE_VARYING_FOG_INTENSITY);

      switch(context->fogMode)
      {
        case ES1X_FOGMODE_LINEAR:
          es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_COMPUTE_LINEAR_FOG);
          es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_FOG_END, UNIFORM_FOG_END);
          es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_FOG_RCPENDSTARTDIFF, UNIFORM_FOG_RCPENDSTARTDIFF);
          break;
        case ES1X_FOGMODE_EXP:
          es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_FOG_DENSITY, UNIFORM_FOG_DENSITY);
          es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_COMPUTE_EXP_FOG);
          break;
        case ES1X_FOGMODE_EXP2:
          es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_FOG_DENSITY, UNIFORM_FOG_DENSITY);
          es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_COMPUTE_EXP2_FOG);
          break;

        default:
          ES1X_ASSERT(!"Unhandled case");
          break;
      }
    }

    es1xShaderProgramCode_pushVarying   (shader, DECLARE_VARYING_VERTEX_COLOR);

    if (context->lightingEnabled == GL_FALSE)
      es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_ASSIGN_COLOR);

    /* Points and point sizes*/
    if (context->drawingPoints)
    {
      if (eyePositionComputed == GL_FALSE)
      {
        es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_COMPUTE_EYE_POSITION);
        eyePositionComputed = GL_TRUE;
      }

      if (mvMatrixDeclared == GL_FALSE)
      {
        es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_MV_MATRIX, UNIFORM_MV_MATRIX);
        mvMatrixDeclared = GL_TRUE;
      }

      if (context->pointSizeArrayOESEnabled)
      {
        es1xShaderProgramCode_pushAttribute     (shader, DECLARE_ATTRIBUTE_VERTEX_POINT_SIZE);
        es1xShaderProgramCode_pushRoutine       (shader, mainFunction, CODE_VERTEX_SHADER_INIT_POINT_SIZE_FROM_ATTRIBUTE);
      }
      else
      {
        es1xShaderProgramCode_pushUniform       (shader, DECLARE_UNIFORM_CURRENT_POINT_SIZE, UNIFORM_CURRENT_VERTEX_POINT_SIZE);
        es1xShaderProgramCode_pushRoutine       (shader, mainFunction, CODE_VERTEX_SHADER_INIT_POINT_SIZE_FROM_CURRENT);
      }

      es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_MIN_DERIVED_POINT_WIDTH,                        UNIFORM_MIN_DERIVED_POINT_WIDTH);
      es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_MAX_DERIVED_POINT_WIDTH,                        UNIFORM_MAX_DERIVED_POINT_WIDTH);
      es1xShaderProgramCode_pushUniform(shader, DECLARE_UNIFORM_POINT_SIZE_DISTANCE_ATTENUATION,        UNIFORM_POINT_SIZE_DISTANCE_ATTENUATION);
      es1xShaderProgramCode_pushRoutine(shader, mainFunction, CODE_VERTEX_SHADER_COMPUTE_POINT_DERIVED_SIZE);
    }

    es1xShaderProgramCode_pushRoutine   (shader, mainFunction, CODE_VERTEX_SHADER_END);
  }
}

#ifdef ES1X_DEBUG

/*---------------------------------------------------------------------------*/

static void es1xDumpShader(const char** lines, int numLines)
{
  int i;

  ES1X_UNREF(lines);
  ES1X_UNREF(numLines);

  for(i=0;i<numLines;i++)
    ES1X_LOG(("%s", lines[i]));
}

/*---------------------------------------------------------------------------*/

static void es1xDumpUniformLocations(es1xShaderProgramInfo* info)
{
  int i;
  ES1X_LOG(("\n--- Uniform locations ---"));

  for(i=0;i<NUM_SHADER_CODE_UNIFORMS;i++)
  {
    if (info->uniformLocations[i] >= 0)
    {
      ES1X_LOG(("\n%s: %d", ES1X_SHADER_CODE_UNIFORM_NAME_LOOKUP[i], info->uniformLocations[i]));
    }
  }
  ES1X_LOG(("\n"));
}

/*---------------------------------------------------------------------------*/

static void es1xDumpMappingTables(const es1xShaderContext* context, const es1xIndexMapTable* table)
{
  int i;
  ES1X_LOG(("\n--- Mapping tables ---"));

  ES1X_UNREF(context);
  ES1X_UNREF(table);

  for(i=0;i<ES1X_NUM_SUPPORTED_LIGHTS;i++)
  {
    int lightIndex = table->light[i];
    ES1X_UNREF(lightIndex);
    ES1X_LOG(("\nlight #%d is %s %s-light and it represents light #%d in real context", i, context->lightEnabled[lightIndex] ? "enabled" : "disabled", es1xLightType_toString(context->lightType[lightIndex]), lightIndex));
  }

  ES1X_LOG(("\n"));
}

#endif


/*---------------------------------------------------------------------------*/

void es1xShaderProgram_create(es1xShaderContext* context, es1xShaderProgramInfo* info, const es1xIndexMapTable* mapTable)
{
  es1xShaderProgramCode vertexShader;
  es1xShaderProgramCode fragmentShader;

  es1xConstructFragmentShader(&fragmentShader, context);
  es1xConstructVertexShader(&vertexShader, context, mapTable);

  ES1X_ASSUME_NO_ERROR;

  /* Concatenate shaders */
  {
#define ES1X_NUM_MAX_VERTEX_SHADER_PROGRAM_SOURCE_CODE_LINES    64 /* \todo, just placeholder values */
#define ES1X_NUM_MAX_FRAGMENT_SHADER_PROGRAM_SOURCE_CODE_LINES  64

    const char* fragmentShaderSourceCodeBuffer[ES1X_NUM_MAX_FRAGMENT_SHADER_PROGRAM_SOURCE_CODE_LINES];
    const char* vertexShaderSourceCodeBuffer[ES1X_NUM_MAX_VERTEX_SHADER_PROGRAM_SOURCE_CODE_LINES];

    int numFragmentShaderProgramLines   = es1xShaderProgramCode_concatenate(&fragmentShader,    (const char**)fragmentShaderSourceCodeBuffer,   ES1X_NUM_MAX_FRAGMENT_SHADER_PROGRAM_SOURCE_CODE_LINES);
    int numVertexShaderProgramLines             = es1xShaderProgramCode_concatenate(&vertexShader,              (const char**)vertexShaderSourceCodeBuffer,             ES1X_NUM_MAX_VERTEX_SHADER_PROGRAM_SOURCE_CODE_LINES);

    ES1X_DEBUG_CODE(ES1X_LOG(("\n--- Created fragment shader ---\n\n")));
    ES1X_DEBUG_CODE(es1xDumpShader(fragmentShaderSourceCodeBuffer, numFragmentShaderProgramLines));
    ES1X_DEBUG_CODE(ES1X_LOG(("\n--- Created vertex shader ---\n\n")));
    ES1X_DEBUG_CODE(es1xDumpShader(vertexShaderSourceCodeBuffer, numVertexShaderProgramLines));

    /* Construct GL program object */
    {
#ifdef ES1X_DEBUG
#       define ERROR_BUFFER_SIZE 8096
      char errorBuffer[ERROR_BUFFER_SIZE];
#endif

      GLuint    fragmentShaderHandle;
      GLuint    vertexShaderHandle;
      GLuint    program;
      GLint     shaderCompiled;
      GLint     programLinked;

      program = gl2b->glCreateProgram();
      ES1X_ASSUME_NO_ERROR;

      fragmentShaderHandle = gl2b->glCreateShader(GL_FRAGMENT_SHADER);
      ES1X_ASSUME_NO_ERROR;
      gl2b->glShaderSource(fragmentShaderHandle, numFragmentShaderProgramLines, (const char**)fragmentShaderSourceCodeBuffer, 0);
      ES1X_ASSUME_NO_ERROR;
      gl2b->glCompileShader(fragmentShaderHandle);
      ES1X_ASSUME_NO_ERROR;
      gl2b->glGetShaderiv(fragmentShaderHandle, GL_COMPILE_STATUS, &shaderCompiled);
      ES1X_ASSUME_NO_ERROR;

#ifdef ES1X_DEBUG
      if (shaderCompiled == GL_FALSE)
      {
        gl2b->glGetShaderInfoLog(fragmentShaderHandle, ERROR_BUFFER_SIZE, 0, errorBuffer);
        ES1X_LOG((errorBuffer));
        ES1X_ASSERT(shaderCompiled != GL_FALSE && "fragmentShader");
      }
#endif

      /* Create vertex shader */
      vertexShaderHandle = gl2b->glCreateShader(GL_VERTEX_SHADER);
      ES1X_ASSUME_NO_ERROR;
      gl2b->glShaderSource(vertexShaderHandle, numVertexShaderProgramLines, (const char**)vertexShaderSourceCodeBuffer, 0);
      ES1X_ASSUME_NO_ERROR;
      gl2b->glCompileShader(vertexShaderHandle);
      ES1X_ASSUME_NO_ERROR;
      gl2b->glGetShaderiv(vertexShaderHandle, GL_COMPILE_STATUS, &shaderCompiled);
      ES1X_ASSUME_NO_ERROR;

#ifdef ES1X_DEBUG
      if (shaderCompiled == GL_FALSE)
      {
        gl2b->glGetShaderInfoLog(vertexShaderHandle, ERROR_BUFFER_SIZE, 0, errorBuffer);
        ES1X_LOG((errorBuffer));
        ES1X_ASSERT(shaderCompiled != GL_FALSE && "vertexShader");
      }
#endif

      gl2b->glAttachShader(program, fragmentShaderHandle);
      ES1X_ASSUME_NO_ERROR;
      gl2b->glAttachShader(program, vertexShaderHandle);
      ES1X_ASSUME_NO_ERROR;

      /* Bind vertex attrib arrays */
      gl2b->glBindAttribLocation(program, ES1X_VERTEX_ARRAY_POS,              s_nameLookupAttributeVertexPosition);
      ES1X_ASSUME_NO_ERROR;
      gl2b->glBindAttribLocation(program, ES1X_COLOR_ARRAY_POS,                       s_nameLookupAttributeVertexColor);
      ES1X_ASSUME_NO_ERROR;
      gl2b->glBindAttribLocation(program, ES1X_NORMAL_ARRAY_POS,              s_nameLookupAttributeVertexNormal);
      ES1X_ASSUME_NO_ERROR;
      gl2b->glBindAttribLocation(program, ES1X_POINT_SIZE_ARRAY_POS,  s_nameLookupAttributeVertexPointSize);
      ES1X_ASSUME_NO_ERROR;

      /* Bind texture coord arrays */
      {
        int textureUnit;

        for(textureUnit=0;textureUnit<ES1X_NUM_SUPPORTED_TEXTURE_UNITS;textureUnit++)
        {
          gl2b->glBindAttribLocation(program, ES1X_TEXTURE_COORD_ARRAY0_POS + textureUnit, s_textureCoordAttributeNameLookup[textureUnit]);
          ES1X_ASSUME_NO_ERROR;
        }
      }

      /* Link program */
      gl2b->glLinkProgram(program);
      gl2b->glGetProgramiv(program, GL_LINK_STATUS, &programLinked);

#ifdef ES1X_DEBUG
      if (programLinked == GL_FALSE)
      {
        gl2b->glGetProgramInfoLog(program, ERROR_BUFFER_SIZE, 0, errorBuffer);
        ES1X_LOG((errorBuffer));
        ES1X_ASSERT(programLinked != GL_FALSE && "link");
      }
#endif
      ES1X_ASSUME_NO_ERROR;

      info->program = program;

      /* Query uniform locations so that they can be assigned smoothly without need to query the location for each frame. */
      {
        es1xShaderCodeUniform uniform;

        /* Toggle struct uniform attributes */
        {
          int lightNdx;

          if (vertexShader.uniformRequirements[UNIFORM_LIGHT] == GL_TRUE)
          {
            int numLightUniforms = UNIFORM_LIGHT1_AMBIENT - UNIFORM_LIGHT0_AMBIENT;
            ES1X_ASSERT(numLightUniforms > 0);

            /* \note [090406:christian] This is a bit hack. As shaders might contain u_light- and u_material-uniforms that are
               structures and therefore they cannot be assigned as structs (each field separately instead), we need to mark that
               these uniforms are not required to be assigned, but all field uniforms should be. Consider refactoring this. */
            for(lightNdx=0;lightNdx<mapTable->numEnabledLights;lightNdx++)
            {
              vertexShader.uniformRequirements[UNIFORM_LIGHT0_AMBIENT                                   + lightNdx] = GL_TRUE;
              vertexShader.uniformRequirements[UNIFORM_LIGHT0_DIFFUSE                                   + lightNdx] = GL_TRUE;
              vertexShader.uniformRequirements[UNIFORM_LIGHT0_SPECULAR                          + lightNdx] = GL_TRUE;
              vertexShader.uniformRequirements[UNIFORM_LIGHT0_EYEPOSITION                               + lightNdx] = GL_TRUE;
              vertexShader.uniformRequirements[UNIFORM_LIGHT0_CONSTANT_ATTENUATION      + lightNdx] = GL_TRUE;
              vertexShader.uniformRequirements[UNIFORM_LIGHT0_LINEAR_ATTENUATION                + lightNdx] = GL_TRUE;
              vertexShader.uniformRequirements[UNIFORM_LIGHT0_QUADRATIC_ATTENUATION     + lightNdx] = GL_TRUE;
              vertexShader.uniformRequirements[UNIFORM_LIGHT0_EYEPOSITION                               + lightNdx] = GL_TRUE;
              vertexShader.uniformRequirements[UNIFORM_LIGHT0_SPOT_EYEDIRECTION         + lightNdx] = GL_TRUE;
              vertexShader.uniformRequirements[UNIFORM_LIGHT0_SPOT_EXPONENT                     + lightNdx] = GL_TRUE;
              vertexShader.uniformRequirements[UNIFORM_LIGHT0_COS_SPOT_CUTOFF                   + lightNdx] = GL_TRUE;
            }

            vertexShader.uniformRequirements[UNIFORM_LIGHT] = GL_FALSE;
          }

          if (vertexShader.uniformRequirements[UNIFORM_MATERIAL] == GL_TRUE)
          {
            vertexShader.uniformRequirements[UNIFORM_MATERIAL]                                          = GL_FALSE;
            vertexShader.uniformRequirements[UNIFORM_MATERIAL_AMBIENT]                          = GL_TRUE;
            vertexShader.uniformRequirements[UNIFORM_MATERIAL_DIFFUSE]                          = GL_TRUE;
            vertexShader.uniformRequirements[UNIFORM_MATERIAL_SPECULAR]                         = GL_TRUE;
            vertexShader.uniformRequirements[UNIFORM_MATERIAL_EMISSIVE]                         = GL_TRUE;
            vertexShader.uniformRequirements[UNIFORM_MATERIAL_SPECULAR_EXPONENT]        = GL_TRUE;
          }
        }

        /* Query locations of existing uniforms in the shaders */
        for(uniform=0;uniform<NUM_SHADER_CODE_UNIFORMS;uniform++)
        {
          if (vertexShader.uniformRequirements[uniform] == GL_TRUE || fragmentShader.uniformRequirements[uniform]       == GL_TRUE)
          {
            info->uniformLocations[uniform] = gl2b
                                              ->glGetUniformLocation(program, ES1X_SHADER_CODE_UNIFORM_NAME_LOOKUP[uniform]);
            ES1X_ASSUME_NO_ERROR;
          }
        }

        /* Dump all uniform locations */
        ES1X_DEBUG_CODE(es1xDumpUniformLocations(info));
        ES1X_DEBUG_CODE(es1xDumpMappingTables(context, mapTable));
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

void es1xShaderProgramInfo_init(es1xShaderProgramInfo* info)
{
  es1xShaderCodeUniform uniformNdx;
  ES1X_ASSERT(info);

  info->program         = 0;
  info->priorityData    = 0;

  for(uniformNdx=0;uniformNdx < NUM_SHADER_CODE_UNIFORMS;uniformNdx++)
    info->uniformLocations[uniformNdx] = -1;


  es1xUniformVersions_invalidateAll(&info->uniformVersions);
}

/*---------------------------------------------------------------------------*/
