/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ETC1/etc1.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "common/alog.h"
#include "gles/buffer_data.h"
#include "gles/debug.h"
#include "gles/egl_image.h"
#include "gles/framebuffer_data.h"
#include "gles/gles_context.h"
#include "gles/gles_utils.h"
#include "gles/gles_validate.h"
#include "gles/macros.h"
#include "gles/paletted_texture_util.h"
#include "gles/program_data.h"
#include "gles/renderbuffer_data.h"
#include "gles/shader_data.h"
#include "gles/texture_codecs.h"
#include "gles/texture_data.h"

typedef GlesContext* ContextPtr;

namespace {

static const size_t kMaxParamElementSize = 16;

enum ParamType {
  kParamTypeArray,
  kParamTypeScalar,
};

enum HandlingKind {
  kHandlingKindInvalid = 0,
  kHandlingKindLocalCopy = 1 << 0,    // Track capability in context.
  kHandlingKindTexture = 1 << 1,      // Track capability in texture context.
  kHandlingKindUniform = 1 << 2,      // Track capability in uniform context.
  kHandlingKindIgnored = 1 << 3,      // Unsupported capability, but can be
                                      // ignored.
  kHandlingKindUnsupported = 1 << 4,  // Unsupported capability.
  kHandlingKindPropagate = 1 << 5,    // Pass through to implementation.
};

// The calls to Enable and Disable are both handled very similarly, and we can
// eliminate some code duplication by having a common set of logic that knows
// how each capability should be handled.
int GetCapabilityHandlingKind(GLenum cap) {
  switch (cap) {
    // These capabilities represent state we do not need to emulate.
    case GL_BLEND:
    case GL_CULL_FACE:
    case GL_DEPTH_TEST:
    case GL_DITHER:
    case GL_POLYGON_OFFSET_FILL:
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
    case GL_SAMPLE_COVERAGE:
    case GL_SCISSOR_TEST:
    case GL_STENCIL_TEST:
      return kHandlingKindPropagate | kHandlingKindLocalCopy;

    // These capabilities represent GLES1 state we must emulate under GLES2.
    case GL_ALPHA_TEST:
    case GL_CLIP_PLANE0:
    case GL_CLIP_PLANE1:
    case GL_CLIP_PLANE2:
    case GL_CLIP_PLANE3:
    case GL_CLIP_PLANE4:
    case GL_CLIP_PLANE5:
    case GL_COLOR_MATERIAL:
    case GL_FOG:
    case GL_LIGHTING:
    case GL_LIGHT0:
    case GL_LIGHT1:
    case GL_LIGHT2:
    case GL_LIGHT3:
    case GL_LIGHT4:
    case GL_LIGHT5:
    case GL_LIGHT6:
    case GL_LIGHT7:
    case GL_MATRIX_PALETTE_OES:
    case GL_NORMALIZE:
    case GL_POINT_SMOOTH:
    case GL_POINT_SPRITE_OES:
    case GL_RESCALE_NORMAL:
      return kHandlingKindLocalCopy;

    case GL_TEXTURE_GEN_STR_OES:
      return kHandlingKindUniform;

    case GL_TEXTURE_2D:
    case GL_TEXTURE_EXTERNAL_OES:
    case GL_TEXTURE_CUBE_MAP:
      return kHandlingKindTexture;

    // GL_LINE_SMOOTH is not supported, but ignore it for now.
    case GL_LINE_SMOOTH:
      return kHandlingKindIgnored | kHandlingKindUnsupported;

    // These capabilities represent GLES1 state which are not supported for
    // now.  It is an error to try to enable these, but disabling them or
    // testing for them is okay.
    case GL_COLOR_LOGIC_OP:
    case GL_MULTISAMPLE:
      return kHandlingKindUnsupported;

    default:
      return kHandlingKindInvalid;
  }
}

// Determines the number of elements for a given parameter.
size_t ParamSize(GLenum param) {
  switch (param) {
    case GL_ALPHA_TEST_FUNC:
    case GL_ALPHA_TEST_REF:
    case GL_BLEND_DST:
    case GL_BLEND_SRC:
    case GL_CONSTANT_ATTENUATION:
    case GL_CULL_FACE_MODE:
    case GL_DEPTH_CLEAR_VALUE:
    case GL_DEPTH_WRITEMASK:
    case GL_FOG_DENSITY:
    case GL_FOG_END:
    case GL_FOG_MODE:
    case GL_FOG_START:
    case GL_FRONT_FACE:
    case GL_GENERATE_MIPMAP:
    case GL_LIGHT_MODEL_TWO_SIDE:
    case GL_LINEAR_ATTENUATION:
    case GL_LOGIC_OP_MODE:
    case GL_MATRIX_MODE:
    case GL_MAX_TEXTURE_SIZE:
    case GL_MAX_TEXTURE_UNITS:
    case GL_MODELVIEW_STACK_DEPTH:
    case GL_POINT_FADE_THRESHOLD_SIZE:
    case GL_POINT_SIZE:
    case GL_POINT_SIZE_MAX:
    case GL_POINT_SIZE_MIN:
    case GL_PROJECTION_STACK_DEPTH:
    case GL_QUADRATIC_ATTENUATION:
    case GL_SCISSOR_TEST:
    case GL_SHADE_MODEL:
    case GL_SHININESS:
    case GL_SPOT_EXPONENT:
    case GL_STENCIL_FAIL:
    case GL_STENCIL_PASS_DEPTH_FAIL:
    case GL_STENCIL_PASS_DEPTH_PASS:
    case GL_STENCIL_REF:
    case GL_STENCIL_WRITEMASK:
    case GL_TEXTURE_ENV_MODE:
    case GL_TEXTURE_MAG_FILTER:
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_STACK_DEPTH:
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
      return 1;
    case GL_ALIASED_LINE_WIDTH_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_DEPTH_RANGE:
    case GL_MAX_VIEWPORT_DIMS:
    case GL_SMOOTH_LINE_WIDTH_RANGE:
    case GL_SMOOTH_POINT_SIZE_RANGE:
      return 2;
    case GL_CURRENT_NORMAL:
    case GL_POINT_DISTANCE_ATTENUATION:
    case GL_SPOT_DIRECTION:
      return 3;
    case GL_AMBIENT:
    case GL_CURRENT_COLOR:
    case GL_CURRENT_TEXTURE_COORDS:
    case GL_DIFFUSE:
    case GL_EMISSION:
    case GL_FOG_COLOR:
    case GL_LIGHT_MODEL_AMBIENT:
    case GL_POSITION:
    case GL_SCISSOR_BOX:
    case GL_SPECULAR:
    case GL_TEXTURE_CROP_RECT_OES:
    case GL_TEXTURE_ENV_COLOR:
    case GL_VIEWPORT:
      return 4;
    case GL_MODELVIEW_MATRIX:
    case GL_PROJECTION_MATRIX:
    case GL_TEXTURE_MATRIX:
      return 16;
    default:
      LOG_ALWAYS_FATAL("Unknown parameter name: %s (%x)", GetEnumString(param),
                       param);
      return 1;
  }
}

void Convert(GLfloat* data, size_t num, const GLfixed* params) {
  for (size_t i = 0; i < num; ++i) {
    data[i] = X2F(params[i]);
  }
}

void Convert(GLfixed* data, size_t num, const GLfloat* params) {
  for (size_t i = 0; i < num; ++i) {
    data[i] = F2X(params[i]);
  }
}

GLuint ArrayEnumToIndex(const ContextPtr& c, GLenum array) {
  switch (array) {
    case GL_VERTEX_ARRAY:
    case GL_VERTEX_ARRAY_POINTER:
      return kPositionVertexAttribute;
    case GL_NORMAL_ARRAY:
    case GL_NORMAL_ARRAY_POINTER:
      return kNormalVertexAttribute;
    case GL_COLOR_ARRAY:
    case GL_COLOR_ARRAY_POINTER:
      return kColorVertexAttribute;
    case GL_POINT_SIZE_ARRAY_OES:
    case GL_POINT_SIZE_ARRAY_POINTER_OES:
      return kPointSizeVertexAttribute;
    case GL_TEXTURE_COORD_ARRAY:
    case GL_TEXTURE_COORD_ARRAY_POINTER:
      return c->texture_context_.GetClientActiveTextureCoordAttrib();
    case GL_WEIGHT_ARRAY_OES:
    case GL_WEIGHT_ARRAY_POINTER_OES:
      return kWeightVertexAttribute;
    case GL_MATRIX_INDEX_ARRAY_OES:
    case GL_MATRIX_INDEX_ARRAY_POINTER_OES:
      return kMatrixIndexVertexAttribute;
    default:
      LOG_ALWAYS_FATAL("Unknown array %s(0x%x)", GetEnumString(array), array);
  }
}

}  // namespace

// Selects the server texture unit state that will be modified by server
// texture state calls.
GLES_APIENTRY(void, ActiveTexture, GLenum texture) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }
  if (!c->uniform_context_.SetActiveTexture(texture)) {
    GLES_ERROR_INVALID_ENUM(texture);
  }
  if (!c->texture_context_.SetActiveTexture(texture)) {
    GLES_ERROR_INVALID_ENUM(texture);
  }
  PASS_THROUGH(c, ActiveTexture, texture);
}

// Discard pixel writes based on an alpha value test.
GLES_APIENTRY(void, AlphaFunc, GLenum func, GLclampf ref) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidAlphaFunc(func)) {
    GLES_ERROR_INVALID_ENUM(func);
    return;
  }

  AlphaTest& alpha = c->uniform_context_.MutateAlphaTest();
  alpha.func = func;
  alpha.value = ClampValue(ref, 0.f, 1.f);
}

GLES_APIENTRY(void, AlphaFuncx, GLenum func, GLclampx ref) {
  glAlphaFunc(func, X2F(ref));
}

GLES_APIENTRY(void, AlphaFuncxOES, GLenum func, GLclampx ref) {
  glAlphaFunc(func, X2F(ref));
}

// Attaches a shader object to a program object.
GLES_APIENTRY(void, AttachShader, GLuint program, GLuint shader) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ShaderDataPtr shader_data = sg->GetShaderData(shader);
  if (shader_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Invalid shader %d", shader);
    return;
  }
  ProgramDataPtr program_data = sg->GetProgramData(program);
  if (program_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
    return;
  }

  program_data->AttachShader(shader_data);
}

// Requests a named shader attribute be assigned a particular attribute index.
GLES_APIENTRY(void, BindAttribLocation, GLuint program, GLuint index,
              const GLchar* name) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ProgramDataPtr program_data = sg->GetProgramData(program);
  if (program_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
    return;
  }

  program_data->BindAttribLocation(index, name);
}

// Bind the specified buffer as vertex buffer or index buffer.
GLES_APIENTRY(void, BindBuffer, GLenum target, GLuint buffer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidBufferTarget(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();

  BufferDataPtr obj = sg->GetBufferData(buffer);
  if (obj == NULL && buffer != 0) {
    sg->CreateBufferData(buffer);
  }

  if (target == GL_ARRAY_BUFFER) {
    c->array_buffer_binding_ = buffer;
  } else {
    c->element_buffer_binding_ = buffer;
  }
  PASS_THROUGH(c, BindBuffer, target, sg->GetBufferGlobalName(buffer));
}

// Specify framebuffer for off-screen rendering.
GLES_APIENTRY(void, BindFramebuffer, GLenum target, GLuint framebuffer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidFramebufferTarget(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();

  FramebufferDataPtr obj = sg->GetFramebufferData(framebuffer);
  if (obj == NULL && framebuffer != 0) {
    sg->CreateFramebufferData(framebuffer);
  }
  c->BindFramebuffer(framebuffer);
}

GLES_APIENTRY(void, BindFramebufferOES, GLenum target, GLuint framebuffer) {
  glBindFramebuffer(target, framebuffer);
}

// Specify the renderbuffer that will be modified by renderbuffer calls.
GLES_APIENTRY(void, BindRenderbuffer, GLenum target, GLuint renderbuffer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidRenderbufferTarget(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();

  RenderbufferDataPtr obj = sg->GetRenderbufferData(renderbuffer);
  if (obj == NULL && renderbuffer != 0) {
    sg->CreateRenderbufferData(renderbuffer);
  }

  c->renderbuffer_binding_ = renderbuffer;
  const GLuint global_name = sg->GetRenderbufferGlobalName(renderbuffer);
  PASS_THROUGH(c, BindRenderbuffer, GL_RENDERBUFFER, global_name);
}

GLES_APIENTRY(void, BindRenderbufferOES, GLenum target, GLuint renderbuffer) {
  glBindRenderbuffer(target, renderbuffer);
}

// This call makes the given texture name the current texture of
// kind "target" (which is usually GL_TEXTURE_2D) which later
// environment/parameter calls will reference, and S/T texture
// mapping coordinates apply to.
GLES_APIENTRY(void, BindTexture, GLenum target, GLuint texture) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidTextureTarget(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();

  bool first_use = false;
  TextureDataPtr tex;

  // Handle special-case 0 texture.
  if (texture == 0) {
    tex = c->texture_context_.GetDefaultTextureData(target);
  } else {
    tex = sg->GetTextureData(texture);
    if (tex == NULL) {
      tex = sg->CreateTextureData(texture);
      first_use = true;
    }
  }
  LOG_ALWAYS_FATAL_IF(tex == NULL);
  if (!c->texture_context_.BindTextureToTarget(tex, target)) {
    GLES_ERROR(GL_INVALID_OPERATION, "Texture cannot be bound to target: %s",
               GetEnumString(target));
  }

  // Most textures use the same global texture target as used by the caller.
  // There is special class of exceptions -- external images targets, which
  // could be any target (potentially not even a normally valid GLES2 target!)
  bool init_external_oes_as_2d = false;
  GLenum global_texture_target = target;
  if (tex->IsEglImageAttached()) {
    global_texture_target = tex->GetAttachedEglImage()->global_texture_target;
  } else if (target == GL_TEXTURE_EXTERNAL_OES) {
    // It is perfectly acceptable but problematic here to call
    // BindTexture(TEXTURE_EXTERNAL_OES, some_texture) before calling
    // EGLImageTargetTexture2DOES(). Since we are emulating
    // TEXTURE_EXTERNAL_OES, we do not want to pass it on to the underlying
    // implementation. So we assume that the global target should be TEXTURE_2D
    // for this case. If for some reason that is not the actual global target,
    // we will fix it up in handling the EGLImageTargetTexture2DOES call.
    global_texture_target = GL_TEXTURE_2D;
    init_external_oes_as_2d = first_use;
  }
  c->texture_context_.SetTargetTexture(target, texture, global_texture_target);

  if (init_external_oes_as_2d) {
    // When a new TEXTURE_2D is used to back the TEXTURE_EXTERNAL_OES,
    // we need to set the default texture state to match that default state
    // required by the extension, where different from the defaults for a
    // TEXTURE_2D.
    const GLenum global_target =
        c->texture_context_.EnsureCorrectBinding(target);
    LOG_ALWAYS_FATAL_IF(global_target != GL_TEXTURE_2D);
    PASS_THROUGH(c, TexParameteri, global_target, GL_TEXTURE_MIN_FILTER,
                 GL_LINEAR);
    PASS_THROUGH(c, TexParameteri, global_target, GL_TEXTURE_WRAP_S,
                 GL_CLAMP_TO_EDGE);
    PASS_THROUGH(c, TexParameteri, global_target, GL_TEXTURE_WRAP_T,
                 GL_CLAMP_TO_EDGE);
    c->texture_context_.RestoreBinding(target);
  }

  // Only immediately bind the texture to the target if the global texture
  // target matches the local target. If we are remapping the texture target,
  // we will not know until later which texture should be bound.
  if (target == global_texture_target) {
    const GLuint global_texture_name =
        c->GetShareGroup()->GetTextureGlobalName(texture);
    PASS_THROUGH(c, BindTexture, target, global_texture_name);
  }
}

// Used in compositing the fragment shader output with the framebuffer.
GLES_APIENTRY(void, BlendColor, GLfloat red, GLfloat green, GLfloat blue,
              GLfloat alpha) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, BlendColor, red, green, blue, alpha);
}

// Used in compositing the fragment shader output with the framebuffer.
GLES_APIENTRY(void, BlendEquation, GLenum mode) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, BlendEquation, mode);
}

GLES_APIENTRY(void, BlendEquationOES, GLenum mode) {
  glBlendEquation(mode);
}

// Used in compositing the fragment shader output with the framebuffer.
GLES_APIENTRY(void, BlendEquationSeparate, GLenum modeRGB, GLenum modeAlpha) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, BlendEquationSeparate, modeRGB, modeAlpha);
}

GLES_APIENTRY(void, BlendEquationSeparateOES, GLenum modeRGB,
              GLenum modeAlpha) {
  glBlendEquationSeparate(modeRGB, modeAlpha);
}

// Used in compositing the fragment shader output with the framebuffer.
GLES_APIENTRY(void, BlendFunc, GLenum sfactor, GLenum dfactor) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidBlendFunc(sfactor)) {
    GLES_ERROR_INVALID_ENUM(sfactor);
    return;
  }
  if (!IsValidBlendFunc(dfactor)) {
    GLES_ERROR_INVALID_ENUM(dfactor);
    return;
  }

  c->blend_func_src_alpha_.Mutate() = sfactor;
  c->blend_func_src_rgb_.Mutate() = sfactor;
  c->blend_func_dst_alpha_.Mutate() = dfactor;
  c->blend_func_dst_rgb_.Mutate() = dfactor;
  PASS_THROUGH(c, BlendFunc, sfactor, dfactor);
}

// Used in compositing the fragment shader output with the framebuffer.
GLES_APIENTRY(void, BlendFuncSeparate, GLenum srcRGB, GLenum dstRGB,
              GLenum srcAlpha, GLenum dstAlpha) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidBlendFunc(srcRGB)) {
    GLES_ERROR_INVALID_ENUM(srcRGB);
    return;
  }
  if (!IsValidBlendFunc(dstRGB)) {
    GLES_ERROR_INVALID_ENUM(dstRGB);
    return;
  }
  if (!IsValidBlendFunc(srcAlpha)) {
    GLES_ERROR_INVALID_ENUM(srcAlpha);
    return;
  }
  if (!IsValidBlendFunc(dstAlpha)) {
    GLES_ERROR_INVALID_ENUM(dstAlpha);
    return;
  }

  c->blend_func_src_alpha_.Mutate() = srcAlpha;
  c->blend_func_src_rgb_.Mutate() = srcRGB;
  c->blend_func_dst_alpha_.Mutate() = dstAlpha;
  c->blend_func_dst_rgb_.Mutate() = dstRGB;
  PASS_THROUGH(c, BlendFuncSeparate, srcRGB, dstRGB, srcAlpha, dstAlpha);
}

GLES_APIENTRY(void, BlendFuncSeparateOES, GLenum srcRGB, GLenum dstRGB,
              GLenum srcAlpha, GLenum dstAlpha) {
  glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

// Loads arbitrary binary data into a buffer.
GLES_APIENTRY(void, BufferData, GLenum target, GLsizeiptr size,
              const GLvoid* data, GLenum usage) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidBufferTarget(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }
  if (!IsValidBufferUsage(usage)) {
    GLES_ERROR_INVALID_ENUM(usage);
    return;
  }
  if (size < 0) {
    GLES_ERROR_INVALID_VALUE_UINT((GLuint)size);
    return;
  }
  BufferDataPtr vbo = c->GetBoundTargetBufferData(target);
  if (vbo == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Buffer not bound.");
    return;
  }
  vbo->SetBufferData(target, size, data, usage);
  PASS_THROUGH(c, BufferData, target, size, data, usage);
}

// Loads arbitrary binary data into a portion of a buffer.
GLES_APIENTRY(void, BufferSubData, GLenum target, GLintptr offset,
              GLsizeiptr size, const GLvoid* data) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidBufferTarget(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }
  if (size < 0) {
    GLES_ERROR_INVALID_VALUE_UINT((GLuint)size);
    return;
  }
  if (offset < 0) {
    GLES_ERROR_INVALID_VALUE_UINT((GLuint)offset);
    return;
  }
  BufferDataPtr vbo = c->GetBoundTargetBufferData(target);
  if (vbo == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Buffer not bound.");
    return;
  }

  if (static_cast<GLuint>(offset + size) > vbo->GetSize()) {
    GLES_ERROR(GL_INVALID_VALUE, "Unable to set buffer subdata.");
    return;
  }
  vbo->SetBufferSubData(target, offset, size, data);
  PASS_THROUGH(c, BufferSubData, target, offset, size, data);
}

// Verifies if a framebuffer object is set up correctly.
GLES_APIENTRY(GLenum, CheckFramebufferStatus, GLenum target) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return GL_FRAMEBUFFER_UNSUPPORTED;
  }
  return PASS_THROUGH(c, CheckFramebufferStatus, target);
}

GLES_APIENTRY(GLenum, CheckFramebufferStatusOES, GLenum target) {
  return glCheckFramebufferStatus(target);
}

// Clears the framebuffer to the current clear color.
GLES_APIENTRY(void, Clear, GLbitfield mask) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }
  c->EnsureSurfaceReadyToDraw();
  PASS_THROUGH(c, Clear, mask);
}

// Sets the color to use when clearing the framebuffer.
GLES_APIENTRY(void, ClearColor, GLclampf red, GLclampf green, GLclampf blue,
              GLclampf alpha) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  red = ClampValue(red, 0.f, 1.f);
  green = ClampValue(green, 0.f, 1.f);
  blue = ClampValue(blue, 0.f, 1.f);
  alpha = ClampValue(alpha, 0.f, 1.f);

  GLfloat (&color_clear_value)[4] = c->color_clear_value_.Mutate();
  color_clear_value[0] = red;
  color_clear_value[1] = green;
  color_clear_value[2] = blue;
  color_clear_value[3] = alpha;
  PASS_THROUGH(c, ClearColor, red, green, blue, alpha);
}

GLES_APIENTRY(void, ClearColorx, GLclampx red, GLclampx green, GLclampx blue,
              GLclampx alpha) {
  glClearColor(X2F(red), X2F(green), X2F(blue), X2F(alpha));
}

GLES_APIENTRY(void, ClearColorxOES, GLclampx red, GLclampx green,
              GLclampx blue, GLclampx alpha) {
  glClearColor(X2F(red), X2F(green), X2F(blue), X2F(alpha));
}

// Sets the depth value to use when clearing the framebuffer.
GLES_APIENTRY(void, ClearDepthf, GLfloat depth) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  depth = ClampValue(depth, 0.f, 1.f);

  c->depth_clear_value_.Mutate() = depth;
  PASS_THROUGH(c, ClearDepthf, depth);
}

GLES_APIENTRY(void, ClearDepthfOES, GLclampf depth) {
  glClearDepthf(depth);
}

GLES_APIENTRY(void, ClearDepthx, GLclampx depth) {
  glClearDepthf(X2F(depth));
}

GLES_APIENTRY(void, ClearDepthxOES, GLclampx depth) {
  glClearDepthf(X2F(depth));
}

// Sets the stencil bit pattern to use when clearing the framebuffer.
GLES_APIENTRY(void, ClearStencil, GLint s) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, ClearStencil, s);
}

// Selects the client texture unit state that will be modified by client
// texture state calls.
GLES_APIENTRY(void, ClientActiveTexture, GLenum texture) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!c->texture_context_.SetClientActiveTexture(texture)) {
    GLES_ERROR_INVALID_ENUM(texture);
  }
}

// Specify a plane against which all geometry is clipped.
GLES_APIENTRY(void, ClipPlanef, GLenum pname, const GLfloat* equation) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  emugl::Vector* plane = c->uniform_context_.MutateClipPlane(pname);
  if (!plane) {
    GLES_ERROR_INVALID_ENUM(pname);
    return;
  }

  // Convert the plane equations coefficients into eye coordinates.  See
  // es_full_spec.1.1.12.pdf section 2.11.  We also require the transpose
  // due to how we store the matrices.
  emugl::Matrix mv = c->uniform_context_.GetModelViewMatrix();
  mv.Inverse();
  mv.Transpose();
  emugl::Vector value(equation[0], equation[1], equation[2], equation[3]);
  plane->AssignMatrixMultiply(mv, value);
}

GLES_APIENTRY(void, ClipPlanefOES, GLenum pname, const GLfloat* equation) {
  glClipPlanef(pname, equation);
}

GLES_APIENTRY(void, ClipPlanex, GLenum pname, const GLfixed* equation) {
  static const size_t kNumElements = 4;
  GLfloat tmp[kNumElements];
  Convert(tmp, kNumElements, equation);
  glClipPlanef(pname, tmp);
}

GLES_APIENTRY(void, ClipPlanexOES, GLenum pname, const GLfixed* equation) {
  glClipPlanex(pname, equation);
}

// Sets the vertex color that is used when no vertex color array is enabled.
GLES_APIENTRY(void, Color4f, GLfloat red, GLfloat green, GLfloat blue,
              GLfloat alpha) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  emugl::Vector& color = c->uniform_context_.MutateColor();
  color = emugl::Vector(red, green, blue, alpha);
}

GLES_APIENTRY(void, Color4fv, const GLfloat* components) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  emugl::Vector& color = c->uniform_context_.MutateColor();
  color.AssignLinearMapping(components, 4);
}

GLES_APIENTRY(void, Color4ub, GLubyte red, GLubyte green, GLubyte blue,
              GLubyte alpha) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  emugl::Vector& color = c->uniform_context_.MutateColor();
  GLubyte params[4] = {red, green, blue, alpha};
  color.AssignLinearMapping(params, 4);
}

GLES_APIENTRY(void, Color4ubv, const GLubyte* components) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  emugl::Vector& color = c->uniform_context_.MutateColor();
  color.AssignLinearMapping(components, 4);
}

GLES_APIENTRY(void, Color4x, GLfixed red, GLfixed green, GLfixed blue,
              GLfixed alpha) {
  glColor4f(X2F(red), X2F(green), X2F(blue), X2F(alpha));
}

GLES_APIENTRY(void, Color4xOES, GLfixed red, GLfixed green, GLfixed blue,
              GLfixed alpha) {
  glColor4f(X2F(red), X2F(green), X2F(blue), X2F(alpha));
}

// Controls what components output from the fragment shader are written to the
// framebuffer.
GLES_APIENTRY(void, ColorMask, GLboolean red, GLboolean green, GLboolean blue,
              GLboolean alpha) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  GLboolean (&color_writemask)[4] = c->color_writemask_.Mutate();
  color_writemask[0] = red != GL_FALSE ? GL_TRUE : GL_FALSE;
  color_writemask[1] = green != GL_FALSE ? GL_TRUE : GL_FALSE;
  color_writemask[2] = blue != GL_FALSE ? GL_TRUE : GL_FALSE;
  color_writemask[3] = alpha != GL_FALSE ? GL_TRUE : GL_FALSE;
  PASS_THROUGH(c, ColorMask, red, green, blue, alpha);
}

// Specifies source array for per-vertex colors.
GLES_APIENTRY(void, ColorPointer, GLint size, GLenum type, GLsizei stride,
              const GLvoid* pointer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidColorPointerSize(size)) {
    GLES_ERROR_INVALID_VALUE_INT(size);
    return;
  }
  if (!IsValidColorPointerType(type)) {
    GLES_ERROR_INVALID_ENUM(type);
    return;
  }
  if (stride < 0) {
    GLES_ERROR_INVALID_VALUE_INT(stride);
    return;
  }
  const GLboolean normalized = (type != GL_FLOAT && type != GL_FIXED);
  c->pointer_context_.SetPointer(kColorVertexAttribute, size, type, stride,
                                 pointer, normalized);
}

GLES_APIENTRY(void, ColorPointerBounds, GLint size, GLenum type, GLsizei stride,
              const GLvoid* pointer, GLsizei count) {
  glColorPointer(size, type, stride, pointer);
}

// Compiles a shader from the source code loaded into the shader object.
GLES_APIENTRY(void, CompileShader, GLuint shader) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ShaderDataPtr shader_data = sg->GetShaderData(shader);
  if (shader_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Not a valid shader %d", shader);
    return;
  }

  shader_data->Compile();
}

namespace {

void HandleETC1CompressedTexImage2D(
    GLenum target, GLint level, GLenum internalformat, GLsizei width,
    GLsizei height, GLint border, GLsizei image_size, const GLvoid* data) {
  ContextPtr c = GetCurrentGlesContext();
  ALOG_ASSERT(c != NULL);

  if (level < 0 || level > c->texture_context_.GetMaxLevels()) {
    GLES_ERROR_INVALID_VALUE_INT(level);
    return;
  }

  const TextureDataPtr tex = c->GetBoundTextureData(target);
  if (tex == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "No texture bound.");
    return;
  }

  size_t expected_size = etc1_get_encoded_data_size(width, height);
  if (static_cast<size_t>(image_size) < expected_size) {
    ALOGE("Expected to be given %d bytes of compressed data, but actually "
          "given %d bytes.", static_cast<int>(expected_size), image_size);
    GLES_ERROR_INVALID_VALUE_INT(image_size);
    return;
  }

  const GLint unpack_alignment = c->pixel_store_unpack_alignment_.Get();
  PASS_THROUGH(c, PixelStorei, GL_UNPACK_ALIGNMENT, 1);

  tex->Set(level, width, height, GL_RGB, GL_UNSIGNED_BYTE);

  const size_t bytes_per_pixel = 3;
  const size_t stride = bytes_per_pixel * width;
  PASS_THROUGH(c, TexImage2D, target, level, GL_RGB, width,
               height, border, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  GLvoid* buffer = PASS_THROUGH(c, MapTexSubImage2DCHROMIUM, target, level, 0,
                                0, width, height, GL_RGB, GL_UNSIGNED_BYTE,
                                GL_WRITE_ONLY_OES);
  const int rc = etc1_decode_image(static_cast<const etc1_byte*>(data),
                                   static_cast<etc1_byte*>(buffer), width,
                                   height, bytes_per_pixel, stride);
  ALOG_ASSERT(rc == 0);
  PASS_THROUGH(c, UnmapTexSubImage2DCHROMIUM, buffer);
  PASS_THROUGH(c, PixelStorei, GL_UNPACK_ALIGNMENT, unpack_alignment);
}

void HandlePalettedCompressedTexImage2D(
    GLenum target, GLint level, GLenum internalformat, GLsizei width,
    GLsizei height, GLint border, GLsizei image_size, const GLvoid* data) {

  ContextPtr c = GetCurrentGlesContext();
  ALOG_ASSERT(c != NULL);

  GLsizei supplied_levels = -level + 1;
  if (level > 0 || supplied_levels > c->texture_context_.GetMaxLevels()) {
    // For paletted formats, the level parameter must not be positive, and when
    // the |level|+1 is computed, must not exceed the maximum allowed levels.
    // Zero means that only the base mip map level is being set, and every
    // number lower means that one more mipmap level is also being set.
    // For example -3 would mean 4 mipmap levels are being set (levels 0-3)
    GLES_ERROR_INVALID_VALUE_INT(level);
    return;
  }

  const TextureDataPtr tex = c->GetBoundTextureData(target);
  if (tex == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "No texture bound.");
    return;
  }

  const size_t image_bpp =
      PalettedTextureUtil::GetImageBpp(internalformat);
  const size_t palette_entry_size =
      PalettedTextureUtil::GetEntrySizeBytes(internalformat);
  const GLenum palette_format =
      PalettedTextureUtil::GetEntryFormat(internalformat);
  const GLenum palette_type =
      PalettedTextureUtil::GetEntryType(internalformat);
  const size_t palette_size =
      PalettedTextureUtil::ComputePaletteSize(
          image_bpp, palette_entry_size);
  const size_t level0_size =
      PalettedTextureUtil::ComputeLevel0Size(
          width, height, image_bpp);
  const size_t expected_size =
      PalettedTextureUtil::ComputeTotalSize(
          palette_size, level0_size, supplied_levels);

  if (static_cast<size_t>(image_size) < expected_size) {
    ALOGE("Expected to be given %d bytes of compressed data, but actually "
          "given %d bytes.", static_cast<int>(expected_size), image_size);
    GLES_ERROR_INVALID_VALUE_INT(image_size);
    return;
  }

  const uint8_t* const palette_data = static_cast<const uint8_t*>(data);
  const uint8_t* image_data = palette_data + palette_size;

  // We require a minimum uncompressed buffer size of 2 pixels here to handle
  // the corner case of a 1 x 1 4bpp image, which requires only 4 bits of
  // storage, but actually ends up taking a full byte. The code ends up
  // unpacking both the real 4 bit value as well as the "padding" 4 bits.
  std::vector<uint8_t> uncompressed_level(std::min(2, width * height) *
                                          palette_entry_size);

  const GLint unpack_alignment = c->pixel_store_unpack_alignment_.Get();
  PASS_THROUGH(c, PixelStorei, GL_UNPACK_ALIGNMENT, 1);
  for (size_t i = 0; i < static_cast<size_t>(supplied_levels); ++i) {
    const int level_width = width >> i;
    const int level_height = height >> i;
    const int level_size =
        PalettedTextureUtil::ComputeLevelSize(level0_size, i);

    tex->Set(i, level_width, level_height, palette_format, palette_type);

    PASS_THROUGH(c, TexImage2D, target, i, palette_format, level_width,
                 level_height, border, palette_format, palette_type,
                 NULL);
    GLvoid* buffer = PASS_THROUGH(c, MapTexSubImage2DCHROMIUM, target, i, 0,
                                  0, level_width, level_height, palette_format,
                                  palette_type, GL_WRITE_ONLY_OES);
    uint8_t* dst = static_cast<uint8_t*>(buffer);
    dst = PalettedTextureUtil::Decompress(
        image_bpp, level_size, palette_entry_size, image_data, palette_data,
        dst);
    ALOG_ASSERT(dst < static_cast<uint8_t*>(buffer) + level_size);
    PASS_THROUGH(c, UnmapTexSubImage2DCHROMIUM, buffer);
    image_data += level_size;
  }
  ALOG_ASSERT(image_data - image_size <= data);
  PASS_THROUGH(c, PixelStorei, GL_UNPACK_ALIGNMENT, unpack_alignment);
}

void HandleEmulateCompressedTexImage2D(
    GLenum target, GLint level, GLenum internalformat, GLsizei width,
    GLsizei height, GLint border, GLsizei image_size, const GLvoid* data) {

  ContextPtr c = GetCurrentGlesContext();
  ALOG_ASSERT(c != NULL);

  if (width < 0 || width > c->max_texture_size_.Get() || !IsPowerOf2(width)) {
    GLES_ERROR_INVALID_VALUE_INT(width);
    return;
  }
  if (height < 0 || height > c->max_texture_size_.Get() || !IsPowerOf2(height)) {
    GLES_ERROR_INVALID_VALUE_INT(height);
    return;
  }
  if (border != 0) {
    GLES_ERROR_INVALID_VALUE_INT(border);
    return;
  }
  if (!IsValidEmulatedCompressedTextureFormats(internalformat)) {
    GLES_ERROR_INVALID_ENUM(internalformat);
    return;
  }

  if (internalformat == GL_ETC1_RGB8_OES) {
    HandleETC1CompressedTexImage2D(target, level, internalformat, width,
                                   height, border, image_size, data);
  } else {
    HandlePalettedCompressedTexImage2D(target, level, internalformat, width,
                                       height, border, image_size, data);
  }
}

}  // namespace

GLES_APIENTRY(void, CompressedTexImage2D, GLenum target, GLint level,
              GLenum internalformat, GLsizei width, GLsizei height,
              GLint border, GLsizei image_size, const GLvoid* data) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  // TODO(crbug.com/441913): Record that this texture level uses a compressed
  // format, as well as other data we track for this level and this texture.
  if (c->IsCompressedFormatEmulationNeeded(internalformat)) {
    HandleEmulateCompressedTexImage2D(target, level, internalformat, width,
                                      height, border, image_size, data);
    return;
  }
  PASS_THROUGH(c, CompressedTexImage2D, target, level, internalformat, width,
               height, border, image_size, data);
}

GLES_APIENTRY(void, CompressedTexSubImage2D, GLenum target, GLint level,
              GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
              GLenum format, GLsizei image_size, const GLvoid* data) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (c->IsCompressedFormatEmulationNeeded(format)) {
    GLES_ERROR(GL_INVALID_OPERATION, "Not supported for paletted formats.");
    return;
  }
  PASS_THROUGH(c, CompressedTexSubImage2D, target, level, xoffset, yoffset,
               width, height, format, image_size, data);
}

// Extracts a portion of the framebuffer, loading it into the currently bound
// texture.
GLES_APIENTRY(void, CopyTexImage2D, GLenum target, GLint level,
                   GLenum internalformat, GLint x, GLint y, GLsizei width,
                   GLsizei height, GLint border) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  LOG_ALWAYS_FATAL_IF(target == GL_TEXTURE_EXTERNAL_OES);

  // TODO(crbug.com/441914): GL_INVALID_OPERATION if the framebuffer format is
  // not a superset of internalformat.

  // TODO(crbug.com/441914): GL_INVALID_FRAMEBUFFER_OPERATION if the
  // framebuffer is not complete (ie. glCheckFramebufferStatus is not
  // GL_FRAMEBUFFER_COMPLETE).

  // TODO(crbug.com/441914): The framebuffer may be a sixteen bit type and the
  // logic here assumes that it will always be GL_UNSIGNED_BYTE.
  GLenum type = GL_UNSIGNED_BYTE;
  if (!IsValidPixelFormatAndType(internalformat, type)) {
    GLES_ERROR(GL_INVALID_ENUM, "Invalid format/type: %s %s",
                GetEnumString(internalformat), GetEnumString(type));
    return;
  }

  if (level < 0 || level > c->texture_context_.GetMaxLevels()) {
    GLES_ERROR_INVALID_VALUE_INT(level);
    return;
  }
  if (width < 0 || width > c->max_texture_size_.Get()) {
    GLES_ERROR_INVALID_VALUE_INT(width);
    return;
  }
  if (height < 0 || height > c->max_texture_size_.Get()) {
    GLES_ERROR_INVALID_VALUE_INT(height);
    return;
  }
  if (border != 0) {
    GLES_ERROR_INVALID_VALUE_INT(border);
    return;
  }

  TextureDataPtr tex = c->GetBoundTextureData(target);
  if (tex == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "No texture bound.");
    return;
  }

  tex->Set(level, width, height, internalformat, type);

  PASS_THROUGH(c, CopyTexImage2D, target, level, internalformat, x, y, width,
               height, border);
}

// Extracts a portion of another texture, replacing the current bound texture.
GLES_APIENTRY(void, CopyTexSubImage2D, GLenum target, GLint level,
              GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width,
              GLsizei height) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  LOG_ALWAYS_FATAL_IF(target == GL_TEXTURE_EXTERNAL_OES);

  PASS_THROUGH(c, CopyTexSubImage2D, target, level, xoffset, yoffset, x, y,
               width, height);
}

// Creates and returns a new program object.
GLES_APIENTRY(GLuint, CreateProgram) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return 0;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ObjectLocalName name = 0;
  sg->GenPrograms(1, &name);
  sg->CreateProgramData(name);
  return name;
}

// Creates and returns a new shader object.
GLES_APIENTRY(GLuint, CreateShader, GLenum type) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return 0;
  }

  if (!IsValidShaderType(type)) {
    GLES_ERROR_INVALID_ENUM(type);
    return 0;
  }

  ObjectLocalName name = 0;
  ShareGroupPtr sg = c->GetShareGroup();
  if (type == GL_VERTEX_SHADER) {
    sg->GenVertexShaders(1, &name);
    sg->CreateVertexShaderData(name);
  } else {
    sg->GenFragmentShaders(1, &name);
    sg->CreateFragmentShaderData(name);
  }
  return name;
}

// Selects which faces are culled (back, front, both) when culling is enabled.
GLES_APIENTRY(void, CullFace, GLenum mode) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidCullFaceMode(mode)) {
    GLES_ERROR_INVALID_ENUM(mode);
    return;
  }

  c->cull_face_mode_.Mutate() = mode;
  PASS_THROUGH(c, CullFace, mode);
}

// Set the current patette matrix.
GLES_APIENTRY(void, CurrentPaletteMatrixOES, GLuint index) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!c->uniform_context_.SetCurrentPaletteMatrix(index)) {
    GLES_ERROR_INVALID_VALUE_UINT(index);
  }
}

// Delete a number of buffer objects.
GLES_APIENTRY(void, DeleteBuffers, GLsizei n, const GLuint* buffers) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (n < 0) {
    GLES_ERROR_INVALID_VALUE_INT(n);
    return;
  }

  for (int i = 0; i < n; ++i) {
    if (c->array_buffer_binding_ == buffers[i]) {
      c->array_buffer_binding_ = 0;
    }
    if (c->element_buffer_binding_ == buffers[i]) {
      c->element_buffer_binding_ = 0;
    }
  }

  ShareGroupPtr sg = c->GetShareGroup();
  sg->DeleteBuffers(n, buffers);
}

// Delete a number of frame buffer objects.
GLES_APIENTRY(void, DeleteFramebuffers, GLsizei n,
              const GLuint* framebuffers) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (n < 0) {
    GLES_ERROR_INVALID_VALUE_INT(n);
    return;
  }

  for (int i = 0; i < n; ++i) {
    if (c->framebuffer_binding_ == framebuffers[i]) {
      c->framebuffer_binding_ = 0;
    }
  }

  ShareGroupPtr sg = c->GetShareGroup();
  sg->DeleteFramebuffers(n, framebuffers);
}

GLES_APIENTRY(void, DeleteFramebuffersOES, GLsizei n,
              const GLuint* framebuffers) {
  glDeleteFramebuffers(n, framebuffers);
}

// Delete a single program object.
GLES_APIENTRY(void, DeleteProgram, GLuint program) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ProgramDataPtr program_data = sg->GetProgramData(program);
  if (program_data == NULL) {
    return;
  }

  if (c->GetCurrentUserProgram() == program_data) {
    // TODO(crbug.com/424353): Do not automatically unuse a deleted program.
    c->SetCurrentUserProgram(ProgramDataPtr());
  }

  sg->DeletePrograms(1, &program);
}

// Delete a number of render buffer objects.
GLES_APIENTRY(void, DeleteRenderbuffers, GLsizei n,
              const GLuint* renderbuffers) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (n < 0) {
    GLES_ERROR_INVALID_VALUE_INT(n);
    return;
  }

  FramebufferDataPtr fb = c->GetBoundFramebufferData();
  for (int i = 0; i < n; ++i) {
    if (fb != NULL) {
      fb->ClearAttachment(renderbuffers[i], false);
    }
    if (c->renderbuffer_binding_ == renderbuffers[i]) {
      c->renderbuffer_binding_ = 0;
    }
  }

  ShareGroupPtr sg = c->GetShareGroup();
  sg->DeleteRenderbuffers(n, renderbuffers);
}

GLES_APIENTRY(void, DeleteRenderbuffersOES, GLsizei n,
              const GLuint* renderbuffers) {
  glDeleteRenderbuffers(n, renderbuffers);
}

// Delete a single shader object.
GLES_APIENTRY(void, DeleteShader, GLuint shader) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  sg->DeleteShaders(1, &shader);
}

// Delete a number of texture objects.
GLES_APIENTRY(void, DeleteTextures, GLsizei n, const GLuint* textures) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (n < 0) {
    GLES_ERROR_INVALID_VALUE_INT(n);
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  FramebufferDataPtr fb = c->GetBoundFramebufferData();
  for (int i = 0; i < n; ++i) {
    if (fb != NULL) {
      fb->ClearAttachment(textures[i], true);
    }
    // Do not delete texture if it is a target of EGLImage.
    TextureDataPtr tex = sg->GetTextureData(textures[i]);
    if (tex != NULL && tex->IsEglImageAttached()) {
      sg->SetTextureGlobalName(tex->GetLocalName(), 0);
    }
    c->texture_context_.DeleteTexture(textures[i]);
  }
  sg->DeleteTextures(n, textures);
}

// Discard pixel writes based on an depth value test.
GLES_APIENTRY(void, DepthFunc, GLenum func) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidDepthFunc(func)) {
    GLES_ERROR_INVALID_ENUM(func);
    return;
  }

  c->depth_func_.Mutate() = func;
  PASS_THROUGH(c, DepthFunc, func);
}

// Sets whether or not the pixel depth value is written to the framebuffer.
GLES_APIENTRY(void, DepthMask, GLboolean flag) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  c->depth_writemask_.Mutate() = flag != GL_FALSE ? GL_TRUE : GL_FALSE;
  PASS_THROUGH(c, DepthMask, flag);
}

// Pixels outside this range are clipped and the fragment depth is normalized
// on write based on them.
GLES_APIENTRY(void, DepthRangef, GLfloat zNear, GLfloat zFar) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  zNear = ClampValue(zNear, 0.f, 1.f);
  zFar = ClampValue(zFar, 0.f, 1.f);

  GLfloat (&depth_range)[2] = c->depth_range_.Mutate();
  depth_range[0] = static_cast<GLfloat>(zNear);
  depth_range[1] = static_cast<GLfloat>(zFar);
  PASS_THROUGH(c, DepthRangef, zNear, zFar);
}

GLES_APIENTRY(void, DepthRangefOES, GLclampf zNear, GLclampf zFar) {
  glDepthRangef(zNear, zFar);
}

GLES_APIENTRY(void, DepthRangex, GLclampx zNear, GLclampx zFar) {
  glDepthRangef(X2F(zNear), X2F(zFar));
}

GLES_APIENTRY(void, DepthRangexOES, GLclampx zNear, GLclampx zFar) {
  glDepthRangef(X2F(zNear), X2F(zFar));
}

// Removes a shader from a program.
GLES_APIENTRY(void, DetachShader, GLuint program, GLuint shader) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ShaderDataPtr shader_data = sg->GetShaderData(shader);
  if (shader_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Invalid shader %d", shader);
    return;
  }
  ProgramDataPtr program_data = sg->GetProgramData(program);
  if (program_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
    return;
  }

  program_data->DetachShader(shader_data);
}

// Clear server state capability.
GLES_APIENTRY(void, Disable, GLenum cap) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  const int kind = GetCapabilityHandlingKind(cap);
  if (kind == kHandlingKindInvalid) {
    GLES_ERROR_INVALID_ENUM(cap);
    return;
  }

  if (kind & kHandlingKindLocalCopy) {
    c->enabled_set_.erase(cap);
  }
  if (kind & kHandlingKindUniform) {
    c->uniform_context_.Disable(cap);
  }
  if (kind & kHandlingKindTexture) {
    c->texture_context_.Disable(cap);
  }
  if (kind & kHandlingKindPropagate) {
    PASS_THROUGH(c, Disable, cap);
  }
}

// Clear client state capability.
GLES_APIENTRY(void, DisableClientState, GLenum cap) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidClientState(cap)) {
    GLES_ERROR_INVALID_ENUM(cap);
    return;
  }

  const GLuint index = ArrayEnumToIndex(c, cap);
  if (!c->pointer_context_.IsArrayEnabled(index)) {
    return;
  }
  c->pointer_context_.DisableArray(index);
}

// Disables input from an array to the given vertex shader attribute.
GLES_APIENTRY(void, DisableVertexAttribArray, GLuint index) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (index >= static_cast<GLuint>(c->max_vertex_attribs_.Get())) {
    GLES_ERROR_INVALID_VALUE_UINT(index);
    return;
  }
  c->pointer_context_.DisableArray(index);
  PASS_THROUGH(c, DisableVertexAttribArray, index);
}

// Renders primitives using the current state.
GLES_APIENTRY(void, DrawArrays, GLenum mode, GLint first, GLsizei count) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidDrawMode(mode)) {
    GLES_ERROR_INVALID_ENUM(mode);
    return;
  }
  if (first < 0) {
    GLES_ERROR_INVALID_VALUE_INT(first);
    return;
  }
  if (count < 0) {
    GLES_ERROR_INVALID_VALUE_INT(count);
    return;
  }
  c->Draw(GlesContext::kDrawArrays, mode, first, count, 0, NULL);
}

// Renders primitives using the current state.
GLES_APIENTRY(void, DrawElements, GLenum mode, GLsizei count, GLenum type,
              const GLvoid* indices) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidDrawMode(mode)) {
    GLES_ERROR_INVALID_ENUM(mode);
    return;
  }
  if (!IsValidDrawType(type)) {
    GLES_ERROR_INVALID_ENUM(type);
    return;
  }
  if (count < 0) {
    GLES_ERROR_INVALID_VALUE_INT(count);
    return;
  }
  c->Draw(GlesContext::kDrawElements, mode, 0, count, type, indices);
}

GLES_APIENTRY(void, DrawTexfOES, GLfloat x, GLfloat y, GLfloat z, GLfloat width,
              GLfloat height) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  c->DrawTex(x, y, z, width, height);
}

GLES_APIENTRY(void, DrawTexfvOES, const GLfloat* coords) {
  glDrawTexfOES(coords[0], coords[1], coords[2], coords[3], coords[4]);
}

GLES_APIENTRY(void, DrawTexiOES, GLint x, GLint y, GLint z, GLint width,
              GLint height) {
  glDrawTexfOES(x, y, z, width, height);
}

GLES_APIENTRY(void, DrawTexivOES, const GLint* coords) {
  glDrawTexfOES(coords[0], coords[1], coords[2], coords[3], coords[4]);
}

GLES_APIENTRY(void, DrawTexsOES, GLshort x, GLshort y, GLshort z, GLshort width,
              GLshort height) {
  glDrawTexfOES(x, y, z, width, height);
}

GLES_APIENTRY(void, DrawTexsvOES, const GLshort* coords) {
  glDrawTexfOES(coords[0], coords[1], coords[2], coords[3], coords[4]);
}

GLES_APIENTRY(void, DrawTexxOES, GLfixed x, GLfixed y, GLfixed z, GLfixed width,
              GLfixed height) {
  glDrawTexfOES(X2F(x), X2F(y), X2F(z), X2F(width), X2F(height));
}

GLES_APIENTRY(void, DrawTexxvOES, const GLfixed* coords) {
  glDrawTexfOES(X2F(coords[0]), X2F(coords[1]), X2F(coords[2]), X2F(coords[3]),
                X2F(coords[4]));
}

EglImagePtr GetEglImageFromNativeBuffer(GLeglImageOES buffer);

// Set the specified EGL image as the renderbuffer storage.
GLES_APIENTRY(void, EGLImageTargetRenderbufferStorageOES, GLenum target,
              GLeglImageOES buffer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  EglImagePtr image = GetEglImageFromNativeBuffer(buffer);
  if (image == NULL) {
    GLES_ERROR_INVALID_VALUE_PTR(static_cast<void *>(buffer));
    return;
  }
  if (!IsValidRenderbufferTarget(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }

  if (!c->BindImageToRenderbuffer(image)) {
    GLES_ERROR(GL_INVALID_OPERATION, "Unable to bind image to renderbuffer.");
  }
}

// Set the specified EGLimage as the texture.
GLES_APIENTRY(void, EGLImageTargetTexture2DOES, GLenum target,
              GLeglImageOES buffer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  EglImagePtr image = GetEglImageFromNativeBuffer(buffer);
  if (image == NULL) {
      GLES_ERROR_INVALID_VALUE_PTR(static_cast<void *>(buffer));
    return;
  }
  if (!IsValidTextureTargetLimited(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }

  if (!c->BindImageToTexture(target, image)) {
    GLES_ERROR(GL_INVALID_OPERATION, "Unable to bind image to texture");
  }
}

// Set server state capabity.
GLES_APIENTRY(void, Enable, GLenum cap) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  const int kind = GetCapabilityHandlingKind(cap);
  if (kind == kHandlingKindInvalid) {
    GLES_ERROR_INVALID_ENUM(cap);
    return;
  }

  if (kind & kHandlingKindLocalCopy) {
    c->enabled_set_.insert(cap);
  }
  if (kind & kHandlingKindUniform) {
    c->uniform_context_.Enable(cap);
  }
  if (kind & kHandlingKindTexture) {
    c->texture_context_.Enable(cap);
  }
  if (kind & kHandlingKindUnsupported) {
    if (kind & kHandlingKindIgnored) {
      ALOGW("Unsupported capability %s (%x), but ignored.",
            GetEnumString(cap), cap);
    } else {
      LOG_ALWAYS_FATAL("Unsupported capability %s (%x)",
                       GetEnumString(cap), cap);
    }
  }
  if (kind & kHandlingKindPropagate) {
    PASS_THROUGH(c, Enable, cap);
  }
}

// Set client state capability.
GLES_APIENTRY(void, EnableClientState, GLenum cap) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidClientState(cap)) {
    GLES_ERROR_INVALID_ENUM(cap);
    return;
  }

  const GLuint index = ArrayEnumToIndex(c, cap);
  if (c->pointer_context_.IsArrayEnabled(index)) {
    return;
  }
  c->pointer_context_.EnableArray(index);
}

// Enables input from an array to the given vertex shader attribute.
GLES_APIENTRY(void, EnableVertexAttribArray, GLuint index) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (index >= static_cast<GLuint>(c->max_vertex_attribs_.Get())) {
    GLES_ERROR_INVALID_VALUE_UINT(index);
    return;
  }
  c->pointer_context_.EnableArray(index);
  PASS_THROUGH(c, EnableVertexAttribArray, index);
}

// Finishes any commands in the GL command queue, and returns only when all
// commands are complete (including any rendering).
GLES_APIENTRY(void, Finish) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, Finish);
}

// Flushes any commands in the GL command queue, ensuring they are actually
// being processed and not just waiting for some other event to trigger them.
GLES_APIENTRY(void, Flush) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, Flush);
}

namespace {

// Configures the fog computation of the fixed-function shader.
void HandleFog(GLenum name, const GLfloat* params, ParamType param_type) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  Fog& fog = c->uniform_context_.MutateFog();

  const GLenum mode = static_cast<GLenum>(params[0]);
  const GLfloat value = params[0];
  switch (name) {
    case GL_FOG_MODE:
      if (mode != GL_LINEAR && mode != GL_EXP && mode != GL_EXP2) {
        GLES_ERROR_INVALID_ENUM(mode);
        return;
      }
      fog.mode = mode;
      break;
    case GL_FOG_DENSITY:
      if (value < 0.f) {
        GLES_ERROR_INVALID_VALUE_FLOAT(value);
        return;
      }
      fog.density = value;
      break;
    case GL_FOG_START:
      fog.start = value;
      break;
    case GL_FOG_END:
      fog.end = value;
      break;
    case GL_FOG_COLOR:
      if (param_type != kParamTypeArray) {
        GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_FOG_COLOR.");
        return;
      }
      for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
        fog.color.Set(i, ClampValue(params[i], 0.f, 1.f));
      }
      break;
    default:
      GLES_ERROR_INVALID_ENUM(name);
      return;
  }
}

}  // namespace

GLES_APIENTRY(void, Fogf, GLenum name, GLfloat param) {
  HandleFog(name, &param, kParamTypeScalar);
}

GLES_APIENTRY(void, Fogfv, GLenum name, const GLfloat* params) {
  HandleFog(name, params, kParamTypeArray);
}

GLES_APIENTRY(void, Fogx, GLenum pname, GLfixed param) {
  if (pname == GL_FOG_MODE) {
    glFogf(pname, static_cast<GLfloat>(param));
  } else {
    glFogf(pname, X2F(param));
  }
}

GLES_APIENTRY(void, FogxOES, GLenum pname, GLfixed param) {
  glFogx(pname, param);
}

GLES_APIENTRY(void, Fogxv, GLenum pname, const GLfixed* params) {
  if (pname == GL_FOG_MODE) {
    glFogf(pname, static_cast<GLfloat>(params[0]));
  } else {
    GLfloat tmp[kMaxParamElementSize];
    Convert(tmp, ParamSize(pname), params);
    glFogfv(pname, tmp);
  }
}

GLES_APIENTRY(void, FogxvOES, GLenum pname, const GLfixed* params) {
  glFogxv(pname, params);
}

// Attaches a render buffer to a frame buffer.
GLES_APIENTRY(void, FramebufferRenderbuffer, GLenum target, GLenum attachment,
              GLenum renderbuffertarget, GLuint renderbuffer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidFramebufferTarget(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }
  if (!IsValidFramebufferAttachment(attachment)) {
    GLES_ERROR_INVALID_ENUM(attachment);
    return;
  }
  if (!IsValidRenderbufferTarget(renderbuffertarget)) {
    GLES_ERROR_INVALID_ENUM(renderbuffertarget);
    return;
  }

  FramebufferDataPtr fb = c->GetBoundFramebufferData();
  if (renderbuffer == 0) {
    if (fb != NULL) {
      fb->ClearAttachment(attachment);
    }
    PASS_THROUGH(c, FramebufferRenderbuffer, target, attachment,
                 renderbuffertarget, 0);
  } else {
    ShareGroupPtr sg = c->GetShareGroup();
    RenderbufferDataPtr rb = sg->GetRenderbufferData(renderbuffer);
    if (rb == NULL) {
      GLES_ERROR(GL_INVALID_OPERATION, "Invalid renderbuffer.");
      return;
    }

    if (fb != NULL) {
      fb->SetAttachment(attachment, renderbuffertarget, renderbuffer, rb);
    }

    GLint texture = rb->GetEglImageTexture();
    if (texture != 0) {
      // This renderbuffer object is an EGLImage target so attach the
      // EGLImage's texture instead the renderbuffer.
      PASS_THROUGH(c, FramebufferTexture2D, target, attachment, GL_TEXTURE_2D,
                   texture, 0);
    } else {
      const GLuint global_name = sg->GetRenderbufferGlobalName(renderbuffer);
      PASS_THROUGH(c, FramebufferRenderbuffer, target, attachment,
                   renderbuffertarget, global_name);
    }
  }
}

GLES_APIENTRY(void, FramebufferRenderbufferOES, GLenum target,
              GLenum attachment, GLenum renderbuffertarget,
              GLuint renderbuffer) {
  glFramebufferRenderbuffer(target, attachment, renderbuffertarget,
                              renderbuffer);
}

// Sets up a texture to be used as the framebuffer for subsequent rendering.
GLES_APIENTRY(void, FramebufferTexture2D, GLenum target, GLenum attachment,
              GLenum textarget, GLuint texture, GLint level) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidFramebufferTarget(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }
  if (!IsValidFramebufferAttachment(attachment)) {
    GLES_ERROR_INVALID_ENUM(attachment);
    return;
  }
  if (!IsValidTextureTargetEx(textarget)) {
    GLES_ERROR_INVALID_ENUM(textarget);
    return;
  }
  if (level != 0) {
    GLES_ERROR_INVALID_VALUE_INT(level);
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();

  // Update the framebuffer attachment.
  FramebufferDataPtr fb = c->GetBoundFramebufferData();
  if (fb != NULL) {
    fb->SetAttachment(attachment, textarget, texture, RenderbufferDataPtr());
  }

  const GLuint global_name = sg->GetTextureGlobalName(texture);
  PASS_THROUGH(c, FramebufferTexture2D, target, attachment, textarget,
               global_name, level);
}

GLES_APIENTRY(void, FramebufferTexture2DOES, GLenum target, GLenum attachment,
              GLenum textarget, GLuint texture, GLint level) {
  glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

// Sets which order (clockwise, counter-clockwise) is considered front facing.
GLES_APIENTRY(void, FrontFace, GLenum mode) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidFrontFaceMode(mode)) {
    GLES_ERROR_INVALID_ENUM(mode);
    return;
  }

  c->front_face_.Mutate() = mode;
  PASS_THROUGH(c, FrontFace, mode);
}

// Multiplies the current matrix with the specified perspective matrix.
GLES_APIENTRY(void, Frustumf, GLfloat left, GLfloat right, GLfloat bottom,
              GLfloat top, GLfloat zNear, GLfloat zFar) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  c->uniform_context_.MutateActiveMatrix() *=
      emugl::Matrix::GeneratePerspective(left, right, bottom, top, zNear, zFar);
}

GLES_APIENTRY(void, FrustumfOES, GLfloat left, GLfloat right, GLfloat bottom,
              GLfloat top, GLfloat zNear, GLfloat zFar) {
  glFrustumf(left, right, bottom, top, zNear, zFar);
}

GLES_APIENTRY(void, Frustumx, GLfixed left, GLfixed right, GLfixed bottom,
              GLfixed top, GLfixed zNear, GLfixed zFar) {
  glFrustumf(X2F(left), X2F(right), X2F(bottom), X2F(top), X2F(zNear),
             X2F(zFar));
}

GLES_APIENTRY(void, FrustumxOES, GLfixed left, GLfixed right, GLfixed bottom,
              GLfixed top, GLfixed zNear, GLfixed zFar) {
  glFrustumf(X2F(left), X2F(right), X2F(bottom), X2F(top), X2F(zNear),
             X2F(zFar));
}

// Generate N buffer objects.
GLES_APIENTRY(void, GenBuffers, GLsizei n, GLuint* buffers) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (n < 0) {
    GLES_ERROR_INVALID_VALUE_INT(n);
    return;
  }

  c->GetShareGroup()->GenBuffers(n, buffers);
}

// Generate N framebuffer objects.
GLES_APIENTRY(void, GenFramebuffers, GLsizei n, GLuint* framebuffers) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (n < 0) {
    GLES_ERROR_INVALID_VALUE_INT(n);
    return;
  }

  c->GetShareGroup()->GenFramebuffers(n, framebuffers);
}

GLES_APIENTRY(void, GenFramebuffersOES, GLsizei n, GLuint* framebuffers) {
  glGenFramebuffers(n, framebuffers);
}

// Generates N renderbuffer objects.
GLES_APIENTRY(void, GenRenderbuffers, GLsizei n, GLuint* renderbuffers) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (n < 0) {
    GLES_ERROR_INVALID_VALUE_INT(n);
    return;
  }

  c->GetShareGroup()->GenRenderbuffers(n, renderbuffers);
}

GLES_APIENTRY(void, GenRenderbuffersOES, GLsizei n, GLuint* renderbuffers) {
  glGenRenderbuffers(n, renderbuffers);
}

// Creates N texture objects.
GLES_APIENTRY(void, GenTextures, GLsizei n, GLuint* textures) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (n < 0) {
    GLES_ERROR_INVALID_VALUE_INT(n);
    return;
  }

  c->GetShareGroup()->GenTextures(n, textures);
}

// Generates all the mipmaps of a texture from the highest resolution image.
GLES_APIENTRY(void, GenerateMipmap, GLenum target) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  // TODO(crbug.com/441913): Record texture format for texture conversion
  // workaround for Pepper GLES2.
  PASS_THROUGH(c, GenerateMipmap, target);
}

GLES_APIENTRY(void, GenerateMipmapOES, GLenum target) {
  glGenerateMipmap(target);
}

// Gets the list of attribute indices used by the given program.
GLES_APIENTRY(void, GetActiveAttrib, GLuint program, GLuint index,
              GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type,
              GLchar* name) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ProgramDataPtr program_data = sg->GetProgramData(program);
  if (program_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
    return;
  }

  program_data->GetActiveAttrib(index, bufsize, length, size, type, name);
}

// Gets the list of uniform indices used by the given program.
GLES_APIENTRY(void, GetActiveUniform, GLuint program, GLuint index,
              GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type,
              GLchar* name) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ProgramDataPtr program_data = sg->GetProgramData(program);
  if (program_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
    return;
  }

  program_data->GetActiveUniform(index, bufsize, length, size, type, name);
}

// Gets the list of shader objects used by the given program.
GLES_APIENTRY(void, GetAttachedShaders, GLuint program, GLsizei maxcount,
              GLsizei* count, GLuint* shaders) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ProgramDataPtr program_data = sg->GetProgramData(program);
  if (program_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
    return;
  }

  program_data->GetAttachedShaders(maxcount, count, shaders);
}

// Gets the index of an attribute for a program by name.
GLES_APIENTRY(GLint, GetAttribLocation, GLuint program, const GLchar* name) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return -1;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ProgramDataPtr program_data = sg->GetProgramData(program);
  if (program_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
    return -1;
  }

  return program_data->GetAttribLocation(name);
}

// Gets state as a boolean.
GLES_APIENTRY(void, GetBooleanv, GLenum pname, GLboolean* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (c->GetBooleanv(pname, params)) {
    return;
  }
  *params = GL_FALSE;
  PASS_THROUGH(c, GetBooleanv, pname, params);
}

// Get buffer parameter value.
GLES_APIENTRY(void, GetBufferParameteriv, GLenum target, GLenum pname,
              GLint* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidBufferTarget(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }

  BufferDataPtr vbo = c->GetBoundTargetBufferData(target);
  if (vbo == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Target buffer not bound.");
    return;
  }

  switch (pname) {
    case GL_BUFFER_SIZE:
      *params = vbo->GetSize();
      break;
    case GL_BUFFER_USAGE:
      *params = vbo->GetUsage();
      break;
    default:
      GLES_ERROR_INVALID_ENUM(pname);
      return;
  }
}

// Gets the current clip plane parameters.
GLES_APIENTRY(void, GetClipPlanef, GLenum pname, GLfloat* eqn) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  const emugl::Vector* plane = c->uniform_context_.GetClipPlane(pname);
  if (!plane) {
    GLES_ERROR_INVALID_ENUM(pname);
    return;
  }
  for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
    eqn[i] = plane->Get(i);
  }
}

GLES_APIENTRY(void, GetClipPlanefOES, GLenum pname, GLfloat* eqn) {
  glGetClipPlanef(pname, eqn);
}

GLES_APIENTRY(void, GetClipPlanex, GLenum pname, GLfixed* eqn) {
  static const size_t kNumElements = 4;
  GLfloat tmp[kNumElements];
  glGetClipPlanef(pname, tmp);
  Convert(tmp, kNumElements, eqn);
}

GLES_APIENTRY(void, GetClipPlanexOES, GLenum pname, GLfixed* eqn) {
  glGetClipPlanex(pname, eqn);
}

// Returns the oldest error code that was reported.
GLES_APIENTRY(GLenum, GetError) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c || !c->AreChecksEnabled()) {
    return GL_NO_ERROR;
  }

  // N.B. We have no way of knowing which is older -- the oldest error we
  // recorded or any error that might be available downstream. We just return
  // any error we might have, and if none, we pass the request through.
  GLenum error = c->GetGLerror();
  if (error != GL_NO_ERROR) {
    ALOGV("Returning error %s directly.", GetEnumString(error));
  } else {
    error = PASS_THROUGH(c, GetError);
    ALOGV("Returning error %s from passthrough.", GetEnumString(error));
  }
  return error;
}

// Gets state as a fixed.
GLES_APIENTRY(void, GetFixedv, GLenum pname, GLfixed* params) {
  GLfloat tmp[kMaxParamElementSize];
  glGetFloatv(pname, tmp);
  Convert(params, ParamSize(pname), tmp);
}

GLES_APIENTRY(void, GetFixedvOES, GLenum pname, GLfixed* params) {
  glGetFixedv(pname, params);
}

// Gets state as a float.
GLES_APIENTRY(void, GetFloatv, GLenum pname, GLfloat* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (c->GetFloatv(pname, params)) {
    return;
  }
  *params = 0.f;
  PASS_THROUGH(c, GetFloatv, pname, params);
}

GLES_APIENTRY(void, GetFramebufferAttachmentParameteriv, GLenum target,
              GLenum attachment, GLenum pname, GLint* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidFramebufferTarget(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }
  if (!IsValidFramebufferAttachment(attachment)) {
    GLES_ERROR_INVALID_ENUM(attachment);
    return;
  }
  if (!IsValidFramebufferAttachmentParams(pname)) {
    GLES_ERROR_INVALID_ENUM(pname);
    return;
  }

  // Take the attachment attribute from our state - if available
  FramebufferDataPtr fb = c->GetBoundFramebufferData();
  if (fb == NULL) {
    return;
  }

  GLenum attachment_target = 0;
  const GLuint name = fb->GetAttachment(attachment, &attachment_target);
  switch (pname) {
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
      if (attachment_target == GL_TEXTURE_2D) {
        *params = GL_TEXTURE;
      } else if (attachment_target == GL_TEXTURE_EXTERNAL_OES) {
        *params = GL_TEXTURE;
      } else if (attachment_target == GL_RENDERBUFFER) {
        *params = GL_RENDERBUFFER;
      } else {
        LOG_ALWAYS_FATAL("Unknown attachment target: %d", attachment_target);
      }
      break;
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
      *params = name;
      break;
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
      *params = 0;
      break;
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
      PASS_THROUGH(c, GetFramebufferAttachmentParameteriv, target, attachment,
                   pname, params);
      break;
  }
}

GLES_APIENTRY(void, GetFramebufferAttachmentParameterivOES, GLenum target,
              GLenum attachment, GLenum pname, GLint* params) {
  glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
}

// Gets state as an integer.
GLES_APIENTRY(void, GetIntegerv, GLenum pname, GLint* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (c->GetIntegerv(pname, params)) {
    return;
  }
  *params = 0;
  PASS_THROUGH(c, GetIntegerv, pname, params);
}

// Gets fixed function lighting state.
GLES_APIENTRY(void, GetLightfv, GLenum lightid, GLenum name, GLfloat* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  const Light* light = c->uniform_context_.GetLight(lightid);
  if (!light) {
    GLES_ERROR_INVALID_ENUM(lightid);
    return;
  }
  switch (name) {
    case GL_AMBIENT:
      for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
        params[i] = light->ambient.Get(i);
      }
      break;
    case GL_DIFFUSE:
      for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
        params[i] = light->diffuse.Get(i);
      }
      break;
    case GL_SPECULAR:
      for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
        params[i] = light->specular.Get(i);
      }
      break;
    case GL_POSITION:
      for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
        params[i] = light->position.Get(i);
      }
      break;
    case GL_SPOT_DIRECTION:
      for (size_t i = 0; i < 3; ++i) {
        params[i] = light->direction.Get(i);
      }
      break;
    case GL_SPOT_EXPONENT:
      params[0] = light->spot_exponent;
      break;
    case GL_SPOT_CUTOFF:
      params[0] = light->spot_cutoff;
      break;
    case GL_CONSTANT_ATTENUATION:
      params[0] = light->const_attenuation;
      break;
    case GL_LINEAR_ATTENUATION:
      params[0] = light->linear_attenuation;
      break;
    case GL_QUADRATIC_ATTENUATION:
      params[0] = light->quadratic_attenuation;
      break;
    default:
      GLES_ERROR_INVALID_ENUM(name);
      return;
  }
}

GLES_APIENTRY(void, GetLightxv, GLenum light, GLenum pname, GLfixed* params) {
  GLfloat tmp[kMaxParamElementSize];
  glGetLightfv(light, pname, tmp);
  Convert(params, ParamSize(pname), tmp);
}

GLES_APIENTRY(void, GetLightxvOES, GLenum light, GLenum pname,
              GLfixed* params) {
  glGetLightxv(light, pname, params);
}

// Gets fixed function material state.
GLES_APIENTRY(void, GetMaterialfv, GLenum face, GLenum name, GLfloat* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (face != GL_FRONT && face != GL_BACK) {
    GLES_ERROR_INVALID_ENUM(face);
    return;
  }

  const Material& material = c->uniform_context_.GetMaterial();
  switch (name) {
    case GL_AMBIENT:
      for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
        params[i] = material.ambient.Get(i);
      }
      break;
    case GL_DIFFUSE:
      for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
        params[i] = material.diffuse.Get(i);
      }
      break;
    case GL_SPECULAR:
      for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
        params[i] = material.specular.Get(i);
      }
      break;
    case GL_EMISSION:
      for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
        params[i] = material.emission.Get(i);
      }
      break;
    case GL_SHININESS:
      params[0] = material.shininess;
      break;
    default:
      GLES_ERROR_INVALID_ENUM(name);
      return;
  }
}

GLES_APIENTRY(void, GetMaterialxv, GLenum face, GLenum pname, GLfixed* params) {
  GLfloat tmp[kMaxParamElementSize];
  glGetMaterialfv(face, pname, tmp);
  Convert(params, ParamSize(pname), tmp);
}

GLES_APIENTRY(void, GetMaterialxvOES, GLenum face, GLenum pname,
              GLfixed* params) {
  glGetMaterialxv(face, pname, params);
}

// Gets pointer data for the specified array.
GLES_APIENTRY(void, GetPointerv, GLenum pname, void** params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  const GLuint index = ArrayEnumToIndex(c, pname);
  const PointerData* ptr = c->pointer_context_.GetPointerData(index);
  if (!ptr) {
    GLES_ERROR_INVALID_ENUM(pname);
    return;
  } else {
    *params = const_cast<void*>(ptr->pointer);
  }
}

// Gets the output text messages from the last program link/validation request.
GLES_APIENTRY(void, GetProgramInfoLog, GLuint program, GLsizei bufsize,
              GLsizei* length, GLchar* infolog) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ProgramDataPtr program_data = sg->GetProgramData(program);
  if (program_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
    return;
  }

  program_data->GetInfoLog(bufsize, length, infolog);
}

// Gets information about a program.
GLES_APIENTRY(void, GetProgramiv, GLuint program, GLenum pname, GLint* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ProgramDataPtr program_data = sg->GetProgramData(program);
  if (program_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
    return;
  }

  program_data->GetProgramiv(pname, params);
}

// Gets information about the render buffer bound to the indicated target.
GLES_APIENTRY(void, GetRenderbufferParameteriv, GLenum target, GLenum pname,
              GLint* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidRenderbufferTarget(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }
  if (!IsValidRenderbufferParams(pname)) {
    GLES_ERROR_INVALID_ENUM(pname);
    return;
  }

  RenderbufferDataPtr rb = c->GetBoundRenderbufferData();
  if (rb != NULL) {
    switch (pname) {
      case GL_RENDERBUFFER_WIDTH:
        *params = rb->GetWidth();
        break;
      case GL_RENDERBUFFER_HEIGHT:
        *params = rb->GetHeight();
        break;
      case GL_RENDERBUFFER_INTERNAL_FORMAT:
        *params = rb->GetFormat();
        break;
      default:
        LOG_ALWAYS_FATAL("Unsupported buffer parameter %s (%x)",
                         GetEnumString(pname), pname);
        return;
    }
  } else {
    PASS_THROUGH(c, GetRenderbufferParameteriv, target, pname, params);
  }
}

GLES_APIENTRY(void, GetRenderbufferParameterivOES, GLenum target, GLenum pname,
              GLint* params) {
  glGetRenderbufferParameteriv(target, pname, params);
}

// Gets the output text messages from the last shader compile.
GLES_APIENTRY(void, GetShaderInfoLog, GLuint shader, GLsizei bufsize,
              GLsizei* length, GLchar* infolog) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ShaderDataPtr shader_data = sg->GetShaderData(shader);
  if (shader_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Not a valid shader %d", shader);
    return;
  }

  shader_data->GetInfoLog(bufsize, length, infolog);
}

// Gets the shaders precision.
GLES_APIENTRY(void, GetShaderPrecisionFormat, GLenum shadertype,
              GLenum precisiontype, GLint* range, GLint* precision) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, GetShaderPrecisionFormat, shadertype, precisiontype, range,
               precision);
}

// Gets the source code loaded into the indicated shader object.
GLES_APIENTRY(void, GetShaderSource, GLuint shader, GLsizei bufsize,
              GLsizei* length, GLchar* source) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ShaderDataPtr shader_data = sg->GetShaderData(shader);
  if (shader_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Not a valid shader %d", shader);
    return;
  }

  shader_data->GetSource(bufsize, length, source);
}

// Gets information about a shader.
GLES_APIENTRY(void, GetShaderiv, GLuint shader, GLenum pname, GLint* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ShaderDataPtr shader_data = sg->GetShaderData(shader);
  if (shader_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Not a valid shader %d", shader);
    return;
  }

  shader_data->GetShaderiv(pname, params);
}

// Gets string information about the GL implementation.
GLES_APIENTRY(const GLubyte*, GetString, GLenum name) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
      fprintf(stderr, "%s: cannot get current gles context, quitting\n", __FUNCTION__);
    return NULL;
  }

      fprintf(stderr, "%s: getting gl string...\n", __FUNCTION__);
  const GLubyte* str = c->GetString(name);
  if (str == NULL) {
      fprintf(stderr, "%s: string is null. invalid enum trigger\n", __FUNCTION__);
    GLES_ERROR_INVALID_ENUM(name);
  }
      fprintf(stderr, "%s: str=%s\n", __FUNCTION__, str);
  return str;
}

// Gets texture environment parameters.
namespace {

template <typename T>
void HandleGetTexEnv(GLenum env, GLenum pname, T* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  const TexEnv& texenv = c->uniform_context_.GetActiveTexEnv();
  if (env != GL_TEXTURE_ENV) {
    GLES_ERROR_INVALID_ENUM(env);
    return;
  }

  switch (pname) {
    case GL_TEXTURE_ENV_MODE:
      params[0] = static_cast<T>(texenv.mode);
      break;
    case GL_SRC0_RGB:
    case GL_SRC1_RGB:
    case GL_SRC2_RGB:
      params[0] = static_cast<T>(texenv.src_rgb[pname - GL_SRC0_RGB]);
      break;
    case GL_SRC0_ALPHA:
    case GL_SRC1_ALPHA:
    case GL_SRC2_ALPHA:
      params[0] = static_cast<T>(texenv.src_alpha[pname - GL_SRC0_ALPHA]);
      break;
    case GL_OPERAND0_RGB:
    case GL_OPERAND1_RGB:
    case GL_OPERAND2_RGB:
      params[0] = static_cast<T>(texenv.operand_rgb[pname - GL_OPERAND0_RGB]);
      break;
    case GL_OPERAND0_ALPHA:
    case GL_OPERAND1_ALPHA:
    case GL_OPERAND2_ALPHA:
      params[0] =
          static_cast<T>(texenv.operand_alpha[pname - GL_OPERAND0_ALPHA]);
      break;
    case GL_COMBINE_RGB:
      params[0] = static_cast<T>(texenv.combine_rgb);
      break;
    case GL_COMBINE_ALPHA:
      params[0] = static_cast<T>(texenv.combine_alpha);
      break;
    case GL_RGB_SCALE:
      params[0] = static_cast<T>(texenv.rgb_scale);
      break;
    case GL_ALPHA_SCALE:
      params[0] = static_cast<T>(texenv.alpha_scale);
      break;
    case GL_TEXTURE_ENV_COLOR:
      texenv.color.GetLinearMapping(params, 4);
      break;
    case GL_COORD_REPLACE_OES:
      LOG_ALWAYS_FATAL("GL_COORD_REPLACE_OES not supported.");
      break;
    default:
      GLES_ERROR_INVALID_ENUM(pname);
      return;
  }
}

}  // namespace

GLES_APIENTRY(void, GetTexEnvfv, GLenum env, GLenum pname, GLfloat* params) {
  HandleGetTexEnv(env, pname, params);
}

GLES_APIENTRY(void, GetTexEnviv, GLenum env, GLenum pname, GLint* params) {
  HandleGetTexEnv(env, pname, params);
}

GLES_APIENTRY(void, GetTexEnvxv, GLenum env, GLenum pname, GLfixed* params) {
  GLfloat tmp[kMaxParamElementSize];
  glGetTexEnvfv(env, pname, tmp);
  Convert(params, ParamSize(pname), tmp);
}

GLES_APIENTRY(void, GetTexEnvxvOES, GLenum env, GLenum pname,
              GLfixed* params) {
  glGetTexEnvxv(env, pname, params);
}

// Get texture parameter.
GLES_APIENTRY(void, GetTexParameterfv, GLenum target, GLenum pname,
              GLfloat* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidTextureTarget(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }

  const GLenum global_target = c->texture_context_.EnsureCorrectBinding(target);
  PASS_THROUGH(c, GetTexParameterfv, global_target, pname, params);
  c->texture_context_.RestoreBinding(target);
}

GLES_APIENTRY(void, GetTexParameteriv, GLenum target, GLenum pname,
              GLint* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidTextureTarget(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }

  if (pname == GL_TEXTURE_CROP_RECT_OES) {
    TextureDataPtr tex = c->GetBoundTextureData(target);
    if (tex != NULL) {
      memcpy(params, tex->GetCropRect(), 4 * sizeof(GLint));
    }
    return;
  }

  const GLenum global_target = c->texture_context_.EnsureCorrectBinding(target);
  PASS_THROUGH(c, GetTexParameteriv, global_target, pname, params);
  c->texture_context_.RestoreBinding(target);
}

GLES_APIENTRY(void, GetTexParameterxv, GLenum target, GLenum pname,
              GLfixed* params) {
  GLfloat tmp[kMaxParamElementSize];
  glGetTexParameterfv(target, pname, tmp);
  Convert(params, ParamSize(pname), tmp);
}

GLES_APIENTRY(void, GetTexParameterxvOES, GLenum target, GLenum pname,
              GLfixed* params) {
  glGetTexParameterxv(target, pname, params);
}

// Gets the default uniform value for a program.
GLES_APIENTRY(void, GetUniformfv, GLuint program, GLint location,
              GLfloat* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ProgramDataPtr program_data = sg->GetProgramData(program);
  if (program_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
    return;
  }

  program_data->GetUniformfv(location, params);
}

GLES_APIENTRY(void, GetUniformiv, GLuint program, GLint location,
              GLint* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ProgramDataPtr program_data = sg->GetProgramData(program);
  if (program_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
    return;
  }

  program_data->GetUniformiv(location, params);
}

// Gets the index of an uniform for a program by name.
GLES_APIENTRY(GLint, GetUniformLocation, GLuint program, const GLchar* name) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return -1;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ProgramDataPtr program_data = sg->GetProgramData(program);
  if (program_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
    return -1;
  }

  return program_data->GetUniformLocation(name);
}

// Gets a pointer value from the vertex attribute state.
GLES_APIENTRY(void, GetVertexAttribPointerv, GLuint index, GLenum pname,
              GLvoid** pointer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER) {
    GLES_ERROR_INVALID_ENUM(pname);
    return;
  }

  const PointerData* ptr = c->pointer_context_.GetPointerData(index);
  if (!ptr) {
    GLES_ERROR_INVALID_VALUE_UINT(index);
    return;
  }
  *pointer = const_cast<void*>(ptr->pointer);
}

// Gets the vertex attribute state.
GLES_APIENTRY(void, GetVertexAttribfv, GLuint index, GLenum pname,
              GLfloat* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  const PointerData* ptr = c->pointer_context_.GetPointerData(index);
  if (!ptr) {
    GLES_ERROR_INVALID_VALUE_UINT(index);
    return;
  }

  switch (pname) {
    case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
      *params = static_cast<GLfloat>(ptr->buffer_name);
      break;
    case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
      *params = static_cast<GLfloat>(ptr->enabled);
      break;
    case GL_VERTEX_ATTRIB_ARRAY_SIZE:
      *params = static_cast<GLfloat>(ptr->size);
      break;
    case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
      *params = static_cast<GLfloat>(ptr->stride);
      break;
    case GL_VERTEX_ATTRIB_ARRAY_TYPE:
      *params = static_cast<GLfloat>(ptr->type);
      break;
    case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
      *params = static_cast<GLfloat>(ptr->normalize);
      break;
    default:
      PASS_THROUGH(c, GetVertexAttribfv, index, pname, params);
      break;
  }
}

GLES_APIENTRY(void, GetVertexAttribiv, GLuint index, GLenum pname,
              GLint* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  const PointerData* ptr = c->pointer_context_.GetPointerData(index);
  if (!ptr) {
    GLES_ERROR_INVALID_VALUE_UINT(index);
    return;
  }

  switch (pname) {
    case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
      *params = ptr->buffer_name;
      break;
    case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
      *params = ptr->enabled;
      break;
    case GL_VERTEX_ATTRIB_ARRAY_SIZE:
      *params = ptr->size;
      break;
    case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
      *params = ptr->stride;
      break;
    case GL_VERTEX_ATTRIB_ARRAY_TYPE:
      *params = ptr->type;
      break;
    case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
      *params = ptr->normalize;
      break;
    default:
      PASS_THROUGH(c, GetVertexAttribiv, index, pname, params);
      break;
  }
}

// Provides a hint to the GL implementation. Generally the application uses
// hints to suggest where performance matters more than rendering quality.
GLES_APIENTRY(void, Hint, GLenum target, GLenum mode) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  switch (target) {
    // GLES1 specific hints.  We will ignore them since they are only hints.
    case GL_FOG_HINT:
    case GL_LINE_SMOOTH_HINT:
    case GL_PERSPECTIVE_CORRECTION_HINT:
    case GL_POINT_SMOOTH_HINT:
      break;
    // GLES2 supported hints. We pass through except for any caching.
    case GL_GENERATE_MIPMAP_HINT:
      if (!IsValidMipmapHintMode(mode)) {
        GLES_ERROR_INVALID_ENUM(mode);
        return;
      }
      c->generate_mipmap_hint_.Mutate() = mode;
      PASS_THROUGH(c, Hint, target, mode);
      break;
    default:
      GLES_ERROR_INVALID_ENUM(target);
      return;
  }
}

// Returns true if the specified object is a buffer.
GLES_APIENTRY(GLboolean, IsBuffer, GLuint buffer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return GL_FALSE;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  BufferDataPtr ptr = sg->GetBufferData(buffer);
  return ptr != NULL ? GL_TRUE : GL_FALSE;
}

// Returns true if the given capability is enabled.
GLES_APIENTRY(GLboolean, IsEnabled, GLenum cap) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return GL_FALSE;
  }


  // IsEnabled is used for checking both client and server capabilities.
  // First check the client state.
  if (IsValidClientState(cap)) {
    const GLuint index = ArrayEnumToIndex(c, cap);
    return c->pointer_context_.IsArrayEnabled(index);
  }

  const int kind = GetCapabilityHandlingKind(cap);
  if (kind == kHandlingKindInvalid) {
    GLES_ERROR_INVALID_ENUM(cap);
    return 0;
  }

  if (kind & kHandlingKindLocalCopy) {
    return c->IsEnabled(cap);
  }
  if (kind & kHandlingKindUniform) {
    return c->uniform_context_.IsEnabled(cap);
  }
  if (kind & kHandlingKindTexture) {
    return c->texture_context_.IsEnabled(cap);
  }
  if (kind & kHandlingKindPropagate) {
    return PASS_THROUGH(c, IsEnabled, cap);
  }
  return 0;
}

// Returns true if the specified object is a framebuffer.
GLES_APIENTRY(GLboolean, IsFramebuffer, GLuint framebuffer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return GL_FALSE;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  FramebufferDataPtr fb = sg->GetFramebufferData(framebuffer);
  return fb != NULL ? GL_TRUE : GL_FALSE;
}

GLES_APIENTRY(GLboolean, IsFramebufferOES, GLuint framebuffer) {
  return glIsFramebuffer(framebuffer);
}

// Returns true if the specified object is a program.
GLES_APIENTRY(GLboolean, IsProgram, GLuint program) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return GL_FALSE;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ProgramDataPtr ptr = sg->GetProgramData(program);
  return ptr != NULL ? GL_TRUE : GL_FALSE;
}

// Returns true if the specified object is a renderbuffer.
GLES_APIENTRY(GLboolean, IsRenderbuffer, GLuint renderbuffer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return GL_FALSE;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  RenderbufferDataPtr rb = sg->GetRenderbufferData(renderbuffer);
  return rb != NULL ? GL_TRUE : GL_FALSE;
}

GLES_APIENTRY(GLboolean, IsRenderbufferOES, GLuint renderbuffer) {
  return glIsRenderbuffer(renderbuffer);
}

// Returns true if the specified object is a shader.
GLES_APIENTRY(GLboolean, IsShader, GLuint shader) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return GL_FALSE;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ShaderDataPtr ptr = sg->GetShaderData(shader);
  return ptr != NULL ? GL_TRUE : GL_FALSE;
}

// Returns true if the specified object is a texture.
GLES_APIENTRY(GLboolean, IsTexture, GLuint texture) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return GL_FALSE;
  }

  if (texture == 0) {
    return GL_FALSE;
  }
  ShareGroupPtr sg = c->GetShareGroup();
  TextureDataPtr tex = sg->GetTextureData(texture);
  return tex != NULL ? GL_TRUE : GL_FALSE;
}

// Configure fixed function lighting state.
namespace {
void HandleLightModel(GLenum name, const GLfloat* params,
                      ParamType param_type) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  switch (name) {
    case GL_LIGHT_MODEL_TWO_SIDE:
      if (params[0] == 0.0f) {
        c->enabled_set_.erase(name);
      } else {
        c->enabled_set_.insert(name);
      }
      break;
    case GL_LIGHT_MODEL_AMBIENT:
      if (param_type != kParamTypeArray) {
        GLES_ERROR(GL_INVALID_ENUM,
                    "Expected array for GL_LIGHT_MODEL_AMBIENT.");
        return;
      }
      for (size_t i = 0; i < emugl::Vector::kEntries; ++i) {
        c->uniform_context_.MutateAmbient().Set(i, params[i]);
      }
      break;
    default:
      GLES_ERROR_INVALID_ENUM(name);
      return;
  }
}
}  // namespace

GLES_APIENTRY(void, LightModelf, GLenum name, GLfloat param) {
  HandleLightModel(name, &param, kParamTypeScalar);
}

GLES_APIENTRY(void, LightModelfv, GLenum name, const GLfloat* params) {
  HandleLightModel(name, params, kParamTypeArray);
}

GLES_APIENTRY(void, LightModelx, GLenum pname, GLfixed param) {
  glLightModelf(pname, X2F(param));
}

GLES_APIENTRY(void, LightModelxOES, GLenum pname, GLfixed param) {
  glLightModelf(pname, X2F(param));
}

GLES_APIENTRY(void, LightModelxv, GLenum pname, const GLfixed* params) {
  GLfloat tmp[kMaxParamElementSize];
  Convert(tmp, ParamSize(pname), params);
  glLightModelfv(pname, tmp);
}

GLES_APIENTRY(void, LightModelxvOES, GLenum pname, const GLfixed* params) {
  glLightModelxv(pname, params);
}

// Configure fixed function lighting state.
namespace {
void HandleLight(GLenum lightid, GLenum name, const GLfloat* params,
                 ParamType param_type) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  Light* light = c->uniform_context_.MutateLight(lightid);
  if (!light) {
    GLES_ERROR_INVALID_ENUM(lightid);
    return;
  }

  const emugl::Matrix& mv = c->uniform_context_.GetModelViewMatrix();
  const GLfloat value = params[0];
  emugl::Vector vector(params[0], params[1], params[2], params[3]);

  switch (name) {
    case GL_SPOT_EXPONENT:
      if (value < 0.f || value > 128.f) {
        GLES_ERROR_INVALID_VALUE_FLOAT(value);
        return;
      }
      light->spot_exponent = value;
      break;
    case GL_SPOT_CUTOFF:
      if ((value < 0.f || value > 90.f) && value != 180.f) {
        GLES_ERROR_INVALID_VALUE_FLOAT(value);
        return;
      }
      light->spot_cutoff = value;
      break;
    case GL_CONSTANT_ATTENUATION:
      light->const_attenuation = value;
      break;
    case GL_LINEAR_ATTENUATION:
      light->linear_attenuation = value;
      break;
    case GL_QUADRATIC_ATTENUATION:
      light->quadratic_attenuation = value;
      break;
    case GL_AMBIENT:
      if (param_type != kParamTypeArray) {
        GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_AMBIENT.");
        return;
      }
      light->ambient = vector;
      break;
    case GL_DIFFUSE:
      if (param_type != kParamTypeArray) {
        GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_DIFFUSE.");
        return;
      }
      light->diffuse = vector;
      break;
    case GL_SPECULAR:
      if (param_type != kParamTypeArray) {
        GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_SPECULAR.");
        return;
      }
      light->specular = vector;
      break;
    case GL_POSITION:
      if (param_type != kParamTypeArray) {
        GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_POSITION.");
        return;
      }
      light->position.AssignMatrixMultiply(mv, vector);
      break;
    case GL_SPOT_DIRECTION:
      if (param_type != kParamTypeArray) {
        GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_SPOT_DIRECTION.");
        return;
      }
      vector.Set(3, 0.0f);  // No w-component for spot light direction.
      light->direction.AssignMatrixMultiply(mv, vector);
      light->direction.Set(3, 0.0f);
      break;
    default:
      GLES_ERROR_INVALID_ENUM(name);
      return;
  }
}
}  // namespace

GLES_APIENTRY(void, Lightf, GLenum lightid, GLenum name, GLfloat param) {
  HandleLight(lightid, name, &param, kParamTypeScalar);
}

GLES_APIENTRY(void, Lightfv, GLenum lightid, GLenum name,
                   const GLfloat* params) {
  HandleLight(lightid, name, params, kParamTypeArray);
}

GLES_APIENTRY(void, Lightx, GLenum light, GLenum pname, GLfixed param) {
  glLightf(light, pname, X2F(param));
}

GLES_APIENTRY(void, LightxOES, GLenum light, GLenum pname, GLfixed param) {
  glLightf(light, pname, X2F(param));
}

GLES_APIENTRY(void, Lightxv, GLenum light, GLenum pname,
              const GLfixed* params) {
  GLfloat tmp[kMaxParamElementSize];
  Convert(tmp, ParamSize(pname), params);
  glLightfv(light, pname, tmp);
}

GLES_APIENTRY(void, LightxvOES, GLenum light, GLenum pname,
              const GLfixed* params) {
  glLightxv(light, pname, params);
}

// Sets the width of a line
GLES_APIENTRY(void, LineWidth, GLfloat width) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (width < 0.f) {
    GLES_ERROR_INVALID_VALUE_FLOAT(width);
    return;
  }
  width = ClampValue(width, c->aliased_line_width_range_.Get()[0],
                     c->aliased_line_width_range_.Get()[1]);
  c->line_width_.Mutate() = width;
  PASS_THROUGH(c, LineWidth, width);
}

GLES_APIENTRY(void, LineWidthx, GLfixed width) {
  glLineWidth(X2F(width));
}

GLES_APIENTRY(void, LineWidthxOES, GLfixed width) {
  glLineWidth(X2F(width));
}

// Links the current program given the shaders that have been attached to it.
GLES_APIENTRY(void, LinkProgram, GLuint program) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ProgramDataPtr program_data = sg->GetProgramData(program);
  if (program_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
    return;
  }

  program_data->Link();
}

// Loads the identity matrix into the top of theactive matrix stack.
GLES_APIENTRY(void, LoadIdentity) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  c->uniform_context_.MutateActiveMatrix().AssignIdentity();
}

// Loads a matrix into the top of the active matrix stack.
GLES_APIENTRY(void, LoadMatrixf, const GLfloat* m) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  c->uniform_context_.MutateActiveMatrix().SetColumnMajorArray(m);
}

GLES_APIENTRY(void, LoadMatrixx, const GLfixed* m) {
  GLfloat tmp[kMaxParamElementSize];
  Convert(tmp, 16, m);
  glLoadMatrixf(tmp);
}

GLES_APIENTRY(void, LoadMatrixxOES, const GLfixed* m) {
  glLoadMatrixx(m);
}

// Copy the current model view matrix to a matrix in the matrix palette,
// specified by glCurrentPaletteMatrixOES.
GLES_APIENTRY(void, LoadPaletteFromModelViewMatrixOES) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  const emugl::Matrix& mv_matrix = c->uniform_context_.GetModelViewMatrix();
  c->uniform_context_.MutatePaletteMatrix() = mv_matrix;
}

// Configure fixed function material state.
namespace {
void HandleMaterial(GLenum face, GLenum name, const GLfloat* params,
                    ParamType param_type) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (face != GL_FRONT_AND_BACK) {
    GLES_ERROR_INVALID_ENUM(face);
    return;
  }

  Material& material = c->uniform_context_.MutateMaterial();

  const GLfloat value = params[0];
  const emugl::Vector vector(params[0], params[1], params[2], params[3]);

  switch (name) {
    case GL_SHININESS:
      if (value < 0.f || value > 128.f) {
        GLES_ERROR_INVALID_VALUE_FLOAT(value);
        return;
      }
      material.shininess = value;
      break;
    case GL_AMBIENT:
      if (param_type != kParamTypeArray) {
        GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_AMBIENT.");
        return;
      }
      material.ambient = vector;
      break;
    case GL_DIFFUSE:
      if (param_type != kParamTypeArray) {
        GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_DIFFUSE.");
        return;
      }
      material.diffuse = vector;
      break;
    case GL_SPECULAR:
      if (param_type != kParamTypeArray) {
        GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_SPECULAR.");
        return;
      }
      material.specular = vector;
      break;
    case GL_EMISSION:
      if (param_type != kParamTypeArray) {
        GLES_ERROR(GL_INVALID_ENUM, "Expected array for GL_EMISSION.");
        return;
      }
      material.emission = vector;
      break;
    case GL_AMBIENT_AND_DIFFUSE:
      if (param_type != kParamTypeArray) {
        GLES_ERROR(GL_INVALID_ENUM,
                    "Expected array for GL_AMBIENT_AND_DIFFUSE.");
        return;
      }
      material.ambient = vector;
      material.diffuse = vector;
      break;
    default:
      GLES_ERROR_INVALID_ENUM(name);
      return;
  }
}
}  // namespace

GLES_APIENTRY(GLvoid*, MapTexSubImage2DCHROMIUM, GLenum target, GLint level,
              GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
              GLenum format, GLenum type, GLenum access) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return NULL;
  }

  return PASS_THROUGH(c, MapTexSubImage2DCHROMIUM, target, level, xoffset,
                      yoffset, width, height, format, type, access);
}

GLES_APIENTRY(void, Materialf, GLenum face, GLenum name, GLfloat param) {
  HandleMaterial(face, name, &param, kParamTypeScalar);
}

GLES_APIENTRY(void, Materialfv, GLenum face, GLenum name,
              const GLfloat* params) {
  HandleMaterial(face, name, params, kParamTypeArray);
}

GLES_APIENTRY(void, Materialx, GLenum face, GLenum pname, GLfixed param) {
  glMaterialf(face, pname, X2F(param));
}

GLES_APIENTRY(void, MaterialxOES, GLenum face, GLenum pname, GLfixed param) {
  glMaterialx(face, pname, param);
}

GLES_APIENTRY(void, Materialxv, GLenum face, GLenum pname,
              const GLfixed* params) {
  GLfloat tmp[kMaxParamElementSize];
  Convert(tmp, ParamSize(pname), params);
  glMaterialfv(face, pname, tmp);
}

GLES_APIENTRY(void, MaterialxvOES, GLenum face, GLenum pname,
              const GLfixed* params) {
  glMaterialxv(face, pname, params);
}

// Selects the active matrix statck.
GLES_APIENTRY(void, MatrixMode, GLenum mode) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  const bool success = c->uniform_context_.SetMatrixMode(mode);
  if (!success) {
    GLES_ERROR_INVALID_ENUM(mode);
  }
}

// Set matrix indices used to blend corresponding matrices for a given vertex.
GLES_APIENTRY(void, MatrixIndexPointerOES, GLint size, GLenum type,
              GLsizei stride, const GLvoid* pointer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }
  if (!IsValidMatrixIndexPointerSize(size)) {
    GLES_ERROR_INVALID_VALUE_INT(size);
    return;
  }
  if (!IsValidMatrixIndexPointerType(type)) {
    GLES_ERROR_INVALID_ENUM(type);
    return;
  }
  const GLboolean normalized = GL_FALSE;
  c->pointer_context_.SetPointer(kMatrixIndexVertexAttribute, size, type,
                                 stride, pointer, normalized);
}

GLES_APIENTRY(void, MatrixIndexPointerOESBounds, GLint size, GLenum type,
              GLsizei stride, const GLvoid* pointer, GLsizei count) {
  glMatrixIndexPointerOES(size, type, stride, pointer);
}

// Multiplies the matrix on top of the active matrix stack by the given matrix.
GLES_APIENTRY(void, MultMatrixf, const GLfloat* m) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  c->uniform_context_.MutateActiveMatrix() *= emugl::Matrix(m);
}

GLES_APIENTRY(void, MultMatrixx, const GLfixed* m) {
  GLfloat tmp[kMaxParamElementSize];
  Convert(tmp, 16, m);
  glMultMatrixf(tmp);
}

GLES_APIENTRY(void, MultMatrixxOES, const GLfixed* m) {
  glMultMatrixx(m);
}

// Sets up unvarying normal vector for the fixed function pipeline.
GLES_APIENTRY(void, Normal3f, GLfloat nx, GLfloat ny, GLfloat nz) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  emugl::Vector& normal = c->uniform_context_.MutateNormal();
  normal = emugl::Vector(nx, ny, nz, 0.f);
}

GLES_APIENTRY(void, Normal3fv, const GLfloat* coords) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  emugl::Vector& normal = c->uniform_context_.MutateNormal();
  for (int i = 0; i < 3; ++i) {
    normal.Set(i, coords[i]);
  }
}

GLES_APIENTRY(void, Normal3sv, const GLshort* coords) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  emugl::Vector& normal = c->uniform_context_.MutateNormal();
  normal.AssignLinearMapping(coords, 3);
  normal.Set(3, 0.f);
}

GLES_APIENTRY(void, Normal3x, GLfixed nx, GLfixed ny, GLfixed nz) {
  glNormal3f(X2F(nx), X2F(ny), X2F(nz));
}

GLES_APIENTRY(void, Normal3xOES, GLfixed nx, GLfixed ny, GLfixed nz) {
  glNormal3f(X2F(nx), X2F(ny), X2F(nz));
}

// Specifies source array for per-vertex normals.
GLES_APIENTRY(void, NormalPointer, GLenum type, GLsizei stride,
              const GLvoid* pointer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidNormalPointerType(type)) {
    GLES_ERROR_INVALID_ENUM(type);
    return;
  }
  if (stride < 0) {
    GLES_ERROR_INVALID_VALUE_INT(stride);
    return;
  }
  const GLboolean normalized = (type != GL_FLOAT && type != GL_FIXED);
  c->pointer_context_.SetPointer(kNormalVertexAttribute, 3, type, stride,
                                 pointer, normalized);
}

GLES_APIENTRY(void, NormalPointerBounds, GLenum type, GLsizei stride,
              const GLvoid* pointer, GLsizei count) {
  glNormalPointer(type, stride, pointer);
}

// Multiplies the current matrix with the specified orthogaphic matrix.
GLES_APIENTRY(void, Orthof, GLfloat left, GLfloat right, GLfloat bottom,
              GLfloat top, GLfloat z_near, GLfloat z_far) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  c->uniform_context_.MutateActiveMatrix() *=
      emugl::Matrix::GenerateOrthographic(left, right, bottom, top, z_near,
                                        z_far);
}

GLES_APIENTRY(void, OrthofOES, GLfloat left, GLfloat right, GLfloat bottom,
              GLfloat top, GLfloat zNear, GLfloat zFar) {
  glOrthof(left, right, bottom, top, zNear, zFar);
}

GLES_APIENTRY(void, Orthox, GLfixed left, GLfixed right, GLfixed bottom,
              GLfixed top, GLfixed zNear, GLfixed zFar) {
  glOrthof(X2F(left), X2F(right), X2F(bottom), X2F(top), X2F(zNear),
             X2F(zFar));
}

GLES_APIENTRY(void, OrthoxOES, GLfixed left, GLfixed right, GLfixed bottom,
              GLfixed top, GLfixed zNear, GLfixed zFar) {
  glOrthof(X2F(left), X2F(right), X2F(bottom), X2F(top), X2F(zNear),
             X2F(zFar));
}

// Configures pixel storage (write) operation, when transfering to GL.
GLES_APIENTRY(void, PixelStorei, GLenum pname, GLint param) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidPixelStoreName(pname)) {
    GLES_ERROR_INVALID_ENUM(pname);
    return;
  }
  if (!IsValidPixelStoreAlignment(param)) {
    GLES_ERROR_INVALID_VALUE_INT(param);
    return;
  }

  switch (pname) {
    case GL_PACK_ALIGNMENT:
      c->pixel_store_pack_alignment_.Mutate() = param;
      break;
    case GL_UNPACK_ALIGNMENT:
      c->pixel_store_unpack_alignment_.Mutate() = param;
      break;
  }
  PASS_THROUGH(c, PixelStorei, pname, param);
}

// Configure point rendering.
namespace {
void HandlePointParameter(GLenum pname, const GLfloat* params,
                          ParamType param_type) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PointParameters& point_parameters =
      c->uniform_context_.MutatePointParameters();

  switch (pname) {
    case GL_POINT_SIZE_MIN:
      if (params[0] < 0.f) {
        GLES_ERROR_INVALID_VALUE_FLOAT(params[0]);
        return;
      }
      point_parameters.size_min = params[0];
      return;

    case GL_POINT_SIZE_MAX:
      if (params[0] < 0.f) {
        GLES_ERROR_INVALID_VALUE_FLOAT(params[0]);
        return;
      }
      point_parameters.size_max = params[0];
      return;

    case GL_POINT_FADE_THRESHOLD_SIZE:
      if (params[0] < 0.f) {
        GLES_ERROR_INVALID_VALUE_FLOAT(params[0]);
        return;
      }
      // TODO(crbug.com/441918): Support GL_POINT_FADE_THRESHOLD_SIZE.
      LOG_ALWAYS_FATAL(
          "Unsupported PointParameter GL_POINT_FADE_THRESHOLD_SIZE.");
      return;

    case GL_POINT_DISTANCE_ATTENUATION:
      if (param_type == kParamTypeScalar) {
        GLES_ERROR(GL_INVALID_ENUM,
                    "Expected array for GL_POINT_DISTANCE_ATTENUATION.");
        return;
      }
      point_parameters.attenuation[0] = params[0];
      point_parameters.attenuation[1] = params[1];
      point_parameters.attenuation[2] = params[2];
      return;

    default:
      GLES_ERROR(GL_INVALID_ENUM, "Unknown point parameter %s (%d)",
                  GetEnumString(pname), pname);
      return;
  }
}

}  // namespace

GLES_APIENTRY(void, PointParameterf, GLenum pname, GLfloat param) {
  HandlePointParameter(pname, &param, kParamTypeScalar);
}

GLES_APIENTRY(void, PointParameterfv, GLenum pname, const GLfloat* params) {
  HandlePointParameter(pname, params, kParamTypeArray);
}

GLES_APIENTRY(void, PointParameterx, GLenum pname, GLfixed param) {
  glPointParameterf(pname, X2F(param));
}

GLES_APIENTRY(void, PointParameterxOES, GLenum pname, GLfixed param) {
  glPointParameterf(pname, X2F(param));
}

GLES_APIENTRY(void, PointParameterxv, GLenum pname, const GLfixed* params) {
  GLfloat tmp[kMaxParamElementSize];
  Convert(tmp, ParamSize(pname), params);
  glPointParameterfv(pname, tmp);
}

GLES_APIENTRY(void, PointParameterxvOES, GLenum pname, const GLfixed* params) {
  glPointParameterxv(pname, params);
}

// Sets the point size that is used when no vertex point size array is enabled.
GLES_APIENTRY(void, PointSize, GLfloat size) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (size <= 0.f) {
    GLES_ERROR_INVALID_VALUE_FLOAT(size);
    return;
  }
  c->uniform_context_.MutatePointParameters().current_size = size;
}

GLES_APIENTRY(void, PointSizex, GLfixed size) {
  glPointSize(X2F(size));
}

GLES_APIENTRY(void, PointSizexOES, GLfixed size) {
  glPointSize(X2F(size));
}

// Specifies source array for per-vertex point sizes.
GLES_APIENTRY(void, PointSizePointerOES, GLenum type, GLsizei stride,
              const GLvoid* pointer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidPointPointerType(type)) {
    GLES_ERROR_INVALID_ENUM(type);
    return;
  }
  if (stride < 0) {
    GLES_ERROR_INVALID_VALUE_INT(stride);
    return;
  }
  LOG_ALWAYS_FATAL_IF(type != GL_FLOAT, "Untested");
  c->pointer_context_.SetPointer(kPointSizeVertexAttribute, 1, type, stride,
                                 pointer);
}

GLES_APIENTRY(void, PointSizePointer, GLenum type, GLsizei stride,
              const GLvoid* pointer) {
  glPointSizePointerOES(type, stride, pointer);
}

GLES_APIENTRY(void, PointSizePointerOESBounds, GLenum type, GLsizei stride,
              const GLvoid* pointer, GLsizei count) {
  glPointSizePointerOES(type, stride, pointer);
}

// Configures a depth offset applied to all fragments (eg. for "fixing"
// problems with rendering decals/overlays on top of faces).
GLES_APIENTRY(void, PolygonOffset, GLfloat factor, GLfloat units) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  c->polygon_offset_factor_.Mutate() = factor;
  c->polygon_offset_units_.Mutate() = units;
  PASS_THROUGH(c, PolygonOffset, factor, units);
}

GLES_APIENTRY(void, PolygonOffsetx, GLfixed factor, GLfixed units) {
  glPolygonOffset(X2F(factor), X2F(units));
}

GLES_APIENTRY(void, PolygonOffsetxOES, GLfixed factor, GLfixed units) {
  glPolygonOffset(X2F(factor), X2F(units));
}

// Pops matrix state.
GLES_APIENTRY(void, PopMatrix) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  const bool success = c->uniform_context_.ActiveMatrixPop();
  if (!success) {
    GLES_ERROR(GL_STACK_UNDERFLOW, "PopMatrix called when stack is empty.");
  }
}

// Pushes matrix state.
GLES_APIENTRY(void, PushMatrix) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  const bool success = c->uniform_context_.ActiveMatrixPush();
  if (!success) {
    GLES_ERROR(GL_STACK_OVERFLOW, "PushMatrix called when stack is full.");
  }
}

// Reads pixels from the frame buffer.
GLES_APIENTRY(void, ReadPixels, GLint x, GLint y, GLsizei width,
              GLsizei height, GLenum format, GLenum type, GLvoid* pixels) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, ReadPixels, x, y, width, height, format, type, pixels);
}

GLES_APIENTRY(void, ReleaseShaderCompiler) {
  // Since this is only a hint that shader compilations are unlikely to occur,
  // we can ignore it.
}

// Configures a renderbuffer.
GLES_APIENTRY(void, RenderbufferStorage, GLenum target, GLenum internalformat,
              GLsizei width, GLsizei height) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidRenderbufferTarget(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }
  if (!IsValidRenderbufferFormat(internalformat)) {
    GLES_ERROR_INVALID_ENUM(internalformat);
    return;
  }
  if (width < 0 || width > c->max_renderbuffer_size_.Get()) {
    GLES_ERROR_INVALID_VALUE_INT(width);
    return;
  }
  if (height < 0 || height > c->max_renderbuffer_size_.Get()) {
    GLES_ERROR_INVALID_VALUE_INT(height);
    return;
  }

  RenderbufferDataPtr obj = c->GetBoundRenderbufferData();
  if (obj == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "No renderbuffer bound.");
    return;
  }

  obj->SetDataStore(target, internalformat, width, height);
  PASS_THROUGH(c, RenderbufferStorage, target, internalformat, width, height);
}

GLES_APIENTRY(void, RenderbufferStorageOES, GLenum target,
              GLenum internalformat, GLsizei width, GLsizei height) {
  glRenderbufferStorage(target, internalformat, width, height);
}

// Applies a rotation to the top of the current matrix stack.
GLES_APIENTRY(void, Rotatef, GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  emugl::Vector v(x, y, z, 0);
  c->uniform_context_.MutateActiveMatrix() *=
      emugl::Matrix::GenerateRotationByDegrees(angle, v);
}

GLES_APIENTRY(void, Rotatex, GLfixed angle, GLfixed x, GLfixed y, GLfixed z) {
  glRotatef(X2F(angle), X2F(x), X2F(y), X2F(z));
}

GLES_APIENTRY(void, RotatexOES, GLfixed angle, GLfixed x, GLfixed y,
              GLfixed z) {
  glRotatef(X2F(angle), X2F(x), X2F(y), X2F(z));
}

// Changes the way fragment coverage is computed.
GLES_APIENTRY(void, SampleCoverage, GLclampf value, GLboolean invert) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  value = ClampValue(value, 0.f, 1.f);
  c->sample_coverage_value_.Mutate() = value;
  c->sample_coverage_invert_.Mutate() = invert != GL_FALSE ? GL_TRUE : GL_FALSE;
  PASS_THROUGH(c, SampleCoverage, value, invert);
}

GLES_APIENTRY(void, SampleCoveragex, GLclampx value, GLboolean invert) {
  glSampleCoverage(X2F(value), invert);
}

GLES_APIENTRY(void, SampleCoveragexOES, GLclampx value, GLboolean invert) {
  glSampleCoverage(X2F(value), invert);
}

// Applies a scale to the top of the current matrix stack.
GLES_APIENTRY(void, Scalef, GLfloat x, GLfloat y, GLfloat z) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  emugl::Vector v(x, y, z, 1);
  c->uniform_context_.MutateActiveMatrix() *= emugl::Matrix::GenerateScale(v);
}

GLES_APIENTRY(void, Scalex, GLfixed x, GLfixed y, GLfixed z) {
  glScalef(X2F(x), X2F(y), X2F(z));
}

GLES_APIENTRY(void, ScalexOES, GLfixed x, GLfixed y, GLfixed z) {
  glScalef(X2F(x), X2F(y), X2F(z));
}

// Configures fragment rejection based on whether or not it is inside a
// screen-space rectangle.
GLES_APIENTRY(void, Scissor, GLint x, GLint y, GLsizei width, GLsizei height) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, Scissor, x, y, width, height);
}

// Configures the shading model to use (how to interpolate colors across
// triangles).
GLES_APIENTRY(void, ShadeModel, GLenum mode) {
  // There is no efficient way to emulate GL_FLAT mode with fragment
  // shaders since they automatically interpolate color values.
  ALOGW_IF(mode != GL_SMOOTH, "Only GL_SMOOTH shading model supported");
}

// Loads the source code for a shader into a shader object.
GLES_APIENTRY(void, ShaderSource, GLuint shader, GLsizei count,
              const GLchar* const* string, const GLint* length) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (count < 0) {
    GLES_ERROR_INVALID_VALUE_INT(count);
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ShaderDataPtr shader_data = sg->GetShaderData(shader);
  if (shader_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Not a valid shader %d", shader);
    return;
  }

  shader_data->SetSource(count, string, length);
}

// Set front and back function and reference value for stencil testing.
GLES_APIENTRY(void, StencilFunc, GLenum func, GLint ref, GLuint mask) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidStencilFunc(func)) {
    GLES_ERROR_INVALID_ENUM(func);
    return;
  }

  c->stencil_func_.Mutate() = func;
  c->stencil_ref_.Mutate() = ref;
  c->stencil_value_mask_.Mutate() = mask;
  PASS_THROUGH(c, StencilFunc, func, ref, mask);
}

// Set front and/or back function and reference value for stencil testing.
GLES_APIENTRY(void, StencilFuncSeparate, GLenum face, GLenum func, GLint ref,
              GLuint mask) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, StencilFuncSeparate, face, func, ref, mask);
}

// Control the front and back writing of individual bits in the stencil
// planes.
GLES_APIENTRY(void, StencilMask, GLuint mask) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, StencilMask, mask);
}

// Control the front and/or back writing of individual bits in the stencil
// planes.
GLES_APIENTRY(void, StencilMaskSeparate, GLenum face, GLuint mask) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, StencilMaskSeparate, face, mask);
}

// Set front and back stencil test actions.
GLES_APIENTRY(void, StencilOp, GLenum sfail, GLenum zfail, GLenum zpass) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, StencilOp, sfail, zfail, zpass);
}

// Set front and/or back stencil test actions.
GLES_APIENTRY(void, StencilOpSeparate, GLenum face, GLenum sfail, GLenum zfail,
              GLenum zpass) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, StencilOpSeparate, face, sfail, zfail, zpass);
}

// Specifies source array for per-vertex texture coordinates.
GLES_APIENTRY(void, TexCoordPointer, GLint size, GLenum type, GLsizei stride,
              const GLvoid* pointer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidTexCoordPointerType(type)) {
    GLES_ERROR_INVALID_ENUM(type);
    return;
  }
  if (!IsValidTexCoordPointerSize(size)) {
    GLES_ERROR_INVALID_VALUE_INT(size);
    return;
  }
  if (stride < 0) {
    GLES_ERROR_INVALID_VALUE_INT(stride);
    return;
  }
  const GLuint index = c->texture_context_.GetClientActiveTextureCoordAttrib();
  c->pointer_context_.SetPointer(index, size, type, stride, pointer);
}

GLES_APIENTRY(void, TexCoordPointerBounds, GLint size, GLenum type,
              GLsizei stride, const GLvoid* pointer, GLsizei count) {
  glTexCoordPointer(size, type, stride, pointer);
}

// Sets up a texture environment for the current texture unit for the fixed
// function pipeline.
namespace {
template <typename T>
void HandleTexEnv(GLenum target, GLenum pname, T* params,
                  ParamType param_type) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  TexEnv& texenv = c->uniform_context_.MutateActiveTexEnv();
  if (target != GL_TEXTURE_ENV) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }

  const GLenum param = static_cast<GLenum>(params[0]);
  const float value = static_cast<float>(params[0]);
  switch (pname) {
    case GL_TEXTURE_ENV_MODE:
      if (!IsValidTexEnvMode(param)) {
        GLES_ERROR_INVALID_ENUM(param);
        return;
      }
      texenv.mode = param;
      break;
    case GL_SRC0_RGB:
    case GL_SRC1_RGB:
    case GL_SRC2_RGB:
      if (!IsValidTexEnvSource(param)) {
        GLES_ERROR_INVALID_ENUM(param);
        return;
      }
      texenv.src_rgb[pname - GL_SRC0_RGB] = param;
      break;
    case GL_SRC0_ALPHA:
    case GL_SRC1_ALPHA:
    case GL_SRC2_ALPHA:
      if (!IsValidTexEnvSource(param)) {
        GLES_ERROR_INVALID_ENUM(param);
        return;
      }
      texenv.src_alpha[pname - GL_SRC0_ALPHA] = param;
      break;
    case GL_OPERAND0_RGB:
    case GL_OPERAND1_RGB:
    case GL_OPERAND2_RGB:
      if (!IsValidTexEnvOperandRgb(param)) {
        GLES_ERROR_INVALID_ENUM(param);
        return;
      }
      texenv.operand_rgb[pname - GL_OPERAND0_RGB] = param;
      break;
    case GL_OPERAND0_ALPHA:
    case GL_OPERAND1_ALPHA:
    case GL_OPERAND2_ALPHA:
      if (!IsValidTexEnvOperandAlpha(param)) {
        GLES_ERROR_INVALID_ENUM(param);
        return;
      }
      texenv.operand_alpha[pname - GL_OPERAND0_ALPHA] = param;
      break;
    case GL_COMBINE_RGB:
      if (!IsValidTexEnvCombineRgb(param)) {
        GLES_ERROR_INVALID_ENUM(param);
        return;
      }
      texenv.combine_rgb = param;
      break;
    case GL_COMBINE_ALPHA:
      if (!IsValidTexEnvCombineAlpha(param)) {
        GLES_ERROR_INVALID_ENUM(param);
        return;
      }
      texenv.combine_alpha = param;
      break;
    case GL_RGB_SCALE:
      if (!IsValidTexEnvScale(value)) {
        GLES_ERROR_INVALID_VALUE_FLOAT(value);
        return;
      }
      texenv.rgb_scale = value;
      break;
    case GL_ALPHA_SCALE:
      if (!IsValidTexEnvScale(value)) {
        GLES_ERROR_INVALID_VALUE_FLOAT(value);
        return;
      }
      texenv.alpha_scale = value;
      break;
    case GL_TEXTURE_ENV_COLOR:
      if (param_type != kParamTypeArray) {
        GLES_ERROR(GL_INVALID_ENUM,
                    "Expected array for GL_TEXTURE_ENV_COLOR.");
        return;
      }
      texenv.color.AssignLinearMapping(params, 4);
      texenv.color.Clamp(0.f, 1.f);
      break;
    case GL_COORD_REPLACE_OES:
      LOG_ALWAYS_FATAL("GL_COORD_REPLACE_OES not supported.");
      break;
    default:
      GLES_ERROR_INVALID_ENUM(pname);
      break;
  }
}
}  // namespace

GLES_APIENTRY(void, TexEnvf, GLenum target, GLenum pname, GLfloat param) {
  HandleTexEnv(target, pname, &param, kParamTypeScalar);
}

GLES_APIENTRY(void, TexEnvfv, GLenum target, GLenum pname,
                   const GLfloat* params) {
  HandleTexEnv(target, pname, params, kParamTypeArray);
}

GLES_APIENTRY(void, TexEnvi, GLenum target, GLenum pname, GLint param) {
  HandleTexEnv(target, pname, &param, kParamTypeScalar);
}

GLES_APIENTRY(void, TexEnviv, GLenum target, GLenum pname,
                   const GLint* params) {
  HandleTexEnv(target, pname, params, kParamTypeArray);
}

GLES_APIENTRY(void, TexEnvx, GLenum target, GLenum pname, GLfixed param) {
  glTexEnvf(target, pname, static_cast<GLfloat>(param));
}

GLES_APIENTRY(void, TexEnvxOES, GLenum target, GLenum pname, GLfixed param) {
  glTexEnvf(target, pname, static_cast<GLfloat>(param));
}

GLES_APIENTRY(void, TexEnvxv, GLenum target, GLenum pname,
              const GLfixed* params) {
  GLfloat tmp[kMaxParamElementSize];
  if (pname == GL_TEXTURE_ENV_COLOR) {
    Convert(tmp, ParamSize(pname), params);
  } else {
    tmp[0] = static_cast<GLfloat>(params[0]);
  }
  glTexEnvfv(target, pname, tmp);
}

GLES_APIENTRY(void, TexEnvxvOES, GLenum target, GLenum pname,
              const GLfixed* params) {
  glTexEnvxv(target, pname, params);
}

// Control the generation of texture coordinates.
GLES_APIENTRY(void, TexGeniOES, GLenum coord, GLenum pname, GLint param) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidTexGenCoord(coord)) {
    GLES_ERROR_INVALID_ENUM(coord);
    return;
  }

  // opengles_spec_1_1_extension_pack.pdf 4.1
  // Only GL_TEXTURE_GEN_MODE_OES is supported
  switch (pname) {
    case GL_TEXTURE_GEN_MODE_OES: {
        if (!IsValidTexGenMode(param)) {
          GLES_ERROR_INVALID_ENUM(param);
          return;
        }
        TexGen& texgen = c->uniform_context_.MutateActiveTexGen();
        texgen.mode = param;
        break;
      }
    default:
      GLES_ERROR_INVALID_ENUM(pname);
      break;
  }
}

GLES_APIENTRY(void, TexGenivOES, GLenum coord, GLenum pname,
              const GLint* params) {
  glTexGeniOES(coord, pname, params[0]);
}

GLES_APIENTRY(void, TexGenxOES, GLenum coord, GLenum pname, GLfixed param) {
  glTexGeniOES(coord, pname, param);
}

GLES_APIENTRY(void, TexGenxvOES, GLenum coord, GLenum pname,
              const GLfixed* params) {
  glTexGenivOES(coord, pname, params);
}

// Loads pixel data into a texture object.
GLES_APIENTRY(void, TexImage2D, GLenum target, GLint level,
              GLint internalformat, GLsizei width, GLsizei height,
              GLint border, GLenum format, GLenum type,
              const GLvoid* pixels) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (target == GL_TEXTURE_EXTERNAL_OES) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }

  if (!IsValidTextureTargetEx(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }
  if (!IsValidPixelFormat(format)) {
    GLES_ERROR_INVALID_ENUM(format);
    return;
  }
  if (!IsValidPixelType(type)) {
    GLES_ERROR_INVALID_ENUM(type);
    return;
  }
  if (!IsValidPixelFormatAndType(format, type)) {
    GLES_ERROR(GL_INVALID_ENUM, "Invalid format/type: %s %s",
                GetEnumString(format), GetEnumString(type));
    return;
  }
  if (internalformat != ((GLint)format)) {
    GLES_ERROR(GL_INVALID_OPERATION, "Format must match internal format.");
    return;
  }
  if (border != 0) {
    GLES_ERROR_INVALID_VALUE_INT(border);
    return;
  }
  if (level < 0 || level > c->texture_context_.GetMaxLevels()) {
    GLES_ERROR_INVALID_VALUE_INT(level);
    return;
  }
  if (width < 0 || width > c->max_texture_size_.Get()) {
    GLES_ERROR_INVALID_VALUE_INT(width);
    return;
  }
  if (height < 0 || height > c->max_texture_size_.Get()) {
    GLES_ERROR_INVALID_VALUE_INT(height);
    return;
  }

  TextureDataPtr tex = c->GetBoundTextureData(target);
  LOG_ALWAYS_FATAL_IF(tex == NULL, "There should always be a texture bound.");

  tex->Set(level, width, height, format, type);

  if (tex->IsEglImageAttached()) {
    // This texture was a target of EGLImage, but now it is re-defined
    // so we need to detach from the EGLImage and re-generate a
    // global texture name for it, and reset the texture to be bound to a
    // TEXTURE_2D target.
    tex->DetachEglImage();
    GLuint global = 0;
    PASS_THROUGH(c, GenTextures, 1, &global);
    const GLuint name = tex->GetLocalName();
    c->GetShareGroup()->SetTextureGlobalName(name, global);
    c->texture_context_.SetTargetTexture(GL_TEXTURE_2D, name, GL_TEXTURE_2D);
    PASS_THROUGH(c, BindTexture, GL_TEXTURE_2D, global);
  }

  c->texture_context_.EnsureCorrectBinding(target);
  PASS_THROUGH(c, TexImage2D, target, level, internalformat, width, height,
               border, format, type, pixels);
  c->texture_context_.RestoreBinding(target);

  if (tex->IsAutoMipmap() && level == 0) {
    // TODO(crbug.com/441913): Update information for all levels.
    PASS_THROUGH(c, GenerateMipmap, target);
  }
}

// Configure a texture object.
GLES_APIENTRY(void, TexParameterf, GLenum target, GLenum pname,
              GLfloat param) {
  glTexParameterfv(target, pname, &param);
}

GLES_APIENTRY(void, TexParameterfv, GLenum target, GLenum pname,
              const GLfloat* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidTextureTarget(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }
  if (!IsValidTextureParam(pname)) {
    GLES_ERROR_INVALID_ENUM(pname);
    return;
  }

  if (pname == GL_TEXTURE_CROP_RECT_OES) {
    TextureDataPtr tex = c->GetBoundTextureData(target);
    if (tex != NULL) {
      tex->SetCropRect(reinterpret_cast<const GLint*>(params));
    }
  } else if (pname == GL_GENERATE_MIPMAP) {
    TextureDataPtr tex = c->GetBoundTextureData(target);
    if (tex != NULL) {
      tex->SetAutoMipmap(*params);
    }
  } else {
    GLenum global_target = c->texture_context_.EnsureCorrectBinding(target);
    PASS_THROUGH(c, TexParameterfv, global_target, pname, params);
    c->texture_context_.RestoreBinding(target);
  }
}

GLES_APIENTRY(void, TexParameteri, GLenum target, GLenum pname, GLint param) {
  glTexParameteriv(target, pname, &param);
}

GLES_APIENTRY(void, TexParameteriv, GLenum target, GLenum pname,
              const GLint* params) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidTextureTarget(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }
  if (!IsValidTextureParam(pname)) {
    GLES_ERROR_INVALID_ENUM(pname);
    return;
  }

  if (pname == GL_TEXTURE_CROP_RECT_OES) {
    TextureDataPtr tex = c->GetBoundTextureData(target);
    if (tex != NULL) {
      tex->SetCropRect(params);
    }
  } else if (pname == GL_GENERATE_MIPMAP) {
    TextureDataPtr tex = c->GetBoundTextureData(target);
    if (tex != NULL) {
      tex->SetAutoMipmap(*params);
    }
  } else {
    GLenum global_target = c->texture_context_.EnsureCorrectBinding(target);
    PASS_THROUGH(c, TexParameteriv, global_target, pname, params);
    c->texture_context_.RestoreBinding(target);
  }
}

GLES_APIENTRY(void, TexParameterx, GLenum target, GLenum pname,
              GLfixed param) {
  glTexParameterxv(target, pname, &param);
}

GLES_APIENTRY(void, TexParameterxOES, GLenum target, GLenum pname,
              GLfixed param) {
  glTexParameterxv(target, pname, &param);
}

GLES_APIENTRY(void, TexParameterxv, GLenum target, GLenum pname,
              const GLfixed* params) {
  GLfloat tmp[kMaxParamElementSize];
  Convert(tmp, ParamSize(pname), params);
  if (pname == GL_TEXTURE_WRAP_S || pname == GL_TEXTURE_WRAP_T ||
      pname == GL_TEXTURE_CROP_RECT_OES) {
    glTexParameteriv(target, pname, reinterpret_cast<const GLint*>(params));
  } else {
    glTexParameterfv(target, pname, tmp);
  }
}

GLES_APIENTRY(void, TexParameterxvOES, GLenum target, GLenum pname,
              const GLfixed* params) {
  glTexParameterxv(target, pname, params);
}

namespace {

bool HandleTexSubImage2D(GLenum target, GLint level, GLint xoffset,
                         GLint yoffset, GLsizei width, GLsizei height,
                         GLenum format, GLenum type, const GLvoid* pixels) {
  ContextPtr c = GetCurrentGlesContext();
  ALOG_ASSERT(c != NULL);
  TextureDataPtr tex = c->GetBoundTextureData(target);
  const GLenum current_format = tex->GetFormat(level);
  const GLenum current_type = tex->GetType(level);

  const bool requires_conversion =
      (format != current_format || type != current_type);
  if (!requires_conversion) {
    PASS_THROUGH(c, TexSubImage2D, target, level, xoffset, yoffset, width,
                 height, format, type, pixels);
  } else {
    TextureConverter converter(format, type, current_format, current_type);
    if (!converter.IsValid()) {
      GLES_ERROR(GL_INVALID_OPERATION,
                  "Texture conversion from %s %s to %s %s not supported.",
                  GetEnumString(format), GetEnumString(type),
                  GetEnumString(current_format), GetEnumString(current_type));
      return false;
    }

    // Set unpack alignment to 4, because TextureConverter always generates
    // texture with alignment 4.
    if (c->pixel_store_unpack_alignment_.Get() != 4) {
      PASS_THROUGH(c, PixelStorei, GL_UNPACK_ALIGNMENT, 4);
    }

    GLvoid* buffer = PASS_THROUGH(c, MapTexSubImage2DCHROMIUM, target, level,
                                  xoffset, yoffset, width, height,
                                  current_format, current_type,
                                  GL_WRITE_ONLY_OES);
    if (!buffer) {
      GLES_ERROR(GL_OUT_OF_MEMORY, "Out of memory.");
      return false;
    }
    converter.Convert(width, height, c->pixel_store_unpack_alignment_.Get(),
                      pixels, buffer);
    PASS_THROUGH(c, UnmapTexSubImage2DCHROMIUM, buffer);

    // Restore unpack alignment.
    if (c->pixel_store_unpack_alignment_.Get() != 4) {
      PASS_THROUGH(c, PixelStorei, GL_UNPACK_ALIGNMENT,
                   c->pixel_store_unpack_alignment_.Get());
    }
  }
  return true;
}

}  // namespace

// Redefines a contiguous subregion of a texture.  In GLES 2 profile
// width, height, format, type, and data must match the values originally
// specified to TexImage2D.  See es_full_spec_2.0.25.pdf section 3.7.2.
GLES_APIENTRY(void, TexSubImage2D, GLenum target, GLint level, GLint xoffset,
              GLint yoffset, GLsizei width, GLsizei height, GLenum format,
              GLenum type, const GLvoid* pixels) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  LOG_ALWAYS_FATAL_IF(target == GL_TEXTURE_EXTERNAL_OES);

  if (!IsValidTextureTargetEx(target)) {
    GLES_ERROR_INVALID_ENUM(target);
    return;
  }
  if (!IsValidPixelFormat(format)) {
    GLES_ERROR_INVALID_ENUM(format);
    return;
  }
  if (!IsValidPixelType(type)) {
    GLES_ERROR_INVALID_ENUM(type);
    return;
  }
  if (!IsValidPixelFormatAndType(format, type)) {
    GLES_ERROR(GL_INVALID_ENUM, "Invalid format/type: %s %s",
                GetEnumString(format), GetEnumString(type));
    return;
  }
  if (level < 0 || level > c->texture_context_.GetMaxLevels()) {
    GLES_ERROR_INVALID_VALUE_INT(level);
    return;
  }
  TextureDataPtr tex = c->GetBoundTextureData(target);
  if (width < 0 || (xoffset + width) > static_cast<GLsizei>(tex->GetWidth())) {
    GLES_ERROR_INVALID_VALUE_INT(width);
    return;
  }
  if (height < 0 || (yoffset + height) > static_cast<GLsizei>(tex->GetHeight())) {
    GLES_ERROR_INVALID_VALUE_INT(height);
    return;
  }
  if (xoffset < 0) {
    GLES_ERROR_INVALID_VALUE_INT(xoffset);
    return;
  }
  if (yoffset < 0) {
    GLES_ERROR_INVALID_VALUE_INT(yoffset);
    return;
  }

  if (HandleTexSubImage2D(target, level, xoffset, yoffset, width, height,
                          format, type, pixels)) {
    if (tex != NULL && tex->IsAutoMipmap() && level == 0) {
      // TODO(crbug.com/441913): Update information for all levels.
      PASS_THROUGH(c, GenerateMipmap, target);
    }
  }
}

// Applies a translation to the top of the current matrix stack.
GLES_APIENTRY(void, Translatef, GLfloat x, GLfloat y, GLfloat z) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  emugl::Vector v(x, y, z, 1);
  c->uniform_context_.MutateActiveMatrix() *=
      emugl::Matrix::GenerateTranslation(v);
}

GLES_APIENTRY(void, Translatex, GLfixed x, GLfixed y, GLfixed z) {
  glTranslatef(X2F(x), X2F(y), X2F(z));
}

GLES_APIENTRY(void, TranslatexOES, GLfixed x, GLfixed y, GLfixed z) {
  glTranslatef(X2F(x), X2F(y), X2F(z));
}

namespace {

ProgramDataPtr GetCurrentProgramData() {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return ProgramDataPtr();
  }

  ProgramDataPtr program_data = c->GetCurrentUserProgram();
  if (program_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "No active program on this context");
  }
  return program_data;
}

}  // namespace

// Loads values into the active uniform state.
GLES_APIENTRY(void, Uniform1f, GLint location, GLfloat x) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    program_data->Uniformfv(location, GL_FLOAT, 1, &x);
  }
}

GLES_APIENTRY(void, Uniform1fv, GLint location, GLsizei count,
              const GLfloat* v) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    program_data->Uniformfv(location, GL_FLOAT, count, v);
  }
}

GLES_APIENTRY(void, Uniform1i, GLint location, GLint x) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    program_data->Uniformiv(location, GL_INT, 1, &x);
  }
}

GLES_APIENTRY(void, Uniform1iv, GLint location, GLsizei count,
              const GLint* v) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    program_data->Uniformiv(location, GL_INT, count, v);
  }
}

GLES_APIENTRY(void, Uniform2f, GLint location, GLfloat x, GLfloat y) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    GLfloat params[] = {x, y};
    program_data->Uniformfv(location, GL_FLOAT_VEC2, 1, params);
  }
}

GLES_APIENTRY(void, Uniform2fv, GLint location, GLsizei count,
              const GLfloat* v) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    program_data->Uniformfv(location, GL_FLOAT_VEC2, count, v);
  }
}

GLES_APIENTRY(void, Uniform2i, GLint location, GLint x, GLint y) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    GLint params[] = {x, y};
    program_data->Uniformiv(location, GL_INT_VEC2, 1, params);
  }
}

GLES_APIENTRY(void, Uniform2iv, GLint location, GLsizei count,
              const GLint* v) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    program_data->Uniformiv(location, GL_INT_VEC2, count, v);
  }
}

GLES_APIENTRY(void, Uniform3f, GLint location, GLfloat x, GLfloat y,
              GLfloat z) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    GLfloat params[] = {x, y, z};
    program_data->Uniformfv(location, GL_FLOAT_VEC3, 1, params);
  }
}

GLES_APIENTRY(void, Uniform3fv, GLint location, GLsizei count,
              const GLfloat* v) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    program_data->Uniformfv(location, GL_FLOAT_VEC3, count, v);
  }
}

GLES_APIENTRY(void, Uniform3i, GLint location, GLint x, GLint y, GLint z) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    GLint params[] = {x, y, z};
    program_data->Uniformiv(location, GL_INT_VEC3, 1, params);
  }
}

GLES_APIENTRY(void, Uniform3iv, GLint location, GLsizei count,
              const GLint* v) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    program_data->Uniformiv(location, GL_INT_VEC3, count, v);
  }
}

GLES_APIENTRY(void, Uniform4f, GLint location, GLfloat x, GLfloat y, GLfloat z,
              GLfloat w) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    GLfloat params[] = {x, y, z, w};
    program_data->Uniformfv(location, GL_FLOAT_VEC4, 1, params);
  }
}

GLES_APIENTRY(void, Uniform4fv, GLint location, GLsizei count,
              const GLfloat* v) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    program_data->Uniformfv(location, GL_FLOAT_VEC4, count, v);
  }
}

GLES_APIENTRY(void, Uniform4i, GLint location, GLint x, GLint y, GLint z,
              GLint w) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    GLint params[] = {x, y, z, w};
    program_data->Uniformiv(location, GL_INT_VEC4, 1, params);
  }
}

GLES_APIENTRY(void, Uniform4iv, GLint location, GLsizei count,
              const GLint* v) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    program_data->Uniformiv(location, GL_INT_VEC4, count, v);
  }
}

GLES_APIENTRY(void, UniformMatrix2fv, GLint location, GLsizei count,
              GLboolean transpose, const GLfloat* value) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    program_data->UniformMatrixfv(
        location, GL_FLOAT_MAT2, count, transpose, value);
  }
}

GLES_APIENTRY(void, UniformMatrix3fv, GLint location, GLsizei count,
              GLboolean transpose, const GLfloat* value) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    program_data->UniformMatrixfv(
        location, GL_FLOAT_MAT3, count, transpose, value);
  }
}

GLES_APIENTRY(void, UniformMatrix4fv, GLint location, GLsizei count,
              GLboolean transpose, const GLfloat* value) {
  ProgramDataPtr program_data = GetCurrentProgramData();
  if (program_data != NULL) {
    program_data->UniformMatrixfv(
        location, GL_FLOAT_MAT4, count, transpose, value);
  }
}

GLES_APIENTRY(void, UnmapTexSubImage2DCHROMIUM, const GLvoid* mem) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, UnmapTexSubImage2DCHROMIUM, mem);
}

// Selects a program to use for subsequent rendering.
GLES_APIENTRY(void, UseProgram, GLuint program) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ProgramDataPtr program_ptr = sg->GetProgramData(program);
  if (program_ptr == NULL) {
    if (program) {
      GLES_ERROR(GL_INVALID_OPERATION, "Unknown program name %d", program);
    }
    return;
  }

  c->SetCurrentUserProgram(program_ptr);
}

// Validates the indicated program.
GLES_APIENTRY(void, ValidateProgram, GLuint program) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  ShareGroupPtr sg = c->GetShareGroup();
  ProgramDataPtr program_data = sg->GetProgramData(program);
  if (program_data == NULL) {
    GLES_ERROR(GL_INVALID_OPERATION, "Invalid program %d", program);
    return;
  }

  program_data->Validate();
}

// Sets up a vertex attribute value or array to use for the vertex shader.
GLES_APIENTRY(void, VertexAttrib1f, GLuint indx, GLfloat x) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, VertexAttrib1f, indx, x);
}

GLES_APIENTRY(void, VertexAttrib1fv, GLuint indx, const GLfloat* values) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, VertexAttrib1fv, indx, values);
}

GLES_APIENTRY(void, VertexAttrib2f, GLuint indx, GLfloat x, GLfloat y) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, VertexAttrib2f, indx, x, y);
}

GLES_APIENTRY(void, VertexAttrib2fv, GLuint indx, const GLfloat* values) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, VertexAttrib2fv, indx, values);
}

GLES_APIENTRY(void, VertexAttrib3f, GLuint indx, GLfloat x, GLfloat y,
              GLfloat z) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, VertexAttrib3f, indx, x, y, z);
}

GLES_APIENTRY(void, VertexAttrib3fv, GLuint indx, const GLfloat* values) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, VertexAttrib3fv, indx, values);
}

GLES_APIENTRY(void, VertexAttrib4f, GLuint indx, GLfloat x, GLfloat y,
              GLfloat z, GLfloat w) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, VertexAttrib4f, indx, x, y, z, w);
}

GLES_APIENTRY(void, VertexAttrib4fv, GLuint indx, const GLfloat* values) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  PASS_THROUGH(c, VertexAttrib4fv, indx, values);
}

// Specify an array of generic vertex attribute data.
GLES_APIENTRY(void, VertexAttribPointer, GLuint indx, GLint size, GLenum type,
              GLboolean normalized, GLsizei stride, const GLvoid* pointer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (indx >= (GLuint)c->max_vertex_attribs_.Get()) {
    GLES_ERROR(GL_INVALID_VALUE, "Invalid index: %d", indx);
    return;
  }
  if (!IsValidVertexAttribSize(size)) {
    GLES_ERROR(GL_INVALID_VALUE, "Invalid size: %d", size);
    return;
  }
  if (!IsValidVertexAttribType(type)) {
    GLES_ERROR(GL_INVALID_ENUM, "Invalid type: %d", type);
    return;
  }
  if (stride < 0) {
    GLES_ERROR(GL_INVALID_VALUE, "Invalid stride: %d.", stride);
    return;
  }
  c->pointer_context_.SetPointer(indx, size, type, stride, pointer, normalized);
}

// Specifies source array for vertex coordinates.
GLES_APIENTRY(void, VertexPointer, GLint size, GLenum type, GLsizei stride,
              const GLvoid* pointer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidVertexPointerSize(size)) {
    GLES_ERROR_INVALID_VALUE_INT(size);
    return;
  }
  if (!IsValidVertexPointerType(type)) {
    GLES_ERROR_INVALID_ENUM(type);
    return;
  }
  if (stride < 0) {
    GLES_ERROR_INVALID_VALUE_INT(stride);
    return;
  }
  c->pointer_context_.SetPointer(kPositionVertexAttribute, size, type, stride,
                                 pointer);
}

GLES_APIENTRY(void, VertexPointerBounds, GLint size, GLenum type,
              GLsizei stride, const GLvoid* pointer, GLsizei count) {
  glVertexPointer(size, type, stride, pointer);
}

// Sets up the current viewport.
GLES_APIENTRY(void, Viewport, GLint x, GLint y, GLsizei width, GLsizei height) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (width < 0) {
    GLES_ERROR_INVALID_VALUE_INT(width);
    return;
  }
  if (height < 0) {
    GLES_ERROR_INVALID_VALUE_INT(height);
    return;
  }

  width = ClampValue(width, 0, c->max_viewport_dims_.Get()[0]);
  height = ClampValue(height, 0, c->max_viewport_dims_.Get()[1]);
  GLint (&viewport)[4] = c->viewport_.Mutate();
  viewport[0] = x;
  viewport[1] = y;
  viewport[2] = width;
  viewport[3] = height;
  PASS_THROUGH(c, Viewport, x, y, width, height);
}

// Set the weights used to blend corresponding matrices for a given vertex.
GLES_APIENTRY(void, WeightPointerOES, GLint size, GLenum type, GLsizei stride,
              const GLvoid* pointer) {
  ContextPtr c = GetCurrentGlesContext();
  if (!c) {
    return;
  }

  if (!IsValidWeightPointerSize(size)) {
    GLES_ERROR_INVALID_VALUE_INT(size);
    return;
  }
  if (!IsValidWeightPointerType(type)) {
    GLES_ERROR_INVALID_ENUM(type);
    return;
  }

  const GLboolean normalized = GL_FALSE;
  c->pointer_context_.SetPointer(kWeightVertexAttribute, size, type, stride,
                                 pointer, normalized);
}

GLES_APIENTRY(void, WeightPointerOESBounds, GLint size, GLenum type,
              GLsizei stride, const GLvoid* pointer, GLsizei count) {
  glWeightPointerOES(size, type, stride, pointer);
}
