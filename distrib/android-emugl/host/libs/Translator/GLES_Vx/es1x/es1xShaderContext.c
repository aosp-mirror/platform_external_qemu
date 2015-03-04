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

 
#include "es1xShaderContext.h"
#include "es1xMemory.h"
#include "es1xExternalConfig.h"
#include "es1xContext.h"

#define ES1X_SHADER_CONTEXT_PUSH_FLOAT(VALUE)   \
  *array++ = ((int)(VALUE * (1 << 16)));        \
  *array++ = 32;

#define ES1X_SHADER_CONTEXT_PUSH_INTEGER(VALUE) \
  *array++ = VALUE;                             \
  *array++ = 32;

#define ES1X_SHADER_CONTEXT_PUSH_BOOLEAN(VALUE) \
  *array++ = VALUE;                             \
  *array++ = 1;

#define ES1X_SHADER_CONTEXT_PUSH_ENUM(VALUE, BITS)      \
  *array++ = VALUE;                                     \
  *array++ = BITS;

#define ES1X_SHADER_CONTEXT_PUSH_VEC4(VALUE)    \
  ES1X_SHADER_CONTEXT_PUSH_FLOAT(VALUE.x)       \
  ES1X_SHADER_CONTEXT_PUSH_FLOAT(VALUE.y)       \
  ES1X_SHADER_CONTEXT_PUSH_FLOAT(VALUE.z)       \
  ES1X_SHADER_CONTEXT_PUSH_FLOAT(VALUE.w)

#define ES1X_SHADER_CONTEXT_PUSH_VEC3(VALUE)    \
  ES1X_SHADER_CONTEXT_PUSH_FLOAT(VALUE.x)       \
  ES1X_SHADER_CONTEXT_PUSH_FLOAT(VALUE.y)       \
  ES1X_SHADER_CONTEXT_PUSH_FLOAT(VALUE.z)

#define ES1X_SHADER_CONTEXT_PUSH_VEC2(VALUE)    \
  ES1X_SHADER_CONTEXT_PUSH_FLOAT(VALUE.x)       \
  ES1X_SHADER_CONTEXT_PUSH_FLOAT(VALUE.y)

#define ES1X_SHADER_CONTEXT_PUSH_COLOR(VALUE)   \
  ES1X_SHADER_CONTEXT_PUSH_FLOAT(VALUE.r)       \
  ES1X_SHADER_CONTEXT_PUSH_FLOAT(VALUE.g)       \
  ES1X_SHADER_CONTEXT_PUSH_FLOAT(VALUE.b)       \
  ES1X_SHADER_CONTEXT_PUSH_FLOAT(VALUE.a)


/*---------------------------------------------------------------------------*/

es1xShaderContext* es1xShaderContext_create()
{
  es1xShaderContext* context = (es1xShaderContext*) es1xMalloc(sizeof(es1xShaderContext));
  return context;
}

void es1xIndexMapTable_init(es1xIndexMapTable* table, const void* source)
{
  const es1xContext* context = (const es1xContext*) source;

  int i                 = 0;
  int j                 = 0;
  int slotNdx           = 0;
  int tailNdx           = ES1X_NUM_SUPPORTED_LIGHTS - 1;

  table->numEnabledLights = 0;

  for(j=0;j<ES1X_NUM_LIGHT_TYPES;j++)
  {
    table->numEnabledLightsOfType[j] = 0;

    for(i=0;i<ES1X_NUM_SUPPORTED_LIGHTS;i++)
    {
      if (context->lightType[i] == j)
      {
        if (context->lightEnabled[i])
        {
          ES1X_ASSERT(i < (1 << (sizeof(deUint8) * 8)));

          table->light[slotNdx++] = (deUint8)i;
          table->numEnabledLightsOfType[j]++;
          table->numEnabledLights++;
        }
        else
          table->light[tailNdx--] = (deUint8)i;
      }
    }
  }

  ES1X_ASSERT(slotNdx == tailNdx + 1);
  slotNdx = 0;

  for(i=0;i<ES1X_NUM_SUPPORTED_TEXTURE_UNITS;i++)
    table->textureUnit[i] = (deUint8)i;
}

/*---------------------------------------------------------------------------*/

void es1xShaderContext_init(es1xShaderContext* context, const void* /* es1xContext* */ voidPtr, GLenum drawingPrimitive, const es1xIndexMapTable* mapTable)
{
  const es1xContext* source = (const es1xContext*) voidPtr;

  int i                 = 0;

  /* Query formats of bound textures */
  for(i=0;i<ES1X_NUM_SUPPORTED_TEXTURE_UNITS;i++)
  {
    es1xTexture2D* texture = source->boundTexture[i];
    ES1X_ASSERT(texture);
    context->textureFormat[i] = texture->format;
  }

  context->drawingPoints = drawingPrimitive == GL_POINTS ? GL_TRUE : GL_FALSE;

#include "es1xShaderContextInit.inl"

}

/*---------------------------------------------------------------------------*/

void es1xShaderContext_optimize(es1xShaderContext* context)
{
  /* Alpha test */
  if (context->alphaTestEnabled == GL_FALSE)
    context->alphaTestFunc = ES1X_COMPAREFUNC_ALWAYS;
  else
  {
    switch(context->alphaTestFunc)
    {
      case ES1X_COMPAREFUNC_ALWAYS:
        /* Same if alpha test is off */
        context->alphaTestEnabled = GL_FALSE;
        break;

      case ES1X_COMPAREFUNC_NEVER:
        /*      This basically means that all fragments will be discarded,
                which makes both shaders practically useless. */
        context->vertexArrayEnabled = GL_FALSE;
        break;

      case ES1X_COMPAREFUNC_LESS:
      case ES1X_COMPAREFUNC_LEQUAL:
      case ES1X_COMPAREFUNC_EQUAL:
      case ES1X_COMPAREFUNC_GEQUAL:
      case ES1X_COMPAREFUNC_GREATER:
      case ES1X_COMPAREFUNC_NOTEQUAL:
        break;

      default:
        ES1X_ASSERT(!"Internal error");
        break;
    }
  }

  /*  Set default values for lights that are turned off
      or if lighting is completely disabled */
  {
    int lightNdx;

    for (lightNdx=0;lightNdx<ES1X_NUM_SUPPORTED_LIGHTS;lightNdx++)
    {
      if (context->lightingEnabled              == GL_FALSE ||
          context->lightEnabled[lightNdx]       == GL_FALSE)
      {
        context->lightEnabled[lightNdx] =       GL_FALSE;
        context->lightType[lightNdx]    =       ES1X_LIGHT_TYPE_DIRECTIONAL;
      }
    }
  }

  /* Set default fog values if fog is off */
  if (context->fogEnabled == GL_FALSE)
    context->fogMode = ES1X_FOGMODE_LINEAR;

  /* Texturing */
  {
    int unitNdx;

    for(unitNdx=0;unitNdx<ES1X_NUM_SUPPORTED_TEXTURE_UNITS;unitNdx++)
    {
      if (context->textureUnitEnabled[unitNdx] == GL_FALSE ||
          context->texturingEnabled == GL_FALSE)
      {
        context->textureUnitEnabled[unitNdx]            = GL_FALSE;
        context->textureFormat[unitNdx]                         = ES1X_TEXTUREFORMAT_RGBA;
        context->textureCoordArrayEnabled[unitNdx]      = GL_FALSE;
        context->texEnvMode[unitNdx]                            = ES1X_TEXENVMODE_MODULATE;
        context->texEnvCombineRGB[unitNdx]                      = ES1X_TEXENVCOMBINERGB_MODULATE;
        context->texEnvCombineAlpha[unitNdx]            = ES1X_TEXENVCOMBINEALPHA_MODULATE;
        context->texEnvCoordReplaceOES[unitNdx]         = GL_FALSE;
        context->texEnvSrc0RGB[unitNdx]                         = ES1X_TEXENVSRC_TEXTURE;
        context->texEnvSrc1RGB[unitNdx]                         = ES1X_TEXENVSRC_TEXTURE;
        context->texEnvSrc2RGB[unitNdx]                         = ES1X_TEXENVSRC_TEXTURE;
        context->texEnvSrc0Alpha[unitNdx]                       = ES1X_TEXENVSRC_TEXTURE;
        context->texEnvSrc1Alpha[unitNdx]                       = ES1X_TEXENVSRC_TEXTURE;
        context->texEnvSrc2Alpha[unitNdx]                       = ES1X_TEXENVSRC_TEXTURE;
        context->texEnvOperand0RGB[unitNdx]                     = ES1X_TEXENVOPERANDRGB_SRC_COLOR;
        context->texEnvOperand1RGB[unitNdx]                     = ES1X_TEXENVOPERANDRGB_SRC_COLOR;
        context->texEnvOperand2RGB[unitNdx]                     = ES1X_TEXENVOPERANDRGB_SRC_COLOR;
        context->texEnvOperand0Alpha[unitNdx]           = ES1X_TEXENVOPERANDALPHA_SRC_ALPHA;
        context->texEnvOperand1Alpha[unitNdx]           = ES1X_TEXENVOPERANDALPHA_SRC_ALPHA;
        context->texEnvOperand2Alpha[unitNdx]           = ES1X_TEXENVOPERANDALPHA_SRC_ALPHA;
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

void es1xShaderContext_constructKey(const es1xShaderContext* context, es1xShaderContextKey* key)
{
  deUint32 buffer[ES1X_SHADER_CONTEXT_HASH_KEY_BUFFER_SIZE] = { 0 };
  deUint32* dst = &buffer[0];

#include "es1xShaderContextPacking.inl"

  es1xShaderContextKey_set(key, &buffer[0], ES1X_SHADER_CONTEXT_HASH_KEY_BUFFER_SIZE);
}

#ifdef ES1X_DEBUG
#       include "es1xDebug.h"

void es1xShaderContext_dump(const es1xShaderContext* context)
{
  ES1X_UNREF(context);

#       include "es1xShaderContextDump.inl"

}

#endif
