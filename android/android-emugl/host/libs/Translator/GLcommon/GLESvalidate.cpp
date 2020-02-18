/*
* Copyright 2011 The Android Open Source Project
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

#include <GLcommon/GLESvalidate.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <OpenglCodecCommon/ErrorLog.h>


bool  GLESvalidate::textureEnum(GLenum e,unsigned int maxTex) {
    return e >= GL_TEXTURE0 && e < (GL_TEXTURE0 + maxTex);
}

bool GLESvalidate::pixelType(GLEScontext * ctx, GLenum type) {
    if ((ctx && ctx->getCaps()->GL_EXT_PACKED_DEPTH_STENCIL) &&
       (type == GL_UNSIGNED_INT_24_8_OES) )
        return true;

    if (ctx &&
       (ctx->getCaps()->GL_ARB_HALF_FLOAT_PIXEL || ctx->getCaps()->GL_NV_HALF_FLOAT) &&
       (type == GL_HALF_FLOAT_OES || type == GL_HALF_FLOAT))
        return true;

    switch(type) {
    case GL_UNSIGNED_BYTE:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_FLOAT:
        return true;
    }
    return false;
}

bool GLESvalidate::pixelOp(GLenum format,GLenum type) {
     switch(type) {
     case GL_UNSIGNED_SHORT_4_4_4_4:
     case GL_UNSIGNED_SHORT_5_5_5_1:
         return format == GL_RGBA;
     case GL_UNSIGNED_SHORT_5_6_5:
         return format == GL_RGB;
     }
     return true;
}

bool GLESvalidate::pixelFrmt(GLEScontext* ctx ,GLenum format) {
    if (ctx && ctx->getCaps()->GL_EXT_TEXTURE_FORMAT_BGRA8888 && format == GL_BGRA_EXT)
      return true;
    if (ctx && ctx->getCaps()->GL_EXT_PACKED_DEPTH_STENCIL && format == GL_DEPTH_STENCIL_OES)
      return true;
    switch(format) {
    case GL_ALPHA:
    case GL_RGB:
    case GL_RGBA:
    case GL_LUMINANCE:
    case GL_LUMINANCE_ALPHA:
        return true;
    }
    return false;
}

bool GLESvalidate::bufferTarget(GLenum target) {
    return target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER;
}

bool GLESvalidate::bufferUsage(GLenum usage) {
    switch(usage) {
        case GL_STREAM_DRAW:
        case GL_STATIC_DRAW:
        case GL_DYNAMIC_DRAW:
            return true;
    }
    return false;
}

bool GLESvalidate::bufferParam(GLenum param) {
 return  (param == GL_BUFFER_SIZE) || (param == GL_BUFFER_USAGE);
}

bool GLESvalidate::drawMode(GLenum mode) {
    switch(mode) {
    case GL_POINTS:
    case GL_LINE_STRIP:
    case GL_LINE_LOOP:
    case GL_LINES:
    case GL_TRIANGLE_STRIP:
    case GL_TRIANGLE_FAN:
    case GL_TRIANGLES:
        return true;
    }
    return false;
}

bool GLESvalidate::drawType(GLenum mode) {
    return  mode == GL_UNSIGNED_BYTE ||
            mode == GL_UNSIGNED_SHORT ||
            mode == GL_UNSIGNED_INT;
}

bool GLESvalidate::textureTarget(GLenum target) {
    return target==GL_TEXTURE_2D || target==GL_TEXTURE_CUBE_MAP;
}

bool GLESvalidate::textureTargetLimited(GLenum target) {
    return target==GL_TEXTURE_2D;
}

bool GLESvalidate::textureTargetEx(GLenum target) {
    switch(target) {
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X_OES:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_OES:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_OES:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_OES:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_OES:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_OES:
    case GL_TEXTURE_2D:
      return true;
    } 
    return false;
}

bool GLESvalidate::blendEquationMode(GLenum mode){
    return mode == GL_FUNC_ADD             ||
           mode == GL_FUNC_SUBTRACT        ||
           mode == GL_FUNC_REVERSE_SUBTRACT;
}

bool GLESvalidate::framebufferTarget(GLenum target){
    return target == GL_FRAMEBUFFER;
}

bool GLESvalidate::framebufferAttachment(GLenum attachment){
    switch(attachment){
    case GL_COLOR_ATTACHMENT0:
    case GL_DEPTH_ATTACHMENT:
    case GL_STENCIL_ATTACHMENT:
        return true;
    }
    return false;
}

bool GLESvalidate::framebufferAttachmentParams(GLenum pname){
    switch(pname){
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
        return true;
    }
    return false;
}

bool GLESvalidate::renderbufferTarget(GLenum target){
    return target == GL_RENDERBUFFER;
}

bool GLESvalidate::renderbufferParams(GLenum pname){
    switch(pname){
    case GL_RENDERBUFFER_WIDTH:
    case GL_RENDERBUFFER_HEIGHT:
    case GL_RENDERBUFFER_INTERNAL_FORMAT:
    case GL_RENDERBUFFER_RED_SIZE:
    case GL_RENDERBUFFER_GREEN_SIZE:
    case GL_RENDERBUFFER_BLUE_SIZE:
    case GL_RENDERBUFFER_ALPHA_SIZE:
    case GL_RENDERBUFFER_DEPTH_SIZE:
    case GL_RENDERBUFFER_STENCIL_SIZE:
        return true;
    }
    return false;
}

bool GLESvalidate::texImgDim(GLsizei width,GLsizei height,int maxTexSize) {

 if( width < 0 || height < 0 || width > maxTexSize || height > maxTexSize)
    return false;
 return isPowerOf2(width) && isPowerOf2(height);
}

static GLsizei paramValuesCount(GLenum e) {
    size_t s = 0;

    switch(param)
    {
    case GL_DEPTH_TEST:
    case GL_DEPTH_FUNC:
    case GL_DEPTH_BITS:
    case GL_MAX_CLIP_PLANES:
    case GL_GREEN_BITS:
    case GL_MAX_MODELVIEW_STACK_DEPTH:
    case GL_MAX_PROJECTION_STACK_DEPTH:
    case GL_MAX_TEXTURE_STACK_DEPTH:
    case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
    case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
    case GL_MAX_TEXTURE_SIZE:
    case GL_TEXTURE_GEN_MODE_OES:
    case GL_TEXTURE_ENV_MODE:
    case GL_FOG_MODE:
    case GL_FOG_DENSITY:
    case GL_FOG_START:
    case GL_FOG_END:
    case GL_SPOT_EXPONENT:
    case GL_CONSTANT_ATTENUATION:
    case GL_LINEAR_ATTENUATION:
    case GL_QUADRATIC_ATTENUATION:
    case GL_SHININESS:
    case GL_LIGHT_MODEL_TWO_SIDE:
    case GL_POINT_SIZE:
    case GL_POINT_SIZE_MIN:
    case GL_POINT_SIZE_MAX:
    case GL_POINT_FADE_THRESHOLD_SIZE:
    case GL_CULL_FACE_MODE:
    case GL_FRONT_FACE:
    case GL_SHADE_MODEL:
    case GL_DEPTH_WRITEMASK:
    case GL_DEPTH_CLEAR_VALUE:
    case GL_STENCIL_FAIL:
    case GL_STENCIL_PASS_DEPTH_FAIL:
    case GL_STENCIL_PASS_DEPTH_PASS:
    case GL_STENCIL_REF:
    case GL_STENCIL_WRITEMASK:
    case GL_MATRIX_MODE:
    case GL_MODELVIEW_STACK_DEPTH:
    case GL_PROJECTION_STACK_DEPTH:
    case GL_TEXTURE_STACK_DEPTH:
    case GL_ALPHA_TEST_FUNC:
    case GL_ALPHA_TEST_REF:
    case GL_ALPHA_TEST:
    case GL_BLEND_DST:
    case GL_BLEND_SRC:
    case GL_BLEND:
    case GL_LOGIC_OP_MODE:
    case GL_SCISSOR_TEST:
    case GL_MAX_TEXTURE_UNITS:
    case GL_ACTIVE_TEXTURE:
    case GL_ALPHA_BITS:
    case GL_ARRAY_BUFFER_BINDING:
    case GL_BLUE_BITS:
    case GL_CLIENT_ACTIVE_TEXTURE:
    case GL_CLIP_PLANE0:
    case GL_CLIP_PLANE1:
    case GL_CLIP_PLANE2:
    case GL_CLIP_PLANE3:
    case GL_CLIP_PLANE4:
    case GL_CLIP_PLANE5:
    case GL_COLOR_ARRAY:
    case GL_COLOR_ARRAY_BUFFER_BINDING:
    case GL_COLOR_ARRAY_SIZE:
    case GL_COLOR_ARRAY_STRIDE:
    case GL_COLOR_ARRAY_TYPE:
    case GL_COLOR_LOGIC_OP:
    case GL_COLOR_MATERIAL:
    case GL_PACK_ALIGNMENT:
    case GL_PERSPECTIVE_CORRECTION_HINT:
    case GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES:
    case GL_POINT_SIZE_ARRAY_STRIDE_OES:
    case GL_POINT_SIZE_ARRAY_TYPE_OES:
    case GL_POINT_SMOOTH:
    case GL_POINT_SMOOTH_HINT:
    case GL_POINT_SPRITE_OES:
    case GL_COORD_REPLACE_OES:
    case GL_COMBINE_ALPHA:
    case GL_SRC0_RGB:
    case GL_SRC1_RGB:
    case GL_SRC2_RGB:
    case GL_OPERAND0_RGB:
    case GL_OPERAND1_RGB:
    case GL_OPERAND2_RGB:
    case GL_SRC0_ALPHA:
    case GL_SRC1_ALPHA:
    case GL_SRC2_ALPHA:
    case GL_OPERAND0_ALPHA:
    case GL_OPERAND1_ALPHA:
    case GL_OPERAND2_ALPHA:
    case GL_RGB_SCALE:
    case GL_ALPHA_SCALE:
    case GL_COMBINE_RGB:
    case GL_POLYGON_OFFSET_FACTOR:
    case GL_POLYGON_OFFSET_FILL:
    case GL_POLYGON_OFFSET_UNITS:
    case GL_RED_BITS:
    case GL_RESCALE_NORMAL:
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
    case GL_SAMPLE_ALPHA_TO_ONE:
    case GL_SAMPLE_BUFFERS:
    case GL_SAMPLE_COVERAGE:
    case GL_SAMPLE_COVERAGE_INVERT:
    case GL_SAMPLE_COVERAGE_VALUE:
    case GL_SAMPLES:
    case GL_STENCIL_BITS:
    case GL_STENCIL_CLEAR_VALUE:
    case GL_STENCIL_FUNC:
    case GL_STENCIL_TEST:
    case GL_STENCIL_VALUE_MASK:
    case GL_STENCIL_BACK_FUNC:
    case GL_STENCIL_BACK_VALUE_MASK:
    case GL_STENCIL_BACK_REF:
    case GL_STENCIL_BACK_FAIL:
    case GL_STENCIL_BACK_PASS_DEPTH_FAIL:
    case GL_STENCIL_BACK_PASS_DEPTH_PASS:
    case GL_STENCIL_BACK_WRITEMASK:
    case GL_TEXTURE_2D:
    case GL_TEXTURE_BINDING_2D:
    case GL_TEXTURE_BINDING_CUBE_MAP:
    case GL_TEXTURE_BINDING_EXTERNAL_OES:
    case GL_TEXTURE_COORD_ARRAY:
    case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING:
    case GL_TEXTURE_COORD_ARRAY_SIZE:
    case GL_TEXTURE_COORD_ARRAY_STRIDE:
    case GL_TEXTURE_COORD_ARRAY_TYPE:
    case GL_UNPACK_ALIGNMENT:
    case GL_VERTEX_ARRAY:
    case GL_VERTEX_ARRAY_BUFFER_BINDING:
    case GL_VERTEX_ARRAY_SIZE:
    case GL_VERTEX_ARRAY_STRIDE:
    case GL_VERTEX_ARRAY_TYPE:
    case GL_SPOT_CUTOFF:
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_MAG_FILTER:
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
    case GL_GENERATE_MIPMAP:
    case GL_GENERATE_MIPMAP_HINT:
    case GL_RENDERBUFFER_WIDTH_OES:
    case GL_RENDERBUFFER_HEIGHT_OES:
    case GL_RENDERBUFFER_INTERNAL_FORMAT_OES:
    case GL_RENDERBUFFER_RED_SIZE_OES:
    case GL_RENDERBUFFER_GREEN_SIZE_OES:
    case GL_RENDERBUFFER_BLUE_SIZE_OES:
    case GL_RENDERBUFFER_ALPHA_SIZE_OES:
    case GL_RENDERBUFFER_DEPTH_SIZE_OES:
    case GL_RENDERBUFFER_STENCIL_SIZE_OES:
    case GL_RENDERBUFFER_BINDING:
    case GL_FRAMEBUFFER_BINDING:
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES:
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES:
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_OES:
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_OES:
    case GL_FENCE_STATUS_NV:
    case GL_FENCE_CONDITION_NV:
    case GL_TEXTURE_WIDTH_QCOM:
    case GL_TEXTURE_HEIGHT_QCOM:
    case GL_TEXTURE_DEPTH_QCOM:
    case GL_TEXTURE_INTERNAL_FORMAT_QCOM:
    case GL_TEXTURE_FORMAT_QCOM:
    case GL_TEXTURE_TYPE_QCOM:
    case GL_TEXTURE_IMAGE_VALID_QCOM:
    case GL_TEXTURE_NUM_LEVELS_QCOM:
    case GL_TEXTURE_TARGET_QCOM:
    case GL_TEXTURE_OBJECT_VALID_QCOM:
    case GL_BLEND_EQUATION_RGB_OES:
    case GL_BLEND_EQUATION_ALPHA_OES:
    case GL_BLEND_DST_RGB_OES:
    case GL_BLEND_SRC_RGB_OES:
    case GL_BLEND_DST_ALPHA_OES:
    case GL_BLEND_SRC_ALPHA_OES:
    case GL_MAX_LIGHTS:
    case GL_SHADER_TYPE:
    case GL_DELETE_STATUS:
    case GL_COMPILE_STATUS:
    case GL_INFO_LOG_LENGTH:
    case GL_SHADER_SOURCE_LENGTH:
    case GL_CURRENT_PROGRAM:
    case GL_LINK_STATUS:
    case GL_VALIDATE_STATUS:
    case GL_ATTACHED_SHADERS:
    case GL_ACTIVE_UNIFORMS:
    case GL_ACTIVE_ATTRIBUTES:
    case GL_SUBPIXEL_BITS:
    case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
    case GL_NUM_SHADER_BINARY_FORMATS:
    case GL_SHADER_COMPILER:
    case GL_MAX_VERTEX_ATTRIBS:
    case GL_MAX_VERTEX_UNIFORM_VECTORS:
    case GL_MAX_VARYING_VECTORS:
    case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
    case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
    case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
    case GL_MAX_RENDERBUFFER_SIZE:
    case GL_MAX_TEXTURE_IMAGE_UNITS:
    case GL_REQUIRED_TEXTURE_IMAGE_UNITS_OES:
    case GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES:
    case GL_LINE_WIDTH:
        s = 1;
        break;
    case GL_ALIASED_LINE_WIDTH_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_DEPTH_RANGE:
    case GL_MAX_VIEWPORT_DIMS:
    case GL_SMOOTH_POINT_SIZE_RANGE:
    case GL_SMOOTH_LINE_WIDTH_RANGE:
        s= 2;
        break;
    case GL_SPOT_DIRECTION:
    case GL_POINT_DISTANCE_ATTENUATION:
    case GL_CURRENT_NORMAL:
        s =  3;
        break;
    case GL_CURRENT_VERTEX_ATTRIB:
    case GL_CURRENT_TEXTURE_COORDS:
    case GL_CURRENT_COLOR:
    case GL_FOG_COLOR:
    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_EMISSION:
    case GL_POSITION:
    case GL_LIGHT_MODEL_AMBIENT:
    case GL_TEXTURE_ENV_COLOR:
    case GL_SCISSOR_BOX:
    case GL_VIEWPORT:
    case GL_TEXTURE_CROP_RECT_OES:
    case GL_COLOR_CLEAR_VALUE:
    case GL_COLOR_WRITEMASK:
    case GL_AMBIENT_AND_DIFFUSE:
    case GL_BLEND_COLOR:
        s =  4;
        break;
    case GL_MODELVIEW_MATRIX:
    case GL_PROJECTION_MATRIX:
    case GL_TEXTURE_MATRIX:
        s = 16;
    break;
    default:
        ERR("glUtilsParamSize: unknow param 0x%08x\n", param);
        s = 1; // assume 1
    }
    return s;
}
