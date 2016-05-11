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
#include "GLESv2Validate.h"
#include <string.h>

bool GLESv2Validate::blendEquationMode(GLenum mode){
    return mode == GL_FUNC_ADD             ||
           mode == GL_FUNC_SUBTRACT        ||
           mode == GL_FUNC_REVERSE_SUBTRACT;
}

bool GLESv2Validate::blendSrc(GLenum s) {
   switch(s) {
   case GL_ZERO:
   case GL_ONE:
   case GL_SRC_COLOR:
   case GL_ONE_MINUS_SRC_COLOR:
   case GL_DST_COLOR:
   case GL_ONE_MINUS_DST_COLOR:
   case GL_SRC_ALPHA:
   case GL_ONE_MINUS_SRC_ALPHA:
   case GL_DST_ALPHA:
   case GL_ONE_MINUS_DST_ALPHA:
   case GL_CONSTANT_COLOR:
   case GL_ONE_MINUS_CONSTANT_COLOR:
   case GL_CONSTANT_ALPHA:
   case GL_ONE_MINUS_CONSTANT_ALPHA:
   case GL_SRC_ALPHA_SATURATE:
        return true;
  }
  return false;
}


bool GLESv2Validate::blendDst(GLenum d) {
   switch(d) {
   case GL_ZERO:
   case GL_ONE:
   case GL_SRC_COLOR:
   case GL_ONE_MINUS_SRC_COLOR:
   case GL_DST_COLOR:
   case GL_ONE_MINUS_DST_COLOR:
   case GL_SRC_ALPHA:
   case GL_ONE_MINUS_SRC_ALPHA:
   case GL_DST_ALPHA:
   case GL_ONE_MINUS_DST_ALPHA:
   case GL_CONSTANT_COLOR:
   case GL_ONE_MINUS_CONSTANT_COLOR:
   case GL_CONSTANT_ALPHA:
   case GL_ONE_MINUS_CONSTANT_ALPHA:
        return true;
   }
   return false;
}

bool GLESv2Validate::textureParams(GLenum param){
    switch(param) {
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_MAG_FILTER:
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
    case GL_TEXTURE_MAX_ANISOTROPY_EXT:
        return true;
    default:
        return false;
    }
}

bool GLESv2Validate::hintTargetMode(GLenum target,GLenum mode){

   switch(mode) {
   case GL_FASTEST:
   case GL_NICEST:
   case GL_DONT_CARE:
       break;
   default: return false;
   }
   return target == GL_GENERATE_MIPMAP_HINT || target == GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES;
}

bool GLESv2Validate::capability(GLenum cap){
    switch(cap){
    case GL_BLEND:
    case GL_CULL_FACE:
    case GL_DEPTH_TEST:
    case GL_DITHER:
    case GL_POLYGON_OFFSET_FILL:
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
    case GL_SAMPLE_COVERAGE:
    case GL_SCISSOR_TEST:
    case GL_STENCIL_TEST:
        return true;
    }
    return false;
}

bool GLESv2Validate::pixelStoreParam(GLenum param){
    return param == GL_PACK_ALIGNMENT || param == GL_UNPACK_ALIGNMENT;
}

bool GLESv2Validate::readPixelFrmt(GLenum format){
    switch(format) {
    case GL_ALPHA:
    case GL_LUMINANCE_ALPHA:
    case GL_RGB:
    case GL_RGBA:
        return true;
    }
    return false;
}


bool GLESv2Validate::shaderType(GLenum type){
    return type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER;
}

bool GLESv2Validate::precisionType(GLenum type){
    switch(type){
    case GL_LOW_FLOAT:
    case GL_MEDIUM_FLOAT:
    case GL_HIGH_FLOAT:
    case GL_LOW_INT:
    case GL_MEDIUM_INT:
    case GL_HIGH_INT:
        return true;
    }
    return false;
}

bool GLESv2Validate::arrayIndex(GLEScontext * ctx,GLuint index) {
    return index < (GLuint)ctx->getCaps()->maxVertexAttribs;
}

#define GL_RED                              0x1903
#define GL_RG                               0x8227
#define GL_R8                               0x8229
#define GL_RG8                              0x822B
#define GL_R16F                             0x822D
#define GL_RG16F                            0x822F
#define GL_RGBA16F                          0x881A
#define GL_RGB16F                           0x881B
#define GL_R11F_G11F_B10F                   0x8C3A
#define GL_UNSIGNED_INT_10F_11F_11F_REV     0x8C3B

bool GLESv2Validate::pixelType(GLEScontext * ctx,GLenum type) {
    if(type == GL_UNSIGNED_SHORT || type == GL_UNSIGNED_INT
            || type == GL_UNSIGNED_INT_10F_11F_11F_REV)
        return true;

    return GLESvalidate::pixelType(ctx, type);
}

bool GLESv2Validate::pixelFrmt(GLEScontext* ctx,GLenum format) {
    switch (format) {
    case GL_DEPTH_COMPONENT:
    // GLES3 compatible
    // Required in dEQP
    case GL_RED:
    case GL_RG:
        return true;
    }
    return GLESvalidate::pixelFrmt(ctx, format);
}

bool GLESv2Validate::pixelItnlFrmt(GLEScontext* ctx ,GLenum internalformat) {
    switch (internalformat) {
    case GL_R8:
    case GL_RG8:
    case GL_R16F:
    case GL_RG16F:
    case GL_RGBA16F:
    case GL_RGB16F:
    case GL_R11F_G11F_B10F:
        return true;
    }
    return pixelFrmt(ctx, internalformat);
}

bool GLESv2Validate::pixelSizedFrmt(GLEScontext* ctx, GLenum internalformat,
                                    GLenum format, GLenum type) {
    if (internalformat == format) {
        return true;
    }
    switch (format) {
        case GL_RED:
            switch (type) {
                case GL_UNSIGNED_BYTE:
                    return internalformat == GL_R8;
                case GL_HALF_FLOAT:
                case GL_FLOAT:
                    return internalformat == GL_R16F;
                default:
                    return false;
            }
            break;
        case GL_RG:
            switch (type) {
                case GL_UNSIGNED_BYTE:
                    return internalformat == GL_RG8;
                case GL_HALF_FLOAT:
                case GL_FLOAT:
                    return internalformat == GL_RG16F;
                default:
                    return false;
            }
            break;
        case GL_RGB:
            switch (type) {
                case GL_HALF_FLOAT:
                case GL_FLOAT:
                    return internalformat == GL_RGB16F
                            || internalformat == GL_R11F_G11F_B10F;
                case GL_UNSIGNED_INT_10F_11F_11F_REV:
                    return internalformat == GL_R11F_G11F_B10F;
                default:
                    return false;
            }
            break;
        case GL_RGBA:
            switch (type) {
                case GL_HALF_FLOAT:
                case GL_FLOAT:
                    return internalformat == GL_RGBA16F;
                default:
                    return false;
            }
            break;
    }
    return false;
}

bool GLESv2Validate::attribName(const GLchar* name){
    const GLchar* found = strstr(name,"gl_");
    return (!found) ||
           (found != name) ; // attrib name does not start with gl_
}

bool GLESv2Validate::attribIndex(int index){
    return index >=0 && index < GL_MAX_VERTEX_ATTRIBS;
}

bool GLESv2Validate::programParam(GLenum pname){
    switch(pname){
        case GL_DELETE_STATUS:
        case GL_LINK_STATUS:
        case GL_VALIDATE_STATUS:
        case GL_INFO_LOG_LENGTH:
        case GL_ATTACHED_SHADERS:
        case GL_ACTIVE_ATTRIBUTES:
        case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
        case GL_ACTIVE_UNIFORMS:
        case GL_ACTIVE_UNIFORM_MAX_LENGTH:
            return true;
    }
    return false;
}

bool GLESv2Validate::textureIsCubeMap(GLenum target){
    switch(target){
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            return true;
    }
    return false;
}
