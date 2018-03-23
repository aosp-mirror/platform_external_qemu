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

/* gallium blitter implementation in GL */
/* for when we can't use glBlitFramebuffer */
#include "igl.h"

#include <stdio.h>
#include "pipe/p_shader_tokens.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_dual_blend.h"

#include "util/u_double_list.h"
#include "util/u_format.h"
#include "util/u_texture.h"
#include "tgsi/tgsi_parse.h"

#include "vrend_object.h"
#include "vrend_shader.h"

#include "vrend_renderer.h"

#include "vrend_blitter.h"

struct vrend_blitter_ctx {
   virgl_gl_context gl_context;
   bool initialised;
   bool use_gles;

   GLuint vaoid;

   GLuint vs;
   GLuint vs_pos_only;
   GLuint fs_texfetch_col[PIPE_MAX_TEXTURE_TYPES];
   GLuint fs_texfetch_col_emu_alpha[PIPE_MAX_TEXTURE_TYPES];
   GLuint fs_texfetch_depth[PIPE_MAX_TEXTURE_TYPES];
   GLuint fs_texfetch_depth_msaa[PIPE_MAX_TEXTURE_TYPES];
   GLuint fb_id;

   unsigned dst_width;
   unsigned dst_height;

   GLuint vbo_id;
   GLfloat vertices[4][2][4];   /**< {pos, color} or {pos, texcoord} */
};

static struct vrend_blitter_ctx vrend_blit_ctx;

static bool build_and_check(GLuint id, const char *buf)
{
   GLint param;
   glShaderSource(id, 1, (const char **)&buf, NULL);
   glCompileShader(id);

   glGetShaderiv(id, GL_COMPILE_STATUS, &param);
   if (param == GL_FALSE) {
      char infolog[65536];
      int len;
      glGetShaderInfoLog(id, 65536, &len, infolog);
      fprintf(stderr,"shader failed to compile\n%s\n", infolog);
      fprintf(stderr,"GLSL:\n%s\n", buf);
      return false;
   }
   return true;
}

static bool blit_build_vs_passthrough(struct vrend_blitter_ctx *blit_ctx)
{
   blit_ctx->vs = glCreateShader(GL_VERTEX_SHADER);

   if (!build_and_check(blit_ctx->vs,
        blit_ctx->use_gles ? VS_PASSTHROUGH_GLES : VS_PASSTHROUGH_GL)) {
      glDeleteShader(blit_ctx->vs);
      blit_ctx->vs = 0;
      return false;
   }
   return true;
}

static GLuint blit_build_frag_tex_col(struct vrend_blitter_ctx *blit_ctx, int tgsi_tex_target)
{
   GLuint fs_id;
   char shader_buf[4096];
   int is_shad;
   const char *twm;
   const char *ext_str = "";
   switch (tgsi_tex_target) {
   case TGSI_TEXTURE_1D:
   case TGSI_TEXTURE_BUFFER:
      twm = ".x";
      break;
   case TGSI_TEXTURE_1D_ARRAY:
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
   case TGSI_TEXTURE_2D_MSAA:
   default:
      twm = ".xy";
      break;
   case TGSI_TEXTURE_SHADOW1D:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
   case TGSI_TEXTURE_SHADOWRECT:
   case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_CUBE:
   case TGSI_TEXTURE_2D_ARRAY:
   case TGSI_TEXTURE_2D_ARRAY_MSAA:
      twm = ".xyz";
      break;
   case TGSI_TEXTURE_SHADOWCUBE:
   case TGSI_TEXTURE_SHADOW2D_ARRAY:
   case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
   case TGSI_TEXTURE_CUBE_ARRAY:
      twm = "";
      break;
   }

   if (tgsi_tex_target == TGSI_TEXTURE_CUBE_ARRAY ||
       tgsi_tex_target == TGSI_TEXTURE_SHADOWCUBE_ARRAY)
      ext_str = "#extension GL_ARB_texture_cube_map_array : require\n";

   snprintf(shader_buf, 4096, blit_ctx->use_gles ? FS_TEXFETCH_COL_GLES : FS_TEXFETCH_COL_GL,
      ext_str, vrend_shader_samplertypeconv(tgsi_tex_target, &is_shad), twm, "");

   fs_id = glCreateShader(GL_FRAGMENT_SHADER);

   if (!build_and_check(fs_id, shader_buf)) {
      glDeleteShader(fs_id);
      return 0;
   }

   return fs_id;
}

static GLuint blit_build_frag_tex_col_emu_alpha(struct vrend_blitter_ctx *blit_ctx, int tgsi_tex_target)
{
   GLuint fs_id;
   char shader_buf[4096];
   int is_shad;
   const char *twm;
   const char *ext_str = "";
   switch (tgsi_tex_target) {
   case TGSI_TEXTURE_1D:
   case TGSI_TEXTURE_BUFFER:
      twm = ".x";
      break;
   case TGSI_TEXTURE_1D_ARRAY:
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
   case TGSI_TEXTURE_2D_MSAA:
   default:
      twm = ".xy";
      break;
   case TGSI_TEXTURE_SHADOW1D:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
   case TGSI_TEXTURE_SHADOWRECT:
   case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_CUBE:
   case TGSI_TEXTURE_2D_ARRAY:
   case TGSI_TEXTURE_2D_ARRAY_MSAA:
      twm = ".xyz";
      break;
   case TGSI_TEXTURE_SHADOWCUBE:
   case TGSI_TEXTURE_SHADOW2D_ARRAY:
   case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
   case TGSI_TEXTURE_CUBE_ARRAY:
      twm = "";
      break;
   }

   if (tgsi_tex_target == TGSI_TEXTURE_CUBE_ARRAY ||
       tgsi_tex_target == TGSI_TEXTURE_SHADOWCUBE_ARRAY)
      ext_str = "#extension GL_ARB_texture_cube_map_array : require\n";

   snprintf(shader_buf, 4096, blit_ctx->use_gles ? FS_TEXFETCH_COL_ALPHA_DEST_GLES : FS_TEXFETCH_COL_ALPHA_DEST_GL,
      ext_str, vrend_shader_samplertypeconv(tgsi_tex_target, &is_shad), twm, "");

   fs_id = glCreateShader(GL_FRAGMENT_SHADER);

   if (!build_and_check(fs_id, shader_buf)) {
      glDeleteShader(fs_id);
      return 0;
   }

   return fs_id;
}

static GLuint blit_build_frag_tex_writedepth(struct vrend_blitter_ctx *blit_ctx, int tgsi_tex_target)
{
   GLuint fs_id;
   char shader_buf[4096];
   int is_shad;
   const char *twm;

   switch (tgsi_tex_target) {
   case TGSI_TEXTURE_1D:
   case TGSI_TEXTURE_BUFFER:
      twm = ".x";
      break;
   case TGSI_TEXTURE_1D_ARRAY:
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
   case TGSI_TEXTURE_2D_MSAA:
   default:
      twm = ".xy";
      break;
   case TGSI_TEXTURE_SHADOW1D:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
   case TGSI_TEXTURE_SHADOWRECT:
   case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_CUBE:
   case TGSI_TEXTURE_2D_ARRAY:
   case TGSI_TEXTURE_2D_ARRAY_MSAA:
      twm = ".xyz";
      break;
   case TGSI_TEXTURE_SHADOWCUBE:
   case TGSI_TEXTURE_SHADOW2D_ARRAY:
   case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
   case TGSI_TEXTURE_CUBE_ARRAY:
      twm = "";
      break;
   }

   snprintf(shader_buf, 4096, blit_ctx->use_gles ? FS_TEXFETCH_DS_GLES : FS_TEXFETCH_DS_GL,
      vrend_shader_samplertypeconv(tgsi_tex_target, &is_shad), twm);

   fs_id = glCreateShader(GL_FRAGMENT_SHADER);

   if (!build_and_check(fs_id, shader_buf)) {
      glDeleteShader(fs_id);
      return 0;
   }

   return fs_id;
}

static GLuint blit_build_frag_blit_msaa_depth(struct vrend_blitter_ctx *blit_ctx, int tgsi_tex_target)
{
   GLuint fs_id;
   char shader_buf[4096];
   int is_shad;
   const char *twm;
   const char *ivec;

   switch (tgsi_tex_target) {
   case TGSI_TEXTURE_2D_MSAA:
      twm = ".xy";
      ivec = "ivec2";
      break;
   case TGSI_TEXTURE_2D_ARRAY_MSAA:
      twm = ".xyz";
      ivec = "ivec3";
      break;
   default:
      return 0;
   }

   snprintf(shader_buf, 4096, blit_ctx->use_gles ? FS_TEXFETCH_DS_MSAA_GLES : FS_TEXFETCH_DS_MSAA_GL,
      vrend_shader_samplertypeconv(tgsi_tex_target, &is_shad), ivec, twm);

   fs_id = glCreateShader(GL_FRAGMENT_SHADER);

   if (!build_and_check(fs_id, shader_buf)) {
      glDeleteShader(fs_id);
      return 0;
   }

   return fs_id;
}

static GLuint blit_get_frag_tex_writedepth(struct vrend_blitter_ctx *blit_ctx, int pipe_tex_target, unsigned nr_samples)
{
   assert(pipe_tex_target < PIPE_MAX_TEXTURE_TYPES);

   if (nr_samples > 1) {
      GLuint *shader = &blit_ctx->fs_texfetch_depth_msaa[pipe_tex_target];

      if (!*shader) {
         unsigned tgsi_tex = util_pipe_tex_to_tgsi_tex(pipe_tex_target, nr_samples);

         *shader = blit_build_frag_blit_msaa_depth(blit_ctx, tgsi_tex);
      }
      return *shader;

   } else {
      GLuint *shader = &blit_ctx->fs_texfetch_depth[pipe_tex_target];

      if (!*shader) {
         unsigned tgsi_tex = util_pipe_tex_to_tgsi_tex(pipe_tex_target, 0);

         *shader = blit_build_frag_tex_writedepth(blit_ctx, tgsi_tex);
      }
      return *shader;
   }
}

static GLuint blit_get_frag_tex_col(struct vrend_blitter_ctx *blit_ctx, int pipe_tex_target, unsigned nr_samples)
{
   assert(pipe_tex_target < PIPE_MAX_TEXTURE_TYPES);

   if (nr_samples > 1) {
      return 0;
   } else {
      GLuint *shader = &blit_ctx->fs_texfetch_col[pipe_tex_target];

      if (!*shader) {
         unsigned tgsi_tex = util_pipe_tex_to_tgsi_tex(pipe_tex_target, 0);

         *shader = blit_build_frag_tex_col(blit_ctx, tgsi_tex);
      }
      return *shader;
   }
}

static GLuint blit_get_frag_tex_col_emu_alpha(struct vrend_blitter_ctx *blit_ctx, int pipe_tex_target, unsigned nr_samples)
{
   assert(pipe_tex_target < PIPE_MAX_TEXTURE_TYPES);

   if (nr_samples > 1) {
      return 0;
   } else {
      GLuint *shader = &blit_ctx->fs_texfetch_col_emu_alpha[pipe_tex_target];

      if (!*shader) {
         unsigned tgsi_tex = util_pipe_tex_to_tgsi_tex(pipe_tex_target, 0);

         *shader = blit_build_frag_tex_col_emu_alpha(blit_ctx, tgsi_tex);
      }
      return *shader;
   }
}

static void vrend_renderer_init_blit_ctx(struct vrend_blitter_ctx *blit_ctx)
{
   struct virgl_gl_ctx_param ctx_params;
   int i;
   if (blit_ctx->initialised) {
      vrend_clicbs->make_current(0, blit_ctx->gl_context);
      return;
   }

   blit_ctx->initialised = true;
   blit_ctx->use_gles = epoxy_is_desktop_gl() == 0;
   ctx_params.shared = true;
   for (i = 0; i < ARRAY_SIZE(gl_versions); i++) {
      ctx_params.major_ver = gl_versions[i].major;
      ctx_params.minor_ver = gl_versions[i].minor;

      blit_ctx->gl_context = vrend_clicbs->create_gl_context(0, &ctx_params);
      if (blit_ctx->gl_context)
         break;
   }

   vrend_clicbs->make_current(0, blit_ctx->gl_context);
   glGenVertexArrays(1, &blit_ctx->vaoid);
   glGenFramebuffers(1, &blit_ctx->fb_id);

   glGenBuffers(1, &blit_ctx->vbo_id);
   blit_build_vs_passthrough(blit_ctx);

   for (i = 0; i < 4; i++)
      blit_ctx->vertices[i][0][3] = 1; /*v.w*/
   glBindVertexArray(blit_ctx->vaoid);
   glBindBuffer(GL_ARRAY_BUFFER, blit_ctx->vbo_id);
}

static inline GLenum convert_mag_filter(unsigned int filter)
{
   if (filter == PIPE_TEX_FILTER_NEAREST)
      return GL_NEAREST;
   return GL_LINEAR;
}

static void blitter_set_dst_dim(struct vrend_blitter_ctx *blit_ctx,
                                unsigned width, unsigned height)
{
   blit_ctx->dst_width = width;
   blit_ctx->dst_height = height;
}

static void blitter_set_rectangle(struct vrend_blitter_ctx *blit_ctx,
                                  int x1, int y1, int x2, int y2,
                                  float depth)
{
   int i;

   /* set vertex positions */
   blit_ctx->vertices[0][0][0] = (float)x1 / blit_ctx->dst_width * 2.0f - 1.0f; /*v0.x*/
   blit_ctx->vertices[0][0][1] = (float)y1 / blit_ctx->dst_height * 2.0f - 1.0f; /*v0.y*/

   blit_ctx->vertices[1][0][0] = (float)x2 / blit_ctx->dst_width * 2.0f - 1.0f; /*v1.x*/
   blit_ctx->vertices[1][0][1] = (float)y1 / blit_ctx->dst_height * 2.0f - 1.0f; /*v1.y*/

   blit_ctx->vertices[2][0][0] = (float)x2 / blit_ctx->dst_width * 2.0f - 1.0f; /*v2.x*/
   blit_ctx->vertices[2][0][1] = (float)y2 / blit_ctx->dst_height * 2.0f - 1.0f; /*v2.y*/

   blit_ctx->vertices[3][0][0] = (float)x1 / blit_ctx->dst_width * 2.0f - 1.0f; /*v3.x*/
   blit_ctx->vertices[3][0][1] = (float)y2 / blit_ctx->dst_height * 2.0f - 1.0f; /*v3.y*/

   for (i = 0; i < 4; i++)
      blit_ctx->vertices[i][0][2] = depth; /*z*/

   glViewport(0, 0, blit_ctx->dst_width, blit_ctx->dst_height);
}

static void get_texcoords(struct vrend_resource *src_res,
                          int src_level,
                          int x1, int y1, int x2, int y2,
                          float out[4])
{
   bool normalized = src_res->base.target != PIPE_TEXTURE_RECT &&
      src_res->base.nr_samples <= 1;

   if (normalized) {
      out[0] = x1 / (float)u_minify(src_res->base.width0,  src_level);
      out[1] = y1 / (float)u_minify(src_res->base.height0, src_level);
      out[2] = x2 / (float)u_minify(src_res->base.width0,  src_level);
      out[3] = y2 / (float)u_minify(src_res->base.height0, src_level);
   } else {
      out[0] = (float) x1;
      out[1] = (float) y1;
      out[2] = (float) x2;
      out[3] = (float) y2;
   }
}
static void set_texcoords_in_vertices(const float coord[4],
                                      float *out, unsigned stride)
{
   out[0] = coord[0]; /*t0.s*/
   out[1] = coord[1]; /*t0.t*/
   out += stride;
   out[0] = coord[2]; /*t1.s*/
   out[1] = coord[1]; /*t1.t*/
   out += stride;
   out[0] = coord[2]; /*t2.s*/
   out[1] = coord[3]; /*t2.t*/
   out += stride;
   out[0] = coord[0]; /*t3.s*/
   out[1] = coord[3]; /*t3.t*/
}

static void blitter_set_texcoords(struct vrend_blitter_ctx *blit_ctx,
                                  struct vrend_resource *src_res,
                                  int level,
                                  float layer, unsigned sample,
                                  int x1, int y1, int x2, int y2)
{
   float coord[4];
   float face_coord[4][2];
   int i;
   get_texcoords(src_res, level, x1, y1, x2, y2, coord);

   if (src_res->base.target == PIPE_TEXTURE_CUBE ||
       src_res->base.target == PIPE_TEXTURE_CUBE_ARRAY) {
      set_texcoords_in_vertices(coord, &face_coord[0][0], 2);
      util_map_texcoords2d_onto_cubemap((unsigned)layer % 6,
                                        /* pointer, stride in floats */
                                        &face_coord[0][0], 2,
                                        &blit_ctx->vertices[0][1][0], 8,
                                        FALSE);
   } else {
      set_texcoords_in_vertices(coord, &blit_ctx->vertices[0][1][0], 8);
   }

   switch (src_res->base.target) {
   case PIPE_TEXTURE_3D:
   {
      float r = layer / (float)u_minify(src_res->base.depth0,
                                        level);
      for (i = 0; i < 4; i++)
         blit_ctx->vertices[i][1][2] = r; /*r*/
   }
   break;

   case PIPE_TEXTURE_1D_ARRAY:
      for (i = 0; i < 4; i++)
         blit_ctx->vertices[i][1][1] = (float) layer; /*t*/
      break;

   case PIPE_TEXTURE_2D_ARRAY:
      for (i = 0; i < 4; i++) {
         blit_ctx->vertices[i][1][2] = (float) layer;  /*r*/
         blit_ctx->vertices[i][1][3] = (float) sample; /*q*/
      }
      break;
   case PIPE_TEXTURE_CUBE_ARRAY:
      for (i = 0; i < 4; i++)
         blit_ctx->vertices[i][1][3] = (float) ((unsigned)layer / 6); /*w*/
      break;
   case PIPE_TEXTURE_2D:
      for (i = 0; i < 4; i++) {
         blit_ctx->vertices[i][1][3] = (float) sample; /*r*/
      }
      break;
   default:;
   }
}
#if 0
static void set_dsa_keep_depth_stencil(void)
{
   glDisable(GL_STENCIL_TEST);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
}
#endif

static void set_dsa_write_depth_keep_stencil(void)
{
   glDisable(GL_STENCIL_TEST);
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_ALWAYS);
   glDepthMask(GL_TRUE);
}

/* implement blitting using OpenGL. */
void vrend_renderer_blit_gl(struct vrend_context *ctx,
                            struct vrend_resource *src_res,
                            struct vrend_resource *dst_res,
                            const struct pipe_blit_info *info)
{
   struct vrend_blitter_ctx *blit_ctx = &vrend_blit_ctx;
   GLuint buffers;
   GLuint prog_id;
   GLuint fs_id;
   GLint lret;
   GLenum filter;
   GLuint pos_loc, tc_loc;
   GLuint samp_loc;
   bool has_depth, has_stencil;
   bool blit_stencil, blit_depth;
   int dst_z;
   const struct util_format_description *src_desc =
      util_format_description(src_res->base.format);
   const struct util_format_description *dst_desc =
      util_format_description(dst_res->base.format);

   has_depth = util_format_has_depth(src_desc) &&
      util_format_has_depth(dst_desc);
   has_stencil = util_format_has_stencil(src_desc) &&
      util_format_has_stencil(dst_desc);

   blit_depth = has_depth && (info->mask & PIPE_MASK_Z);
   blit_stencil = has_stencil && (info->mask & PIPE_MASK_S) & 0;

   filter = convert_mag_filter(info->filter);
   vrend_renderer_init_blit_ctx(blit_ctx);

   blitter_set_dst_dim(blit_ctx,
                       u_minify(dst_res->base.width0, info->dst.level),
                       u_minify(dst_res->base.height0, info->dst.level));

   blitter_set_rectangle(blit_ctx, info->dst.box.x, info->dst.box.y,
                         info->dst.box.x + info->dst.box.width,
                         info->dst.box.y + info->dst.box.height, 0);


   prog_id = glCreateProgram();
   glAttachShader(prog_id, blit_ctx->vs);

   if (blit_depth || blit_stencil)
      fs_id = blit_get_frag_tex_writedepth(blit_ctx, src_res->base.target, src_res->base.nr_samples);
   else if (vrend_format_is_emulated_alpha(info->dst.format))
      fs_id = blit_get_frag_tex_col_emu_alpha(blit_ctx, src_res->base.target, src_res->base.nr_samples);
   else
      fs_id = blit_get_frag_tex_col(blit_ctx, src_res->base.target, src_res->base.nr_samples);
   glAttachShader(prog_id, fs_id);

   glLinkProgram(prog_id);
   glGetProgramiv(prog_id, GL_LINK_STATUS, &lret);
   if (lret == GL_FALSE) {
      char infolog[65536];
      int len;
      glGetProgramInfoLog(prog_id, 65536, &len, infolog);
      fprintf(stderr,"got error linking\n%s\n", infolog);
      /* dump shaders */
      glDeleteProgram(prog_id);
      return;
   }

   glUseProgram(prog_id);

   glBindFramebuffer(GL_FRAMEBUFFER_EXT, blit_ctx->fb_id);
   vrend_fb_bind_texture(dst_res, 0, info->dst.level, info->dst.box.z);

   buffers = GL_COLOR_ATTACHMENT0_EXT;
   glDrawBuffers(1, &buffers);

   glBindTexture(src_res->target, src_res->id);

   if (vrend_format_is_emulated_alpha(info->src.format))
      glTexParameteri(src_res->target, GL_TEXTURE_SWIZZLE_A, GL_RED);

   glTexParameteri(src_res->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(src_res->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(src_res->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

   glTexParameteri(src_res->target, GL_TEXTURE_BASE_LEVEL, info->src.level);
   glTexParameteri(src_res->target, GL_TEXTURE_MAX_LEVEL, info->src.level);
   glTexParameterf(src_res->target, GL_TEXTURE_MAG_FILTER, filter);
   glTexParameterf(src_res->target, GL_TEXTURE_MIN_FILTER, filter);
   pos_loc = glGetAttribLocation(prog_id, "arg0");
   tc_loc = glGetAttribLocation(prog_id, "arg1");
   samp_loc = glGetUniformLocation(prog_id, "samp");

   glUniform1i(samp_loc, 0);

   glVertexAttribPointer(pos_loc, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
   glVertexAttribPointer(tc_loc, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(4 * sizeof(float)));

   glEnableVertexAttribArray(pos_loc);
   glEnableVertexAttribArray(tc_loc);

   set_dsa_write_depth_keep_stencil();

   for (dst_z = 0; dst_z < info->dst.box.depth; dst_z++) {
      float dst2src_scale = info->src.box.depth / (float)info->dst.box.depth;
      float dst_offset = ((info->src.box.depth - 1) -
                          (info->dst.box.depth - 1) * dst2src_scale) * 0.5;
      float src_z = (dst_z + dst_offset) * dst2src_scale;
      uint32_t layer = (dst_res->target == GL_TEXTURE_CUBE_MAP) ? info->dst.box.z : dst_z;

      glBindFramebuffer(GL_FRAMEBUFFER_EXT, blit_ctx->fb_id);
      vrend_fb_bind_texture(dst_res, 0, info->dst.level, layer);

      buffers = GL_COLOR_ATTACHMENT0_EXT;
      glDrawBuffers(1, &buffers);
      blitter_set_texcoords(blit_ctx, src_res, info->src.level,
                            info->src.box.z + src_z, 0,
                            info->src.box.x, info->src.box.y,
                            info->src.box.x + info->src.box.width,
                            info->src.box.y + info->src.box.height);

      glBufferData(GL_ARRAY_BUFFER, sizeof(blit_ctx->vertices), blit_ctx->vertices, GL_STATIC_DRAW);
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
   }
}
