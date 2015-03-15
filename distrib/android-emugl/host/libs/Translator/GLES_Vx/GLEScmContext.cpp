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

#include "GLEScmContext.h"
#include <stdio.h>
#include "es1xContext.h"

GLEScmContext::GLEScmContext():GLESv2Context(), m_es1xContext(NULL) {
  fprintf(stdout, "Create new GLESv1Context\n");
  return;
}

GLEScmContext::~GLEScmContext(){
  if(m_es1xContext != NULL) {
    fprintf(stdout, "Destroy new GLESv1Context; es1xCtx @ %p\n", m_es1xContext);
    es1xContext_destroy(m_es1xContext);
    m_es1xContext = NULL;
  }
}

void GLEScmContext::init() {
  fprintf(stdout, "___ Init GLESv1Context @ %p\n", this);
  if(!m_initialized) {
    GLESv2Context::init(); // init GLES_2_0 dispatchFuncs, set m_initialized
    m_es1xContext = (void *) es1xContext_create();
  }
  fprintf(stdout, "--- Init GLESv1Context @ %p; es1xCtx @ %p\n", this, m_es1xContext);
}

void * GLEScmContext::getES1xContext() const {
  return m_es1xContext;
}

void GLEScmContext::initExtensionString() {
    // *s_glExtensions = "GL_OES_blend_func_separate GL_OES_blend_equation_separate GL_OES_blend_subtract "
    //                   "GL_OES_byte_coordinates GL_OES_compressed_paletted_texture GL_OES_point_size_array "
    //                   "GL_OES_point_sprite GL_OES_single_precision GL_OES_stencil_wrap GL_OES_texture_env_crossbar "
    //                   "GL_OES_texture_mirored_repeat GL_OES_EGL_image GL_OES_element_index_uint GL_OES_draw_texture "
    //                   "GL_OES_texture_cube_map GL_OES_draw_texture ";
    // if (s_glSupport.GL_OES_READ_FORMAT)
    //     *s_glExtensions+="GL_OES_read_format ";
    // if (s_glSupport.GL_EXT_FRAMEBUFFER_OBJECT) {
    //     *s_glExtensions+="GL_OES_framebuffer_object GL_OES_depth24 GL_OES_depth32 GL_OES_fbo_render_mipmap "
    //                      "GL_OES_rgb8_rgba8 GL_OES_stencil1 GL_OES_stencil4 GL_OES_stencil8 ";
    // }
    // if (s_glSupport.GL_EXT_PACKED_DEPTH_STENCIL)
    //     *s_glExtensions+="GL_OES_packed_depth_stencil ";
    // if (s_glSupport.GL_EXT_TEXTURE_FORMAT_BGRA8888)
    //     *s_glExtensions+="GL_EXT_texture_format_BGRA8888 GL_APPLE_texture_format_BGRA8888 ";
    // if (s_glSupport.GL_ARB_MATRIX_PALETTE && s_glSupport.GL_ARB_VERTEX_BLEND) {
    //     *s_glExtensions+="GL_OES_matrix_palette ";
    //     GLint max_palette_matrices=0;
    //     GLint max_vertex_units=0;
    //     dispatcher().glGetIntegerv(GL_MAX_PALETTE_MATRICES_OES,&max_palette_matrices);
    //     dispatcher().glGetIntegerv(GL_MAX_VERTEX_UNITS_OES,&max_vertex_units);
    //     if (max_palette_matrices>=32 && max_vertex_units>=4)
    //         *s_glExtensions+="GL_OES_extended_matrix_palette ";
    // }
    // *s_glExtensions+="GL_OES_compressed_ETC1_RGB8_texture ";
  *s_glExtensions = "GL_OES_EGL_image "
                    "GL_OES_draw_texture ";
  return;
}
