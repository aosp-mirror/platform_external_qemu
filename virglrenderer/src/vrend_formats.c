/**************************************************************************
 *
 * Copyright (C) 2014 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
#include "igl.h"

#include "vrend_renderer.h"
#include "util/u_memory.h"
#include "util/u_format.h"
/* fill the format table */
static struct vrend_format_table base_rgba_formats[] =
  {
    { VIRGL_FORMAT_B8G8R8X8_UNORM, GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE, 0 },
    { VIRGL_FORMAT_B8G8R8A8_UNORM, GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE, 0 },

    { VIRGL_FORMAT_R8G8B8X8_UNORM, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 0 },
    { VIRGL_FORMAT_R8G8B8A8_UNORM, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 0 },

    { VIRGL_FORMAT_A8R8G8B8_UNORM, GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, 0 },
    { VIRGL_FORMAT_X8R8G8B8_UNORM, GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, 0 },

    { VIRGL_FORMAT_A8B8G8R8_UNORM, GL_RGBA8, GL_ABGR_EXT, GL_UNSIGNED_BYTE, 0},

    { VIRGL_FORMAT_B4G4R4A4_UNORM, GL_RGBA4, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV },
    { VIRGL_FORMAT_B4G4R4X4_UNORM, GL_RGBA4, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV },
    { VIRGL_FORMAT_B5G5R5X1_UNORM, GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV },
    { VIRGL_FORMAT_B5G5R5A1_UNORM, GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV },

    { VIRGL_FORMAT_B5G6R5_UNORM, GL_RGB4, GL_RGB, GL_UNSIGNED_SHORT_5_6_5 },
    { VIRGL_FORMAT_B2G3R3_UNORM, GL_R3_G3_B2, GL_RGB, GL_UNSIGNED_BYTE_3_3_2 },

    { VIRGL_FORMAT_R16G16B16X16_UNORM, GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT },

    { VIRGL_FORMAT_R16G16B16A16_UNORM, GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT },
  };

static struct vrend_format_table base_depth_formats[] =
  {
    { VIRGL_FORMAT_Z16_UNORM, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, 0 },
    { VIRGL_FORMAT_Z32_UNORM, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0 },
    { VIRGL_FORMAT_S8_UINT_Z24_UNORM, GL_DEPTH24_STENCIL8_EXT, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0 },
    { VIRGL_FORMAT_Z24X8_UNORM, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT },
    { VIRGL_FORMAT_Z32_FLOAT, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT },
    /* this is probably a separate format */
    { VIRGL_FORMAT_Z32_FLOAT_S8X24_UINT, GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV},
    { VIRGL_FORMAT_X24S8_UINT, GL_STENCIL_INDEX8, GL_STENCIL_INDEX, GL_UNSIGNED_INT_24_8, 0 },
  };

static struct vrend_format_table base_la_formats[] = {
  { VIRGL_FORMAT_A8_UNORM, GL_ALPHA8, GL_ALPHA, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_L8_UNORM, GL_LUMINANCE8, GL_LUMINANCE, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_L8A8_UNORM, GL_LUMINANCE8_ALPHA8, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_A16_UNORM, GL_ALPHA16, GL_ALPHA, GL_UNSIGNED_SHORT },
  { VIRGL_FORMAT_L16_UNORM, GL_LUMINANCE16, GL_LUMINANCE, GL_UNSIGNED_SHORT },
  { VIRGL_FORMAT_L16A16_UNORM, GL_LUMINANCE16_ALPHA16, GL_LUMINANCE_ALPHA, GL_UNSIGNED_SHORT },
};

static struct vrend_format_table rg_base_formats[] = {
  { VIRGL_FORMAT_R8_UNORM, GL_R8, GL_RED, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_R8G8_UNORM, GL_RG8, GL_RG, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_R16_UNORM, GL_R16, GL_RED, GL_UNSIGNED_SHORT },
  { VIRGL_FORMAT_R16G16_UNORM, GL_RG16, GL_RG, GL_UNSIGNED_SHORT },
};

static struct vrend_format_table integer_base_formats[] = {
  { VIRGL_FORMAT_R8G8B8A8_UINT, GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_R8G8B8A8_SINT, GL_RGBA8I, GL_RGBA_INTEGER, GL_BYTE },

  { VIRGL_FORMAT_R16G16B16A16_UINT, GL_RGBA16UI, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT },
  { VIRGL_FORMAT_R16G16B16A16_SINT, GL_RGBA16I, GL_RGBA_INTEGER, GL_SHORT },

  { VIRGL_FORMAT_R32G32B32A32_UINT, GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT },
  { VIRGL_FORMAT_R32G32B32A32_SINT, GL_RGBA32I, GL_RGBA_INTEGER, GL_INT },
};

static struct vrend_format_table integer_3comp_formats[] = {
  { VIRGL_FORMAT_R8G8B8_UINT, GL_RGB8UI, GL_RGB_INTEGER, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_R8G8B8_SINT, GL_RGB8I, GL_RGB_INTEGER, GL_BYTE },
  { VIRGL_FORMAT_R16G16B16_UINT, GL_RGB16UI, GL_RGB_INTEGER, GL_UNSIGNED_SHORT },
  { VIRGL_FORMAT_R16G16B16_SINT, GL_RGB16I, GL_RGB_INTEGER, GL_SHORT },
  { VIRGL_FORMAT_R32G32B32_UINT, GL_RGB32UI, GL_RGB_INTEGER, GL_UNSIGNED_INT },
  { VIRGL_FORMAT_R32G32B32_SINT, GL_RGB32I, GL_RGB_INTEGER, GL_INT },
};

static struct vrend_format_table float_base_formats[] = {
  { VIRGL_FORMAT_R16G16B16A16_FLOAT, GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT },
  { VIRGL_FORMAT_R32G32B32A32_FLOAT, GL_RGBA32F, GL_RGBA, GL_FLOAT },
};

static struct vrend_format_table float_la_formats[] = {
  { VIRGL_FORMAT_A16_FLOAT, GL_ALPHA16F_ARB, GL_ALPHA, GL_HALF_FLOAT },
  { VIRGL_FORMAT_L16_FLOAT, GL_LUMINANCE16F_ARB, GL_LUMINANCE, GL_HALF_FLOAT },
  { VIRGL_FORMAT_L16A16_FLOAT, GL_LUMINANCE_ALPHA16F_ARB, GL_LUMINANCE_ALPHA, GL_HALF_FLOAT },

  { VIRGL_FORMAT_A32_FLOAT, GL_ALPHA32F_ARB, GL_ALPHA, GL_FLOAT },
  { VIRGL_FORMAT_L32_FLOAT, GL_LUMINANCE32F_ARB, GL_LUMINANCE, GL_FLOAT },
  { VIRGL_FORMAT_L32A32_FLOAT, GL_LUMINANCE_ALPHA32F_ARB, GL_LUMINANCE_ALPHA, GL_FLOAT },
};

static struct vrend_format_table integer_rg_formats[] = {
  { VIRGL_FORMAT_R8_UINT, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_R8G8_UINT, GL_RG8UI, GL_RG_INTEGER, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_R8_SINT, GL_R8I, GL_RED_INTEGER, GL_BYTE },
  { VIRGL_FORMAT_R8G8_SINT, GL_RG8I, GL_RG_INTEGER, GL_BYTE },

  { VIRGL_FORMAT_R16_UINT, GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT },
  { VIRGL_FORMAT_R16G16_UINT, GL_RG16UI, GL_RG_INTEGER, GL_UNSIGNED_SHORT },
  { VIRGL_FORMAT_R16_SINT, GL_R16I, GL_RED_INTEGER, GL_SHORT },
  { VIRGL_FORMAT_R16G16_SINT, GL_RG16I, GL_RG_INTEGER, GL_SHORT },

  { VIRGL_FORMAT_R32_UINT, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT },
  { VIRGL_FORMAT_R32G32_UINT, GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT },
  { VIRGL_FORMAT_R32_SINT, GL_R32I, GL_RED_INTEGER, GL_INT },
  { VIRGL_FORMAT_R32G32_SINT, GL_RG32I, GL_RG_INTEGER, GL_INT },
};

static struct vrend_format_table float_rg_formats[] = {
  { VIRGL_FORMAT_R16_FLOAT, GL_R16F, GL_RED, GL_HALF_FLOAT },
  { VIRGL_FORMAT_R16G16_FLOAT, GL_RG16F, GL_RG, GL_HALF_FLOAT },
  { VIRGL_FORMAT_R32_FLOAT, GL_R32F, GL_RED, GL_FLOAT },
  { VIRGL_FORMAT_R32G32_FLOAT, GL_RG32F, GL_RG, GL_FLOAT },
};

static struct vrend_format_table float_3comp_formats[] = {
  { VIRGL_FORMAT_R16G16B16_FLOAT, GL_RGB16F, GL_RGB, GL_HALF_FLOAT },
  { VIRGL_FORMAT_R32G32B32_FLOAT, GL_RGB32F, GL_RGB, GL_FLOAT },
};


static struct vrend_format_table integer_la_formats[] = {
  { VIRGL_FORMAT_A8_UINT, GL_ALPHA8UI_EXT, GL_ALPHA_INTEGER, GL_UNSIGNED_BYTE},
  { VIRGL_FORMAT_L8_UINT, GL_LUMINANCE8UI_EXT, GL_LUMINANCE_INTEGER_EXT, GL_UNSIGNED_BYTE},
  { VIRGL_FORMAT_L8A8_UINT, GL_LUMINANCE_ALPHA8UI_EXT, GL_LUMINANCE_ALPHA_INTEGER_EXT, GL_UNSIGNED_BYTE},
  { VIRGL_FORMAT_A8_SINT, GL_ALPHA8I_EXT, GL_ALPHA_INTEGER, GL_BYTE},
  { VIRGL_FORMAT_L8_SINT, GL_LUMINANCE8I_EXT, GL_LUMINANCE_INTEGER_EXT, GL_BYTE},
  { VIRGL_FORMAT_L8A8_SINT, GL_LUMINANCE_ALPHA8I_EXT, GL_LUMINANCE_ALPHA_INTEGER_EXT, GL_BYTE},

  { VIRGL_FORMAT_A16_UINT, GL_ALPHA16UI_EXT, GL_ALPHA_INTEGER, GL_UNSIGNED_SHORT},
  { VIRGL_FORMAT_L16_UINT, GL_LUMINANCE16UI_EXT, GL_LUMINANCE_INTEGER_EXT, GL_UNSIGNED_SHORT},
  { VIRGL_FORMAT_L16A16_UINT, GL_LUMINANCE_ALPHA16UI_EXT, GL_LUMINANCE_ALPHA_INTEGER_EXT, GL_UNSIGNED_SHORT},

  { VIRGL_FORMAT_A16_SINT, GL_ALPHA16I_EXT, GL_ALPHA_INTEGER, GL_SHORT},
  { VIRGL_FORMAT_L16_SINT, GL_LUMINANCE16I_EXT, GL_LUMINANCE_INTEGER_EXT, GL_SHORT},
  { VIRGL_FORMAT_L16A16_SINT, GL_LUMINANCE_ALPHA16I_EXT, GL_LUMINANCE_ALPHA_INTEGER_EXT, GL_SHORT},

  { VIRGL_FORMAT_A32_UINT, GL_ALPHA32UI_EXT, GL_ALPHA_INTEGER, GL_UNSIGNED_INT},
  { VIRGL_FORMAT_L32_UINT, GL_LUMINANCE32UI_EXT, GL_LUMINANCE_INTEGER_EXT, GL_UNSIGNED_INT},
  { VIRGL_FORMAT_L32A32_UINT, GL_LUMINANCE_ALPHA32UI_EXT, GL_LUMINANCE_ALPHA_INTEGER_EXT, GL_UNSIGNED_INT},

  { VIRGL_FORMAT_A32_SINT, GL_ALPHA32I_EXT, GL_ALPHA_INTEGER, GL_INT},
  { VIRGL_FORMAT_L32_SINT, GL_LUMINANCE32I_EXT, GL_LUMINANCE_INTEGER_EXT, GL_INT},
  { VIRGL_FORMAT_L32A32_SINT, GL_LUMINANCE_ALPHA32I_EXT, GL_LUMINANCE_ALPHA_INTEGER_EXT, GL_INT},


};

static struct vrend_format_table snorm_formats[] = {
  { VIRGL_FORMAT_R8_SNORM, GL_R8_SNORM, GL_RED, GL_BYTE },
  { VIRGL_FORMAT_R8G8_SNORM, GL_RG8_SNORM, GL_RG, GL_BYTE },

  { VIRGL_FORMAT_R8G8B8A8_SNORM, GL_RGBA8_SNORM, GL_RGBA, GL_BYTE },
  { VIRGL_FORMAT_R8G8B8X8_SNORM, GL_RGBA8_SNORM, GL_RGBA, GL_BYTE },

  { VIRGL_FORMAT_R16_SNORM, GL_R16_SNORM, GL_RED, GL_SHORT },
  { VIRGL_FORMAT_R16G16_SNORM, GL_RG16_SNORM, GL_RG, GL_SHORT },
  { VIRGL_FORMAT_R16G16B16A16_SNORM, GL_RGBA16_SNORM, GL_RGBA, GL_SHORT },

  { VIRGL_FORMAT_R16G16B16X16_SNORM, GL_RGBA16_SNORM, GL_RGBA, GL_SHORT },
};

static struct vrend_format_table snorm_la_formats[] = {
  { VIRGL_FORMAT_A8_SNORM, GL_ALPHA8_SNORM, GL_ALPHA, GL_BYTE },
  { VIRGL_FORMAT_L8_SNORM, GL_LUMINANCE8_SNORM, GL_LUMINANCE, GL_BYTE },
  { VIRGL_FORMAT_L8A8_SNORM, GL_LUMINANCE8_ALPHA8_SNORM, GL_LUMINANCE_ALPHA, GL_BYTE },
  { VIRGL_FORMAT_A16_SNORM, GL_ALPHA16_SNORM, GL_ALPHA, GL_SHORT },
  { VIRGL_FORMAT_L16_SNORM, GL_LUMINANCE16_SNORM, GL_LUMINANCE, GL_SHORT },
  { VIRGL_FORMAT_L16A16_SNORM, GL_LUMINANCE16_ALPHA16_SNORM, GL_LUMINANCE_ALPHA, GL_SHORT },
};

static struct vrend_format_table dxtn_formats[] = {
  { VIRGL_FORMAT_DXT1_RGB, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, GL_RGB, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_DXT1_RGBA, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_RGBA, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_DXT3_RGBA, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_RGBA, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_DXT5_RGBA, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, GL_UNSIGNED_BYTE },
};

static struct vrend_format_table dxtn_srgb_formats[] = {
  { VIRGL_FORMAT_DXT1_SRGB, GL_COMPRESSED_SRGB_S3TC_DXT1_EXT, GL_RGB, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_DXT1_SRGBA, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, GL_RGBA, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_DXT3_SRGBA, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, GL_RGBA, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_DXT5_SRGBA, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_RGBA, GL_UNSIGNED_BYTE },
};

static struct vrend_format_table rgtc_formats[] = {
  { VIRGL_FORMAT_RGTC1_UNORM, GL_COMPRESSED_RED_RGTC1, GL_RED, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_RGTC1_SNORM, GL_COMPRESSED_SIGNED_RED_RGTC1, GL_RED, GL_BYTE },

  { VIRGL_FORMAT_RGTC2_UNORM, GL_COMPRESSED_RG_RGTC2, GL_RG, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_RGTC2_SNORM, GL_COMPRESSED_SIGNED_RG_RGTC2, GL_RG, GL_BYTE },
};

static struct vrend_format_table srgb_formats[] = {

  { VIRGL_FORMAT_B8G8R8X8_SRGB, GL_SRGB8_ALPHA8, GL_BGRA, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_B8G8R8A8_SRGB, GL_SRGB8_ALPHA8, GL_BGRA, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_R8G8B8X8_SRGB, GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE },

  { VIRGL_FORMAT_L8_SRGB, GL_SLUMINANCE8_EXT, GL_LUMINANCE, GL_UNSIGNED_BYTE },
  { VIRGL_FORMAT_L8A8_SRGB, GL_SLUMINANCE8_ALPHA8_EXT, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE },
};

static struct vrend_format_table bit10_formats[] = {
  { VIRGL_FORMAT_B10G10R10X2_UNORM, GL_RGB10_A2, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV },
  { VIRGL_FORMAT_B10G10R10A2_UNORM, GL_RGB10_A2, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV },
  { VIRGL_FORMAT_B10G10R10A2_UINT, GL_RGB10_A2UI, GL_BGRA_INTEGER, GL_UNSIGNED_INT_2_10_10_10_REV },
};

static struct vrend_format_table packed_float_formats[] = {
  { VIRGL_FORMAT_R11G11B10_FLOAT, GL_R11F_G11F_B10F, GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV},
};

static struct vrend_format_table exponent_float_formats[] = {
  { VIRGL_FORMAT_R9G9B9E5_FLOAT, GL_RGB9_E5, GL_RGB, GL_UNSIGNED_INT_5_9_9_9_REV},
};

static struct vrend_format_table bptc_formats[] = {
   { VIRGL_FORMAT_BPTC_RGBA_UNORM, GL_COMPRESSED_RGBA_BPTC_UNORM, GL_RGBA, GL_UNSIGNED_BYTE },
   { VIRGL_FORMAT_BPTC_SRGBA, GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM, GL_RGBA, GL_UNSIGNED_BYTE },
   { VIRGL_FORMAT_BPTC_RGB_FLOAT, GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT, GL_RGB, GL_UNSIGNED_BYTE },
   { VIRGL_FORMAT_BPTC_RGB_UFLOAT, GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT, GL_RGB, GL_UNSIGNED_BYTE },
};

static struct vrend_format_table gles_bgra_formats[] = {
  { VIRGL_FORMAT_B8G8R8X8_UNORM, GL_BGRA_EXT, GL_BGRA_EXT, GL_UNSIGNED_BYTE, 0 },
  { VIRGL_FORMAT_B8G8R8A8_UNORM, GL_BGRA_EXT, GL_BGRA_EXT, GL_UNSIGNED_BYTE, 0 },
};

static struct vrend_format_table gles_z32_format[] = {
  { VIRGL_FORMAT_Z32_UNORM, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0 },
};

static void vrend_add_formats(struct vrend_format_table *table, int num_entries)
{
  int i;
  uint32_t binding = 0;
  GLuint buffers;
  GLuint tex_id, fb_id;

  for (i = 0; i < num_entries; i++) {
    GLenum status;
    bool is_depth = false;
    /**/
    glGenTextures(1, &tex_id);
    glGenFramebuffers(1, &fb_id);

    glBindTexture(GL_TEXTURE_2D, tex_id);
    glBindFramebuffer(GL_FRAMEBUFFER, fb_id);

    glTexImage2D(GL_TEXTURE_2D, 0, table[i].internalformat, 32, 32, 0, table[i].glformat, table[i].gltype, NULL);
    status = glGetError();
    if (status == GL_INVALID_VALUE) {
      struct vrend_format_table *entry = NULL;
      uint8_t swizzle[4];
      binding = VREND_BIND_SAMPLER | VREND_BIND_RENDER | VREND_BIND_NEED_SWIZZLE;

      switch (table[i].format) {
      case PIPE_FORMAT_A8_UNORM:
        entry = &rg_base_formats[0];
        swizzle[0] = swizzle[1] = swizzle[2] = PIPE_SWIZZLE_ZERO;
        swizzle[3] = PIPE_SWIZZLE_RED;
        break;
      case PIPE_FORMAT_A16_UNORM:
        entry = &rg_base_formats[2];
        swizzle[0] = swizzle[1] = swizzle[2] = PIPE_SWIZZLE_ZERO;
        swizzle[3] = PIPE_SWIZZLE_RED;
        break;
      default:
        break;
      }

      if (entry) {
        vrend_insert_format_swizzle(table[i].format, entry, binding, swizzle);
      }
      glDeleteTextures(1, &tex_id);
      glDeleteFramebuffers(1, &fb_id);
      continue;
    }

    if (util_format_is_depth_or_stencil(table[i].format)) {
      GLenum attachment;

      if (table[i].format == VIRGL_FORMAT_Z24X8_UNORM || table[i].format == VIRGL_FORMAT_Z32_UNORM || table[i].format == VIRGL_FORMAT_Z16_UNORM || table[i].format == VIRGL_FORMAT_Z32_FLOAT)
        attachment = GL_DEPTH_ATTACHMENT;
      else
        attachment = GL_DEPTH_STENCIL_ATTACHMENT;
      glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_2D, tex_id, 0);

      is_depth = true;

      buffers = GL_NONE;
      glDrawBuffers(1, &buffers);
    } else {
      glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tex_id, 0);

      buffers = GL_COLOR_ATTACHMENT0_EXT;
      glDrawBuffers(1, &buffers);
    }

    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    binding = VREND_BIND_SAMPLER;
    if (status == GL_FRAMEBUFFER_COMPLETE)
      binding |= (is_depth ? VREND_BIND_DEPTHSTENCIL : VREND_BIND_RENDER);

    glDeleteTextures(1, &tex_id);
    glDeleteFramebuffers(1, &fb_id);
    vrend_insert_format(&table[i], binding);
  }
}

#define add_formats(x) vrend_add_formats((x), ARRAY_SIZE((x)))

void vrend_build_format_list(void)
{
  add_formats(base_rgba_formats);
  add_formats(base_depth_formats);
  add_formats(base_la_formats);

  /* float support */
  add_formats(float_base_formats);
  add_formats(float_la_formats);
  add_formats(float_3comp_formats);

  /* texture integer support ? */
  add_formats(integer_base_formats);
  add_formats(integer_la_formats);
  add_formats(integer_3comp_formats);

  /* RG support? */
  add_formats(rg_base_formats);
  /* integer + rg */
  add_formats(integer_rg_formats);
  /* float + rg */
  add_formats(float_rg_formats);

  /* snorm */
  add_formats(snorm_formats);
  add_formats(snorm_la_formats);

  /* compressed */
  add_formats(rgtc_formats);
  add_formats(dxtn_formats);
  add_formats(dxtn_srgb_formats);

  add_formats(srgb_formats);

  add_formats(bit10_formats);

  add_formats(packed_float_formats);
  add_formats(exponent_float_formats);

  add_formats(bptc_formats);
}

void vrend_build_format_list_gles(void)
{
  vrend_build_format_list();

  /* The BGR[A|X] formats is required but OpenGL ES does not
   * support rendering to it. Try to use GL_BGRA_EXT from the
   * GL_EXT_texture_format_BGRA8888 extension. But the
   * GL_BGRA_EXT format is not supported by OpenGL Desktop.
   */
  add_formats(gles_bgra_formats);


  /* The Z32 format is required, but OpenGL ES does not support
   * using it as a depth buffer. We just fake support with Z24
   * and hope nobody notices.
   */
  add_formats(gles_z32_format);
}
