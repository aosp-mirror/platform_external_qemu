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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "igl.h"

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "pipe/p_shader_tokens.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_dual_blend.h"

#include "os/os_thread.h"
#include "util/u_double_list.h"
#include "util/u_format.h"
#include "tgsi/tgsi_parse.h"

#include "vrend_object.h"
#include "vrend_shader.h"

#include "vrend_renderer.h"

#include "virgl_hw.h"

#include "tgsi/tgsi_text.h"

#ifdef HAVE_EVENTFD
#include <sys/eventfd.h>
#endif

/* debugging aid to dump shaders */
int vrend_dump_shaders;

/* debugging via KHR_debug extension */
int vrend_use_debug_cb = 0;

struct vrend_if_cbs *vrend_clicbs;

struct vrend_fence {
   uint32_t fence_id;
   uint32_t ctx_id;
   GLsync syncobj;
   struct list_head fences;
};

struct vrend_query {
   struct list_head waiting_queries;

   GLuint id;
   GLuint type;
   GLuint index;
   GLuint gltype;
   int ctx_id;
   struct vrend_resource *res;
   uint64_t current_total;
};

struct global_error_state {
   enum virgl_errors last_error;
};

struct global_renderer_state {
   int gl_major_ver;
   int gl_minor_ver;

   struct vrend_context *current_ctx;
   struct vrend_context *current_hw_ctx;
   struct list_head waiting_query_list;

   bool inited;
   bool use_gles;
   bool use_core_profile;

   bool have_debug_cb;
   bool have_mesa_invert;
   bool have_samplers;
   bool have_gles_khr_robustness;
   bool have_arb_robustness;
   bool have_arb_or_gles_ext_texture_buffer;
   bool have_multisample;
   bool have_ms_scaled_blit;
   bool have_nv_prim_restart;
   bool have_gl_prim_restart;
   bool have_bit_encoding;
   bool have_vertex_attrib_binding;
   bool have_tf2;
   bool have_stencil_texturing;
   bool have_sample_shading;

   /* these appeared broken on at least one driver */
   bool use_explicit_locations;
   uint32_t max_uniform_blocks;
   struct list_head active_ctx_list;

   /* threaded sync */
   bool stop_sync_thread;
   int eventfd;

   pipe_mutex fence_mutex;
   struct list_head fence_list;
   struct list_head fence_wait_list;
   pipe_condvar fence_cond;

   pipe_thread sync_thread;
   virgl_gl_context sync_context;
};

static struct global_renderer_state vrend_state;

struct vrend_linked_shader_program {
   struct list_head head;
   struct list_head sl[PIPE_SHADER_TYPES];
   GLuint id;

   bool dual_src_linked;
   struct vrend_shader *ss[PIPE_SHADER_TYPES];

   uint32_t samplers_used_mask[PIPE_SHADER_TYPES];
   GLuint *samp_locs[PIPE_SHADER_TYPES];

   GLuint *shadow_samp_mask_locs[PIPE_SHADER_TYPES];
   GLuint *shadow_samp_add_locs[PIPE_SHADER_TYPES];

   GLuint *const_locs[PIPE_SHADER_TYPES];

   GLuint *attrib_locs;
   uint32_t shadow_samp_mask[PIPE_SHADER_TYPES];

   GLuint *ubo_locs[PIPE_SHADER_TYPES];
   GLuint vs_ws_adjust_loc;

   GLuint fs_stipple_loc;

   GLuint clip_locs[8];
};

struct vrend_shader {
   struct vrend_shader *next_variant;
   struct vrend_shader_selector *sel;

   GLchar *glsl_prog;
   GLuint id;
   GLuint compiled_fs_id;
   struct vrend_shader_key key;
   struct list_head programs;
};

struct vrend_shader_selector {
   struct pipe_reference reference;

   unsigned num_shaders;
   unsigned type;
   struct vrend_shader_info sinfo;

   struct vrend_shader *current;
   struct tgsi_token *tokens;

   char *tmp_buf;
   uint32_t buf_len;
   uint32_t buf_offset;
};

struct vrend_texture {
   struct vrend_resource base;
   struct pipe_sampler_state state;
   GLenum cur_swizzle_r;
   GLenum cur_swizzle_g;
   GLenum cur_swizzle_b;
   GLenum cur_swizzle_a;
   GLuint srgb_decode;
};

struct vrend_surface {
   struct pipe_reference reference;
   GLuint id;
   GLuint res_handle;
   GLuint format;
   GLuint val0, val1;
   struct vrend_resource *texture;
};

struct vrend_sampler_state {
   struct pipe_sampler_state base;
   GLuint id;
};

struct vrend_so_target {
   struct pipe_reference reference;
   GLuint res_handle;
   unsigned buffer_offset;
   unsigned buffer_size;
   struct vrend_resource *buffer;
   struct vrend_sub_context *sub_ctx;
};

struct vrend_sampler_view {
   struct pipe_reference reference;
   GLuint id;
   GLuint format;
   GLuint val0, val1;
   GLuint gl_swizzle_r;
   GLuint gl_swizzle_g;
   GLuint gl_swizzle_b;
   GLuint gl_swizzle_a;
   GLuint cur_base, cur_max;
   GLenum depth_texture_mode;
   GLuint srgb_decode;
   GLuint swizzle_r:3;
   GLuint swizzle_g:3;
   GLuint swizzle_b:3;
   GLuint swizzle_a:3;
   struct vrend_resource *texture;
};

struct vrend_vertex_element {
   struct pipe_vertex_element base;
   GLenum type;
   GLboolean norm;
   GLuint nr_chan;
};

struct vrend_vertex_element_array {
   unsigned count;
   struct vrend_vertex_element elements[PIPE_MAX_ATTRIBS];
   GLuint id;
};

struct vrend_constants {
   unsigned int *consts;
   uint32_t num_consts;
};

struct vrend_shader_view {
   int num_views;
   struct vrend_sampler_view *views[PIPE_MAX_SHADER_SAMPLER_VIEWS];
   uint32_t res_id[PIPE_MAX_SHADER_SAMPLER_VIEWS];
   uint32_t old_ids[PIPE_MAX_SHADER_SAMPLER_VIEWS];
};

struct vrend_viewport {
   GLint cur_x, cur_y;
   GLsizei width, height;
   GLclampd near_val, far_val;
};

/* create a streamout object to support pause/resume */
struct vrend_streamout_object {
   GLuint id;
   uint32_t num_targets;
   uint32_t handles[16];
   struct list_head head;
   int xfb_state;
   struct vrend_so_target *so_targets[16];
};

#define XFB_STATE_OFF 0
#define XFB_STATE_STARTED_NEED_BEGIN 1
#define XFB_STATE_STARTED 2
#define XFB_STATE_PAUSED 3

struct vrend_sub_context {
   struct list_head head;

   virgl_gl_context gl_context;

   int sub_ctx_id;

   GLuint vaoid;
   uint32_t enabled_attribs_bitmask;

   struct list_head programs;
   struct util_hash_table *object_hash;

   struct vrend_vertex_element_array *ve;
   int num_vbos;
   int old_num_vbos; /* for cleaning up */
   struct pipe_vertex_buffer vbo[PIPE_MAX_ATTRIBS];
   uint32_t vbo_res_ids[PIPE_MAX_ATTRIBS];

   struct pipe_index_buffer ib;
   uint32_t index_buffer_res_id;

   bool vbo_dirty;
   bool shader_dirty;
   bool sampler_state_dirty;
   bool stencil_state_dirty;

   uint32_t long_shader_in_progress_handle[PIPE_SHADER_TYPES];
   struct vrend_shader_selector *shaders[PIPE_SHADER_TYPES];
   struct vrend_linked_shader_program *prog;

   int prog_ids[PIPE_SHADER_TYPES];
   struct vrend_shader_view views[PIPE_SHADER_TYPES];

   struct vrend_constants consts[PIPE_SHADER_TYPES];
   bool const_dirty[PIPE_SHADER_TYPES];
   struct vrend_sampler_state *sampler_state[PIPE_SHADER_TYPES][PIPE_MAX_SAMPLERS];

   struct pipe_constant_buffer cbs[PIPE_SHADER_TYPES][PIPE_MAX_CONSTANT_BUFFERS];
   uint32_t const_bufs_used_mask[PIPE_SHADER_TYPES];

   int num_sampler_states[PIPE_SHADER_TYPES];

   uint32_t fb_id;
   int nr_cbufs, old_nr_cbufs;
   struct vrend_surface *zsurf;
   struct vrend_surface *surf[PIPE_MAX_COLOR_BUFS];

   struct vrend_viewport vps[PIPE_MAX_VIEWPORTS];
   float depth_transform, depth_scale;
   /* viewport is negative */
   uint32_t scissor_state_dirty;
   uint32_t viewport_state_dirty;

   uint32_t fb_height;

   struct pipe_scissor_state ss[PIPE_MAX_VIEWPORTS];

   struct pipe_blend_state blend_state;
   struct pipe_depth_stencil_alpha_state dsa_state;
   struct pipe_rasterizer_state rs_state;

   uint8_t stencil_refs[2];
   bool viewport_is_negative;
   /* this is set if the contents of the FBO look upside down when viewed
      with 0,0 as the bottom corner */
   bool inverted_fbo_content;

   GLuint blit_fb_ids[2];

   struct pipe_depth_stencil_alpha_state *dsa;

   struct pipe_clip_state ucp_state;

   bool blend_enabled;
   bool depth_test_enabled;
   bool alpha_test_enabled;
   bool stencil_test_enabled;

   GLuint program_id;
   int last_shader_idx;

   struct pipe_rasterizer_state hw_rs_state;
   struct pipe_blend_state hw_blend_state;

   struct list_head streamout_list;
   struct vrend_streamout_object *current_so;

   struct pipe_blend_color blend_color;

   uint32_t cond_render_q_id;
   GLenum cond_render_gl_mode;
};

struct vrend_context {
   char debug_name[64];

   struct list_head sub_ctxs;

   struct vrend_sub_context *sub;
   struct vrend_sub_context *sub0;

   int ctx_id;
   /* has this ctx gotten an error? */
   bool in_error;
   bool ctx_switch_pending;
   bool pstip_inited;

   GLuint pstipple_tex_id;

   enum virgl_ctx_errors last_error;

   /* resource bounds to this context */
   struct util_hash_table *res_hash;

   struct list_head active_nontimer_query_list;
   struct list_head ctx_entry;

   struct vrend_shader_cfg shader_cfg;
};

static struct vrend_resource *vrend_renderer_ctx_res_lookup(struct vrend_context *ctx, int res_handle);
static void vrend_pause_render_condition(struct vrend_context *ctx, bool pause);
static void vrend_update_viewport_state(struct vrend_context *ctx);
static void vrend_update_scissor_state(struct vrend_context *ctx);
static void vrend_destroy_query_object(void *obj_ptr);
static void vrend_finish_context_switch(struct vrend_context *ctx);
static void vrend_patch_blend_state(struct vrend_context *ctx);
static void vrend_update_frontface_state(struct vrend_context *ctx);
static void vrender_get_glsl_version(int *glsl_version);
static void vrend_destroy_resource_object(void *obj_ptr);
static void vrend_renderer_detach_res_ctx_p(struct vrend_context *ctx, int res_handle);
static void vrend_destroy_program(struct vrend_linked_shader_program *ent);
static void vrend_apply_sampler_state(struct vrend_context *ctx,
                                      struct vrend_resource *res,
                                      uint32_t shader_type,
                                      int id, int sampler_id, uint32_t srgb_decode);
void vrend_update_stencil_state(struct vrend_context *ctx);

static struct vrend_format_table tex_conv_table[VIRGL_FORMAT_MAX];

static inline bool vrend_format_can_sample(int format)
{
   return tex_conv_table[format].bindings & VREND_BIND_SAMPLER;
}
static inline bool vrend_format_can_render(int format)
{
   return tex_conv_table[format].bindings & VREND_BIND_RENDER;
}

static inline bool vrend_format_is_ds(int format)
{
   return tex_conv_table[format].bindings & VREND_BIND_DEPTHSTENCIL;
}

bool vrend_is_ds_format(int format)
{
   return vrend_format_is_ds(format);
}

bool vrend_format_is_emulated_alpha(int format)
{
   if (!vrend_state.use_core_profile)
      return false;
   return (format == VIRGL_FORMAT_A8_UNORM ||
           format == VIRGL_FORMAT_A16_UNORM);
}

static inline const char *pipe_shader_to_prefix(int shader_type)
{
   switch (shader_type) {
   case PIPE_SHADER_VERTEX: return "vs";
   case PIPE_SHADER_FRAGMENT: return "fs";
   case PIPE_SHADER_GEOMETRY: return "gs";
   default:
      return NULL;
   };
}

static const char *vrend_ctx_error_strings[] = { "None", "Unknown", "Illegal shader", "Illegal handle", "Illegal resource", "Illegal surface", "Illegal vertex format", "Illegal command buffer" };

static void __report_context_error(const char *fname, struct vrend_context *ctx, enum virgl_ctx_errors error, uint32_t value)
{
   ctx->in_error = true;
   ctx->last_error = error;
   fprintf(stderr,"%s: context error reported %d \"%s\" %s %d\n", fname, ctx->ctx_id, ctx->debug_name, vrend_ctx_error_strings[error], value);
}
#define report_context_error(ctx, error, value) __report_context_error(__func__, ctx, error, value)

void vrend_report_buffer_error(struct vrend_context *ctx, int cmd)
{
   report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_CMD_BUFFER, cmd);
}

#define CORE_PROFILE_WARN_NONE 0
#define CORE_PROFILE_WARN_STIPPLE 1
#define CORE_PROFILE_WARN_POLYGON_MODE 2
#define CORE_PROFILE_WARN_TWO_SIDE 3
#define CORE_PROFILE_WARN_CLAMP 4
#define CORE_PROFILE_WARN_SHADE_MODEL 5

static const char *vrend_core_profile_warn_strings[] = { "None", "Stipple", "Polygon Mode", "Two Side", "Clamping", "Shade Model" };

static void __report_core_warn(const char *fname, struct vrend_context *ctx, enum virgl_ctx_errors error, uint32_t value)
{
   fprintf(stderr,"%s: core profile violation reported %d \"%s\" %s %d\n", fname, ctx->ctx_id, ctx->debug_name, vrend_core_profile_warn_strings[error], value);
}
#define report_core_warn(ctx, error, value) __report_core_warn(__func__, ctx, error, value)


#define GLES_WARN_NONE 0
#define GLES_WARN_STIPPLE 1
#define GLES_WARN_POLYGON_MODE 2
#define GLES_WARN_DEPTH_RANGE 3
#define GLES_WARN_POINT_SIZE 4
#define GLES_WARN_LOD_BIAS 5
#define GLES_WARN_SRGB_FB 6
#define GLES_WARN_TEXTURE_RECT 7
#define GLES_WARN_OFFSET_LINE 8
#define GLES_WARN_OFFSET_POINT 9
#define GLES_WARN_DEPTH_CLIP 10
#define GLES_WARN_FLATSHADE_FIRST 11
#define GLES_WARN_LINE_SMOOTH 12
#define GLES_WARN_POLY_SMOOTH 13
#define GLES_WARN_DEPTH_CLEAR 14
#define GLES_WARN_LOGIC_OP 15
#define GLES_WARN_TIMESTAMP 16

static const char *vrend_gles_warn_strings[] = {
   "None", "Stipple", "Polygon Mode", "Depth Range", "Point Size", "Lod Bias",
   "SRGB Framebuffer", "Texture Rect", "Offset Line", "Offset Point",
   "Depth Clip", "Flatshade First", "Line Smooth", "Poly Smooth",
   "Depth Clear", "LogicOp", "GL_TIMESTAMP"
};

static void __report_gles_warn(const char *fname, struct vrend_context *ctx, enum virgl_ctx_errors error, uint32_t value)
{
   int id = ctx ? ctx->ctx_id : -1;
   const char *name = ctx ? ctx->debug_name : "NO_CONTEXT";
   fprintf(stderr,"%s: gles violation reported %d \"%s\" %s %d\n", fname, id, ctx->debug_name, vrend_gles_warn_strings[error], value);
}
#define report_gles_warn(ctx, error, value) __report_gles_warn(__func__, ctx, error, value)

static void __report_gles_missing_func(const char *fname, struct vrend_context *ctx, const char *missf)
{
   int id = ctx ? ctx->ctx_id : -1;
   const char *name = ctx ? ctx->debug_name : "NO_CONTEXT";
   fprintf(stderr,"%s: gles violation reported %d \"%s\" %s is missing\n", fname, id, ctx->debug_name, missf);
}
#define report_gles_missing_func(ctx, missf) __report_gles_missing_func(__func__, ctx, missf)


static inline bool should_invert_viewport(struct vrend_context *ctx)
{
   /* if we have a negative viewport then gallium wanted to invert it,
      however since we are rendering to GL FBOs we need to invert it
      again unless we are rendering upside down already
      - confused?
      so if gallium asks for a negative viewport */
   return !(ctx->sub->viewport_is_negative ^ ctx->sub->inverted_fbo_content);
}

static void vrend_destroy_surface(struct vrend_surface *surf)
{
   vrend_resource_reference(&surf->texture, NULL);
   free(surf);
}

static inline void
vrend_surface_reference(struct vrend_surface **ptr, struct vrend_surface *surf)
{
   struct vrend_surface *old_surf = *ptr;

   if (pipe_reference(&(*ptr)->reference, &surf->reference))
      vrend_destroy_surface(old_surf);
   *ptr = surf;
}

static void vrend_destroy_sampler_view(struct vrend_sampler_view *samp)
{
   vrend_resource_reference(&samp->texture, NULL);
   free(samp);
}

static inline void
vrend_sampler_view_reference(struct vrend_sampler_view **ptr, struct vrend_sampler_view *view)
{
   struct vrend_sampler_view *old_view = *ptr;

   if (pipe_reference(&(*ptr)->reference, &view->reference))
      vrend_destroy_sampler_view(old_view);
   *ptr = view;
}

static void vrend_destroy_so_target(struct vrend_so_target *target)
{
   vrend_resource_reference(&target->buffer, NULL);
   free(target);
}

static inline void
vrend_so_target_reference(struct vrend_so_target **ptr, struct vrend_so_target *target)
{
   struct vrend_so_target *old_target = *ptr;

   if (pipe_reference(&(*ptr)->reference, &target->reference))
      vrend_destroy_so_target(old_target);
   *ptr = target;
}

static void vrend_shader_destroy(struct vrend_shader *shader)
{
   struct vrend_linked_shader_program *ent, *tmp;

   LIST_FOR_EACH_ENTRY_SAFE(ent, tmp, &shader->programs, sl[shader->sel->type]) {
      vrend_destroy_program(ent);
   }

   glDeleteShader(shader->id);
   free(shader->glsl_prog);
   free(shader);
}

static void vrend_destroy_shader_selector(struct vrend_shader_selector *sel)
{
   struct vrend_shader *p = sel->current, *c;
   int i;
   while (p) {
      c = p->next_variant;
      vrend_shader_destroy(p);
      p = c;
   }
   if (sel->sinfo.so_names)
      for (i = 0; i < sel->sinfo.so_info.num_outputs; i++)
         free(sel->sinfo.so_names[i]);
   free(sel->tmp_buf);
   free(sel->sinfo.so_names);
   free(sel->sinfo.interpinfo);
   free(sel->tokens);
   free(sel);
}

static bool vrend_compile_shader(struct vrend_context *ctx,
                                 struct vrend_shader *shader)
{
   GLint param;
   glShaderSource(shader->id, 1, (const char **)&shader->glsl_prog, NULL);
   glCompileShader(shader->id);
   glGetShaderiv(shader->id, GL_COMPILE_STATUS, &param);
   if (param == GL_FALSE) {
      char infolog[65536];
      int len;
      glGetShaderInfoLog(shader->id, 65536, &len, infolog);
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_SHADER, 0);
      fprintf(stderr,"shader failed to compile\n%s\n", infolog);
      fprintf(stderr,"GLSL:\n%s\n", shader->glsl_prog);
      return false;
   }
   return true;
}

static inline void
vrend_shader_state_reference(struct vrend_shader_selector **ptr, struct vrend_shader_selector *shader)
{
   struct vrend_shader_selector *old_shader = *ptr;

   if (pipe_reference(&(*ptr)->reference, &shader->reference))
      vrend_destroy_shader_selector(old_shader);
   *ptr = shader;
}

void
vrend_insert_format(struct vrend_format_table *entry, uint32_t bindings)
{
   tex_conv_table[entry->format] = *entry;
   tex_conv_table[entry->format].bindings = bindings;
}

void
vrend_insert_format_swizzle(int override_format, struct vrend_format_table *entry, uint32_t bindings, uint8_t swizzle[4])
{
   int i;
   tex_conv_table[override_format] = *entry;
   tex_conv_table[override_format].bindings = bindings;
   tex_conv_table[override_format].flags = VREND_BIND_NEED_SWIZZLE;
   for (i = 0; i < 4; i++)
      tex_conv_table[override_format].swizzle[i] = swizzle[i];
}

static bool vrend_is_timer_query(GLenum gltype)
{
   return gltype == GL_TIMESTAMP ||
      gltype == GL_TIME_ELAPSED;
}

static void vrend_use_program(struct vrend_context *ctx, GLuint program_id)
{
   if (ctx->sub->program_id != program_id) {
      glUseProgram(program_id);
      ctx->sub->program_id = program_id;
   }
}

static void vrend_init_pstipple_texture(struct vrend_context *ctx)
{
   glGenTextures(1, &ctx->pstipple_tex_id);
   glBindTexture(GL_TEXTURE_2D, ctx->pstipple_tex_id);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 32, 32, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   ctx->pstip_inited = true;
}

static void vrend_blend_enable(struct vrend_context *ctx, bool blend_enable)
{
   if (ctx->sub->blend_enabled != blend_enable) {
      ctx->sub->blend_enabled = blend_enable;
      if (blend_enable)
         glEnable(GL_BLEND);
      else
         glDisable(GL_BLEND);
   }
}

static void vrend_depth_test_enable(struct vrend_context *ctx, bool depth_test_enable)
{
   if (ctx->sub->depth_test_enabled != depth_test_enable) {
      ctx->sub->depth_test_enabled = depth_test_enable;
      if (depth_test_enable)
         glEnable(GL_DEPTH_TEST);
      else
         glDisable(GL_DEPTH_TEST);
   }
}

static void vrend_alpha_test_enable(struct vrend_context *ctx, bool alpha_test_enable)
{
   if (vrend_state.use_core_profile) {
      /* handled in shaders */
      return;
   }
   if (ctx->sub->alpha_test_enabled != alpha_test_enable) {
      ctx->sub->alpha_test_enabled = alpha_test_enable;
      if (alpha_test_enable)
         glEnable(GL_ALPHA_TEST);
      else
         glDisable(GL_ALPHA_TEST);
   }
}

static void vrend_stencil_test_enable(struct vrend_context *ctx, bool stencil_test_enable)
{
   if (ctx->sub->stencil_test_enabled != stencil_test_enable) {
      ctx->sub->stencil_test_enabled = stencil_test_enable;
      if (stencil_test_enable)
         glEnable(GL_STENCIL_TEST);
      else
         glDisable(GL_STENCIL_TEST);
   }
}

static void dump_stream_out(struct pipe_stream_output_info *so)
{
   int i;
   if (!so)
      return;
   printf("streamout: %d\n", so->num_outputs);
   printf("strides: ");
   for (i = 0; i < 4; i++)
      printf("%d ", so->stride[i]);
   printf("\n");
   printf("outputs:\n");
   for (i = 0; i < so->num_outputs; i++) {
      printf("\t%d: reg: %d sc: %d, nc: %d ob: %d do: %d\n",
             i,
             so->output[i].register_index,
             so->output[i].start_component,
             so->output[i].num_components,
             so->output[i].output_buffer,
             so->output[i].dst_offset);
   }
}

static char *get_skip_str(int *skip_val)
{
   char *start_skip = NULL;
   if (*skip_val < 0) {
      *skip_val = 0;
      return NULL;
   }

   if (*skip_val == 1) {
      start_skip = strdup("gl_SkipComponents1");
      *skip_val -= 1;
   } else if (*skip_val == 2) {
      start_skip = strdup("gl_SkipComponents2");
      *skip_val -= 2;
   } else if (*skip_val == 3) {
      start_skip = strdup("gl_SkipComponents3");
      *skip_val -= 3;
   } else if (*skip_val >= 4) {
      start_skip = strdup("gl_SkipComponents4");
      *skip_val -= 4;
   }
   return start_skip;
}

static void set_stream_out_varyings(int prog_id, struct vrend_shader_info *sinfo)
{
   struct pipe_stream_output_info *so = &sinfo->so_info;
   char *varyings[PIPE_MAX_SHADER_OUTPUTS*2];
   int i, j;
   int n_outputs = 0;
   int last_buffer = 0;
   char *start_skip;
   int buf_offset = 0;
   int skip;
   if (!so->num_outputs)
      return;

   if (vrend_dump_shaders)
      dump_stream_out(so);

   for (i = 0; i < so->num_outputs; i++) {
      if (last_buffer != so->output[i].output_buffer) {

         skip = so->stride[last_buffer] - buf_offset;
         while (skip) {
            start_skip = get_skip_str(&skip);
            if (start_skip)
               varyings[n_outputs++] = start_skip;
         }
         for (j = last_buffer; j < so->output[i].output_buffer; j++)
            varyings[n_outputs++] = strdup("gl_NextBuffer");
         last_buffer = so->output[i].output_buffer;
         buf_offset = 0;
      }

      skip = so->output[i].dst_offset - buf_offset;
      while (skip) {
         start_skip = get_skip_str(&skip);
         if (start_skip)
            varyings[n_outputs++] = start_skip;
      }
      buf_offset = so->output[i].dst_offset;

      buf_offset += so->output[i].num_components;
      if (sinfo->so_names[i])
         varyings[n_outputs++] = strdup(sinfo->so_names[i]);
   }

   skip = so->stride[last_buffer] - buf_offset;
   while (skip) {
      start_skip = get_skip_str(&skip);
      if (start_skip)
         varyings[n_outputs++] = start_skip;
   }

   glTransformFeedbackVaryings(prog_id, n_outputs,
                               (const GLchar **)varyings, GL_INTERLEAVED_ATTRIBS_EXT);

   for (i = 0; i < n_outputs; i++)
      if (varyings[i])
         free(varyings[i]);
}

static struct vrend_linked_shader_program *add_shader_program(struct vrend_context *ctx,
                                                              struct vrend_shader *vs,
                                                              struct vrend_shader *fs,
                                                              struct vrend_shader *gs)
{
   struct vrend_linked_shader_program *sprog = CALLOC_STRUCT(vrend_linked_shader_program);
   char name[32];
   int i;
   GLuint prog_id;
   GLint lret;
   int id;
   int last_shader;
   if (!sprog)
      return NULL;

   /* need to rewrite VS code to add interpolation params */
   if ((gs && gs->compiled_fs_id != fs->id) ||
       (!gs && vs->compiled_fs_id != fs->id)) {
      bool ret;

      if (gs)
         vrend_patch_vertex_shader_interpolants(&ctx->shader_cfg, gs->glsl_prog,
                                                &gs->sel->sinfo,
                                                &fs->sel->sinfo, "gso", fs->key.flatshade);
      else
         vrend_patch_vertex_shader_interpolants(&ctx->shader_cfg, vs->glsl_prog,
                                                &vs->sel->sinfo,
                                                &fs->sel->sinfo, "vso", fs->key.flatshade);
      ret = vrend_compile_shader(ctx, gs ? gs : vs);
      if (ret == false) {
         glDeleteShader(gs ? gs->id : vs->id);
         free(sprog);
         return NULL;
      }
      if (gs)
         gs->compiled_fs_id = fs->id;
      else
         vs->compiled_fs_id = fs->id;
   }

   prog_id = glCreateProgram();
   glAttachShader(prog_id, vs->id);
   if (gs) {
      if (gs->id > 0)
         glAttachShader(prog_id, gs->id);
      set_stream_out_varyings(prog_id, &gs->sel->sinfo);
   }
   else
      set_stream_out_varyings(prog_id, &vs->sel->sinfo);
   glAttachShader(prog_id, fs->id);

   if (fs->sel->sinfo.num_outputs > 1) {
      if (util_blend_state_is_dual(&ctx->sub->blend_state, 0)) {
         glBindFragDataLocationIndexed(prog_id, 0, 0, "fsout_c0");
         glBindFragDataLocationIndexed(prog_id, 0, 1, "fsout_c1");
         sprog->dual_src_linked = true;
      } else {
         glBindFragDataLocationIndexed(prog_id, 0, 0, "fsout_c0");
         glBindFragDataLocationIndexed(prog_id, 1, 0, "fsout_c1");
         sprog->dual_src_linked = false;
      }
   } else
      sprog->dual_src_linked = false;

   if (vrend_state.have_vertex_attrib_binding) {
      uint32_t mask = vs->sel->sinfo.attrib_input_mask;
      while (mask) {
         i = u_bit_scan(&mask);
         snprintf(name, 32, "in_%d", i);
         glBindAttribLocation(prog_id, i, name);
      }
   }

   glLinkProgram(prog_id);

   glGetProgramiv(prog_id, GL_LINK_STATUS, &lret);
   if (lret == GL_FALSE) {
      char infolog[65536];
      int len;
      glGetProgramInfoLog(prog_id, 65536, &len, infolog);
      fprintf(stderr,"got error linking\n%s\n", infolog);
      /* dump shaders */
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_SHADER, 0);
      fprintf(stderr,"vert shader: %d GLSL\n%s\n", vs->id, vs->glsl_prog);
      if (gs)
         fprintf(stderr,"geom shader: %d GLSL\n%s\n", gs->id, gs->glsl_prog);
      fprintf(stderr,"frag shader: %d GLSL\n%s\n", fs->id, fs->glsl_prog);
      glDeleteProgram(prog_id);
      free(sprog);
      return NULL;
   }

   sprog->ss[PIPE_SHADER_VERTEX] = vs;
   sprog->ss[PIPE_SHADER_FRAGMENT] = fs;
   sprog->ss[PIPE_SHADER_GEOMETRY] = gs;

   list_add(&sprog->sl[PIPE_SHADER_VERTEX], &vs->programs);
   list_add(&sprog->sl[PIPE_SHADER_FRAGMENT], &fs->programs);
   if (gs)
      list_add(&sprog->sl[PIPE_SHADER_GEOMETRY], &gs->programs);

   last_shader = gs ? PIPE_SHADER_GEOMETRY : PIPE_SHADER_FRAGMENT;
   sprog->id = prog_id;

   list_addtail(&sprog->head, &ctx->sub->programs);

   if (fs->key.pstipple_tex)
      sprog->fs_stipple_loc = glGetUniformLocation(prog_id, "pstipple_sampler");
   else
      sprog->fs_stipple_loc = -1;
   sprog->vs_ws_adjust_loc = glGetUniformLocation(prog_id, "winsys_adjust_y");
   for (id = PIPE_SHADER_VERTEX; id <= last_shader; id++) {
      if (sprog->ss[id]->sel->sinfo.samplers_used_mask) {
         uint32_t mask = sprog->ss[id]->sel->sinfo.samplers_used_mask;
         int nsamp = util_bitcount(sprog->ss[id]->sel->sinfo.samplers_used_mask);
         int index;
         sprog->shadow_samp_mask[id] = sprog->ss[id]->sel->sinfo.shadow_samp_mask;
         if (sprog->ss[id]->sel->sinfo.shadow_samp_mask) {
            sprog->shadow_samp_mask_locs[id] = calloc(nsamp, sizeof(uint32_t));
            sprog->shadow_samp_add_locs[id] = calloc(nsamp, sizeof(uint32_t));
         } else {
            sprog->shadow_samp_mask_locs[id] = sprog->shadow_samp_add_locs[id] = NULL;
         }
         sprog->samp_locs[id] = calloc(nsamp, sizeof(uint32_t));
         if (sprog->samp_locs[id]) {
            const char *prefix = pipe_shader_to_prefix(id);
            index = 0;
            while(mask) {
               i = u_bit_scan(&mask);
               snprintf(name, 32, "%ssamp%d", prefix, i);
               sprog->samp_locs[id][index] = glGetUniformLocation(prog_id, name);
               if (sprog->ss[id]->sel->sinfo.shadow_samp_mask & (1 << i)) {
                  snprintf(name, 32, "%sshadmask%d", prefix, i);
                  sprog->shadow_samp_mask_locs[id][index] = glGetUniformLocation(prog_id, name);
                  snprintf(name, 32, "%sshadadd%d", prefix, i);
                  sprog->shadow_samp_add_locs[id][index] = glGetUniformLocation(prog_id, name);
               }
               index++;
            }
         }
      } else {
         sprog->samp_locs[id] = NULL;
         sprog->shadow_samp_mask_locs[id] = NULL;
         sprog->shadow_samp_add_locs[id] = NULL;
         sprog->shadow_samp_mask[id] = 0;
      }
      sprog->samplers_used_mask[id] = sprog->ss[id]->sel->sinfo.samplers_used_mask;
   }

   for (id = PIPE_SHADER_VERTEX; id <= last_shader; id++) {
      if (sprog->ss[id]->sel->sinfo.num_consts) {
         sprog->const_locs[id] = calloc(sprog->ss[id]->sel->sinfo.num_consts, sizeof(uint32_t));
         if (sprog->const_locs[id]) {
            const char *prefix = pipe_shader_to_prefix(id);
            for (i = 0; i < sprog->ss[id]->sel->sinfo.num_consts; i++) {
               snprintf(name, 32, "%sconst0[%d]", prefix, i);
               sprog->const_locs[id][i] = glGetUniformLocation(prog_id, name);
            }
         }
      } else
         sprog->const_locs[id] = NULL;
   }

   if (!vrend_state.have_vertex_attrib_binding) {
      if (vs->sel->sinfo.num_inputs) {
         sprog->attrib_locs = calloc(vs->sel->sinfo.num_inputs, sizeof(uint32_t));
         if (sprog->attrib_locs) {
            for (i = 0; i < vs->sel->sinfo.num_inputs; i++) {
               snprintf(name, 32, "in_%d", i);
               sprog->attrib_locs[i] = glGetAttribLocation(prog_id, name);
            }
         }
      } else
         sprog->attrib_locs = NULL;
   }

   for (id = PIPE_SHADER_VERTEX; id <= last_shader; id++) {
      if (sprog->ss[id]->sel->sinfo.num_ubos) {
         const char *prefix = pipe_shader_to_prefix(id);

         sprog->ubo_locs[id] = calloc(sprog->ss[id]->sel->sinfo.num_ubos, sizeof(uint32_t));
         for (i = 0; i < sprog->ss[id]->sel->sinfo.num_ubos; i++) {
            snprintf(name, 32, "%subo%d", prefix, i + 1);
            sprog->ubo_locs[id][i] = glGetUniformBlockIndex(prog_id, name);
         }
      } else
         sprog->ubo_locs[id] = NULL;
   }

   if (vs->sel->sinfo.num_ucp) {
      for (i = 0; i < vs->sel->sinfo.num_ucp; i++) {
         snprintf(name, 32, "clipp[%d]", i);
         sprog->clip_locs[i] = glGetUniformLocation(prog_id, name);
      }
   }
   return sprog;
}

static struct vrend_linked_shader_program *lookup_shader_program(struct vrend_context *ctx,
                                                                 GLuint vs_id, GLuint fs_id, GLuint gs_id, bool dual_src)
{
   struct vrend_linked_shader_program *ent;
   LIST_FOR_EACH_ENTRY(ent, &ctx->sub->programs, head) {
      if (ent->dual_src_linked != dual_src)
         continue;
      if (ent->ss[PIPE_SHADER_VERTEX]->id == vs_id && ent->ss[PIPE_SHADER_FRAGMENT]->id == fs_id) {
         if (!ent->ss[PIPE_SHADER_GEOMETRY] && gs_id == 0)
            return ent;
         if (ent->ss[PIPE_SHADER_GEOMETRY] && ent->ss[PIPE_SHADER_GEOMETRY]->id == gs_id)
            return ent;
      }
   }
   return NULL;
}

static void vrend_destroy_program(struct vrend_linked_shader_program *ent)
{
   int i;
   glDeleteProgram(ent->id);
   list_del(&ent->head);

   for (i = PIPE_SHADER_VERTEX; i <= PIPE_SHADER_GEOMETRY; i++) {
      if (ent->ss[i])
         list_del(&ent->sl[i]);
      free(ent->shadow_samp_mask_locs[i]);
      free(ent->shadow_samp_add_locs[i]);
      free(ent->samp_locs[i]);
      free(ent->const_locs[i]);
      free(ent->ubo_locs[i]);
   }
   free(ent->attrib_locs);
   free(ent);
}

static void vrend_free_programs(struct vrend_sub_context *sub)
{
   struct vrend_linked_shader_program *ent, *tmp;

   if (LIST_IS_EMPTY(&sub->programs))
      return;

   LIST_FOR_EACH_ENTRY_SAFE(ent, tmp, &sub->programs, head) {
      vrend_destroy_program(ent);
   }
}

static void vrend_destroy_streamout_object(struct vrend_streamout_object *obj)
{
   int i;
   list_del(&obj->head);
   for (i = 0; i < obj->num_targets; i++)
      vrend_so_target_reference(&obj->so_targets[i], NULL);
   if (vrend_state.have_tf2)
      glDeleteTransformFeedbacks(1, &obj->id);
   FREE(obj);
}

int vrend_create_surface(struct vrend_context *ctx,
                         uint32_t handle,
                         uint32_t res_handle, uint32_t format,
                         uint32_t val0, uint32_t val1)
{
   struct vrend_surface *surf;
   struct vrend_resource *res;
   uint32_t ret_handle;

   if (format >= PIPE_FORMAT_COUNT) {
      return EINVAL;
   }

   res = vrend_renderer_ctx_res_lookup(ctx, res_handle);
   if (!res) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_RESOURCE, res_handle);
      return EINVAL;
   }

   surf = CALLOC_STRUCT(vrend_surface);
   if (!surf)
      return ENOMEM;

   surf->res_handle = res_handle;
   surf->format = format;
   surf->val0 = val0;
   surf->val1 = val1;
   pipe_reference_init(&surf->reference, 1);

   vrend_resource_reference(&surf->texture, res);

   ret_handle = vrend_renderer_object_insert(ctx, surf, sizeof(*surf), handle, VIRGL_OBJECT_SURFACE);
   if (ret_handle == 0) {
      FREE(surf);
      return ENOMEM;
   }
   return 0;
}

static void vrend_destroy_surface_object(void *obj_ptr)
{
   struct vrend_surface *surface = obj_ptr;

   vrend_surface_reference(&surface, NULL);
}

static void vrend_destroy_sampler_view_object(void *obj_ptr)
{
   struct vrend_sampler_view *samp = obj_ptr;

   vrend_sampler_view_reference(&samp, NULL);
}

static void vrend_destroy_so_target_object(void *obj_ptr)
{
   struct vrend_so_target *target = obj_ptr;
   struct vrend_sub_context *sub_ctx = target->sub_ctx;
   struct vrend_streamout_object *obj, *tmp;
   bool found;
   int i;

   LIST_FOR_EACH_ENTRY_SAFE(obj, tmp, &sub_ctx->streamout_list, head) {
      found = false;
      for (i = 0; i < obj->num_targets; i++) {
         if (obj->so_targets[i] == target) {
            found = true;
            break;
         }
      }
      if (found) {
         if (obj == sub_ctx->current_so)
            sub_ctx->current_so = NULL;
         if (obj->xfb_state == XFB_STATE_PAUSED) {
               if (vrend_state.have_tf2)
                  glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, obj->id);
               glEndTransformFeedback();
            if (sub_ctx->current_so && vrend_state.have_tf2)
               glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, sub_ctx->current_so->id);
         }
         vrend_destroy_streamout_object(obj);
      }
   }

   vrend_so_target_reference(&target, NULL);
}

static void vrend_destroy_vertex_elements_object(void *obj_ptr)
{
   struct vrend_vertex_element_array *v = obj_ptr;

   if (vrend_state.have_vertex_attrib_binding) {
      glDeleteVertexArrays(1, &v->id);
   }
   FREE(v);
}

static void vrend_destroy_sampler_state_object(void *obj_ptr)
{
   struct vrend_sampler_state *state = obj_ptr;

   glDeleteSamplers(1, &state->id);
   FREE(state);
}

static GLuint convert_wrap(int wrap)
{
   switch(wrap){
   case PIPE_TEX_WRAP_REPEAT: return GL_REPEAT;
   case PIPE_TEX_WRAP_CLAMP: if (vrend_state.use_core_profile == false) return GL_CLAMP; else return GL_CLAMP_TO_EDGE;

   case PIPE_TEX_WRAP_CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER: return GL_CLAMP_TO_BORDER;

   case PIPE_TEX_WRAP_MIRROR_REPEAT: return GL_MIRRORED_REPEAT;
   case PIPE_TEX_WRAP_MIRROR_CLAMP: return GL_MIRROR_CLAMP_EXT;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE: return GL_MIRROR_CLAMP_TO_EDGE_EXT;
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER: return GL_MIRROR_CLAMP_TO_BORDER_EXT;
   default:
      assert(0);
      return -1;
   }
}

static inline GLenum convert_mag_filter(unsigned int filter)
{
   if (filter == PIPE_TEX_FILTER_NEAREST)
      return GL_NEAREST;
   return GL_LINEAR;
}

static inline GLenum convert_min_filter(unsigned int filter, unsigned int mip_filter)
{
   if (mip_filter == PIPE_TEX_MIPFILTER_NONE)
      return convert_mag_filter(filter);
   else if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
      if (filter == PIPE_TEX_FILTER_NEAREST)
         return GL_NEAREST_MIPMAP_LINEAR;
      else
         return GL_LINEAR_MIPMAP_LINEAR;
   } else if (mip_filter == PIPE_TEX_MIPFILTER_NEAREST) {
      if (filter == PIPE_TEX_FILTER_NEAREST)
         return GL_NEAREST_MIPMAP_NEAREST;
      else
         return GL_LINEAR_MIPMAP_NEAREST;
   }
   assert(0);
   return 0;
}

int vrend_create_sampler_state(struct vrend_context *ctx,
                               uint32_t handle,
                               struct pipe_sampler_state *templ)
{
   struct vrend_sampler_state *state = CALLOC_STRUCT(vrend_sampler_state);
   int ret_handle;

   if (!state)
      return ENOMEM;

   state->base = *templ;

   if (vrend_state.have_samplers) {
      glGenSamplers(1, &state->id);

      glSamplerParameteri(state->id, GL_TEXTURE_WRAP_S, convert_wrap(templ->wrap_s));
      glSamplerParameteri(state->id, GL_TEXTURE_WRAP_T, convert_wrap(templ->wrap_t));
      glSamplerParameteri(state->id, GL_TEXTURE_WRAP_R, convert_wrap(templ->wrap_r));
      glSamplerParameterf(state->id, GL_TEXTURE_MIN_FILTER, convert_min_filter(templ->min_img_filter, templ->min_mip_filter));
      glSamplerParameterf(state->id, GL_TEXTURE_MAG_FILTER, convert_mag_filter(templ->mag_img_filter));
      glSamplerParameterf(state->id, GL_TEXTURE_MIN_LOD, templ->min_lod);
      glSamplerParameterf(state->id, GL_TEXTURE_MAX_LOD, templ->max_lod);
      glSamplerParameteri(state->id, GL_TEXTURE_COMPARE_MODE, templ->compare_mode ? GL_COMPARE_R_TO_TEXTURE : GL_NONE);
      glSamplerParameteri(state->id, GL_TEXTURE_COMPARE_FUNC, GL_NEVER + templ->compare_func);
      if (vrend_state.use_gles) {
         if (templ->lod_bias != 0.0f) {
            report_gles_warn(ctx, GLES_WARN_LOD_BIAS, 0);
         }
      } else {
         glSamplerParameterf(state->id, GL_TEXTURE_LOD_BIAS, templ->lod_bias);
      }

      glSamplerParameterIuiv(state->id, GL_TEXTURE_BORDER_COLOR, templ->border_color.ui);
   }
   ret_handle = vrend_renderer_object_insert(ctx, state, sizeof(struct vrend_sampler_state), handle,
                                             VIRGL_OBJECT_SAMPLER_STATE);
   if (!ret_handle) {
      if (vrend_state.have_samplers)
         glDeleteSamplers(1, &state->id);
      FREE(state);
      return ENOMEM;
   }
   return 0;
}

static inline GLenum to_gl_swizzle(int swizzle)
{
   switch (swizzle) {
   case PIPE_SWIZZLE_RED: return GL_RED;
   case PIPE_SWIZZLE_GREEN: return GL_GREEN;
   case PIPE_SWIZZLE_BLUE: return GL_BLUE;
   case PIPE_SWIZZLE_ALPHA: return GL_ALPHA;
   case PIPE_SWIZZLE_ZERO: return GL_ZERO;
   case PIPE_SWIZZLE_ONE: return GL_ONE;
   default:
      assert(0);
      return 0;
   }
}

int vrend_create_sampler_view(struct vrend_context *ctx,
                              uint32_t handle,
                              uint32_t res_handle, uint32_t format,
                              uint32_t val0, uint32_t val1, uint32_t swizzle_packed)
{
   struct vrend_sampler_view *view;
   struct vrend_resource *res;
   int ret_handle;
   res = vrend_renderer_ctx_res_lookup(ctx, res_handle);
   if (!res) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_RESOURCE, res_handle);
      return EINVAL;
   }

   view = CALLOC_STRUCT(vrend_sampler_view);
   if (!view)
      return ENOMEM;

   pipe_reference_init(&view->reference, 1);
   view->format = format;
   view->val0 = val0;
   view->val1 = val1;
   view->swizzle_r = swizzle_packed & 0x7;
   view->swizzle_g = (swizzle_packed >> 3) & 0x7;
   view->swizzle_b = (swizzle_packed >> 6) & 0x7;
   view->swizzle_a = (swizzle_packed >> 9) & 0x7;
   view->cur_base = -1;
   view->cur_max = 10000;

   vrend_resource_reference(&view->texture, res);

   view->srgb_decode = GL_DECODE_EXT;
   if (view->format != view->texture->base.format) {
      if (util_format_is_srgb(view->texture->base.format) &&
          !util_format_is_srgb(view->format))
         view->srgb_decode = GL_SKIP_DECODE_EXT;
   }

   view->gl_swizzle_a = to_gl_swizzle(view->swizzle_a);
   view->gl_swizzle_r = to_gl_swizzle(view->swizzle_r);
   view->gl_swizzle_g = to_gl_swizzle(view->swizzle_g);
   view->gl_swizzle_b = to_gl_swizzle(view->swizzle_b);

   if (!(util_format_has_alpha(format) || util_format_is_depth_or_stencil(format))) {
       if (view->gl_swizzle_a == GL_ALPHA)
           view->gl_swizzle_a = GL_ONE;
       if (view->gl_swizzle_r == GL_ALPHA)
           view->gl_swizzle_r = GL_ONE;
       if (view->gl_swizzle_g == GL_ALPHA)
           view->gl_swizzle_g = GL_ONE;
       if (view->gl_swizzle_b == GL_ALPHA)
           view->gl_swizzle_b = GL_ONE;
   }

   if (tex_conv_table[format].flags & VREND_BIND_NEED_SWIZZLE) {
      view->gl_swizzle_r = to_gl_swizzle(tex_conv_table[format].swizzle[0]);
      view->gl_swizzle_g = to_gl_swizzle(tex_conv_table[format].swizzle[1]);
      view->gl_swizzle_b = to_gl_swizzle(tex_conv_table[format].swizzle[2]);
      view->gl_swizzle_a = to_gl_swizzle(tex_conv_table[format].swizzle[3]);
   }
   ret_handle = vrend_renderer_object_insert(ctx, view, sizeof(*view), handle, VIRGL_OBJECT_SAMPLER_VIEW);
   if (ret_handle == 0) {
      FREE(view);
      return ENOMEM;
   }
   return 0;
}

void vrend_fb_bind_texture(struct vrend_resource *res,
                           int idx,
                           uint32_t level, uint32_t layer)
{
   const struct util_format_description *desc = util_format_description(res->base.format);
   GLenum attachment = GL_COLOR_ATTACHMENT0_EXT + idx;

   if (vrend_format_is_ds(res->base.format)) { {
         if (util_format_has_stencil(desc)) {
            if (util_format_has_depth(desc))
               attachment = GL_DEPTH_STENCIL_ATTACHMENT;
            else
               attachment = GL_STENCIL_ATTACHMENT;
         } else
            attachment = GL_DEPTH_ATTACHMENT;
      }
   }

   switch (res->target) {
   case GL_TEXTURE_1D_ARRAY:
   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
      if (layer == 0xffffffff)
         glFramebufferTexture(GL_FRAMEBUFFER_EXT, attachment,
                              res->id, level);
      else
         glFramebufferTextureLayer(GL_FRAMEBUFFER_EXT, attachment,
                                   res->id, level, layer);
      break;
   case GL_TEXTURE_3D:
      if (layer == 0xffffffff)
         glFramebufferTexture(GL_FRAMEBUFFER_EXT, attachment,
                              res->id, level);
      else
         glFramebufferTexture3DEXT(GL_FRAMEBUFFER_EXT, attachment,
                                   res->target, res->id, level, layer);
      break;
   case GL_TEXTURE_CUBE_MAP:
      if (layer == 0xffffffff)
         glFramebufferTexture(GL_FRAMEBUFFER_EXT, attachment,
                              res->id, level);
      else
         glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachment,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, res->id, level);
      break;
   case GL_TEXTURE_1D:
      glFramebufferTexture1DEXT(GL_FRAMEBUFFER_EXT, attachment,
                                res->target, res->id, level);
      break;
   case GL_TEXTURE_2D:
   default:
      glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachment,
                                res->target, res->id, level);
      break;
   }

   if (attachment == GL_DEPTH_ATTACHMENT) {
      switch (res->target) {
      case GL_TEXTURE_1D:
         glFramebufferTexture1DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT,
                                   GL_TEXTURE_1D, 0, 0);
         break;
      case GL_TEXTURE_2D:
      default:
         glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT,
                                   GL_TEXTURE_2D, 0, 0);
         break;
      }
   }
}

static void vrend_hw_set_zsurf_texture(struct vrend_context *ctx)
{
   struct vrend_resource *tex;
   int first_layer, last_layer;
   if (!ctx->sub->zsurf) {
      glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT,
                                GL_TEXTURE_2D, 0, 0);
      return;

   }
   tex = ctx->sub->zsurf->texture;
   if (!tex)
      return;

   first_layer = ctx->sub->zsurf->val1 & 0xffff;
   last_layer = (ctx->sub->zsurf->val1 >> 16) & 0xffff;

   vrend_fb_bind_texture(tex, 0, ctx->sub->zsurf->val0,
                         first_layer != last_layer ? 0xffffffff : first_layer);
}

static void vrend_hw_set_color_surface(struct vrend_context *ctx, int index)
{
   struct vrend_resource *tex;

   if (!ctx->sub->surf[index]) {
      GLenum attachment = GL_COLOR_ATTACHMENT0 + index;

      glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachment,
                                GL_TEXTURE_2D, 0, 0);
   } else {
      int first_layer = ctx->sub->surf[index]->val1 & 0xffff;
      int last_layer = (ctx->sub->surf[index]->val1 >> 16) & 0xffff;
      tex = ctx->sub->surf[index]->texture;


      vrend_fb_bind_texture(tex, index, ctx->sub->surf[index]->val0,
                            first_layer != last_layer ? 0xffffffff : first_layer);
   }


}

static void vrend_hw_emit_framebuffer_state(struct vrend_context *ctx)
{
   static const GLenum buffers[8] = {
      GL_COLOR_ATTACHMENT0_EXT,
      GL_COLOR_ATTACHMENT1_EXT,
      GL_COLOR_ATTACHMENT2_EXT,
      GL_COLOR_ATTACHMENT3_EXT,
      GL_COLOR_ATTACHMENT4_EXT,
      GL_COLOR_ATTACHMENT5_EXT,
      GL_COLOR_ATTACHMENT6_EXT,
      GL_COLOR_ATTACHMENT7_EXT,
   };
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ctx->sub->fb_id);

   if (ctx->sub->nr_cbufs == 0) {
      glReadBuffer(GL_NONE);
      if (!vrend_state.use_gles) {
         glDisable(GL_FRAMEBUFFER_SRGB_EXT);
      }
   } else {
      struct vrend_surface *surf = NULL;
      bool use_srgb = false;
      int i;
      for (i = 0; i < ctx->sub->nr_cbufs; i++) {
         if (ctx->sub->surf[i]) {
            surf = ctx->sub->surf[i];
            if (util_format_is_srgb(surf->format)) {
               use_srgb = true;
            }
         }
      }
      if (use_srgb) {
         if (!vrend_state.use_gles) {
            glEnable(GL_FRAMEBUFFER_SRGB_EXT);
         } else {
            report_gles_warn(ctx, GLES_WARN_SRGB_FB, 0);
         }
      } else {
         if (!vrend_state.use_gles) {
            glDisable(GL_FRAMEBUFFER_SRGB_EXT);
         }
      }
   }
   glDrawBuffers(ctx->sub->nr_cbufs, buffers);
}

void vrend_set_framebuffer_state(struct vrend_context *ctx,
                                 uint32_t nr_cbufs, uint32_t surf_handle[PIPE_MAX_COLOR_BUFS],
                                 uint32_t zsurf_handle)
{
   struct vrend_surface *surf, *zsurf;
   int i;
   int old_num;
   GLenum status;
   GLint new_height = -1;
   bool new_ibf = false;

   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ctx->sub->fb_id);

   if (zsurf_handle) {
      zsurf = vrend_object_lookup(ctx->sub->object_hash, zsurf_handle, VIRGL_OBJECT_SURFACE);
      if (!zsurf) {
         report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_SURFACE, zsurf_handle);
         return;
      }
   } else
      zsurf = NULL;

   if (ctx->sub->zsurf != zsurf) {
      vrend_surface_reference(&ctx->sub->zsurf, zsurf);
      vrend_hw_set_zsurf_texture(ctx);
   }

   old_num = ctx->sub->nr_cbufs;
   ctx->sub->nr_cbufs = nr_cbufs;
   ctx->sub->old_nr_cbufs = old_num;

   for (i = 0; i < nr_cbufs; i++) {
      if (surf_handle[i] != 0) {
         surf = vrend_object_lookup(ctx->sub->object_hash, surf_handle[i], VIRGL_OBJECT_SURFACE);
         if (!surf) {
            report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_SURFACE, surf_handle[i]);
            return;
         }
      } else
         surf = NULL;

      if (ctx->sub->surf[i] != surf) {
         vrend_surface_reference(&ctx->sub->surf[i], surf);
         vrend_hw_set_color_surface(ctx, i);
      }
   }

   if (old_num > ctx->sub->nr_cbufs) {
      for (i = ctx->sub->nr_cbufs; i < old_num; i++) {
         vrend_surface_reference(&ctx->sub->surf[i], NULL);
         vrend_hw_set_color_surface(ctx, i);
      }
   }

   /* find a buffer to set fb_height from */
   if (ctx->sub->nr_cbufs == 0 && !ctx->sub->zsurf) {
      new_height = 0;
      new_ibf = false;
   } else if (ctx->sub->nr_cbufs == 0) {
      new_height = u_minify(ctx->sub->zsurf->texture->base.height0, ctx->sub->zsurf->val0);
      new_ibf = ctx->sub->zsurf->texture->y_0_top ? true : false;
   }
   else {
      surf = NULL;
      for (i = 0; i < ctx->sub->nr_cbufs; i++) {
         if (ctx->sub->surf[i]) {
            surf = ctx->sub->surf[i];
            break;
         }
      }
      if (surf == NULL) {
         report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_SURFACE, i);
         return;
      }
      new_height = u_minify(surf->texture->base.height0, surf->val0);
      new_ibf = surf->texture->y_0_top ? true : false;
   }

   if (new_height != -1) {
      if (ctx->sub->fb_height != new_height || ctx->sub->inverted_fbo_content != new_ibf) {
         ctx->sub->fb_height = new_height;
         ctx->sub->inverted_fbo_content = new_ibf;
         ctx->sub->scissor_state_dirty = (1 << 0);
         ctx->sub->viewport_state_dirty = (1 << 0);
      }
   }

   vrend_hw_emit_framebuffer_state(ctx);

   if (ctx->sub->nr_cbufs > 0 || ctx->sub->zsurf) {
      status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
      if (status != GL_FRAMEBUFFER_COMPLETE)
         fprintf(stderr,"failed to complete framebuffer 0x%x %s\n", status, ctx->debug_name);
   }
   ctx->sub->shader_dirty = true;
}

/*
 * if the viewport Y scale factor is > 0 then we are rendering to
 * an FBO already so don't need to invert rendering?
 */
void vrend_set_viewport_states(struct vrend_context *ctx,
                               uint32_t start_slot,
                               uint32_t num_viewports,
                               const struct pipe_viewport_state *state)
{
   /* convert back to glViewport */
   GLint x, y;
   GLsizei width, height;
   GLclampd near_val, far_val;
   bool viewport_is_negative = (state[0].scale[1] < 0) ? true : false;
   int i, idx;

   if (num_viewports > PIPE_MAX_VIEWPORTS ||
       start_slot > (PIPE_MAX_VIEWPORTS - num_viewports)) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_CMD_BUFFER, num_viewports);
      return;
   }

   for (i = 0; i < num_viewports; i++) {
      GLfloat abs_s1 = fabsf(state[i].scale[1]);

      idx = start_slot + i;
      width = state[i].scale[0] * 2.0f;
      height = abs_s1 * 2.0f;
      x = state[i].translate[0] - state[i].scale[0];
      y = state[i].translate[1] - state[i].scale[1];

      near_val = state[i].translate[2] - state[i].scale[2];
      far_val = near_val + (state[i].scale[2] * 2.0);

      if (ctx->sub->vps[idx].cur_x != x ||
          ctx->sub->vps[idx].cur_y != y ||
          ctx->sub->vps[idx].width != width ||
          ctx->sub->vps[idx].height != height) {
         ctx->sub->viewport_state_dirty |= (1 << idx);
         ctx->sub->vps[idx].cur_x = x;
         ctx->sub->vps[idx].cur_y = y;
         ctx->sub->vps[idx].width = width;
         ctx->sub->vps[idx].height = height;
      }

      if (idx == 0) {
         if (ctx->sub->viewport_is_negative != viewport_is_negative)
            ctx->sub->viewport_is_negative = viewport_is_negative;

         ctx->sub->depth_scale = fabs(far_val - near_val);
         ctx->sub->depth_transform = near_val;
      }

      if (ctx->sub->vps[idx].near_val != near_val ||
          ctx->sub->vps[idx].far_val != far_val) {
         ctx->sub->vps[idx].near_val = near_val;
         ctx->sub->vps[idx].far_val = far_val;

         if (vrend_state.use_gles) {
            if (near_val < 0.0f || far_val < 0.0f ||
                near_val > 1.0f || far_val > 1.0f || idx) {
               report_gles_warn(ctx, GLES_WARN_DEPTH_RANGE, 0);
            }

            /* Best effort despite the warning, gles will clamp. */
            glDepthRangef(ctx->sub->vps[idx].near_val, ctx->sub->vps[idx].far_val);
         } else if (idx)
            glDepthRangeIndexed(idx, ctx->sub->vps[idx].near_val, ctx->sub->vps[idx].far_val);
         else
            glDepthRange(ctx->sub->vps[idx].near_val, ctx->sub->vps[idx].far_val);
      }
   }
}

int vrend_create_vertex_elements_state(struct vrend_context *ctx,
                                       uint32_t handle,
                                       unsigned num_elements,
                                       const struct pipe_vertex_element *elements)
{
   struct vrend_vertex_element_array *v;
   const struct util_format_description *desc;
   GLenum type;
   int i;
   uint32_t ret_handle;

   if (num_elements > PIPE_MAX_ATTRIBS)
      return EINVAL;

   v = CALLOC_STRUCT(vrend_vertex_element_array);
   if (!v)
      return ENOMEM;

   v->count = num_elements;
   for (i = 0; i < num_elements; i++) {
      memcpy(&v->elements[i].base, &elements[i], sizeof(struct pipe_vertex_element));

      desc = util_format_description(elements[i].src_format);
      if (!desc) {
         FREE(v);
         return EINVAL;
      }

      type = GL_FALSE;
      if (desc->channel[0].type == UTIL_FORMAT_TYPE_FLOAT) {
         if (desc->channel[0].size == 32)
            type = GL_FLOAT;
         else if (desc->channel[0].size == 64)
            type = GL_DOUBLE;
         else if (desc->channel[0].size == 16)
            type = GL_HALF_FLOAT;
      } else if (desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED &&
                 desc->channel[0].size == 8)
         type = GL_UNSIGNED_BYTE;
      else if (desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED &&
               desc->channel[0].size == 8)
         type = GL_BYTE;
      else if (desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED &&
               desc->channel[0].size == 16)
         type = GL_UNSIGNED_SHORT;
      else if (desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED &&
               desc->channel[0].size == 16)
         type = GL_SHORT;
      else if (desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED &&
               desc->channel[0].size == 32)
         type = GL_UNSIGNED_INT;
      else if (desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED &&
               desc->channel[0].size == 32)
         type = GL_INT;
      else if (elements[i].src_format == PIPE_FORMAT_R10G10B10A2_SSCALED ||
               elements[i].src_format == PIPE_FORMAT_R10G10B10A2_SNORM ||
               elements[i].src_format == PIPE_FORMAT_B10G10R10A2_SNORM)
         type = GL_INT_2_10_10_10_REV;
      else if (elements[i].src_format == PIPE_FORMAT_R10G10B10A2_USCALED ||
               elements[i].src_format == PIPE_FORMAT_R10G10B10A2_UNORM ||
               elements[i].src_format == PIPE_FORMAT_B10G10R10A2_UNORM)
         type = GL_UNSIGNED_INT_2_10_10_10_REV;
      else if (elements[i].src_format == PIPE_FORMAT_R11G11B10_FLOAT)
         type = GL_UNSIGNED_INT_10F_11F_11F_REV;

      if (type == GL_FALSE) {
         report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_VERTEX_FORMAT, elements[i].src_format);
         FREE(v);
         return EINVAL;
      }

      v->elements[i].type = type;
      if (desc->channel[0].normalized)
         v->elements[i].norm = GL_TRUE;
      if (desc->nr_channels == 4 && desc->swizzle[0] == UTIL_FORMAT_SWIZZLE_Z)
         v->elements[i].nr_chan = GL_BGRA;
      else if (elements[i].src_format == PIPE_FORMAT_R11G11B10_FLOAT)
         v->elements[i].nr_chan = 3;
      else
         v->elements[i].nr_chan = desc->nr_channels;
   }

   if (vrend_state.have_vertex_attrib_binding) {
      glGenVertexArrays(1, &v->id);
      glBindVertexArray(v->id);
      for (i = 0; i < num_elements; i++) {
         struct vrend_vertex_element *ve = &v->elements[i];

         if (util_format_is_pure_integer(ve->base.src_format))
            glVertexAttribIFormat(i, ve->nr_chan, ve->type, ve->base.src_offset);
         else
            glVertexAttribFormat(i, ve->nr_chan, ve->type, ve->norm, ve->base.src_offset);
         glVertexAttribBinding(i, ve->base.vertex_buffer_index);
         glVertexBindingDivisor(i, ve->base.instance_divisor);
         glEnableVertexAttribArray(i);
      }
   }
   ret_handle = vrend_renderer_object_insert(ctx, v, sizeof(struct vrend_vertex_element), handle,
                                             VIRGL_OBJECT_VERTEX_ELEMENTS);
   if (!ret_handle) {
      FREE(v);
      return ENOMEM;
   }
   return 0;
}

void vrend_bind_vertex_elements_state(struct vrend_context *ctx,
                                      uint32_t handle)
{
   struct vrend_vertex_element_array *v;

   if (!handle) {
      ctx->sub->ve = NULL;
      return;
   }
   v = vrend_object_lookup(ctx->sub->object_hash, handle, VIRGL_OBJECT_VERTEX_ELEMENTS);
   if (!v) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_HANDLE, handle);
      return;
   }

   if (ctx->sub->ve != v)
      ctx->sub->vbo_dirty = true;
   ctx->sub->ve = v;
}

void vrend_set_constants(struct vrend_context *ctx,
                         uint32_t shader,
                         uint32_t index,
                         uint32_t num_constant,
                         float *data)
{
   struct vrend_constants *consts;
   int i;

   consts = &ctx->sub->consts[shader];
   ctx->sub->const_dirty[shader] = true;

   consts->consts = realloc(consts->consts, num_constant * sizeof(float));
   if (!consts->consts)
      return;

   consts->num_consts = num_constant;
   for (i = 0; i < num_constant; i++)
      consts->consts[i] = ((unsigned int *)data)[i];
}

void vrend_set_uniform_buffer(struct vrend_context *ctx,
                              uint32_t shader,
                              uint32_t index,
                              uint32_t offset,
                              uint32_t length,
                              uint32_t res_handle)
{
   struct vrend_resource *res;

   if (res_handle) {
      res = vrend_renderer_ctx_res_lookup(ctx, res_handle);

      if (!res) {
         report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_RESOURCE, res_handle);
         return;
      }
      vrend_resource_reference((struct vrend_resource **)&ctx->sub->cbs[shader][index].buffer, res);
      ctx->sub->cbs[shader][index].buffer_offset = offset;
      ctx->sub->cbs[shader][index].buffer_size = length;

      ctx->sub->const_bufs_used_mask[shader] |= (1 << index);
   } else {
      vrend_resource_reference((struct vrend_resource **)&ctx->sub->cbs[shader][index].buffer, NULL);
      ctx->sub->cbs[shader][index].buffer_offset = 0;
      ctx->sub->cbs[shader][index].buffer_size = 0;
      ctx->sub->const_bufs_used_mask[shader] &= ~(1 << index);
   }
}

void vrend_set_index_buffer(struct vrend_context *ctx,
                            uint32_t res_handle,
                            uint32_t index_size,
                            uint32_t offset)
{
   struct vrend_resource *res;

   ctx->sub->ib.index_size = index_size;
   ctx->sub->ib.offset = offset;
   if (res_handle) {
      if (ctx->sub->index_buffer_res_id != res_handle) {
         res = vrend_renderer_ctx_res_lookup(ctx, res_handle);
         if (!res) {
            vrend_resource_reference((struct vrend_resource **)&ctx->sub->ib.buffer, NULL);
            ctx->sub->index_buffer_res_id = 0;
            report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_RESOURCE, res_handle);
            return;
         }
         vrend_resource_reference((struct vrend_resource **)&ctx->sub->ib.buffer, res);
         ctx->sub->index_buffer_res_id = res_handle;
      }
   } else {
      vrend_resource_reference((struct vrend_resource **)&ctx->sub->ib.buffer, NULL);
      ctx->sub->index_buffer_res_id = 0;
   }
}

void vrend_set_single_vbo(struct vrend_context *ctx,
                          int index,
                          uint32_t stride,
                          uint32_t buffer_offset,
                          uint32_t res_handle)
{
   struct vrend_resource *res;

   if (ctx->sub->vbo[index].stride != stride ||
       ctx->sub->vbo[index].buffer_offset != buffer_offset ||
       ctx->sub->vbo_res_ids[index] != res_handle)
      ctx->sub->vbo_dirty = true;

   ctx->sub->vbo[index].stride = stride;
   ctx->sub->vbo[index].buffer_offset = buffer_offset;

   if (res_handle == 0) {
      vrend_resource_reference((struct vrend_resource **)&ctx->sub->vbo[index].buffer, NULL);
      ctx->sub->vbo_res_ids[index] = 0;
   } else if (ctx->sub->vbo_res_ids[index] != res_handle) {
      res = vrend_renderer_ctx_res_lookup(ctx, res_handle);
      if (!res) {
         report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_RESOURCE, res_handle);
         ctx->sub->vbo_res_ids[index] = 0;
         return;
      }
      vrend_resource_reference((struct vrend_resource **)&ctx->sub->vbo[index].buffer, res);
      ctx->sub->vbo_res_ids[index] = res_handle;
   }
}

void vrend_set_num_vbo(struct vrend_context *ctx,
                       int num_vbo)
{
   int old_num = ctx->sub->num_vbos;
   int i;

   ctx->sub->num_vbos = num_vbo;
   ctx->sub->old_num_vbos = old_num;

   if (old_num != num_vbo)
      ctx->sub->vbo_dirty = true;

   for (i = num_vbo; i < old_num; i++) {
      vrend_resource_reference((struct vrend_resource **)&ctx->sub->vbo[i].buffer, NULL);
      ctx->sub->vbo_res_ids[i] = 0;
   }

}

void vrend_set_single_sampler_view(struct vrend_context *ctx,
                                   uint32_t shader_type,
                                   int index,
                                   uint32_t handle)
{
   struct vrend_sampler_view *view = NULL;
   struct vrend_texture *tex;

   if (handle) {
      view = vrend_object_lookup(ctx->sub->object_hash, handle, VIRGL_OBJECT_SAMPLER_VIEW);
      if (!view) {
         ctx->sub->views[shader_type].views[index] = NULL;
         report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_HANDLE, handle);
         return;
      }
      if (ctx->sub->views[shader_type].views[index] == view) {
         return;
      }
      /* we should have a reference to this texture taken at create time */
      tex = (struct vrend_texture *)view->texture;
      if (!tex) {
         return;
      }
      if (view->texture->target != GL_TEXTURE_BUFFER) {
         glBindTexture(view->texture->target, view->texture->id);

         if (util_format_is_depth_or_stencil(view->format)) {
            if (vrend_state.use_core_profile == false) {
               /* setting depth texture mode is deprecated in core profile */
               if (view->depth_texture_mode != GL_RED) {
                  glTexParameteri(view->texture->target, GL_DEPTH_TEXTURE_MODE, GL_RED);
                  view->depth_texture_mode = GL_RED;
               }
            }
            if (vrend_state.have_stencil_texturing) {
               const struct util_format_description *desc = util_format_description(view->format);
               if (!util_format_has_depth(desc)) {
                  glTexParameteri(view->texture->target, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
               } else {
                  glTexParameteri(view->texture->target, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);
               }
            }
         }

         if (view->cur_base != (view->val1 & 0xff)) {
            view->cur_base = view->val1 & 0xff;
            glTexParameteri(view->texture->target, GL_TEXTURE_BASE_LEVEL, view->cur_base);
         }
         if (view->cur_max != ((view->val1 >> 8) & 0xff)) {
            view->cur_max = (view->val1 >> 8) & 0xff;
            glTexParameteri(view->texture->target, GL_TEXTURE_MAX_LEVEL, view->cur_max);
         }
         if (tex->cur_swizzle_r != view->gl_swizzle_r) {
            glTexParameteri(view->texture->target, GL_TEXTURE_SWIZZLE_R, view->gl_swizzle_r);
            tex->cur_swizzle_r = view->gl_swizzle_r;
         }
         if (tex->cur_swizzle_g != view->gl_swizzle_g) {
            glTexParameteri(view->texture->target, GL_TEXTURE_SWIZZLE_G, view->gl_swizzle_g);
            tex->cur_swizzle_g = view->gl_swizzle_g;
         }
         if (tex->cur_swizzle_b != view->gl_swizzle_b) {
            glTexParameteri(view->texture->target, GL_TEXTURE_SWIZZLE_B, view->gl_swizzle_b);
            tex->cur_swizzle_b = view->gl_swizzle_b;
         }
         if (tex->cur_swizzle_a != view->gl_swizzle_a) {
            glTexParameteri(view->texture->target, GL_TEXTURE_SWIZZLE_A, view->gl_swizzle_a);
            tex->cur_swizzle_a = view->gl_swizzle_a;
         }
         if (tex->srgb_decode != view->srgb_decode && util_format_is_srgb(tex->base.base.format)) {
            if (vrend_state.have_samplers)
               ctx->sub->sampler_state_dirty = true;
            else {
               glTexParameteri(view->texture->target, GL_TEXTURE_SRGB_DECODE_EXT,
                               view->srgb_decode);
               tex->srgb_decode = view->srgb_decode;
            }
         }
      } else {
         GLenum internalformat;

         glBindTexture(GL_TEXTURE_BUFFER, view->texture->tbo_tex_id);
         internalformat = tex_conv_table[view->format].internalformat;
         glTexBuffer(GL_TEXTURE_BUFFER, internalformat, view->texture->id);
      }
   }

   vrend_sampler_view_reference(&ctx->sub->views[shader_type].views[index], view);
}

void vrend_set_num_sampler_views(struct vrend_context *ctx,
                                 uint32_t shader_type,
                                 uint32_t start_slot,
                                 int num_sampler_views)
{
   if (start_slot + num_sampler_views < ctx->sub->views[shader_type].num_views) {
      int i;
      for (i = start_slot + num_sampler_views; i < ctx->sub->views[shader_type].num_views; i++)
         vrend_sampler_view_reference(&ctx->sub->views[shader_type].views[i], NULL);
   }
   ctx->sub->views[shader_type].num_views = start_slot + num_sampler_views;
}

static void vrend_destroy_shader_object(void *obj_ptr)
{
   struct vrend_shader_selector *state = obj_ptr;

   vrend_shader_state_reference(&state, NULL);
}

static inline void vrend_fill_shader_key(struct vrend_context *ctx,
                                         unsigned type,
                                         struct vrend_shader_key *key)
{
   if (vrend_state.use_core_profile == true) {
      int i;
      bool add_alpha_test = true;
      key->cbufs_are_a8_bitmask = 0;
      for (i = 0; i < ctx->sub->nr_cbufs; i++) {
         if (!ctx->sub->surf[i])
            continue;
         if (vrend_format_is_emulated_alpha(ctx->sub->surf[i]->format))
            key->cbufs_are_a8_bitmask |= (1 << i);
         if (util_format_is_pure_integer(ctx->sub->surf[i]->format))
            add_alpha_test = false;
      }
      if (add_alpha_test) {
         key->add_alpha_test = ctx->sub->dsa_state.alpha.enabled;
         key->alpha_test = ctx->sub->dsa_state.alpha.func;
         key->alpha_ref_val = ctx->sub->dsa_state.alpha.ref_value;
      }

      key->pstipple_tex = ctx->sub->rs_state.poly_stipple_enable;
      key->color_two_side = ctx->sub->rs_state.light_twoside;

      key->clip_plane_enable = ctx->sub->rs_state.clip_plane_enable;
      key->flatshade = ctx->sub->rs_state.flatshade ? true : false;
   } else {
      key->add_alpha_test = 0;
      key->pstipple_tex = 0;
   }
   key->invert_fs_origin = !ctx->sub->inverted_fbo_content;
   key->coord_replace = ctx->sub->rs_state.point_quad_rasterization ? ctx->sub->rs_state.sprite_coord_enable : 0;

   if (ctx->sub->shaders[PIPE_SHADER_GEOMETRY])
      key->gs_present = true;

   if (type == PIPE_SHADER_GEOMETRY && ctx->sub->shaders[PIPE_SHADER_VERTEX]) {
      key->vs_has_pervertex = ctx->sub->shaders[PIPE_SHADER_VERTEX]->sinfo.num_pervertex_clip > 0;
      key->vs_pervertex_num_clip = ctx->sub->shaders[PIPE_SHADER_VERTEX]->sinfo.num_pervertex_clip;
   }
}

static inline int conv_shader_type(int type)
{
   switch (type) {
   case PIPE_SHADER_VERTEX: return GL_VERTEX_SHADER;
   case PIPE_SHADER_FRAGMENT: return GL_FRAGMENT_SHADER;
   case PIPE_SHADER_GEOMETRY: return GL_GEOMETRY_SHADER;
   default:
      return 0;
   };
}

static int vrend_shader_create(struct vrend_context *ctx,
                               struct vrend_shader *shader,
                               struct vrend_shader_key key)
{

   if (!shader->sel->tokens) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_SHADER, 0);
      return -1;
   }

   shader->id = glCreateShader(conv_shader_type(shader->sel->type));
   shader->compiled_fs_id = 0;
   shader->glsl_prog = vrend_convert_shader(&ctx->shader_cfg, shader->sel->tokens, &key, &shader->sel->sinfo);
   if (!shader->glsl_prog) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_SHADER, 0);
      glDeleteShader(shader->id);
      return -1;
   }
   shader->key = key;
   if (1) {//shader->sel->type == PIPE_SHADER_FRAGMENT || shader->sel->type == PIPE_SHADER_GEOMETRY) {
      bool ret;

      ret = vrend_compile_shader(ctx, shader);
      if (ret == false) {
         glDeleteShader(shader->id);
         free(shader->glsl_prog);
         return -1;
      }
   }
   return 0;
}

static int vrend_shader_select(struct vrend_context *ctx,
                               struct vrend_shader_selector *sel,
                               bool *dirty)
{
   struct vrend_shader_key key;
   struct vrend_shader *shader = NULL;
   int r;

   memset(&key, 0, sizeof(key));
   vrend_fill_shader_key(ctx, sel->type, &key);

   if (sel->current && !memcmp(&sel->current->key, &key, sizeof(key)))
      return 0;

   if (sel->num_shaders > 1) {
      struct vrend_shader *p = sel->current, *c = p->next_variant;
      while (c && memcmp(&c->key, &key, sizeof(key)) != 0) {
         p = c;
         c = c->next_variant;
      }
      if (c) {
         p->next_variant = c->next_variant;
         shader = c;
      }
   }

   if (!shader) {
      shader = CALLOC_STRUCT(vrend_shader);
      shader->sel = sel;
      list_inithead(&shader->programs);

      r = vrend_shader_create(ctx, shader, key);
      if (r) {
         sel->current = NULL;
         FREE(shader);
         return r;
      }
      sel->num_shaders++;
   }
   if (dirty)
      *dirty = true;

   shader->next_variant = sel->current;
   sel->current = shader;
   return 0;
}

static void *vrend_create_shader_state(struct vrend_context *ctx,
                                       const struct pipe_stream_output_info *so_info,
                                       unsigned pipe_shader_type)
{
   struct vrend_shader_selector *sel = CALLOC_STRUCT(vrend_shader_selector);

   if (!sel)
      return NULL;

   sel->type = pipe_shader_type;
   sel->sinfo.so_info = *so_info;
   pipe_reference_init(&sel->reference, 1);

   return sel;
}

static int vrend_finish_shader(struct vrend_context *ctx,
                               struct vrend_shader_selector *sel,
                               const struct tgsi_token *tokens)
{
   int r;

   sel->tokens = tgsi_dup_tokens(tokens);

   r = vrend_shader_select(ctx, sel, NULL);
   if (r) {
      return EINVAL;
   }
   return 0;
}

int vrend_create_shader(struct vrend_context *ctx,
                        uint32_t handle,
                        const struct pipe_stream_output_info *so_info,
                        const char *shd_text, uint32_t offlen, uint32_t num_tokens,
                        uint32_t type, uint32_t pkt_length)
{
   struct vrend_shader_selector *sel = NULL;
   int ret_handle;
   bool new_shader = true, long_shader = false;
   bool finished = false;
   int ret;

   if (type > PIPE_SHADER_GEOMETRY)
      return EINVAL;

   if (offlen & VIRGL_OBJ_SHADER_OFFSET_CONT)
      new_shader = false;
   else if (((offlen + 3) / 4) > pkt_length)
      long_shader = true;

   /* if we have an in progress one - don't allow a new shader
      of that type or a different handle. */
   if (ctx->sub->long_shader_in_progress_handle[type]) {
      if (new_shader == true)
         return EINVAL;
      if (handle != ctx->sub->long_shader_in_progress_handle[type])
         return EINVAL;
   }

   if (new_shader) {
     sel = vrend_create_shader_state(ctx, so_info, type);
     if (sel == NULL)
       return ENOMEM;

     if (long_shader) {
        sel->buf_len = ((offlen + 3) / 4) * 4; /* round up buffer size */
        sel->tmp_buf = malloc(sel->buf_len);
        if (!sel->tmp_buf) {
           ret = ENOMEM;
           goto error;
        }
        memcpy(sel->tmp_buf, shd_text, pkt_length * 4);
        sel->buf_offset = pkt_length * 4;
        ctx->sub->long_shader_in_progress_handle[type] = handle;
     } else
        finished = true;
   } else {
      sel = vrend_object_lookup(ctx->sub->object_hash, handle, VIRGL_OBJECT_SHADER);
      if (!sel) {
         fprintf(stderr, "got continuation without original shader %d\n", handle);
         ret = EINVAL;
         goto error;
      }

      offlen &= ~VIRGL_OBJ_SHADER_OFFSET_CONT;
      if (offlen != sel->buf_offset) {
         fprintf(stderr, "Got mismatched shader continuation %d vs %d\n",
                 offlen, sel->buf_offset);
         ret = EINVAL;
         goto error;
      }

      /*make sure no overflow */
      if (pkt_length * 4 < pkt_length ||
          pkt_length * 4 + sel->buf_offset < pkt_length * 4 ||
          pkt_length * 4 + sel->buf_offset < sel->buf_offset) {
            ret = EINVAL;
            goto error;
          }

      if ((pkt_length * 4 + sel->buf_offset) > sel->buf_len) {
         fprintf(stderr, "Got too large shader continuation %d vs %d\n",
                 pkt_length * 4 + sel->buf_offset, sel->buf_len);
         ret = EINVAL;
         goto error;
      }

      memcpy(sel->tmp_buf + sel->buf_offset, shd_text, pkt_length * 4);

      sel->buf_offset += pkt_length * 4;
      if (sel->buf_offset >= sel->buf_len) {
         finished = true;
         shd_text = sel->tmp_buf;
      }
   }

   if (finished) {
      struct tgsi_token *tokens;

      tokens = calloc(num_tokens + 10, sizeof(struct tgsi_token));
      if (!tokens) {
         ret = ENOMEM;
         goto error;
      }

      if (vrend_dump_shaders)
         fprintf(stderr,"shader\n%s\n", shd_text);
      if (!tgsi_text_translate((const char *)shd_text, tokens, num_tokens + 10)) {
         free(tokens);
         ret = EINVAL;
         goto error;
      }

      if (vrend_finish_shader(ctx, sel, tokens)) {
         free(tokens);
         ret = EINVAL;
         goto error;
      } else {
         free(sel->tmp_buf);
         sel->tmp_buf = NULL;
      }
      free(tokens);
      ctx->sub->long_shader_in_progress_handle[type] = 0;
   }

   if (new_shader) {
      ret_handle = vrend_renderer_object_insert(ctx, sel, sizeof(*sel), handle, VIRGL_OBJECT_SHADER);
      if (ret_handle == 0) {
         ret = ENOMEM;
         goto error;
      }
   }

   return 0;

error:
   if (new_shader)
      vrend_destroy_shader_selector(sel);
   else
      vrend_renderer_object_destroy(ctx, handle);

   return ret;
}

void vrend_bind_shader(struct vrend_context *ctx,
                       uint32_t handle, uint32_t type)
{
   struct vrend_shader_selector *sel;

   if (type > PIPE_SHADER_GEOMETRY)
      return;

   if (handle == 0) {
      ctx->sub->shader_dirty = true;
      vrend_shader_state_reference(&ctx->sub->shaders[type], NULL);
      return;
   }

   sel = vrend_object_lookup(ctx->sub->object_hash, handle, VIRGL_OBJECT_SHADER);
   if (!sel)
      return;

   if (sel->type != type)
      return;

   if (ctx->sub->shaders[sel->type] != sel) {
      ctx->sub->shader_dirty = true;
      ctx->sub->prog_ids[sel->type] = 0;
   }

   vrend_shader_state_reference(&ctx->sub->shaders[sel->type], sel);
}

void vrend_clear(struct vrend_context *ctx,
                 unsigned buffers,
                 const union pipe_color_union *color,
                 double depth, unsigned stencil)
{
   GLbitfield bits = 0;

   if (ctx->in_error)
      return;

   if (ctx->ctx_switch_pending)
      vrend_finish_context_switch(ctx);

   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ctx->sub->fb_id);

   vrend_update_frontface_state(ctx);
   if (ctx->sub->stencil_state_dirty)
      vrend_update_stencil_state(ctx);
   if (ctx->sub->scissor_state_dirty)
      vrend_update_scissor_state(ctx);
   if (ctx->sub->viewport_state_dirty)
      vrend_update_viewport_state(ctx);

   vrend_use_program(ctx, 0);

   if (buffers & PIPE_CLEAR_COLOR) {
      if (ctx->sub->nr_cbufs && ctx->sub->surf[0] && vrend_format_is_emulated_alpha(ctx->sub->surf[0]->format)) {
         glClearColor(color->f[3], 0.0, 0.0, 0.0);
      } else {
         glClearColor(color->f[0], color->f[1], color->f[2], color->f[3]);
      }

      /* This function implements Gallium's full clear callback (st->pipe->clear) on the host. This
         callback requires no color component be masked. We must unmask all components before
         calling glClear* and restore the previous colormask afterwards, as Gallium expects. */
      if (ctx->sub->hw_blend_state.independent_blend_enable) {
         /* ARB_draw_buffers_blend is required for this */
         int i;
         for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++)
            glColorMaskIndexedEXT(i, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      } else
         glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   }

   if (buffers & PIPE_CLEAR_DEPTH) {
      /* gallium clears don't respect depth mask */
      glDepthMask(GL_TRUE);
      if (vrend_state.use_gles) {
         if (0.0f < depth && depth > 1.0f) {
            // Only warn, it is clamped by the function.
            report_gles_warn(ctx, GLES_WARN_DEPTH_CLEAR, 0);
         }
         glClearDepthf(depth);
      } else {
         glClearDepth(depth);
      }
   }

   if (buffers & PIPE_CLEAR_STENCIL) {
      glStencilMask(~0u);
      glClearStencil(stencil);
   }

   if (buffers & PIPE_CLEAR_COLOR) {
      uint32_t mask = 0;
      int i;
      for (i = 0; i < ctx->sub->nr_cbufs; i++) {
         if (ctx->sub->surf[i])
            mask |= (1 << i);
      }
      if (mask != (buffers >> 2)) {
         mask = buffers >> 2;
         while (mask) {
            i = u_bit_scan(&mask);
            if (i < PIPE_MAX_COLOR_BUFS && ctx->sub->surf[i] && util_format_is_pure_uint(ctx->sub->surf[i] && ctx->sub->surf[i]->format))
               glClearBufferuiv(GL_COLOR,
                                i, (GLuint *)color);
            else if (i < PIPE_MAX_COLOR_BUFS && ctx->sub->surf[i] && util_format_is_pure_sint(ctx->sub->surf[i] && ctx->sub->surf[i]->format))
               glClearBufferiv(GL_COLOR,
                                i, (GLint *)color);
            else
               glClearBufferfv(GL_COLOR,
                                i, (GLfloat *)color);
         }
      }
      else
         bits |= GL_COLOR_BUFFER_BIT;
   }
   if (buffers & PIPE_CLEAR_DEPTH)
      bits |= GL_DEPTH_BUFFER_BIT;
   if (buffers & PIPE_CLEAR_STENCIL)
      bits |= GL_STENCIL_BUFFER_BIT;

   if (bits)
      glClear(bits);

   if (buffers & PIPE_CLEAR_DEPTH)
      if (!ctx->sub->dsa_state.depth.writemask)
         glDepthMask(GL_FALSE);

   /* Restore previous stencil buffer write masks for both front and back faces */
   if (buffers & PIPE_CLEAR_STENCIL) {
      glStencilMaskSeparate(GL_FRONT, ctx->sub->dsa_state.stencil[0].writemask);
      glStencilMaskSeparate(GL_BACK, ctx->sub->dsa_state.stencil[1].writemask);
   }

   /* Restore previous colormask */
   if (buffers & PIPE_CLEAR_COLOR) {
      if (ctx->sub->hw_blend_state.independent_blend_enable) {
         /* ARB_draw_buffers_blend is required for this */
         int i;
         for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
            struct pipe_blend_state *blend = &ctx->sub->hw_blend_state;
            glColorMaskIndexedEXT(i, blend->rt[i].colormask & PIPE_MASK_R ? GL_TRUE : GL_FALSE,
                                  blend->rt[i].colormask & PIPE_MASK_G ? GL_TRUE : GL_FALSE,
                                  blend->rt[i].colormask & PIPE_MASK_B ? GL_TRUE : GL_FALSE,
                                  blend->rt[i].colormask & PIPE_MASK_A ? GL_TRUE : GL_FALSE);
         }
      } else {
         glColorMask(ctx->sub->hw_blend_state.rt[0].colormask & PIPE_MASK_R ? GL_TRUE : GL_FALSE,
                     ctx->sub->hw_blend_state.rt[0].colormask & PIPE_MASK_G ? GL_TRUE : GL_FALSE,
                     ctx->sub->hw_blend_state.rt[0].colormask & PIPE_MASK_B ? GL_TRUE : GL_FALSE,
                     ctx->sub->hw_blend_state.rt[0].colormask & PIPE_MASK_A ? GL_TRUE : GL_FALSE);
      }
   }
}

static void vrend_update_scissor_state(struct vrend_context *ctx)
{
   struct pipe_scissor_state *ss;
   struct pipe_rasterizer_state *state = &ctx->sub->rs_state;
   GLint y;
   GLuint idx;
   unsigned mask = ctx->sub->scissor_state_dirty;

   if (state->scissor)
      glEnable(GL_SCISSOR_TEST);
   else
      glDisable(GL_SCISSOR_TEST);

   while (mask) {
      idx = u_bit_scan(&mask);
      if (idx >= PIPE_MAX_VIEWPORTS) {
         vrend_report_buffer_error(ctx, 0);
         break;
      }
      ss = &ctx->sub->ss[idx];
      if (ctx->sub->viewport_is_negative)
         y = ss->miny;
      else
         y = ss->miny;

      if (idx > 0)
         glScissorIndexed(idx, ss->minx, y, ss->maxx - ss->minx, ss->maxy - ss->miny);
      else
         glScissor(ss->minx, y, ss->maxx - ss->minx, ss->maxy - ss->miny);
   }
   ctx->sub->scissor_state_dirty = 0;
}

static void vrend_update_viewport_state(struct vrend_context *ctx)
{
   GLint cy;
   unsigned mask = ctx->sub->viewport_state_dirty;
   int idx;
   while (mask) {
      idx = u_bit_scan(&mask);

      if (ctx->sub->viewport_is_negative)
         cy = ctx->sub->vps[idx].cur_y - ctx->sub->vps[idx].height;
      else
         cy = ctx->sub->vps[idx].cur_y;
      if (idx > 0)
         glViewportIndexedf(idx, ctx->sub->vps[idx].cur_x, cy, ctx->sub->vps[idx].width, ctx->sub->vps[idx].height);
      else
         glViewport(ctx->sub->vps[idx].cur_x, cy, ctx->sub->vps[idx].width, ctx->sub->vps[idx].height);
   }

   ctx->sub->viewport_state_dirty = 0;
}

static GLenum get_gs_xfb_mode(GLenum mode)
{
   switch (mode) {
   case GL_POINTS:
      return GL_POINTS;
   case GL_LINE_STRIP:
      return GL_LINES;
   case GL_TRIANGLE_STRIP:
      return GL_TRIANGLES;
   default:
      fprintf(stderr, "illegal gs transform feedback mode %d\n", mode);
      return GL_POINTS;
   }
}

static GLenum get_xfb_mode(GLenum mode)
{
   switch (mode) {
   case GL_POINTS:
      return GL_POINTS;
   case GL_TRIANGLES:
   case GL_TRIANGLE_STRIP:
   case GL_TRIANGLE_FAN:
   case GL_QUADS:
   case GL_QUAD_STRIP:
   case GL_POLYGON:
      return GL_TRIANGLES;
   case GL_LINES:
   case GL_LINE_LOOP:
   case GL_LINE_STRIP:
      return GL_LINES;
   default:
      fprintf(stderr, "failed to translate TFB %d\n", mode);
      return GL_POINTS;
   }
}

static void vrend_draw_bind_vertex_legacy(struct vrend_context *ctx,
                                          struct vrend_vertex_element_array *va)
{
   uint32_t num_enable;
   uint32_t enable_bitmask;
   uint32_t disable_bitmask;
   int i;

   num_enable = va->count;
   enable_bitmask = 0;
   disable_bitmask = ~((1ull << num_enable) - 1);
   for (i = 0; i < va->count; i++) {
      struct vrend_vertex_element *ve = &va->elements[i];
      int vbo_index = ve->base.vertex_buffer_index;
      struct vrend_resource *res;
      GLint loc;

      if (i >= ctx->sub->prog->ss[PIPE_SHADER_VERTEX]->sel->sinfo.num_inputs) {
         /* XYZZY: debug this? */
         num_enable = ctx->sub->prog->ss[PIPE_SHADER_VERTEX]->sel->sinfo.num_inputs;
         break;
      }
      res = (struct vrend_resource *)ctx->sub->vbo[vbo_index].buffer;

      if (!res) {
         fprintf(stderr,"cannot find vbo buf %d %d %d\n", i, va->count, ctx->sub->prog->ss[PIPE_SHADER_VERTEX]->sel->sinfo.num_inputs);
         continue;
      }

      if (vrend_state.use_explicit_locations || vrend_state.have_vertex_attrib_binding) {
         loc = i;
      } else {
         if (ctx->sub->prog->attrib_locs) {
            loc = ctx->sub->prog->attrib_locs[i];
         } else loc = -1;

         if (loc == -1) {
            fprintf(stderr,"%s: cannot find loc %d %d %d\n", ctx->debug_name, i, va->count, ctx->sub->prog->ss[PIPE_SHADER_VERTEX]->sel->sinfo.num_inputs);
            num_enable--;
            if (i == 0) {
               fprintf(stderr,"%s: shader probably didn't compile - skipping rendering\n", ctx->debug_name);
               return;
            }
            continue;
         }
      }

      if (ve->type == GL_FALSE) {
         fprintf(stderr,"failed to translate vertex type - skipping render\n");
         return;
      }

      glBindBuffer(GL_ARRAY_BUFFER, res->id);

      if (ctx->sub->vbo[vbo_index].stride == 0) {
         void *data;
         /* for 0 stride we are kinda screwed */
         data = glMapBufferRange(GL_ARRAY_BUFFER, ctx->sub->vbo[vbo_index].buffer_offset, ve->nr_chan * sizeof(GLfloat), GL_MAP_READ_BIT);

         switch (ve->nr_chan) {
         case 1:
            glVertexAttrib1fv(loc, data);
            break;
         case 2:
            glVertexAttrib2fv(loc, data);
            break;
         case 3:
            glVertexAttrib3fv(loc, data);
            break;
         case 4:
         default:
            glVertexAttrib4fv(loc, data);
            break;
         }
         glUnmapBuffer(GL_ARRAY_BUFFER);
         disable_bitmask |= (1 << loc);
      } else {
         enable_bitmask |= (1 << loc);
         if (util_format_is_pure_integer(ve->base.src_format)) {
            glVertexAttribIPointer(loc, ve->nr_chan, ve->type, ctx->sub->vbo[vbo_index].stride, (void *)(uintptr_t)(ve->base.src_offset + ctx->sub->vbo[vbo_index].buffer_offset));
         } else {
            glVertexAttribPointer(loc, ve->nr_chan, ve->type, ve->norm, ctx->sub->vbo[vbo_index].stride, (void *)(uintptr_t)(ve->base.src_offset + ctx->sub->vbo[vbo_index].buffer_offset));
         }
         glVertexAttribDivisorARB(loc, ve->base.instance_divisor);
      }
   }
   if (ctx->sub->enabled_attribs_bitmask != enable_bitmask) {
      uint32_t mask = ctx->sub->enabled_attribs_bitmask & disable_bitmask;

      while (mask) {
         i = u_bit_scan(&mask);
         glDisableVertexAttribArray(i);
      }
      ctx->sub->enabled_attribs_bitmask &= ~disable_bitmask;

      mask = ctx->sub->enabled_attribs_bitmask ^ enable_bitmask;
      while (mask) {
         i = u_bit_scan(&mask);
         glEnableVertexAttribArray(i);
      }

      ctx->sub->enabled_attribs_bitmask = enable_bitmask;
   }
}

static void vrend_draw_bind_vertex_binding(struct vrend_context *ctx,
                                           struct vrend_vertex_element_array *va)
{
   int i;

   glBindVertexArray(va->id);

   if (ctx->sub->vbo_dirty) {
      for (i = 0; i < ctx->sub->num_vbos; i++) {
         struct vrend_resource *res = (struct vrend_resource *)ctx->sub->vbo[i].buffer;
         if (!res)
            glBindVertexBuffer(i, 0, 0, 0);
         else
            glBindVertexBuffer(i,
                               res->id,
                               ctx->sub->vbo[i].buffer_offset,
                               ctx->sub->vbo[i].stride);
      }
      for (i = ctx->sub->num_vbos; i < ctx->sub->old_num_vbos; i++) {
         glBindVertexBuffer(i, 0, 0, 0);
      }
      ctx->sub->vbo_dirty = false;
   }
}

static void vrend_draw_bind_samplers(struct vrend_context *ctx)
{
   int sampler_id;
   int i;
   int shader_type;

   sampler_id = 0;
   for (shader_type = PIPE_SHADER_VERTEX; shader_type <= ctx->sub->last_shader_idx; shader_type++) {
      int index = 0;
      for (i = 0; i < ctx->sub->views[shader_type].num_views; i++) {
         struct vrend_resource *texture = NULL;

         if (ctx->sub->views[shader_type].views[i]) {
            texture = ctx->sub->views[shader_type].views[i]->texture;
         }

         if (!(ctx->sub->prog->samplers_used_mask[shader_type] & (1 << i)))
            continue;

         if (ctx->sub->prog->samp_locs[shader_type])
            glUniform1i(ctx->sub->prog->samp_locs[shader_type][index], sampler_id);

         if (ctx->sub->prog->shadow_samp_mask[shader_type] & (1 << i)) {
            struct vrend_sampler_view *tview = ctx->sub->views[shader_type].views[i];
            glUniform4f(ctx->sub->prog->shadow_samp_mask_locs[shader_type][index],
                        (tview->gl_swizzle_r == GL_ZERO || tview->gl_swizzle_r == GL_ONE) ? 0.0 : 1.0,
                        (tview->gl_swizzle_g == GL_ZERO || tview->gl_swizzle_g == GL_ONE) ? 0.0 : 1.0,
                        (tview->gl_swizzle_b == GL_ZERO || tview->gl_swizzle_b == GL_ONE) ? 0.0 : 1.0,
                        (tview->gl_swizzle_a == GL_ZERO || tview->gl_swizzle_a == GL_ONE) ? 0.0 : 1.0);
            glUniform4f(ctx->sub->prog->shadow_samp_add_locs[shader_type][index],
                        tview->gl_swizzle_r == GL_ONE ? 1.0 : 0.0,
                        tview->gl_swizzle_g == GL_ONE ? 1.0 : 0.0,
                        tview->gl_swizzle_b == GL_ONE ? 1.0 : 0.0,
                        tview->gl_swizzle_a == GL_ONE ? 1.0 : 0.0);
         }

         glActiveTexture(GL_TEXTURE0 + sampler_id);
         if (texture) {
            int id;

            if (texture->target == GL_TEXTURE_BUFFER)
               id = texture->tbo_tex_id;
            else
               id = texture->id;

            glBindTexture(texture->target, id);
            if (ctx->sub->views[shader_type].old_ids[i] != id || ctx->sub->sampler_state_dirty) {
               vrend_apply_sampler_state(ctx, texture, shader_type, i, sampler_id, ctx->sub->views[shader_type].views[i]->srgb_decode);
               ctx->sub->views[shader_type].old_ids[i] = id;
            }
            if (ctx->sub->rs_state.point_quad_rasterization) {
               if (vrend_state.use_core_profile == false) {
                  if (ctx->sub->rs_state.sprite_coord_enable & (1 << i))
                     glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
                  else
                     glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_FALSE);
               }
            }
            sampler_id++;
         }
         index++;
      }
   }

   if (vrend_state.use_core_profile && ctx->sub->prog->fs_stipple_loc != -1) {
      glActiveTexture(GL_TEXTURE0 + sampler_id);
      glBindTexture(GL_TEXTURE_2D, ctx->pstipple_tex_id);
      glUniform1i(ctx->sub->prog->fs_stipple_loc, sampler_id);
   }
   ctx->sub->sampler_state_dirty = false;
}

static void vrend_draw_bind_ubo(struct vrend_context *ctx)
{
   int i;
   int ubo_id;
   int shader_type;

   ubo_id = 0;
   for (shader_type = PIPE_SHADER_VERTEX; shader_type <= ctx->sub->last_shader_idx; shader_type++) {
      uint32_t mask;
      int shader_ubo_idx = 0;
      struct pipe_constant_buffer *cb;
      struct vrend_resource *res;
      if (!ctx->sub->const_bufs_used_mask[shader_type])
         continue;

      if (!ctx->sub->prog->ubo_locs[shader_type])
         continue;

      mask = ctx->sub->const_bufs_used_mask[shader_type];
      while (mask) {
         i = u_bit_scan(&mask);

         cb = &ctx->sub->cbs[shader_type][i];
         res = (struct vrend_resource *)cb->buffer;
         glBindBufferRange(GL_UNIFORM_BUFFER, ubo_id, res->id,
                           cb->buffer_offset, cb->buffer_size);
         glUniformBlockBinding(ctx->sub->prog->id, ctx->sub->prog->ubo_locs[shader_type][shader_ubo_idx], ubo_id);
         shader_ubo_idx++;
         ubo_id++;
      }
   }
}

void vrend_draw_vbo(struct vrend_context *ctx,
                    const struct pipe_draw_info *info,
                    uint32_t cso, uint32_t indirect_handle,
                    uint32_t indirect_draw_count_handle)
{
   int i;
   bool new_program = false;
   uint32_t shader_type;
   struct vrend_resource *indirect_res = NULL;

   if (ctx->in_error)
      return;

   if (indirect_handle) {
      indirect_res = vrend_renderer_ctx_res_lookup(ctx, indirect_handle);
      if (!indirect_res) {
         report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_RESOURCE, indirect_handle);
         return;
      }
   }

   /* this must be zero until we support the feature */
   if (indirect_draw_count_handle) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_RESOURCE, indirect_handle);
      return;
   }

   if (ctx->ctx_switch_pending)
      vrend_finish_context_switch(ctx);

   vrend_update_frontface_state(ctx);
   if (ctx->sub->stencil_state_dirty)
      vrend_update_stencil_state(ctx);
   if (ctx->sub->scissor_state_dirty)
      vrend_update_scissor_state(ctx);

   if (ctx->sub->viewport_state_dirty)
      vrend_update_viewport_state(ctx);

   vrend_patch_blend_state(ctx);

   if (ctx->sub->shader_dirty) {
      struct vrend_linked_shader_program *prog;
      bool fs_dirty, vs_dirty, gs_dirty;
      bool dual_src = util_blend_state_is_dual(&ctx->sub->blend_state, 0);
      bool same_prog;
      if (!ctx->sub->shaders[PIPE_SHADER_VERTEX] || !ctx->sub->shaders[PIPE_SHADER_FRAGMENT]) {
         fprintf(stderr,"dropping rendering due to missing shaders: %s\n", ctx->debug_name);
         return;
      }

      vrend_shader_select(ctx, ctx->sub->shaders[PIPE_SHADER_FRAGMENT], &fs_dirty);
      vrend_shader_select(ctx, ctx->sub->shaders[PIPE_SHADER_VERTEX], &vs_dirty);
      if (ctx->sub->shaders[PIPE_SHADER_GEOMETRY])
         vrend_shader_select(ctx, ctx->sub->shaders[PIPE_SHADER_GEOMETRY], &gs_dirty);

      if (!ctx->sub->shaders[PIPE_SHADER_VERTEX]->current || !ctx->sub->shaders[PIPE_SHADER_FRAGMENT]->current || (ctx->sub->shaders[PIPE_SHADER_GEOMETRY] && !ctx->sub->shaders[PIPE_SHADER_GEOMETRY]->current)) {
         fprintf(stderr, "failure to compile shader variants: %s\n", ctx->debug_name);
         return;
      }
      same_prog = true;
      if (ctx->sub->shaders[PIPE_SHADER_VERTEX]->current->id != ctx->sub->prog_ids[PIPE_SHADER_VERTEX])
         same_prog = false;
      if (ctx->sub->shaders[PIPE_SHADER_FRAGMENT]->current->id != ctx->sub->prog_ids[PIPE_SHADER_FRAGMENT])
         same_prog = false;
      if (ctx->sub->shaders[PIPE_SHADER_GEOMETRY] && ctx->sub->shaders[PIPE_SHADER_GEOMETRY]->current->id != ctx->sub->prog_ids[PIPE_SHADER_GEOMETRY])
         same_prog = false;

      if (!same_prog) {
         prog = lookup_shader_program(ctx, ctx->sub->shaders[PIPE_SHADER_VERTEX]->current->id, ctx->sub->shaders[PIPE_SHADER_FRAGMENT]->current->id, ctx->sub->shaders[PIPE_SHADER_GEOMETRY] ? ctx->sub->shaders[PIPE_SHADER_GEOMETRY]->current->id : 0, dual_src);
         if (!prog) {
            prog = add_shader_program(ctx,
                                      ctx->sub->shaders[PIPE_SHADER_VERTEX]->current,
                                      ctx->sub->shaders[PIPE_SHADER_FRAGMENT]->current, ctx->sub->shaders[PIPE_SHADER_GEOMETRY] ? ctx->sub->shaders[PIPE_SHADER_GEOMETRY]->current : NULL);
            if (!prog)
               return;
         }

         ctx->sub->last_shader_idx = ctx->sub->shaders[PIPE_SHADER_GEOMETRY] ? PIPE_SHADER_GEOMETRY : PIPE_SHADER_FRAGMENT;
      } else
         prog = ctx->sub->prog;
      if (ctx->sub->prog != prog) {
         new_program = true;
         ctx->sub->prog_ids[PIPE_SHADER_VERTEX] = ctx->sub->shaders[PIPE_SHADER_VERTEX]->current->id;
         ctx->sub->prog_ids[PIPE_SHADER_FRAGMENT] = ctx->sub->shaders[PIPE_SHADER_FRAGMENT]->current->id;
         if (ctx->sub->shaders[PIPE_SHADER_GEOMETRY])
            ctx->sub->prog_ids[PIPE_SHADER_GEOMETRY] = ctx->sub->shaders[PIPE_SHADER_GEOMETRY]->current->id;
         ctx->sub->prog = prog;
      }
   }
   if (!ctx->sub->prog) {
      fprintf(stderr,"dropping rendering due to missing shaders: %s\n", ctx->debug_name);
      return;
   }
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ctx->sub->fb_id);

   vrend_use_program(ctx, ctx->sub->prog->id);

   for (shader_type = PIPE_SHADER_VERTEX; shader_type <= ctx->sub->last_shader_idx; shader_type++) {
      if (ctx->sub->consts[shader_type].consts &&
          ctx->sub->prog->const_locs[shader_type] &&
          (ctx->sub->const_dirty[shader_type] || new_program)) {
         for (i = 0; i < ctx->sub->shaders[shader_type]->sinfo.num_consts; i++) {
            if (ctx->sub->prog->const_locs[shader_type][i] != -1)
               glUniform4uiv(ctx->sub->prog->const_locs[shader_type][i], 1, &ctx->sub->consts[shader_type].consts[i * 4]);
         }
         ctx->sub->const_dirty[shader_type] = false;
      }
   }

   vrend_draw_bind_samplers(ctx);

   vrend_draw_bind_ubo(ctx);

   if (!ctx->sub->ve) {
      fprintf(stderr,"illegal VE setup - skipping renderering\n");
      return;
   }
   glUniform1f(ctx->sub->prog->vs_ws_adjust_loc, ctx->sub->viewport_is_negative ? -1.0 : 1.0);

   if (ctx->sub->rs_state.clip_plane_enable) {
      for (i = 0 ; i < 8; i++) {
         glUniform4fv(ctx->sub->prog->clip_locs[i], 1, (const GLfloat *)&ctx->sub->ucp_state.ucp[i]);
      }
   }

   if (vrend_state.have_vertex_attrib_binding)
      vrend_draw_bind_vertex_binding(ctx, ctx->sub->ve);
   else
      vrend_draw_bind_vertex_legacy(ctx, ctx->sub->ve);

   for (i = 0 ; i < ctx->sub->prog->ss[PIPE_SHADER_VERTEX]->sel->sinfo.num_inputs; i++) {
      struct vrend_vertex_element_array *va = ctx->sub->ve;
      struct vrend_vertex_element *ve = &va->elements[i];
      int vbo_index = ve->base.vertex_buffer_index;
      if (!ctx->sub->vbo[vbo_index].buffer) {
         fprintf(stderr, "VBO missing vertex buffer\n");
         return;
      }
   }

   if (info->indexed) {
      struct vrend_resource *res = (struct vrend_resource *)ctx->sub->ib.buffer;
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, res->id);
   } else
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

   if (ctx->sub->current_so) {
      if (ctx->sub->current_so->xfb_state == XFB_STATE_STARTED_NEED_BEGIN) {
         if (ctx->sub->shaders[PIPE_SHADER_GEOMETRY])
            glBeginTransformFeedback(get_gs_xfb_mode(ctx->sub->shaders[PIPE_SHADER_GEOMETRY]->sinfo.gs_out_prim));
         else
            glBeginTransformFeedback(get_xfb_mode(info->mode));
         ctx->sub->current_so->xfb_state = XFB_STATE_STARTED;
      } else if (ctx->sub->current_so->xfb_state == XFB_STATE_PAUSED) {
         glResumeTransformFeedback();
         ctx->sub->current_so->xfb_state = XFB_STATE_STARTED;
      }
   }

   if (info->primitive_restart) {
      if (vrend_state.have_nv_prim_restart) {
         glEnableClientState(GL_PRIMITIVE_RESTART_NV);
         glPrimitiveRestartIndexNV(info->restart_index);
      } else {
         glEnable(GL_PRIMITIVE_RESTART);
         glPrimitiveRestartIndex(info->restart_index);
      }
   }

   if (indirect_res)
      glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_res->id);
   else
      glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

   /* set the vertex state up now on a delay */
   if (!info->indexed) {
      GLenum mode = info->mode;
      int count = cso ? cso : info->count;
      int start = cso ? 0 : info->start;

      if (indirect_handle)
         glDrawArraysIndirect(mode, (GLvoid const *)(uintptr_t)info->indirect.offset);
      else if (info->instance_count <= 1)
         glDrawArrays(mode, start, count);
      else if (info->start_instance)
         glDrawArraysInstancedBaseInstance(mode, start, count, info->instance_count, info->start_instance);
      else
         glDrawArraysInstancedARB(mode, start, count, info->instance_count);
   } else {
      GLenum elsz;
      GLenum mode = info->mode;
      switch (ctx->sub->ib.index_size) {
      case 1:
         elsz = GL_UNSIGNED_BYTE;
         break;
      case 2:
         elsz = GL_UNSIGNED_SHORT;
         break;
      case 4:
      default:
         elsz = GL_UNSIGNED_INT;
         break;
      }

      if (indirect_handle)
         glDrawElementsIndirect(mode, elsz, (GLvoid const *)(uintptr_t)info->indirect.offset);
      else if (info->index_bias) {
         if (info->instance_count > 1)
            glDrawElementsInstancedBaseVertex(mode, info->count, elsz, (void *)(uintptr_t)ctx->sub->ib.offset, info->instance_count, info->index_bias);
         else if (info->min_index != 0 || info->max_index != -1)
            glDrawRangeElementsBaseVertex(mode, info->min_index, info->max_index, info->count, elsz, (void *)(uintptr_t)ctx->sub->ib.offset, info->index_bias);
         else
            glDrawElementsBaseVertex(mode, info->count, elsz, (void *)(uintptr_t)ctx->sub->ib.offset, info->index_bias);
      } else if (info->instance_count > 1) {
         glDrawElementsInstancedARB(mode, info->count, elsz, (void *)(uintptr_t)ctx->sub->ib.offset, info->instance_count);
      } else if (info->min_index != 0 || info->max_index != -1)
         glDrawRangeElements(mode, info->min_index, info->max_index, info->count, elsz, (void *)(uintptr_t)ctx->sub->ib.offset);
      else
         glDrawElements(mode, info->count, elsz, (void *)(uintptr_t)ctx->sub->ib.offset);
   }

   if (info->primitive_restart) {
      if (vrend_state.have_nv_prim_restart)
         glDisableClientState(GL_PRIMITIVE_RESTART_NV);
      else if (vrend_state.have_gl_prim_restart)
         glDisable(GL_PRIMITIVE_RESTART);
   }

   if (ctx->sub->current_so && vrend_state.have_tf2) {
      if (ctx->sub->current_so->xfb_state == XFB_STATE_STARTED) {
         glPauseTransformFeedback();
         ctx->sub->current_so->xfb_state = XFB_STATE_PAUSED;
      }
   }
}

static GLenum translate_blend_func(uint32_t pipe_blend)
{
   switch(pipe_blend){
   case PIPE_BLEND_ADD: return GL_FUNC_ADD;
   case PIPE_BLEND_SUBTRACT: return GL_FUNC_SUBTRACT;
   case PIPE_BLEND_REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
   case PIPE_BLEND_MIN: return GL_MIN;
   case PIPE_BLEND_MAX: return GL_MAX;
   default:
      assert("invalid blend token()" == NULL);
      return 0;
   }
}

static GLenum translate_blend_factor(uint32_t pipe_factor)
{
   switch (pipe_factor) {
   case PIPE_BLENDFACTOR_ONE: return GL_ONE;
   case PIPE_BLENDFACTOR_SRC_COLOR: return GL_SRC_COLOR;
   case PIPE_BLENDFACTOR_SRC_ALPHA: return GL_SRC_ALPHA;

   case PIPE_BLENDFACTOR_DST_COLOR: return GL_DST_COLOR;
   case PIPE_BLENDFACTOR_DST_ALPHA: return GL_DST_ALPHA;

   case PIPE_BLENDFACTOR_CONST_COLOR: return GL_CONSTANT_COLOR;
   case PIPE_BLENDFACTOR_CONST_ALPHA: return GL_CONSTANT_ALPHA;

   case PIPE_BLENDFACTOR_SRC1_COLOR: return GL_SRC1_COLOR;
   case PIPE_BLENDFACTOR_SRC1_ALPHA: return GL_SRC1_ALPHA;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE: return GL_SRC_ALPHA_SATURATE;
   case PIPE_BLENDFACTOR_ZERO: return GL_ZERO;


   case PIPE_BLENDFACTOR_INV_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;

   case PIPE_BLENDFACTOR_INV_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
   case PIPE_BLENDFACTOR_INV_DST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;

   case PIPE_BLENDFACTOR_INV_CONST_COLOR: return GL_ONE_MINUS_CONSTANT_COLOR;
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA: return GL_ONE_MINUS_CONSTANT_ALPHA;

   case PIPE_BLENDFACTOR_INV_SRC1_COLOR: return GL_ONE_MINUS_SRC1_COLOR;
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA: return GL_ONE_MINUS_SRC1_ALPHA;

   default:
      assert("invalid blend token()" == NULL);
      return 0;
   }
}

static GLenum
translate_logicop(GLuint pipe_logicop)
{
   switch (pipe_logicop) {
#define CASE(x) case PIPE_LOGICOP_##x: return GL_##x
      CASE(CLEAR);
      CASE(NOR);
      CASE(AND_INVERTED);
      CASE(COPY_INVERTED);
      CASE(AND_REVERSE);
      CASE(INVERT);
      CASE(XOR);
      CASE(NAND);
      CASE(AND);
      CASE(EQUIV);
      CASE(NOOP);
      CASE(OR_INVERTED);
      CASE(COPY);
      CASE(OR_REVERSE);
      CASE(OR);
      CASE(SET);
   default:
      assert("invalid logicop token()" == NULL);
      return 0;
   }
#undef CASE
}

static GLenum
translate_stencil_op(GLuint op)
{
   switch (op) {
#define CASE(x) case PIPE_STENCIL_OP_##x: return GL_##x
      CASE(KEEP);
      CASE(ZERO);
      CASE(REPLACE);
      CASE(INCR);
      CASE(DECR);
      CASE(INCR_WRAP);
      CASE(DECR_WRAP);
      CASE(INVERT);
   default:
      assert("invalid stencilop token()" == NULL);
      return 0;
   }
#undef CASE
}

static inline bool is_dst_blend(int blend_factor)
{
   return (blend_factor == PIPE_BLENDFACTOR_DST_ALPHA ||
           blend_factor == PIPE_BLENDFACTOR_INV_DST_ALPHA);
}

static inline int conv_a8_blend(int blend_factor)
{
   if (blend_factor == PIPE_BLENDFACTOR_DST_ALPHA)
      return PIPE_BLENDFACTOR_DST_COLOR;
   if (blend_factor == PIPE_BLENDFACTOR_INV_DST_ALPHA)
      return PIPE_BLENDFACTOR_INV_DST_COLOR;
   return blend_factor;
}

static inline int conv_dst_blend(int blend_factor)
{
   if (blend_factor == PIPE_BLENDFACTOR_DST_ALPHA)
      return PIPE_BLENDFACTOR_ONE;
   if (blend_factor == PIPE_BLENDFACTOR_INV_DST_ALPHA)
      return PIPE_BLENDFACTOR_ZERO;
   return blend_factor;
}

static void vrend_hw_emit_blend(struct vrend_context *ctx, struct pipe_blend_state *state)
{
   if (state->logicop_enable != ctx->sub->hw_blend_state.logicop_enable) {
      ctx->sub->hw_blend_state.logicop_enable = state->logicop_enable;
      if (vrend_state.use_gles) {
         if (state->logicop_enable) {
            report_gles_warn(ctx, GLES_WARN_LOGIC_OP, 0);
         }
      } else if (state->logicop_enable) {
         glEnable(GL_COLOR_LOGIC_OP);
         glLogicOp(translate_logicop(state->logicop_func));
      } else {
         glDisable(GL_COLOR_LOGIC_OP);
      }
   }

   if (state->independent_blend_enable) {
      /* ARB_draw_buffers_blend is required for this */
      int i;

      for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
         if (state->rt[i].blend_enable) {
            glBlendFuncSeparateiARB(i, translate_blend_factor(state->rt[i].rgb_src_factor),
                                    translate_blend_factor(state->rt[i].rgb_dst_factor),
                                    translate_blend_factor(state->rt[i].alpha_src_factor),
                                    translate_blend_factor(state->rt[i].alpha_dst_factor));
            glBlendEquationSeparateiARB(i, translate_blend_func(state->rt[i].rgb_func),
                                        translate_blend_func(state->rt[i].alpha_func));
            glEnableIndexedEXT(GL_BLEND, i);
         } else
            glDisableIndexedEXT(GL_BLEND, i);

         if (state->rt[i].colormask != ctx->sub->hw_blend_state.rt[i].colormask) {
            ctx->sub->hw_blend_state.rt[i].colormask = state->rt[i].colormask;
            glColorMaskIndexedEXT(i, state->rt[i].colormask & PIPE_MASK_R ? GL_TRUE : GL_FALSE,
                                  state->rt[i].colormask & PIPE_MASK_G ? GL_TRUE : GL_FALSE,
                                  state->rt[i].colormask & PIPE_MASK_B ? GL_TRUE : GL_FALSE,
                                  state->rt[i].colormask & PIPE_MASK_A ? GL_TRUE : GL_FALSE);
         }
      }
   } else {
      if (state->rt[0].blend_enable) {
         glBlendFuncSeparate(translate_blend_factor(state->rt[0].rgb_src_factor),
                             translate_blend_factor(state->rt[0].rgb_dst_factor),
                             translate_blend_factor(state->rt[0].alpha_src_factor),
                             translate_blend_factor(state->rt[0].alpha_dst_factor));
         glBlendEquationSeparate(translate_blend_func(state->rt[0].rgb_func),
                                 translate_blend_func(state->rt[0].alpha_func));
         vrend_blend_enable(ctx, true);
      }
      else
         vrend_blend_enable(ctx, false);

      if (state->rt[0].colormask != ctx->sub->hw_blend_state.rt[0].colormask) {
         int i;
         for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++)
            ctx->sub->hw_blend_state.rt[i].colormask = state->rt[i].colormask;
         glColorMask(state->rt[0].colormask & PIPE_MASK_R ? GL_TRUE : GL_FALSE,
                     state->rt[0].colormask & PIPE_MASK_G ? GL_TRUE : GL_FALSE,
                     state->rt[0].colormask & PIPE_MASK_B ? GL_TRUE : GL_FALSE,
                     state->rt[0].colormask & PIPE_MASK_A ? GL_TRUE : GL_FALSE);
      }
   }

   if (vrend_state.have_multisample) {
      if (state->alpha_to_coverage)
         glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
      else
         glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

      if (state->alpha_to_one)
         glEnable(GL_SAMPLE_ALPHA_TO_ONE);
      else
         glDisable(GL_SAMPLE_ALPHA_TO_ONE);
   }

   if (state->dither)
      glEnable(GL_DITHER);
   else
      glDisable(GL_DITHER);
}

/* there are a few reasons we might need to patch the blend state.
   a) patching blend factors for dst with no alpha
   b) patching colormask/blendcolor/blendfactors for A8/A16 format
   emulation using GL_R8/GL_R16.
*/
static void vrend_patch_blend_state(struct vrend_context *ctx)
{
   struct pipe_blend_state new_state = ctx->sub->blend_state;
   struct pipe_blend_state *state = &ctx->sub->blend_state;
   bool dest_alpha_only = false, dest_has_no_alpha = false;
   struct pipe_blend_color blend_color = ctx->sub->blend_color;
   int i;

   if (ctx->sub->nr_cbufs == 0)
      return;

   for (i = 0; i < ctx->sub->nr_cbufs; i++) {
      if (!ctx->sub->surf[i])
         continue;

      if (vrend_format_is_emulated_alpha(ctx->sub->surf[i]->format)) {
         dest_alpha_only = true;
      }

      if (!util_format_has_alpha(ctx->sub->surf[i]->format)) {
         dest_has_no_alpha = true;
      }
   }

   if (dest_alpha_only) {
      for (i = 0; i < (state->independent_blend_enable ? PIPE_MAX_COLOR_BUFS : 1); i++) {
         if (state->rt[i].blend_enable) {
            new_state.rt[i].rgb_src_factor = conv_a8_blend(state->rt[i].alpha_src_factor);
            new_state.rt[i].rgb_dst_factor = conv_a8_blend(state->rt[i].alpha_dst_factor);
            new_state.rt[i].alpha_src_factor = PIPE_BLENDFACTOR_ZERO;
            new_state.rt[i].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
         }
         new_state.rt[i].colormask = 0;
         if (state->rt[i].colormask & PIPE_MASK_A)
            new_state.rt[i].colormask |= PIPE_MASK_R;
      }
      blend_color.color[0] = blend_color.color[3];
      blend_color.color[1] = 0.0f;
      blend_color.color[2] = 0.0f;
      blend_color.color[3] = 0.0f;
   } else if (dest_has_no_alpha) {
      for (i = 0; i < (state->independent_blend_enable ? PIPE_MAX_COLOR_BUFS : 1); i++) {
         if (!(is_dst_blend(state->rt[i].rgb_src_factor) ||
               is_dst_blend(state->rt[i].rgb_dst_factor) ||
               is_dst_blend(state->rt[i].alpha_src_factor) ||
               is_dst_blend(state->rt[i].alpha_dst_factor)))
            continue;
         new_state.rt[i].rgb_src_factor = conv_dst_blend(state->rt[i].rgb_src_factor);
         new_state.rt[i].rgb_dst_factor = conv_dst_blend(state->rt[i].rgb_dst_factor);
         new_state.rt[i].alpha_src_factor = conv_dst_blend(state->rt[i].alpha_src_factor);
         new_state.rt[i].alpha_dst_factor = conv_dst_blend(state->rt[i].alpha_dst_factor);
      }
   }

   vrend_hw_emit_blend(ctx, &new_state);
   glBlendColor(blend_color.color[0],
                blend_color.color[1],
                blend_color.color[2],
                blend_color.color[3]);
}

void vrend_object_bind_blend(struct vrend_context *ctx,
                             uint32_t handle)
{
   struct pipe_blend_state *state;

   if (handle == 0) {
      memset(&ctx->sub->blend_state, 0, sizeof(ctx->sub->blend_state));
      vrend_blend_enable(ctx, false);
      return;
   }
   state = vrend_object_lookup(ctx->sub->object_hash, handle, VIRGL_OBJECT_BLEND);
   if (!state) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_HANDLE, handle);
      return;
   }

   ctx->sub->blend_state = *state;

   vrend_hw_emit_blend(ctx, &ctx->sub->blend_state);
}

static void vrend_hw_emit_dsa(struct vrend_context *ctx)
{
   struct pipe_depth_stencil_alpha_state *state = &ctx->sub->dsa_state;

   if (state->depth.enabled) {
      vrend_depth_test_enable(ctx, true);
      glDepthFunc(GL_NEVER + state->depth.func);
      if (state->depth.writemask)
         glDepthMask(GL_TRUE);
      else
         glDepthMask(GL_FALSE);
   } else
      vrend_depth_test_enable(ctx, false);

   if (state->alpha.enabled) {
      vrend_alpha_test_enable(ctx, true);
      if (!vrend_state.use_core_profile)
         glAlphaFunc(GL_NEVER + state->alpha.func, state->alpha.ref_value);
   } else
      vrend_alpha_test_enable(ctx, false);


}
void vrend_object_bind_dsa(struct vrend_context *ctx,
                           uint32_t handle)
{
   struct pipe_depth_stencil_alpha_state *state;

   if (handle == 0) {
      memset(&ctx->sub->dsa_state, 0, sizeof(ctx->sub->dsa_state));
      ctx->sub->dsa = NULL;
      ctx->sub->stencil_state_dirty = true;
      ctx->sub->shader_dirty = true;
      vrend_hw_emit_dsa(ctx);
      return;
   }

   state = vrend_object_lookup(ctx->sub->object_hash, handle, VIRGL_OBJECT_DSA);
   if (!state) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_HANDLE, handle);
      return;
   }

   if (ctx->sub->dsa != state) {
      ctx->sub->stencil_state_dirty = true;
      ctx->sub->shader_dirty = true;
   }
   ctx->sub->dsa_state = *state;
   ctx->sub->dsa = state;

   vrend_hw_emit_dsa(ctx);
}

static void vrend_update_frontface_state(struct vrend_context *ctx)
{
   struct pipe_rasterizer_state *state = &ctx->sub->rs_state;
   int front_ccw = state->front_ccw;

   front_ccw ^= (ctx->sub->inverted_fbo_content ? 0 : 1);
   if (front_ccw)
      glFrontFace(GL_CCW);
   else
      glFrontFace(GL_CW);
}

void vrend_update_stencil_state(struct vrend_context *ctx)
{
   struct pipe_depth_stencil_alpha_state *state = ctx->sub->dsa;
   int i;
   if (!state)
      return;

   if (!state->stencil[1].enabled) {
      if (state->stencil[0].enabled) {
         vrend_stencil_test_enable(ctx, true);

         glStencilOp(translate_stencil_op(state->stencil[0].fail_op),
                     translate_stencil_op(state->stencil[0].zfail_op),
                     translate_stencil_op(state->stencil[0].zpass_op));

         glStencilFunc(GL_NEVER + state->stencil[0].func,
                       ctx->sub->stencil_refs[0],
                       state->stencil[0].valuemask);
         glStencilMask(state->stencil[0].writemask);
      } else
         vrend_stencil_test_enable(ctx, false);
   } else {
      vrend_stencil_test_enable(ctx, true);

      for (i = 0; i < 2; i++) {
         GLenum face = (i == 1) ? GL_BACK : GL_FRONT;
         glStencilOpSeparate(face,
                             translate_stencil_op(state->stencil[i].fail_op),
                             translate_stencil_op(state->stencil[i].zfail_op),
                             translate_stencil_op(state->stencil[i].zpass_op));

         glStencilFuncSeparate(face, GL_NEVER + state->stencil[i].func,
                               ctx->sub->stencil_refs[i],
                               state->stencil[i].valuemask);
         glStencilMaskSeparate(face, state->stencil[i].writemask);
      }
   }
   ctx->sub->stencil_state_dirty = false;
}

static inline GLenum translate_fill(uint32_t mode)
{
   switch (mode) {
   case PIPE_POLYGON_MODE_POINT:
      return GL_POINT;
   case PIPE_POLYGON_MODE_LINE:
      return GL_LINE;
   case PIPE_POLYGON_MODE_FILL:
      return GL_FILL;
   default:
      assert(0);
      return 0;
   }
}

static void vrend_hw_emit_rs(struct vrend_context *ctx)
{
   struct pipe_rasterizer_state *state = &ctx->sub->rs_state;
   int i;

   if (vrend_state.use_gles) {
      if (!state->depth_clip) {
         report_gles_warn(ctx, GLES_WARN_DEPTH_CLIP, 0);
      }
   } else if (state->depth_clip) {
      glDisable(GL_DEPTH_CLAMP);
   } else {
      glEnable(GL_DEPTH_CLAMP);
   }

   if (vrend_state.use_gles) {
      /* guest send invalid glPointSize parameter */
      if (!state->point_size_per_vertex &&
          state->point_size != 1.0f &&
          state->point_size != 0.0f) {
         report_gles_warn(ctx, GLES_WARN_POINT_SIZE, 0);
      }
   } else if (state->point_size_per_vertex) {
      glEnable(GL_PROGRAM_POINT_SIZE);
   } else {
      glDisable(GL_PROGRAM_POINT_SIZE);
      if (state->point_size) {
         glPointSize(state->point_size);
      }
   }

   /* line_width < 0 is invalid, the guest sometimes forgot to set it. */
   glLineWidth(state->line_width <= 0 ? 1.0f : state->line_width);

   if (state->rasterizer_discard != ctx->sub->hw_rs_state.rasterizer_discard) {
      ctx->sub->hw_rs_state.rasterizer_discard = state->rasterizer_discard;
      if (state->rasterizer_discard)
         glEnable(GL_RASTERIZER_DISCARD);
      else
         glDisable(GL_RASTERIZER_DISCARD);
   }

   if (vrend_state.use_gles == true) {
      if (translate_fill(state->fill_front) != GL_FILL) {
         report_gles_warn(ctx, GLES_WARN_POLYGON_MODE, 0);
      }
      if (translate_fill(state->fill_back) != GL_FILL) {
         report_gles_warn(ctx, GLES_WARN_POLYGON_MODE, 0);
      }
   } else if (vrend_state.use_core_profile == false) {
      glPolygonMode(GL_FRONT, translate_fill(state->fill_front));
      glPolygonMode(GL_BACK, translate_fill(state->fill_back));
   } else if (state->fill_front == state->fill_back) {
      glPolygonMode(GL_FRONT_AND_BACK, translate_fill(state->fill_front));
   } else
      report_core_warn(ctx, CORE_PROFILE_WARN_POLYGON_MODE, 0);

   if (state->offset_tri) {
      glEnable(GL_POLYGON_OFFSET_FILL);
   } else {
      glDisable(GL_POLYGON_OFFSET_FILL);
   }

   if (vrend_state.use_gles) {
      if (state->offset_line) {
         report_gles_warn(ctx, GLES_WARN_OFFSET_LINE, 0);
      }
   } else if (state->offset_line) {
      glEnable(GL_POLYGON_OFFSET_LINE);
   } else {
      glDisable(GL_POLYGON_OFFSET_LINE);
   }

   if (vrend_state.use_gles) {
      if (state->offset_point) {
         report_gles_warn(ctx, GLES_WARN_OFFSET_POINT, 0);
      }
   } else if (state->offset_point) {
      glEnable(GL_POLYGON_OFFSET_POINT);
   } else {
      glDisable(GL_POLYGON_OFFSET_POINT);
   }


   if (state->flatshade != ctx->sub->hw_rs_state.flatshade) {
      ctx->sub->hw_rs_state.flatshade = state->flatshade;
      if (vrend_state.use_core_profile == false) {
         if (state->flatshade) {
            glShadeModel(GL_FLAT);
         } else {
            glShadeModel(GL_SMOOTH);
         }
      }
   }

   if (state->flatshade_first != ctx->sub->hw_rs_state.flatshade_first) {
      ctx->sub->hw_rs_state.flatshade_first = state->flatshade_first;
      if (vrend_state.use_gles) {
         if (state->flatshade_first) {
            report_gles_warn(ctx, GLES_WARN_FLATSHADE_FIRST, 0);
         }
      } else if (state->flatshade_first) {
         glProvokingVertexEXT(GL_FIRST_VERTEX_CONVENTION_EXT);
      } else {
         glProvokingVertexEXT(GL_LAST_VERTEX_CONVENTION_EXT);
      }
   }
   glPolygonOffset(state->offset_scale, state->offset_units);

   if (vrend_state.use_core_profile == false) {
      if (state->poly_stipple_enable)
         glEnable(GL_POLYGON_STIPPLE);
      else
         glDisable(GL_POLYGON_STIPPLE);
   } else if (state->poly_stipple_enable) {
      if (!ctx->pstip_inited)
         vrend_init_pstipple_texture(ctx);
   }

   if (state->point_quad_rasterization) {
      if (vrend_state.use_core_profile == false &&
          vrend_state.use_gles == false) {
         glEnable(GL_POINT_SPRITE);
      }

      if (vrend_state.use_gles == false) {
         glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, state->sprite_coord_mode ? GL_UPPER_LEFT : GL_LOWER_LEFT);
      }
   } else {
      if (vrend_state.use_core_profile == false &&
          vrend_state.use_gles == false) {
         glDisable(GL_POINT_SPRITE);
      }
   }

   if (state->cull_face != PIPE_FACE_NONE) {
      switch (state->cull_face) {
      case PIPE_FACE_FRONT:
         glCullFace(GL_FRONT);
         break;
      case PIPE_FACE_BACK:
         glCullFace(GL_BACK);
         break;
      case PIPE_FACE_FRONT_AND_BACK:
         glCullFace(GL_FRONT_AND_BACK);
         break;
      default:
         fprintf(stderr, "unhandled cull-face: %x\n", state->cull_face);
      }
      glEnable(GL_CULL_FACE);
   } else
      glDisable(GL_CULL_FACE);

   /* two sided lighting handled in shader for core profile */
   if (vrend_state.use_core_profile == false) {
      if (state->light_twoside)
         glEnable(GL_VERTEX_PROGRAM_TWO_SIDE);
      else
         glDisable(GL_VERTEX_PROGRAM_TWO_SIDE);
   }

   if (state->clip_plane_enable != ctx->sub->hw_rs_state.clip_plane_enable) {
      ctx->sub->hw_rs_state.clip_plane_enable = state->clip_plane_enable;
      for (i = 0; i < 8; i++) {
         if (state->clip_plane_enable & (1 << i))
            glEnable(GL_CLIP_PLANE0 + i);
         else
            glDisable(GL_CLIP_PLANE0 + i);
      }
   }
   if (vrend_state.use_core_profile == false) {
      glLineStipple(state->line_stipple_factor, state->line_stipple_pattern);
      if (state->line_stipple_enable)
         glEnable(GL_LINE_STIPPLE);
      else
         glDisable(GL_LINE_STIPPLE);
   } else if (state->line_stipple_enable) {
      if (vrend_state.use_gles)
         report_core_warn(ctx, GLES_WARN_STIPPLE, 0);
      else
         report_core_warn(ctx, CORE_PROFILE_WARN_STIPPLE, 0);
   }


   if (vrend_state.use_gles) {
      if (state->line_smooth) {
         report_gles_warn(ctx, GLES_WARN_LINE_SMOOTH, 0);
      }
   } else if (state->line_smooth) {
      glEnable(GL_LINE_SMOOTH);
   } else {
      glDisable(GL_LINE_SMOOTH);
   }

   if (vrend_state.use_gles) {
      if (state->poly_smooth) {
         report_gles_warn(ctx, GLES_WARN_POLY_SMOOTH, 0);
      }
   } else if (state->poly_smooth) {
      glEnable(GL_POLYGON_SMOOTH);
   } else {
      glDisable(GL_POLYGON_SMOOTH);
   }

   if (vrend_state.use_core_profile == false) {
      if (state->clamp_vertex_color)
         glClampColor(GL_CLAMP_VERTEX_COLOR_ARB, GL_TRUE);
      else
         glClampColor(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);

      if (state->clamp_fragment_color)
         glClampColor(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_TRUE);
      else
         glClampColor(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
   } else {
      if (state->clamp_vertex_color || state->clamp_fragment_color)
         report_core_warn(ctx, CORE_PROFILE_WARN_CLAMP, 0);
   }

   if (vrend_state.have_multisample) {
      if (state->multisample) {
         glEnable(GL_MULTISAMPLE);
         glEnable(GL_SAMPLE_MASK);
      } else {
         glDisable(GL_MULTISAMPLE);
         glDisable(GL_SAMPLE_MASK);
      }
      if (vrend_state.have_sample_shading) {
         if (state->force_persample_interp)
            glEnable(GL_SAMPLE_SHADING);
         else
            glDisable(GL_SAMPLE_SHADING);
      }
   }
}

void vrend_object_bind_rasterizer(struct vrend_context *ctx,
                                  uint32_t handle)
{
   struct pipe_rasterizer_state *state;

   if (handle == 0) {
      memset(&ctx->sub->rs_state, 0, sizeof(ctx->sub->rs_state));
      return;
   }

   state = vrend_object_lookup(ctx->sub->object_hash, handle, VIRGL_OBJECT_RASTERIZER);

   if (!state) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_HANDLE, handle);
      return;
   }

   ctx->sub->rs_state = *state;
   ctx->sub->scissor_state_dirty = (1 << 0);
   ctx->sub->shader_dirty = true;
   vrend_hw_emit_rs(ctx);
}

void vrend_bind_sampler_states(struct vrend_context *ctx,
                               uint32_t shader_type,
                               uint32_t start_slot,
                               uint32_t num_states,
                               uint32_t *handles)
{
   int i;
   struct vrend_sampler_state *state;

   if (shader_type >= PIPE_SHADER_TYPES) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_CMD_BUFFER, shader_type);
      return;
   }

   if (num_states > PIPE_MAX_SAMPLERS ||
       start_slot > (PIPE_MAX_SAMPLERS - num_states)) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_CMD_BUFFER, num_states);
      return;
   }

   ctx->sub->num_sampler_states[shader_type] = num_states;

   for (i = 0; i < num_states; i++) {
      if (handles[i] == 0)
         state = NULL;
      else
         state = vrend_object_lookup(ctx->sub->object_hash, handles[i], VIRGL_OBJECT_SAMPLER_STATE);

      ctx->sub->sampler_state[shader_type][i + start_slot] = state;
   }
   ctx->sub->sampler_state_dirty = true;
}

static void vrend_apply_sampler_state(struct vrend_context *ctx,
                                      struct vrend_resource *res,
                                      uint32_t shader_type,
                                      int id,
                                      int sampler_id,
                                      uint32_t srgb_decode)
{
   struct vrend_texture *tex = (struct vrend_texture *)res;
   struct vrend_sampler_state *vstate = ctx->sub->sampler_state[shader_type][id];
   struct pipe_sampler_state *state = &vstate->base;
   bool set_all = false;
   GLenum target = tex->base.target;

   if (!state) {
      fprintf(stderr, "cannot find sampler state for %d %d\n", shader_type, id);
      return;
   }
   if (res->base.nr_samples > 1) {
      tex->state = *state;
      return;
   }

   if (target == GL_TEXTURE_BUFFER) {
      tex->state = *state;
      return;
   }

   if (vrend_state.have_samplers) {
      glBindSampler(sampler_id, vstate->id);
      glSamplerParameteri(vstate->id, GL_TEXTURE_SRGB_DECODE_EXT,
                          srgb_decode);
      return;
   }

   if (tex->state.max_lod == -1)
      set_all = true;

   if (tex->state.wrap_s != state->wrap_s || set_all)
      glTexParameteri(target, GL_TEXTURE_WRAP_S, convert_wrap(state->wrap_s));
   if (tex->state.wrap_t != state->wrap_t || set_all)
      glTexParameteri(target, GL_TEXTURE_WRAP_T, convert_wrap(state->wrap_t));
   if (tex->state.wrap_r != state->wrap_r || set_all)
      glTexParameteri(target, GL_TEXTURE_WRAP_R, convert_wrap(state->wrap_r));
   if (tex->state.min_img_filter != state->min_img_filter ||
       tex->state.min_mip_filter != state->min_mip_filter || set_all)
      glTexParameterf(target, GL_TEXTURE_MIN_FILTER, convert_min_filter(state->min_img_filter, state->min_mip_filter));
   if (tex->state.mag_img_filter != state->mag_img_filter || set_all)
      glTexParameterf(target, GL_TEXTURE_MAG_FILTER, convert_mag_filter(state->mag_img_filter));
   if (res->target != GL_TEXTURE_RECTANGLE) {
      if (tex->state.min_lod != state->min_lod || set_all)
         glTexParameterf(target, GL_TEXTURE_MIN_LOD, state->min_lod);
      if (tex->state.max_lod != state->max_lod || set_all)
         glTexParameterf(target, GL_TEXTURE_MAX_LOD, state->max_lod);
      if (tex->state.lod_bias != state->lod_bias || set_all) {
         if (vrend_state.use_gles) {
            if (state->lod_bias) {
               report_gles_warn(ctx, GLES_WARN_LOD_BIAS, 0);
            }
         } else {
            glTexParameterf(target, GL_TEXTURE_LOD_BIAS, state->lod_bias);
         }
      }
   }

   if (tex->state.compare_mode != state->compare_mode || set_all)
      glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, state->compare_mode ? GL_COMPARE_R_TO_TEXTURE : GL_NONE);
   if (tex->state.compare_func != state->compare_func || set_all)
      glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, GL_NEVER + state->compare_func);

   if (memcmp(&tex->state.border_color, &state->border_color, 16) || set_all)
      glTexParameterIuiv(target, GL_TEXTURE_BORDER_COLOR, state->border_color.ui);
   tex->state = *state;
}

static GLenum tgsitargettogltarget(const enum pipe_texture_target target, int nr_samples)
{
   switch(target) {
   case PIPE_TEXTURE_1D:
      return GL_TEXTURE_1D;
   case PIPE_TEXTURE_2D:
      return (nr_samples > 1) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
   case PIPE_TEXTURE_3D:
      return GL_TEXTURE_3D;
   case PIPE_TEXTURE_RECT:
      return GL_TEXTURE_RECTANGLE_NV;
   case PIPE_TEXTURE_CUBE:
      return GL_TEXTURE_CUBE_MAP;

   case PIPE_TEXTURE_1D_ARRAY:
      return GL_TEXTURE_1D_ARRAY;
   case PIPE_TEXTURE_2D_ARRAY:
      return (nr_samples > 1) ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_ARRAY;
   case PIPE_TEXTURE_CUBE_ARRAY:
      return GL_TEXTURE_CUBE_MAP_ARRAY;
   case PIPE_BUFFER:
   default:
      return PIPE_BUFFER;
   }
   return PIPE_BUFFER;
}

static void vrend_free_sync_thread(void)
{
   if (!vrend_state.sync_thread)
      return;

   pipe_mutex_lock(vrend_state.fence_mutex);
   vrend_state.stop_sync_thread = true;
   pipe_mutex_unlock(vrend_state.fence_mutex);

   pipe_condvar_signal(vrend_state.fence_cond);
   pipe_thread_wait(vrend_state.sync_thread);
   vrend_state.sync_thread = 0;

   pipe_condvar_destroy(vrend_state.fence_cond);
   pipe_mutex_destroy(vrend_state.fence_mutex);
}

#ifdef HAVE_EVENTFD
static ssize_t
write_full(int fd, const void *ptr, size_t count)
{
   const char *buf = ptr;
   ssize_t ret = 0;
   ssize_t total = 0;

   while (count) {
      ret = write(fd, buf, count);
      if (ret < 0) {
         if (errno == EINTR)
            continue;
         break;
      }
      count -= ret;
      buf += ret;
      total += ret;
   }
   return total;
}

static void wait_sync(struct vrend_fence *fence)
{
   GLenum glret;
   ssize_t n;
   uint64_t value = 1;

   do {
      glret = glClientWaitSync(fence->syncobj, 0, 1000000000);

      switch (glret) {
      case GL_WAIT_FAILED:
         fprintf(stderr, "wait sync failed: illegal fence object %p\n", fence->syncobj);
         break;
      case GL_ALREADY_SIGNALED:
      case GL_CONDITION_SATISFIED:
         break;
      default:
         break;
      }
   } while (glret == GL_TIMEOUT_EXPIRED);

   pipe_mutex_lock(vrend_state.fence_mutex);
   list_addtail(&fence->fences, &vrend_state.fence_list);
   pipe_mutex_unlock(vrend_state.fence_mutex);

   n = write_full(vrend_state.eventfd, &value, sizeof(value));
   if (n != sizeof(value)) {
      perror("failed to write to eventfd\n");
   }
}

static int thread_sync(void *arg)
{
   virgl_gl_context gl_context = vrend_state.sync_context;
   struct vrend_fence *fence, *stor;

   pipe_mutex_lock(vrend_state.fence_mutex);
   vrend_clicbs->make_current(0, gl_context);

   while (!vrend_state.stop_sync_thread) {
      if (LIST_IS_EMPTY(&vrend_state.fence_wait_list) &&
          pipe_condvar_wait(vrend_state.fence_cond, vrend_state.fence_mutex) != 0) {
         fprintf(stderr, "error while waiting on condition\n");
         break;
      }

      LIST_FOR_EACH_ENTRY_SAFE(fence, stor, &vrend_state.fence_wait_list, fences) {
         if (vrend_state.stop_sync_thread)
            break;
         list_del(&fence->fences);
         pipe_mutex_unlock(vrend_state.fence_mutex);
         wait_sync(fence);
         pipe_mutex_lock(vrend_state.fence_mutex);
      }
   }

   vrend_clicbs->make_current(0, 0);
   vrend_clicbs->destroy_gl_context(vrend_state.sync_context);
   pipe_mutex_unlock(vrend_state.fence_mutex);
   return 0;
}

static void vrend_renderer_use_threaded_sync(void)
{
   struct virgl_gl_ctx_param ctx_params;

   if (getenv("VIRGL_DISABLE_MT"))
      return;

   ctx_params.shared = true;
   ctx_params.major_ver = vrend_state.gl_major_ver;
   ctx_params.minor_ver = vrend_state.gl_minor_ver;

   vrend_state.stop_sync_thread = false;

   vrend_state.sync_context = vrend_clicbs->create_gl_context(0, &ctx_params);
   if (vrend_state.sync_context == NULL) {
      fprintf(stderr, "failed to create sync opengl context\n");
      return;
   }

   vrend_state.eventfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
   if (vrend_state.eventfd == -1) {
      fprintf(stderr, "Failed to create eventfd\n");
      vrend_clicbs->destroy_gl_context(vrend_state.sync_context);
      return;
   }

   pipe_condvar_init(vrend_state.fence_cond);
   pipe_mutex_init(vrend_state.fence_mutex);

   vrend_state.sync_thread = pipe_thread_create(thread_sync, NULL);
   if (!vrend_state.sync_thread) {
      close(vrend_state.eventfd);
      vrend_state.eventfd = -1;
      vrend_clicbs->destroy_gl_context(vrend_state.sync_context);
      pipe_condvar_destroy(vrend_state.fence_cond);
      pipe_mutex_destroy(vrend_state.fence_mutex);
   }
}
#else
static void vrend_renderer_use_threaded_sync(void)
{
}
#endif

static void vrend_debug_cb(GLenum source, GLenum type, GLuint id,
                           GLenum severity, GLsizei length,
                           const GLchar* message, const void* userParam)
{
   if (type != GL_DEBUG_TYPE_ERROR) {
      return;
   }

   fprintf(stderr, "ERROR: %s\n", message);
}

int vrend_renderer_init(struct vrend_if_cbs *cbs, uint32_t flags)
{
   bool gles;
   int gl_ver;
   virgl_gl_context gl_context;
   struct virgl_gl_ctx_param ctx_params;

   if (!vrend_state.inited) {
      vrend_state.inited = true;
      vrend_object_init_resource_table();
      vrend_clicbs = cbs;
   }

   ctx_params.shared = false;
   int i;
   for (i = 0; i < ARRAY_SIZE(gl_versions); i++) {
      ctx_params.major_ver = gl_versions[i].major;
      ctx_params.minor_ver = gl_versions[i].minor;

      gl_context = vrend_clicbs->create_gl_context(0, &ctx_params);
      if (gl_context)
         break;
   }

   vrend_clicbs->make_current(0, gl_context);
   gl_ver = epoxy_gl_version();

   gles = !epoxy_is_desktop_gl();

   vrend_state.gl_major_ver = gl_ver / 10;
   vrend_state.gl_minor_ver = gl_ver % 10;

   if (gles) {
      fprintf(stderr, "gl_version %d - es profile enabled\n", gl_ver);
      vrend_state.use_gles = true;
      /* for now, makes the rest of the code use the most GLES 3.x like path */
      vrend_state.use_core_profile = 1;
   } else if (gl_ver > 30 && !epoxy_has_gl_extension("GL_ARB_compatibility")) {
      fprintf(stderr, "gl_version %d - core profile enabled\n", gl_ver);
      vrend_state.use_core_profile = 1;
   } else {
      fprintf(stderr, "gl_version %d - compat profile\n", gl_ver);
   }

   if (epoxy_has_gl_extension("GL_ARB_robustness")) {
      vrend_state.have_arb_robustness = true;
   } else if (gles && epoxy_has_gl_extension("GL_KHR_robustness")) {
      vrend_state.have_gles_khr_robustness = true;
   } else {
      fprintf(stderr,"WARNING: running without ARB/KHR robustness in place may crash\n");
   }

   /* used for buffers that we want to texture from */
   if (epoxy_has_gl_extension("GL_ARB_texture_buffer_object")) {
      vrend_state.have_arb_or_gles_ext_texture_buffer = true;
   }
   if (gles && epoxy_has_gl_extension("GL_EXT_texture_buffer")) {
      vrend_state.have_arb_or_gles_ext_texture_buffer = true;
   }

   if (epoxy_has_gl_extension("GL_MESA_pack_invert"))
      vrend_state.have_mesa_invert = true;
   if (gl_ver >= 43 || epoxy_has_gl_extension("GL_ARB_vertex_attrib_binding"))
      vrend_state.have_vertex_attrib_binding = true;
   if (gl_ver >= 33 || epoxy_has_gl_extension("GL_ARB_sampler_objects"))
      vrend_state.have_samplers = true;
   if (gl_ver >= 33 || epoxy_has_gl_extension("GL_ARB_shader_bit_encoding"))
      vrend_state.have_bit_encoding = true;
   if (gl_ver >= 31)
      vrend_state.have_gl_prim_restart = true;
   else if (epoxy_has_gl_extension("GL_NV_primitive_restart"))
      vrend_state.have_nv_prim_restart = true;
   if (gl_ver >= 40 || epoxy_has_gl_extension("GL_ARB_transform_feedback2"))
      vrend_state.have_tf2 = true;

   if (epoxy_has_gl_extension("GL_ARB_stencil_texturing"))
      vrend_state.have_stencil_texturing = true;
   if (epoxy_has_gl_extension("GL_EXT_framebuffer_multisample") && epoxy_has_gl_extension("GL_ARB_texture_multisample")) {
      vrend_state.have_multisample = true;
      if (epoxy_has_gl_extension("GL_EXT_framebuffer_multisample_blit_scaled"))
         vrend_state.have_ms_scaled_blit = true;
   }

   if (gl_ver >= 40 || epoxy_has_gl_extension("GL_ARB_sample_shading"))
      vrend_state.have_sample_shading = true;

   /* callbacks for when we are cleaning up the object table */
   vrend_resource_set_destroy_callback(vrend_destroy_resource_object);
   vrend_object_set_destroy_callback(VIRGL_OBJECT_QUERY, vrend_destroy_query_object);
   vrend_object_set_destroy_callback(VIRGL_OBJECT_SURFACE, vrend_destroy_surface_object);
   vrend_object_set_destroy_callback(VIRGL_OBJECT_SHADER, vrend_destroy_shader_object);
   vrend_object_set_destroy_callback(VIRGL_OBJECT_SAMPLER_VIEW, vrend_destroy_sampler_view_object);
   vrend_object_set_destroy_callback(VIRGL_OBJECT_STREAMOUT_TARGET, vrend_destroy_so_target_object);
   vrend_object_set_destroy_callback(VIRGL_OBJECT_SAMPLER_STATE, vrend_destroy_sampler_state_object);
   vrend_object_set_destroy_callback(VIRGL_OBJECT_VERTEX_ELEMENTS, vrend_destroy_vertex_elements_object);

   /* disable for format testing, spews a lot of errors */
   if (vrend_state.have_debug_cb) {
      glDisable(GL_DEBUG_OUTPUT);
   }

   if (vrend_state.use_gles) {
      vrend_build_format_list_gles();
   } else {
      vrend_build_format_list();
   }

   /* disable for format testing */
   if (vrend_state.have_debug_cb) {
      glDisable(GL_DEBUG_OUTPUT);
   }
   fprintf(stderr, "REAL BEGIN NOW\n");

   vrend_clicbs->destroy_gl_context(gl_context);
   list_inithead(&vrend_state.fence_list);
   list_inithead(&vrend_state.fence_wait_list);
   list_inithead(&vrend_state.waiting_query_list);
   list_inithead(&vrend_state.active_ctx_list);
   /* create 0 context */
   vrend_renderer_context_create_internal(0, 0, NULL);

   vrend_state.eventfd = -1;
   if (flags & VREND_USE_THREAD_SYNC) {
      vrend_renderer_use_threaded_sync();
   }

   return 0;
}

void
vrend_renderer_fini(void)
{
   if (!vrend_state.inited)
      return;

   vrend_free_sync_thread();
   if (vrend_state.eventfd != -1) {
      close(vrend_state.eventfd);
      vrend_state.eventfd = -1;
   }

   vrend_decode_reset(false);
   vrend_object_fini_resource_table();
   vrend_decode_reset(true);

   vrend_state.current_ctx = NULL;
   vrend_state.current_hw_ctx = NULL;
   vrend_state.inited = false;
}

static void vrend_destroy_sub_context(struct vrend_sub_context *sub)
{
   int i, j;
   struct vrend_streamout_object *obj, *tmp;

   if (sub->fb_id)
      glDeleteFramebuffers(1, &sub->fb_id);

   if (sub->blit_fb_ids[0])
      glDeleteFramebuffers(2, sub->blit_fb_ids);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

   if (!vrend_state.have_vertex_attrib_binding) {
      while (sub->enabled_attribs_bitmask) {
         i = u_bit_scan(&sub->enabled_attribs_bitmask);

         glDisableVertexAttribArray(i);
      }
      glDeleteVertexArrays(1, &sub->vaoid);
   }

   glBindVertexArray(0);

   if (sub->current_so)
      glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

   LIST_FOR_EACH_ENTRY_SAFE(obj, tmp, &sub->streamout_list, head) {
      vrend_destroy_streamout_object(obj);
   }

   vrend_shader_state_reference(&sub->shaders[PIPE_SHADER_VERTEX], NULL);
   vrend_shader_state_reference(&sub->shaders[PIPE_SHADER_FRAGMENT], NULL);
   vrend_shader_state_reference(&sub->shaders[PIPE_SHADER_GEOMETRY], NULL);

   vrend_free_programs(sub);
   for (i = 0; i < PIPE_SHADER_TYPES; i++) {
      free(sub->consts[i].consts);
      sub->consts[i].consts = NULL;

      for (j = 0; j < PIPE_MAX_SHADER_SAMPLER_VIEWS; j++) {
         vrend_sampler_view_reference(&sub->views[i].views[j], NULL);
      }
   }

   if (sub->zsurf)
      vrend_surface_reference(&sub->zsurf, NULL);

   for (i = 0; i < sub->nr_cbufs; i++) {
      if (!sub->surf[i])
         continue;
      vrend_surface_reference(&sub->surf[i], NULL);
   }

   vrend_resource_reference((struct vrend_resource **)&sub->ib.buffer, NULL);

   vrend_object_fini_ctx_table(sub->object_hash);
   vrend_clicbs->destroy_gl_context(sub->gl_context);

   list_del(&sub->head);
   FREE(sub);

}

bool vrend_destroy_context(struct vrend_context *ctx)
{
   bool switch_0 = (ctx == vrend_state.current_ctx);
   struct vrend_context *cur = vrend_state.current_ctx;
   struct vrend_sub_context *sub, *tmp;
   if (switch_0) {
      vrend_state.current_ctx = NULL;
      vrend_state.current_hw_ctx = NULL;
   }

   if (vrend_state.use_core_profile) {
      if (ctx->pstip_inited)
         glDeleteTextures(1, &ctx->pstipple_tex_id);
      ctx->pstip_inited = false;
   }
   /* reset references on framebuffers */
   vrend_set_framebuffer_state(ctx, 0, NULL, 0);

   vrend_set_num_sampler_views(ctx, PIPE_SHADER_VERTEX, 0, 0);
   vrend_set_num_sampler_views(ctx, PIPE_SHADER_FRAGMENT, 0, 0);
   vrend_set_num_sampler_views(ctx, PIPE_SHADER_GEOMETRY, 0, 0);

   vrend_set_streamout_targets(ctx, 0, 0, NULL);
   vrend_set_num_vbo(ctx, 0);

   vrend_set_index_buffer(ctx, 0, 0, 0);

   vrend_renderer_force_ctx_0();
   LIST_FOR_EACH_ENTRY_SAFE(sub, tmp, &ctx->sub_ctxs, head)
      vrend_destroy_sub_context(sub);

   vrend_object_fini_ctx_table(ctx->res_hash);

   list_del(&ctx->ctx_entry);

   FREE(ctx);

   if (!switch_0 && cur)
      vrend_hw_switch_context(cur, true);

   return switch_0;
}

struct vrend_context *vrend_create_context(int id, uint32_t nlen, const char *debug_name)
{
   struct vrend_context *grctx = CALLOC_STRUCT(vrend_context);

   if (!grctx)
      return NULL;

   if (nlen && debug_name) {
      strncpy(grctx->debug_name, debug_name, 64);
   }

   grctx->ctx_id = id;

   list_inithead(&grctx->sub_ctxs);
   list_inithead(&grctx->active_nontimer_query_list);

   grctx->res_hash = vrend_object_init_ctx_table();

   grctx->shader_cfg.use_gles = vrend_state.use_gles;
   grctx->shader_cfg.use_core_profile = vrend_state.use_core_profile;
   grctx->shader_cfg.use_explicit_locations = vrend_state.use_explicit_locations;
   vrend_renderer_create_sub_ctx(grctx, 0);
   vrend_renderer_set_sub_ctx(grctx, 0);

   vrender_get_glsl_version(&grctx->shader_cfg.glsl_version);

   list_addtail(&grctx->ctx_entry, &vrend_state.active_ctx_list);
   return grctx;
}

int vrend_renderer_resource_attach_iov(int res_handle, struct iovec *iov,
                                       int num_iovs)
{
   struct vrend_resource *res;

   res = vrend_resource_lookup(res_handle, 0);
   if (!res)
      return EINVAL;

   if (res->iov)
      return 0;

   /* work out size and max resource size */
   res->iov = iov;
   res->num_iovs = num_iovs;
   return 0;
}

void vrend_renderer_resource_detach_iov(int res_handle,
                                        struct iovec **iov_p,
                                        int *num_iovs_p)
{
   struct vrend_resource *res;
   res = vrend_resource_lookup(res_handle, 0);
   if (!res) {
      return;
   }
   if (iov_p)
      *iov_p = res->iov;
   if (num_iovs_p)
      *num_iovs_p = res->num_iovs;

   res->iov = NULL;
   res->num_iovs = 0;
}

static int check_resource_valid(struct vrend_renderer_resource_create_args *args)
{
   /* do not accept handle 0 */
   if (args->handle == 0)
      return -1;

   /* limit the target */
   if (args->target >= PIPE_MAX_TEXTURE_TYPES)
      return -1;

   if (args->format >= VIRGL_FORMAT_MAX)
      return -1;

   /* only texture 2d and 2d array can have multiple samples */
   if (args->nr_samples > 1) {
      if (args->target != PIPE_TEXTURE_2D && args->target != PIPE_TEXTURE_2D_ARRAY)
         return -1;
      /* multisample can't have miplevels */
      if (args->last_level > 0)
         return -1;
   }

   if (args->last_level > 0) {
      /* buffer and rect textures can't have mipmaps */
      if (args->target == PIPE_BUFFER || args->target == PIPE_TEXTURE_RECT)
         return -1;
      if (args->last_level > (floor(log2(MAX2(args->width, args->height))) + 1))
         return -1;
   }
   if (args->flags != 0 && args->flags != VIRGL_RESOURCE_Y_0_TOP)
      return -1;

   if (args->flags & VIRGL_RESOURCE_Y_0_TOP)
      if (args->target != PIPE_TEXTURE_2D && args->target != PIPE_TEXTURE_RECT)
         return -1;

   /* array size for array textures only */
   if (args->target == PIPE_TEXTURE_CUBE) {
      if (args->array_size != 6)
         return -1;
   } else if (args->target == PIPE_TEXTURE_CUBE_ARRAY) {
      if (args->array_size % 6)
         return -1;
   } else if (args->array_size > 1) {
      if (args->target != PIPE_TEXTURE_2D_ARRAY &&
          args->target != PIPE_TEXTURE_1D_ARRAY)
         return -1;
   }

   if (args->bind == 0 ||
       args->bind == VREND_RES_BIND_CUSTOM ||
       args->bind == VREND_RES_BIND_INDEX_BUFFER ||
       args->bind == VREND_RES_BIND_STREAM_OUTPUT ||
       args->bind == VREND_RES_BIND_VERTEX_BUFFER ||
       args->bind == VREND_RES_BIND_CONSTANT_BUFFER) {
      if (args->target != PIPE_BUFFER)
         return -1;
      if (args->height != 1 || args->depth != 1)
         return -1;
   } else {
      if (!((args->bind & VREND_RES_BIND_SAMPLER_VIEW) ||
            (args->bind & VREND_RES_BIND_DEPTH_STENCIL) ||
            (args->bind & VREND_RES_BIND_RENDER_TARGET) ||
            (args->bind & VREND_RES_BIND_CURSOR)))
         return -1;

      if (args->target == PIPE_TEXTURE_2D ||
          args->target == PIPE_TEXTURE_RECT ||
          args->target == PIPE_TEXTURE_CUBE ||
          args->target == PIPE_TEXTURE_2D_ARRAY ||
          args->target == PIPE_TEXTURE_CUBE_ARRAY) {
         if (args->depth != 1)
            return -1;
      }
      if (args->target == PIPE_TEXTURE_1D ||
          args->target == PIPE_TEXTURE_1D_ARRAY) {
         if (args->height != 1 || args->depth != 1)
            return -1;
      }
   }
   return 0;
}
int vrend_renderer_resource_create(struct vrend_renderer_resource_create_args *args, struct iovec *iov, uint32_t num_iovs)
{
   struct vrend_resource *gr;
   int level;
   int ret;

   ret = check_resource_valid(args);
   if (ret)
      return EINVAL;

   gr = (struct vrend_resource *)CALLOC_STRUCT(vrend_texture);
   if (!gr)
      return ENOMEM;

   gr->handle = args->handle;
   gr->iov = iov;
   gr->num_iovs = num_iovs;
   gr->base.width0 = args->width;
   gr->base.height0 = args->height;
   gr->base.depth0 = args->depth;
   gr->base.format = args->format;
   gr->base.target = args->target;
   gr->base.last_level = args->last_level;
   gr->base.nr_samples = args->nr_samples;
   gr->base.array_size = args->array_size;

   if (args->flags & VIRGL_RESOURCE_Y_0_TOP)
      gr->y_0_top = true;

   pipe_reference_init(&gr->base.reference, 1);

   if (args->bind == VREND_RES_BIND_CUSTOM) {
      /* custom should only be for buffers */
      gr->ptr = malloc(args->width);
      if (!gr->ptr) {
         FREE(gr);
         return ENOMEM;
      }
   } else if (args->bind == VREND_RES_BIND_INDEX_BUFFER) {
      gr->target = GL_ELEMENT_ARRAY_BUFFER_ARB;
      glGenBuffersARB(1, &gr->id);
      glBindBufferARB(gr->target, gr->id);
      glBufferData(gr->target, args->width, NULL, GL_STREAM_DRAW);
   } else if (args->bind == VREND_RES_BIND_STREAM_OUTPUT) {
      gr->target = GL_TRANSFORM_FEEDBACK_BUFFER;
      glGenBuffersARB(1, &gr->id);
      glBindBuffer(gr->target, gr->id);
      glBufferData(gr->target, args->width, NULL, GL_STREAM_DRAW);
   } else if (args->bind == VREND_RES_BIND_VERTEX_BUFFER) {
      gr->target = GL_ARRAY_BUFFER_ARB;
      glGenBuffersARB(1, &gr->id);
      glBindBufferARB(gr->target, gr->id);
      glBufferData(gr->target, args->width, NULL, GL_STREAM_DRAW);
   } else if (args->bind == VREND_RES_BIND_CONSTANT_BUFFER) {
      gr->target = GL_UNIFORM_BUFFER;
      glGenBuffersARB(1, &gr->id);
      glBindBufferARB(gr->target, gr->id);
      glBufferData(gr->target, args->width, NULL, GL_STREAM_DRAW);
   } else if (args->target == PIPE_BUFFER && args->bind == 0) {
      gr->target = GL_ARRAY_BUFFER_ARB;
      glGenBuffersARB(1, &gr->id);
      glBindBufferARB(gr->target, gr->id);
      glBufferData(gr->target, args->width, NULL, GL_STREAM_DRAW);
   } else if (args->target == PIPE_BUFFER && (args->bind & VREND_RES_BIND_SAMPLER_VIEW)) {
      GLenum internalformat;

      /*
       * On Desktop we use GL_ARB_texture_buffer_object on GLES we use
       * GL_EXT_texture_buffer (it is in the ANDRIOD extension pack).
       */
#if GL_TEXTURE_BUFFER != GL_TEXTURE_BUFFER_EXT
#error "GL_TEXTURE_BUFFER enums differ, they shouldn't."
#endif

      /* need to check GL version here */
      if (vrend_state.have_arb_or_gles_ext_texture_buffer) {
         gr->target = GL_TEXTURE_BUFFER;
         glGenBuffersARB(1, &gr->id);
         glBindBufferARB(gr->target, gr->id);
         glGenTextures(1, &gr->tbo_tex_id);
         glBufferData(gr->target, args->width, NULL, GL_STREAM_DRAW);

         glBindTexture(gr->target, gr->tbo_tex_id);
         internalformat = tex_conv_table[args->format].internalformat;
         glTexBuffer(gr->target, internalformat, gr->id);
      } else {
         gr->target = GL_PIXEL_PACK_BUFFER_ARB;
         glGenBuffersARB(1, &gr->id);
         glBindBufferARB(gr->target, gr->id);
         glBufferData(gr->target, args->width, NULL, GL_STREAM_DRAW);
      }
   } else {
      struct vrend_texture *gt = (struct vrend_texture *)gr;
      GLenum internalformat, glformat, gltype;
      gr->target = tgsitargettogltarget(args->target, args->nr_samples);

      /* ugly workaround for texture rectangle missing on GLES */
      if (vrend_state.use_gles && gr->target == GL_TEXTURE_RECTANGLE_NV) {
         /* for some guests this is the only usage of rect */
         if (args->width != 1 || args->height != 1) {
            report_gles_warn(NULL, GLES_WARN_TEXTURE_RECT, 0);
         }
         gr->target = GL_TEXTURE_2D;
      }

      /* fallback for 1D textures */
      if (vrend_state.use_gles && gr->target == GL_TEXTURE_1D) {
         gr->target = GL_TEXTURE_2D;
      }

      /* fallback for 1D array textures */
      if (vrend_state.use_gles && gr->target == GL_TEXTURE_1D_ARRAY) {
         gr->target = GL_TEXTURE_2D_ARRAY;
      }

      glGenTextures(1, &gr->id);
      glBindTexture(gr->target, gr->id);

      internalformat = tex_conv_table[args->format].internalformat;
      glformat = tex_conv_table[args->format].glformat;
      gltype = tex_conv_table[args->format].gltype;
      if (internalformat == 0) {
         fprintf(stderr,"unknown format is %d\n", args->format);
         FREE(gr);
         return EINVAL;
      }

      if (args->nr_samples > 1) {
         if (vrend_state.use_gles) {
            report_gles_missing_func(NULL, "glTexImage[2,3]DMultisample");
         } else if (gr->target == GL_TEXTURE_2D_MULTISAMPLE) {
            glTexImage2DMultisample(gr->target, args->nr_samples,
                                    internalformat, args->width, args->height,
                                    GL_TRUE);
         } else {
            glTexImage3DMultisample(gr->target, args->nr_samples,
                                    internalformat, args->width, args->height, args->array_size,
                                    GL_TRUE);
         }

      } else if (gr->target == GL_TEXTURE_CUBE_MAP) {
         int i;
         for (i = 0; i < 6; i++) {
            GLenum ctarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
            for (level = 0; level <= args->last_level; level++) {
               unsigned mwidth = u_minify(args->width, level);
               unsigned mheight = u_minify(args->height, level);
               glTexImage2D(ctarget, level, internalformat, mwidth, mheight, 0, glformat,
                            gltype, NULL);
            }
         }
      } else if (gr->target == GL_TEXTURE_3D ||
                 gr->target == GL_TEXTURE_2D_ARRAY ||
                 gr->target == GL_TEXTURE_CUBE_MAP_ARRAY) {
         for (level = 0; level <= args->last_level; level++) {
            unsigned depth_param = (gr->target == GL_TEXTURE_2D_ARRAY || gr->target == GL_TEXTURE_CUBE_MAP_ARRAY) ? args->array_size : u_minify(args->depth, level);
            unsigned mwidth = u_minify(args->width, level);
            unsigned mheight = u_minify(args->height, level);
            glTexImage3D(gr->target, level, internalformat, mwidth, mheight, depth_param, 0,
                         glformat,
                         gltype, NULL);
         }
      } else if (gr->target == GL_TEXTURE_1D && vrend_state.use_gles) {
         report_gles_missing_func(NULL, "glTexImage1D");
      } else if (gr->target == GL_TEXTURE_1D) {
         for (level = 0; level <= args->last_level; level++) {
            unsigned mwidth = u_minify(args->width, level);
            glTexImage1D(gr->target, level, internalformat, mwidth, 0,
                         glformat,
                         gltype, NULL);
         }
      } else {
         for (level = 0; level <= args->last_level; level++) {
            unsigned mwidth = u_minify(args->width, level);
            unsigned mheight = u_minify(args->height, level);
            glTexImage2D(gr->target, level, internalformat, mwidth, gr->target == GL_TEXTURE_1D_ARRAY ? args->array_size : mheight, 0, glformat,
                         gltype, NULL);
         }
      }

      gt->state.max_lod = -1;
      gt->cur_swizzle_r = gt->cur_swizzle_g = gt->cur_swizzle_b = gt->cur_swizzle_a = -1;
   }

   ret = vrend_resource_insert(gr, args->handle);
   if (ret == 0) {
      vrend_renderer_resource_destroy(gr, true);
      return ENOMEM;
   }
   return 0;
}

void vrend_renderer_resource_destroy(struct vrend_resource *res, bool remove)
{
   if (res->readback_fb_id)
      glDeleteFramebuffers(1, &res->readback_fb_id);

   if (res->ptr)
      free(res->ptr);
   if (res->id) {
      if (res->target == GL_ELEMENT_ARRAY_BUFFER_ARB ||
          res->target == GL_ARRAY_BUFFER_ARB ||
          res->target == GL_UNIFORM_BUFFER||
          res->target == GL_TEXTURE_BUFFER||
          res->target == GL_TRANSFORM_FEEDBACK_BUFFER) {
         glDeleteBuffers(1, &res->id);
         if (res->target == GL_TEXTURE_BUFFER)
            glDeleteTextures(1, &res->tbo_tex_id);
      } else
         glDeleteTextures(1, &res->id);
   }

   if (res->handle && remove)
      vrend_resource_remove(res->handle);
   free(res);
}

static void vrend_destroy_resource_object(void *obj_ptr)
{
   struct vrend_resource *res = obj_ptr;

   if (pipe_reference(&res->base.reference, NULL))
       vrend_renderer_resource_destroy(res, false);
}

void vrend_renderer_resource_unref(uint32_t res_handle)
{
   struct vrend_resource *res;
   struct vrend_context *ctx;

   res = vrend_resource_lookup(res_handle, 0);
   if (!res)
      return;

   /* find in all contexts and detach also */

   /* remove from any contexts */
   LIST_FOR_EACH_ENTRY(ctx, &vrend_state.active_ctx_list, ctx_entry) {
      vrend_renderer_detach_res_ctx_p(ctx, res->handle);
   }

   vrend_resource_remove(res->handle);
}

static int use_sub_data = 0;
struct virgl_sub_upload_data {
   GLenum target;
   struct pipe_box *box;
};

static void iov_buffer_upload(void *cookie, uint32_t doff, void *src, int len)
{
   struct virgl_sub_upload_data *d = cookie;
   glBufferSubData(d->target, d->box->x + doff, len, src);
}

static void vrend_scale_depth(void *ptr, int size, float scale_val)
{
   GLuint *ival = ptr;
   const GLfloat myscale = 1.0f / 0xffffff;
   int i;
   for (i = 0; i < size / 4; i++) {
      GLuint value = ival[i];
      GLfloat d = ((float)(value >> 8) * myscale) * scale_val;
      d = CLAMP(d, 0.0F, 1.0F);
      ival[i] = (int)(d / myscale) << 8;
   }
}

static void read_transfer_data(struct pipe_resource *res,
                               struct iovec *iov,
                               unsigned int num_iovs,
                               char *data,
                               uint32_t src_stride,
                               struct pipe_box *box,
                               uint32_t level,
                               uint64_t offset,
                               bool invert)
{
   int blsize = util_format_get_blocksize(res->format);
   uint32_t size = vrend_get_iovec_size(iov, num_iovs);
   uint32_t send_size = util_format_get_nblocks(res->format, box->width,
                                              box->height) * blsize * box->depth;
   uint32_t bwx = util_format_get_nblocksx(res->format, box->width) * blsize;
   uint32_t bh = util_format_get_nblocksy(res->format, box->height);
   int d, h;

   if ((send_size == size || bh == 1) && !invert && box->depth == 1)
      vrend_read_from_iovec(iov, num_iovs, offset, data, send_size);
   else {
      if (invert) {
         for (d = 0; d < box->depth; d++) {
            uint32_t myoffset = offset + d * src_stride * (res->height0 >> level);
            for (h = bh - 1; h >= 0; h--) {
               void *ptr = data + (h * bwx) + d * (bh * bwx);
               vrend_read_from_iovec(iov, num_iovs, myoffset, ptr, bwx);
               myoffset += src_stride;
            }
         }
      } else {
         for (d = 0; d < box->depth; d++) {
            uint32_t myoffset = offset + d * src_stride * (res->height0 >> level);
            for (h = 0; h < bh; h++) {
               void *ptr = data + (h * bwx) + d * (bh * bwx);
               vrend_read_from_iovec(iov, num_iovs, myoffset, ptr, bwx);
               myoffset += src_stride;
            }
         }
      }
   }
}

static void write_transfer_data(struct pipe_resource *res,
                                struct iovec *iov,
                                unsigned num_iovs,
                                char *data,
                                uint32_t dst_stride,
                                struct pipe_box *box,
                                uint32_t level,
                                uint64_t offset,
                                bool invert)
{
   int blsize = util_format_get_blocksize(res->format);
   uint32_t size = vrend_get_iovec_size(iov, num_iovs);
   uint32_t send_size = util_format_get_nblocks(res->format, box->width,
                                                box->height) * blsize * box->depth;
   uint32_t bwx = util_format_get_nblocksx(res->format, box->width) * blsize;
   uint32_t bh = util_format_get_nblocksy(res->format, box->height);
   int d, h;
   uint32_t stride = dst_stride ? dst_stride : util_format_get_nblocksx(res->format, u_minify(res->width0, level)) * blsize;

   if ((send_size == size || bh == 1) && !invert && box->depth == 1) {
      vrend_write_to_iovec(iov, num_iovs, offset, data, send_size);
   } else if (invert) {
      for (d = 0; d < box->depth; d++) {
         uint32_t myoffset = offset + d * stride * (res->height0 >> level);
         for (h = bh - 1; h >= 0; h--) {
            void *ptr = data + (h * bwx) + d * (bh * bwx);
            vrend_write_to_iovec(iov, num_iovs, myoffset, ptr, bwx);
            myoffset += stride;
         }
      }
   } else {
      for (d = 0; d < box->depth; d++) {
         uint32_t myoffset = offset + d * stride * (res->height0 >> level);
         for (h = 0; h < bh; h++) {
            void *ptr = data + (h * bwx) + d * (bh * bwx);
            vrend_write_to_iovec(iov, num_iovs, myoffset, ptr, bwx);
            myoffset += stride;
         }
      }
   }
}

static bool check_transfer_bounds(struct vrend_resource *res,
                                  const struct vrend_transfer_info *info)
{
   int lwidth, lheight;

   /* check mipmap level is in bounds */
   if (info->level > res->base.last_level)
      return false;
   if (info->box->x < 0 || info->box->y < 0)
      return false;
   /* these will catch bad y/z/w/d with 1D textures etc */
   lwidth = u_minify(res->base.width0, info->level);
   if (info->box->width > lwidth)
      return false;
   if (info->box->x > lwidth)
      return false;
   if (info->box->width + info->box->x > lwidth)
      return false;

   lheight = u_minify(res->base.height0, info->level);
   if (info->box->height > lheight)
      return false;
   if (info->box->y > lheight)
      return false;
   if (info->box->height + info->box->y > lheight)
      return false;

   if (res->base.target == PIPE_TEXTURE_3D) {
      int ldepth = u_minify(res->base.depth0, info->level);
      if (info->box->depth > ldepth)
         return false;
      if (info->box->z > ldepth)
         return false;
      if (info->box->z + info->box->depth > ldepth)
         return false;
   } else {
      if (info->box->depth > res->base.array_size)
         return false;
      if (info->box->z > res->base.array_size)
         return false;
      if (info->box->z + info->box->depth > res->base.array_size)
         return false;
   }

   return true;
}

static bool check_iov_bounds(struct vrend_resource *res,
                             const struct vrend_transfer_info *info,
                             struct iovec *iov, int num_iovs)
{
   GLuint send_size;
   GLuint iovsize = vrend_get_iovec_size(iov, num_iovs);
   GLuint valid_stride, valid_layer_stride;

   /* validate the send size */
   valid_stride = util_format_get_stride(res->base.format, info->box->width);
   if (info->stride) {
      /* only validate passed in stride for boxes with height */
      if (info->box->height > 1) {
         if (info->stride < valid_stride)
            return false;
         valid_stride = info->stride;
      }
   }

   valid_layer_stride = util_format_get_2d_size(res->base.format, valid_stride,
                                                info->box->height);

   /* layer stride only makes sense for 3d,cube and arrays */
   if (info->layer_stride) {
      if ((res->base.target != PIPE_TEXTURE_3D &&
           res->base.target != PIPE_TEXTURE_CUBE &&
           res->base.target != PIPE_TEXTURE_1D_ARRAY &&
           res->base.target != PIPE_TEXTURE_2D_ARRAY &&
           res->base.target != PIPE_TEXTURE_CUBE_ARRAY))
         return false;

      /* only validate passed in layer_stride for boxes with depth */
      if (info->box->depth > 1) {
         if (info->layer_stride < valid_layer_stride)
            return false;
         valid_layer_stride = info->layer_stride;
      }
   }

   send_size = valid_layer_stride * info->box->depth;
   if (iovsize < info->offset)
      return false;
   if (iovsize < send_size)
      return false;
   if (iovsize < info->offset + send_size)
      return false;

   return true;
}

static int vrend_renderer_transfer_write_iov(struct vrend_context *ctx,
                                             struct vrend_resource *res,
                                             struct iovec *iov, int num_iovs,
                                             const struct vrend_transfer_info *info)
{
   void *data;

   if (res->target == 0 && res->ptr) {
      vrend_read_from_iovec(iov, num_iovs, info->offset, res->ptr + info->box->x, info->box->width);
      return 0;
   }
   if (res->target == GL_TRANSFORM_FEEDBACK_BUFFER ||
       res->target == GL_ELEMENT_ARRAY_BUFFER_ARB ||
       res->target == GL_ARRAY_BUFFER_ARB ||
       res->target == GL_TEXTURE_BUFFER ||
       res->target == GL_UNIFORM_BUFFER) {
      struct virgl_sub_upload_data d;
      d.box = info->box;
      d.target = res->target;

      glBindBufferARB(res->target, res->id);
      if (use_sub_data == 1) {
         vrend_read_from_iovec_cb(iov, num_iovs, info->offset, info->box->width, &iov_buffer_upload, &d);
      } else {
         data = glMapBufferRange(res->target, info->box->x, info->box->width, GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_WRITE_BIT);
         if (data == NULL) {
            fprintf(stderr,"map failed for element buffer\n");
            vrend_read_from_iovec_cb(iov, num_iovs, info->offset, info->box->width, &iov_buffer_upload, &d);
         } else {
            vrend_read_from_iovec(iov, num_iovs, info->offset, data, info->box->width);
            glUnmapBuffer(res->target);
         }
      }
   } else {
      GLenum glformat;
      GLenum gltype;
      int need_temp = 0;
      int elsize = util_format_get_blocksize(res->base.format);
      int x = 0, y = 0;
      bool compressed;
      bool invert = false;
      float depth_scale;
      GLuint send_size = 0;
      uint32_t stride = info->stride;

      vrend_use_program(ctx, 0);

      if (!stride)
         stride = util_format_get_nblocksx(res->base.format, u_minify(res->base.width0, info->level)) * elsize;

      compressed = util_format_is_compressed(res->base.format);
      if (num_iovs > 1 || compressed) {
         need_temp = true;
      }

      if (vrend_state.use_core_profile == true && (res->y_0_top || (res->base.format == (enum pipe_format)VIRGL_FORMAT_Z24X8_UNORM))) {
         need_temp = true;
         if (res->y_0_top)
            invert = true;
      }

      if (need_temp) {
         send_size = util_format_get_nblocks(res->base.format, info->box->width,
                                             info->box->height) * elsize * info->box->depth;
         data = malloc(send_size);
         if (!data)
            return ENOMEM;
         read_transfer_data(&res->base, iov, num_iovs, data, stride,
                            info->box, info->level, info->offset, invert);
      } else {
         data = (char*)iov[0].iov_base + info->offset;
      }

      if (stride && !need_temp) {
         glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / elsize);
      } else
         glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

      switch (elsize) {
      case 1:
      case 3:
         glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
         break;
      case 2:
      case 6:
         glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
         break;
      case 4:
      default:
         glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
         break;
      case 8:
         glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
         break;
      }

      glformat = tex_conv_table[res->base.format].glformat;
      gltype = tex_conv_table[res->base.format].gltype;

      if ((!vrend_state.use_core_profile) && (res->y_0_top)) {
         GLuint buffers;

         if (res->readback_fb_id == 0 || res->readback_fb_level != info->level) {
            GLuint fb_id;
            if (res->readback_fb_id)
               glDeleteFramebuffers(1, &res->readback_fb_id);

            glGenFramebuffers(1, &fb_id);
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb_id);
            vrend_fb_bind_texture(res, 0, info->level, 0);

            res->readback_fb_id = fb_id;
            res->readback_fb_level = info->level;
         } else {
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, res->readback_fb_id);
         }

         buffers = GL_COLOR_ATTACHMENT0_EXT;
         glDrawBuffers(1, &buffers);
         vrend_blend_enable(ctx, false);
         vrend_depth_test_enable(ctx, false);
         vrend_alpha_test_enable(ctx, false);
         vrend_stencil_test_enable(ctx, false);
         glPixelZoom(1.0f, res->y_0_top ? -1.0f : 1.0f);
         glWindowPos2i(info->box->x, res->y_0_top ? res->base.height0 - info->box->y : info->box->y);
         glDrawPixels(info->box->width, info->box->height, glformat, gltype,
                      data);
      } else {
         uint32_t comp_size;
         glBindTexture(res->target, res->id);

         if (compressed) {
            glformat = tex_conv_table[res->base.format].internalformat;
            comp_size = util_format_get_nblocks(res->base.format, info->box->width,
                                                info->box->height) * util_format_get_blocksize(res->base.format);
         }

         if (glformat == 0) {
            glformat = GL_BGRA;
            gltype = GL_UNSIGNED_BYTE;
         }

         x = info->box->x;
         y = invert ? res->base.height0 - info->box->y - info->box->height : info->box->y;

         if (res->base.format == (enum pipe_format)VIRGL_FORMAT_Z24X8_UNORM) {
            /* we get values from the guest as 24-bit scaled integers
               but we give them to the host GL and it interprets them
               as 32-bit scaled integers, so we need to scale them here */
            depth_scale = 256.0;
            if (!vrend_state.use_core_profile)
               glPixelTransferf(GL_DEPTH_SCALE, depth_scale);
            else
               vrend_scale_depth(data, send_size, depth_scale);
         }
         if (res->target == GL_TEXTURE_CUBE_MAP) {
            GLenum ctarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + info->box->z;
            if (compressed) {
               glCompressedTexSubImage2D(ctarget, info->level, x, y,
                                         info->box->width, info->box->height,
                                         glformat, comp_size, data);
            } else {
               glTexSubImage2D(ctarget, info->level, x, y, info->box->width, info->box->height,
                               glformat, gltype, data);
            }
         } else if (res->target == GL_TEXTURE_3D || res->target == GL_TEXTURE_2D_ARRAY || res->target == GL_TEXTURE_CUBE_MAP_ARRAY) {
            if (compressed) {
               glCompressedTexSubImage3D(res->target, info->level, x, y, info->box->z,
                                         info->box->width, info->box->height, info->box->depth,
                                         glformat, comp_size, data);
            } else {
               glTexSubImage3D(res->target, info->level, x, y, info->box->z,
                               info->box->width, info->box->height, info->box->depth,
                               glformat, gltype, data);
            }
         } else if (res->target == GL_TEXTURE_1D) {
            if (vrend_state.use_gles) {
               /* Covers both compressed and none compressed. */
               report_gles_missing_func(ctx, "gl[Compressed]TexSubImage1D");
            } else if (compressed) {
               glCompressedTexSubImage1D(res->target, info->level, info->box->x,
                                         info->box->width,
                                         glformat, comp_size, data);
            } else {
               glTexSubImage1D(res->target, info->level, info->box->x, info->box->width,
                               glformat, gltype, data);
            }
         } else {
            if (compressed) {
               glCompressedTexSubImage2D(res->target, info->level, x, res->target == GL_TEXTURE_1D_ARRAY ? info->box->z : y,
                                         info->box->width, info->box->height,
                                         glformat, comp_size, data);
            } else {
               glTexSubImage2D(res->target, info->level, x, res->target == GL_TEXTURE_1D_ARRAY ? info->box->z : y,
                               info->box->width,
                               res->target == GL_TEXTURE_1D_ARRAY ? info->box->depth : info->box->height,
                               glformat, gltype, data);
            }
         }
         if (res->base.format == (enum pipe_format)VIRGL_FORMAT_Z24X8_UNORM) {
            if (!vrend_state.use_core_profile)
               glPixelTransferf(GL_DEPTH_SCALE, 1.0);
         }
      }
      if (stride && !need_temp)
         glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

      if (need_temp)
         free(data);
   }
   return 0;
}

static int vrend_transfer_send_getteximage(struct vrend_context *ctx,
                                           struct vrend_resource *res,
                                           struct iovec *iov, int num_iovs,
                                           const struct vrend_transfer_info *info)
{
   GLenum format, type;
   uint32_t tex_size;
   char *data;
   int elsize = util_format_get_blocksize(res->base.format);
   int compressed = util_format_is_compressed(res->base.format);
   GLenum target;
   uint32_t depth = 1;
   uint32_t send_offset = 0;
   format = tex_conv_table[res->base.format].glformat;
   type = tex_conv_table[res->base.format].gltype;

   if (compressed)
      format = tex_conv_table[res->base.format].internalformat;

   if (res->target == GL_TEXTURE_3D)
      depth = u_minify(res->base.depth0, info->level);
   else if (res->target == GL_TEXTURE_2D_ARRAY || res->target == GL_TEXTURE_1D_ARRAY || res->target == GL_TEXTURE_CUBE_MAP_ARRAY)
      depth = res->base.array_size;

   tex_size = util_format_get_nblocks(res->base.format, u_minify(res->base.width0, info->level), u_minify(res->base.height0, info->level)) * util_format_get_blocksize(res->base.format) * depth;

   if (info->box->z && res->target != GL_TEXTURE_CUBE_MAP) {
      send_offset = util_format_get_nblocks(res->base.format, u_minify(res->base.width0, info->level), u_minify(res->base.height0, info->level)) * util_format_get_blocksize(res->base.format) * info->box->z;
   }

   data = malloc(tex_size);
   if (!data)
      return ENOMEM;

   switch (elsize) {
   case 1:
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      break;
   case 2:
      glPixelStorei(GL_PACK_ALIGNMENT, 2);
      break;
   case 4:
   default:
      glPixelStorei(GL_PACK_ALIGNMENT, 4);
      break;
   case 8:
      glPixelStorei(GL_PACK_ALIGNMENT, 8);
      break;
   }

   glBindTexture(res->target, res->id);
   if (res->target == GL_TEXTURE_CUBE_MAP) {
      target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + info->box->z;
   } else
      target = res->target;

   if (compressed) {
      if (vrend_state.have_arb_robustness) {
         glGetnCompressedTexImageARB(target, info->level, tex_size, data);
      } else if (vrend_state.use_gles) {
         report_gles_missing_func(ctx, "glGetCompressedTexImage");
      } else {
         glGetCompressedTexImage(target, info->level, data);
      }
   } else {
      if (vrend_state.have_arb_robustness) {
         glGetnTexImageARB(target, info->level, format, type, tex_size, data);
      } else if (vrend_state.use_gles) {
         report_gles_missing_func(ctx, "glGetTexImage");
      } else {
         glGetTexImage(target, info->level, format, type, data);
      }
   }

   glPixelStorei(GL_PACK_ALIGNMENT, 4);

   write_transfer_data(&res->base, iov, num_iovs, data + send_offset,
                       info->stride, info->box, info->level, info->offset,
                       false);
   free(data);
   return 0;
}

static int vrend_transfer_send_readpixels(struct vrend_context *ctx,
                                          struct vrend_resource *res,
                                          struct iovec *iov, int num_iovs,
                                          const struct vrend_transfer_info *info)
{
   char *myptr = (char*)iov[0].iov_base + info->offset;
   int need_temp = 0;
   GLuint fb_id;
   char *data;
   bool actually_invert, separate_invert = false;
   GLenum format, type;
   GLint y1;
   uint32_t send_size = 0;
   uint32_t h = u_minify(res->base.height0, info->level);
   int elsize = util_format_get_blocksize(res->base.format);
   float depth_scale;

   vrend_use_program(ctx, 0);

   format = tex_conv_table[res->base.format].glformat;
   type = tex_conv_table[res->base.format].gltype;
   /* if we are asked to invert and reading from a front then don't */

   actually_invert = res->y_0_top;

   if (actually_invert && !vrend_state.have_mesa_invert)
      separate_invert = true;

   if (num_iovs > 1 || separate_invert)
      need_temp = 1;

   if (need_temp) {
      send_size = util_format_get_nblocks(res->base.format, info->box->width, info->box->height) * info->box->depth * util_format_get_blocksize(res->base.format);
      data = malloc(send_size);
      if (!data) {
         fprintf(stderr,"malloc failed %d\n", send_size);
         return ENOMEM;
      }
   } else {
      send_size = iov[0].iov_len - info->offset;
      data = myptr;
   }

   if (res->readback_fb_id == 0 || res->readback_fb_level != info->level || res->readback_fb_z != info->box->z) {

      if (res->readback_fb_id)
         glDeleteFramebuffers(1, &res->readback_fb_id);

      glGenFramebuffers(1, &fb_id);
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb_id);

      vrend_fb_bind_texture(res, 0, info->level, info->box->z);

      res->readback_fb_id = fb_id;
      res->readback_fb_level = info->level;
      res->readback_fb_z = info->box->z;
   } else
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, res->readback_fb_id);
   if (actually_invert)
      y1 = h - info->box->y - info->box->height;
   else
      y1 = info->box->y;

   if (vrend_state.have_mesa_invert && actually_invert)
      glPixelStorei(GL_PACK_INVERT_MESA, 1);
   if (!vrend_format_is_ds(res->base.format))
      glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
   if (!need_temp && info->stride)
      glPixelStorei(GL_PACK_ROW_LENGTH, info->stride / elsize);

   switch (elsize) {
   case 1:
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      break;
   case 2:
      glPixelStorei(GL_PACK_ALIGNMENT, 2);
      break;
   case 4:
   default:
      glPixelStorei(GL_PACK_ALIGNMENT, 4);
      break;
   case 8:
      glPixelStorei(GL_PACK_ALIGNMENT, 8);
      break;
   }

   if (res->base.format == (enum pipe_format)VIRGL_FORMAT_Z24X8_UNORM) {
      /* we get values from the guest as 24-bit scaled integers
         but we give them to the host GL and it interprets them
         as 32-bit scaled integers, so we need to scale them here */
      depth_scale = 1.0 / 256.0;
      if (!vrend_state.use_core_profile) {
         glPixelTransferf(GL_DEPTH_SCALE, depth_scale);
      }
   }
   if (vrend_state.have_arb_robustness)
      glReadnPixelsARB(info->box->x, y1, info->box->width, info->box->height, format, type, send_size, data);
   else if (vrend_state.have_gles_khr_robustness) {
      glReadPixels(info->box->x, y1, info->box->width, info->box->height, format, type, data);
   } else
      glReadPixels(info->box->x, y1, info->box->width, info->box->height, format, type, data);

   if (res->base.format == (enum pipe_format)VIRGL_FORMAT_Z24X8_UNORM) {
      if (!vrend_state.use_core_profile)
         glPixelTransferf(GL_DEPTH_SCALE, 1.0);
      else
         vrend_scale_depth(data, send_size, depth_scale);
   }
   if (vrend_state.have_mesa_invert && actually_invert)
      glPixelStorei(GL_PACK_INVERT_MESA, 0);
   if (!need_temp && info->stride)
      glPixelStorei(GL_PACK_ROW_LENGTH, 0);
   glPixelStorei(GL_PACK_ALIGNMENT, 4);
   if (need_temp) {
      write_transfer_data(&res->base, iov, num_iovs, data,
                          info->stride, info->box, info->level, info->offset,
                          separate_invert);
      free(data);
   }
   return 0;
}

static int vrend_renderer_transfer_send_iov(struct vrend_context *ctx,
                                            struct vrend_resource *res,
                                            struct iovec *iov, int num_iovs,
                                            const struct vrend_transfer_info *info)
{
   if (res->target == 0 && res->ptr) {
      uint32_t send_size = info->box->width * util_format_get_blocksize(res->base.format);
      vrend_write_to_iovec(iov, num_iovs, info->offset, res->ptr + info->box->x, send_size);
      return 0;
   }

   if (res->target == GL_ELEMENT_ARRAY_BUFFER_ARB ||
       res->target == GL_ARRAY_BUFFER_ARB ||
       res->target == GL_TRANSFORM_FEEDBACK_BUFFER ||
       res->target == GL_TEXTURE_BUFFER ||
       res->target == GL_UNIFORM_BUFFER) {
      uint32_t send_size = info->box->width * util_format_get_blocksize(res->base.format);
      void *data;

      glBindBufferARB(res->target, res->id);
      data = glMapBufferRange(res->target, info->box->x, info->box->width, GL_MAP_READ_BIT);
      if (!data)
         fprintf(stderr,"unable to open buffer for reading %d\n", res->target);
      else
         vrend_write_to_iovec(iov, num_iovs, info->offset, data, send_size);
      glUnmapBuffer(res->target);
   } else {
      bool can_readpixels = true;

      can_readpixels = vrend_format_can_render(res->base.format) || vrend_format_is_ds(res->base.format);

      if (can_readpixels) {
         return vrend_transfer_send_readpixels(ctx, res,
                                               iov, num_iovs, info);
      }

      return vrend_transfer_send_getteximage(ctx, res,
                                             iov, num_iovs, info);

   }
   return 0;
}

int vrend_renderer_transfer_iov(const struct vrend_transfer_info *info,
                                int transfer_mode)
{
   struct vrend_resource *res;
   struct vrend_context *ctx;
   struct iovec *iov;
   int num_iovs;

   if (!info->box)
      return EINVAL;

   ctx = vrend_lookup_renderer_ctx(info->ctx_id);
   if (!ctx)
      return EINVAL;

   if (info->ctx_id == 0)
      res = vrend_resource_lookup(info->handle, 0);
   else
      res = vrend_renderer_ctx_res_lookup(ctx, info->handle);

   if (!res) {
      if (info->ctx_id)
         report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_RESOURCE, info->handle);
      return EINVAL;
   }

   iov = info->iovec;
   num_iovs = info->iovec_cnt;

   if (res->iov && (!iov || num_iovs == 0)) {
      iov = res->iov;
      num_iovs = res->num_iovs;
   }

   if (!iov) {
      if (info->ctx_id)
         report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_RESOURCE, info->handle);
      return EINVAL;
   }

   if (!check_transfer_bounds(res, info))
      return EINVAL;

   if (!check_iov_bounds(res, info, iov, num_iovs))
      return EINVAL;

   vrend_hw_switch_context(vrend_lookup_renderer_ctx(0), true);

   if (transfer_mode == VREND_TRANSFER_WRITE)
      return vrend_renderer_transfer_write_iov(ctx, res, iov, num_iovs,
                                               info);
   else
      return vrend_renderer_transfer_send_iov(ctx, res, iov, num_iovs,
                                              info);
}

int vrend_transfer_inline_write(struct vrend_context *ctx,
                                struct vrend_transfer_info *info,
                                unsigned usage)
{
   struct vrend_resource *res;

   res = vrend_renderer_ctx_res_lookup(ctx, info->handle);
   if (!res) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_RESOURCE, info->handle);
      return EINVAL;
   }

   if (!check_transfer_bounds(res, info)) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_CMD_BUFFER, info->handle);
      return EINVAL;
   }

   if (!check_iov_bounds(res, info, info->iovec, info->iovec_cnt)) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_CMD_BUFFER, info->handle);
      return EINVAL;
   }

   return vrend_renderer_transfer_write_iov(ctx, res, info->iovec, info->iovec_cnt, info);

}

void vrend_set_stencil_ref(struct vrend_context *ctx,
                           struct pipe_stencil_ref *ref)
{
   if (ctx->sub->stencil_refs[0] != ref->ref_value[0] ||
       ctx->sub->stencil_refs[1] != ref->ref_value[1]) {
      ctx->sub->stencil_refs[0] = ref->ref_value[0];
      ctx->sub->stencil_refs[1] = ref->ref_value[1];
      ctx->sub->stencil_state_dirty = true;
   }
}

void vrend_set_blend_color(struct vrend_context *ctx,
                           struct pipe_blend_color *color)
{
   ctx->sub->blend_color = *color;
   glBlendColor(color->color[0], color->color[1], color->color[2],
                color->color[3]);
}

void vrend_set_scissor_state(struct vrend_context *ctx,
                             uint32_t start_slot,
                             uint32_t num_scissor,
                             struct pipe_scissor_state *ss)
{
   int i, idx;

   if (start_slot > PIPE_MAX_VIEWPORTS ||
       num_scissor > (PIPE_MAX_VIEWPORTS - start_slot)) {
      vrend_report_buffer_error(ctx, 0);
      return;
   }

   for (i = 0; i < num_scissor; i++) {
      idx = start_slot + i;
      ctx->sub->ss[idx] = ss[i];
      ctx->sub->scissor_state_dirty |= (1 << idx);
   }
}

void vrend_set_polygon_stipple(struct vrend_context *ctx,
                               struct pipe_poly_stipple *ps)
{
   if (vrend_state.use_core_profile) {
      static const unsigned bit31 = 1 << 31;
      GLubyte *stip = calloc(1, 1024);
      int i, j;

      if (!ctx->pstip_inited)
         vrend_init_pstipple_texture(ctx);

      if (!stip)
         return;

      for (i = 0; i < 32; i++) {
         for (j = 0; j < 32; j++) {
            if (ps->stipple[i] & (bit31 >> j))
               stip[i * 32 + j] = 0;
            else
               stip[i * 32 + j] = 255;
         }
      }

      glBindTexture(GL_TEXTURE_2D, ctx->pstipple_tex_id);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 32, 32,
                      GL_RED, GL_UNSIGNED_BYTE, stip);

      free(stip);
      return;
   }
   glPolygonStipple((const GLubyte *)ps->stipple);
}

void vrend_set_clip_state(struct vrend_context *ctx, struct pipe_clip_state *ucp)
{
   if (vrend_state.use_core_profile) {
      ctx->sub->ucp_state = *ucp;
   } else {
      int i, j;
      GLdouble val[4];

      for (i = 0; i < 8; i++) {
         for (j = 0; j < 4; j++)
            val[j] = ucp->ucp[i][j];
         glClipPlane(GL_CLIP_PLANE0 + i, val);
      }
   }
}

void vrend_set_sample_mask(struct vrend_context *ctx, unsigned sample_mask)
{
   glSampleMaski(0, sample_mask);
}

static void vrend_hw_emit_streamout_targets(struct vrend_context *ctx, struct vrend_streamout_object *so_obj)
{
   int i;

   for (i = 0; i < so_obj->num_targets; i++) {
      if (so_obj->so_targets[i]->buffer_offset || so_obj->so_targets[i]->buffer_size < so_obj->so_targets[i]->buffer->base.width0)
         glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, i, so_obj->so_targets[i]->buffer->id, so_obj->so_targets[i]->buffer_offset, so_obj->so_targets[i]->buffer_size);
      else
         glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i, so_obj->so_targets[i]->buffer->id);
   }
}

void vrend_set_streamout_targets(struct vrend_context *ctx,
                                 uint32_t append_bitmask,
                                 uint32_t num_targets,
                                 uint32_t *handles)
{
   struct vrend_so_target *target;
   int i;

   if (num_targets) {
      bool found = false;
      struct vrend_streamout_object *obj;
      LIST_FOR_EACH_ENTRY(obj, &ctx->sub->streamout_list, head) {
         if (obj->num_targets == num_targets) {
            if (!memcmp(handles, obj->handles, num_targets * 4)) {
               found = true;
               break;
            }
         }
      }
      if (found) {
         ctx->sub->current_so = obj;
         glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, obj->id);
         return;
      }

      obj = CALLOC_STRUCT(vrend_streamout_object);
      if (vrend_state.have_tf2) {
         glGenTransformFeedbacks(1, &obj->id);
         glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, obj->id);
      }
      obj->num_targets = num_targets;
      for (i = 0; i < num_targets; i++) {
         obj->handles[i] = handles[i];
         target = vrend_object_lookup(ctx->sub->object_hash, handles[i], VIRGL_OBJECT_STREAMOUT_TARGET);
         if (!target) {
            report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_HANDLE, handles[i]);
            free(obj);
            return;
         }
         vrend_so_target_reference(&obj->so_targets[i], target);
      }
      vrend_hw_emit_streamout_targets(ctx, obj);
      list_addtail(&obj->head, &ctx->sub->streamout_list);
      ctx->sub->current_so = obj;
      obj->xfb_state = XFB_STATE_STARTED_NEED_BEGIN;
   } else {
      if (vrend_state.have_tf2)
         glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
      ctx->sub->current_so = NULL;
   }
}

static void vrend_resource_buffer_copy(struct vrend_context *ctx,
                                       struct vrend_resource *src_res,
                                       struct vrend_resource *dst_res,
                                       uint32_t dstx, uint32_t srcx,
                                       uint32_t width)
{

   glBindBuffer(GL_COPY_READ_BUFFER, src_res->id);
   glBindBuffer(GL_COPY_WRITE_BUFFER, dst_res->id);

   glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, srcx, dstx, width);
   glBindBuffer(GL_COPY_READ_BUFFER, 0);
   glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
}

static void vrend_resource_copy_fallback(struct vrend_context *ctx,
                                         struct vrend_resource *src_res,
                                         struct vrend_resource *dst_res,
                                         uint32_t dst_level,
                                         uint32_t dstx, uint32_t dsty,
                                         uint32_t dstz, uint32_t src_level,
                                         const struct pipe_box *src_box)
{
   char *tptr;
   uint32_t transfer_size;
   GLenum glformat, gltype;
   int elsize = util_format_get_blocksize(dst_res->base.format);
   int compressed = util_format_is_compressed(dst_res->base.format);
   int cube_slice = 1;
   uint32_t slice_size, slice_offset;
   int i;
   if (src_res->target == GL_TEXTURE_CUBE_MAP)
      cube_slice = 6;

   if (src_res->base.format != dst_res->base.format) {
      fprintf(stderr, "copy fallback failed due to mismatched formats %d %d\n", src_res->base.format, dst_res->base.format);
      return;
   }

   /* this is ugly need to do a full GetTexImage */
   slice_size = util_format_get_nblocks(src_res->base.format, u_minify(src_res->base.width0, src_level), u_minify(src_res->base.height0, src_level)) *
      u_minify(src_res->base.depth0, src_level) * util_format_get_blocksize(src_res->base.format);
   transfer_size = slice_size * src_res->base.array_size;

   tptr = malloc(transfer_size);
   if (!tptr)
      return;

   glformat = tex_conv_table[src_res->base.format].glformat;
   gltype = tex_conv_table[src_res->base.format].gltype;

   if (compressed)
      glformat = tex_conv_table[src_res->base.format].internalformat;

   switch (elsize) {
   case 1:
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      break;
   case 2:
      glPixelStorei(GL_PACK_ALIGNMENT, 2);
      break;
   case 4:
   default:
      glPixelStorei(GL_PACK_ALIGNMENT, 4);
      break;
   case 8:
      glPixelStorei(GL_PACK_ALIGNMENT, 8);
      break;
   }
   glBindTexture(src_res->target, src_res->id);

   slice_offset = 0;
   for (i = 0; i < cube_slice; i++) {
      GLenum ctarget = src_res->target == GL_TEXTURE_CUBE_MAP ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + i : src_res->target;
      if (compressed) {
         if (vrend_state.have_arb_robustness)
            glGetnCompressedTexImageARB(ctarget, src_level, transfer_size, tptr + slice_offset);
         else if (vrend_state.use_gles)
            report_gles_missing_func(ctx, "glGetCompressedTexImage");
         else
            glGetCompressedTexImage(ctarget, src_level, tptr + slice_offset);
      } else {
         if (vrend_state.have_arb_robustness)
            glGetnTexImageARB(ctarget, src_level, glformat, gltype, transfer_size, tptr + slice_offset);
         else if (vrend_state.use_gles)
            report_gles_missing_func(ctx, "glGetTexImage");
         else
            glGetTexImage(ctarget, src_level, glformat, gltype, tptr + slice_offset);
      }
      slice_offset += slice_size;
   }

   glPixelStorei(GL_PACK_ALIGNMENT, 4);
   switch (elsize) {
   case 1:
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      break;
   case 2:
      glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
      break;
   case 4:
   default:
      glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
      break;
   case 8:
      glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
      break;
   }

   glBindTexture(dst_res->target, dst_res->id);
   slice_offset = 0;
   for (i = 0; i < cube_slice; i++) {
      GLenum ctarget = dst_res->target == GL_TEXTURE_CUBE_MAP ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + i : dst_res->target;
      if (compressed) {
         if (ctarget == GL_TEXTURE_1D) {
            glCompressedTexSubImage1D(ctarget, dst_level, dstx,
                                      src_box->width,
                                      glformat, transfer_size, tptr + slice_offset);
         } else {
            glCompressedTexSubImage2D(ctarget, dst_level, dstx, dsty,
                                      src_box->width, src_box->height,
                                      glformat, transfer_size, tptr + slice_offset);
         }
      } else {
         if (ctarget == GL_TEXTURE_1D) {
            glTexSubImage1D(ctarget, dst_level, dstx, src_box->width, glformat, gltype, tptr + slice_offset);
         } else if (ctarget == GL_TEXTURE_3D ||
                    ctarget == GL_TEXTURE_2D_ARRAY ||
                    ctarget == GL_TEXTURE_CUBE_MAP_ARRAY) {
            glTexSubImage3D(ctarget, dst_level, dstx, dsty, dstz, src_box->width, src_box->height, src_box->depth, glformat, gltype, tptr + slice_offset);
         } else {
            glTexSubImage2D(ctarget, dst_level, dstx, dsty, src_box->width, src_box->height, glformat, gltype, tptr + slice_offset);
         }
      }
      slice_offset += slice_size;
   }

   glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
   free(tptr);
}

void vrend_renderer_resource_copy_region(struct vrend_context *ctx,
                                         uint32_t dst_handle, uint32_t dst_level,
                                         uint32_t dstx, uint32_t dsty, uint32_t dstz,
                                         uint32_t src_handle, uint32_t src_level,
                                         const struct pipe_box *src_box)
{
   struct vrend_resource *src_res, *dst_res;
   GLbitfield glmask = 0;
   GLint sy1, sy2, dy1, dy2;

   if (ctx->in_error)
      return;

   src_res = vrend_renderer_ctx_res_lookup(ctx, src_handle);
   dst_res = vrend_renderer_ctx_res_lookup(ctx, dst_handle);

   if (!src_res) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_RESOURCE, src_handle);
      return;
   }
   if (!dst_res) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_RESOURCE, dst_handle);
      return;
   }

   if (src_res->base.target == PIPE_BUFFER && dst_res->base.target == PIPE_BUFFER) {
      /* do a buffer copy */
      vrend_resource_buffer_copy(ctx, src_res, dst_res, dstx,
                                 src_box->x, src_box->width);
      return;
   }

   if (!vrend_format_can_render(src_res->base.format) ||
       !vrend_format_can_render(dst_res->base.format)) {
      vrend_resource_copy_fallback(ctx, src_res, dst_res, dst_level, dstx,
                                   dsty, dstz, src_level, src_box);

      return;
   }

   glBindFramebuffer(GL_FRAMEBUFFER_EXT, ctx->sub->blit_fb_ids[0]);
   /* clean out fb ids */
   glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT,
                             GL_TEXTURE_2D, 0, 0);
   vrend_fb_bind_texture(src_res, 0, src_level, src_box->z);

   glBindFramebuffer(GL_FRAMEBUFFER_EXT, ctx->sub->blit_fb_ids[1]);
   glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT,
                             GL_TEXTURE_2D, 0, 0);
   vrend_fb_bind_texture(dst_res, 0, dst_level, dstz);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ctx->sub->blit_fb_ids[1]);

   glBindFramebuffer(GL_READ_FRAMEBUFFER, ctx->sub->blit_fb_ids[0]);

   glmask = GL_COLOR_BUFFER_BIT;
   glDisable(GL_SCISSOR_TEST);

   if (!src_res->y_0_top) {
      sy1 = src_box->y;
      sy2 = src_box->y + src_box->height;
   } else {
      sy1 = src_res->base.height0 - src_box->y - src_box->height;
      sy2 = src_res->base.height0 - src_box->y;
   }

   if (!dst_res->y_0_top) {
      dy1 = dsty;
      dy2 = dsty + src_box->height;
   } else {
      dy1 = dst_res->base.height0 - dsty - src_box->height;
      dy2 = dst_res->base.height0 - dsty;
   }

   glBlitFramebuffer(src_box->x, sy1,
                     src_box->x + src_box->width,
                     sy2,
                     dstx, dy1,
                     dstx + src_box->width,
                     dy2,
                     glmask, GL_NEAREST);

}

static void vrend_renderer_blit_int(struct vrend_context *ctx,
                                    struct vrend_resource *src_res,
                                    struct vrend_resource *dst_res,
                                    const struct pipe_blit_info *info)
{
   GLbitfield glmask = 0;
   int src_y1, src_y2, dst_y1, dst_y2;
   GLenum filter;
   int n_layers = 1, i;
   bool use_gl = false;

   filter = convert_mag_filter(info->filter);

   /* if we can't make FBO's use the fallback path */
   if (!vrend_format_can_render(src_res->base.format) &&
       !vrend_format_is_ds(src_res->base.format))
      use_gl = true;
   if (!vrend_format_can_render(dst_res->base.format) &&
       !vrend_format_is_ds(dst_res->base.format))
      use_gl = true;

   /* different depth formats */
   if (vrend_format_is_ds(src_res->base.format) &&
       vrend_format_is_ds(dst_res->base.format)) {
      if (src_res->base.format != dst_res->base.format) {
         if (!(src_res->base.format == PIPE_FORMAT_S8_UINT_Z24_UNORM &&
               (dst_res->base.format == PIPE_FORMAT_Z24X8_UNORM))) {
            use_gl = true;
         }
      }
   }
   /* glBlitFramebuffer - can support depth stencil with NEAREST
      which we use for mipmaps */
   if ((info->mask & (PIPE_MASK_Z | PIPE_MASK_S)) && info->filter == PIPE_TEX_FILTER_LINEAR)
      use_gl = true;

   /* for scaled MS blits we either need extensions or hand roll */
   if (src_res->base.nr_samples > 1 &&
       src_res->base.nr_samples != dst_res->base.nr_samples &&
       (info->src.box.width != info->dst.box.width ||
        info->src.box.height != info->dst.box.height)) {
      if (vrend_state.have_ms_scaled_blit)
         filter = GL_SCALED_RESOLVE_NICEST_EXT;
      else
         use_gl = true;
   }

   /* for 3D mipmapped blits - hand roll time */
   if (info->src.box.depth != info->dst.box.depth)
      use_gl = true;

   if (vrend_format_is_emulated_alpha(info->dst.format) ||
       vrend_format_is_emulated_alpha(info->src.format))
      use_gl = true;

   if (use_gl) {
      vrend_renderer_blit_gl(ctx, src_res, dst_res, info);
      vrend_clicbs->make_current(0, ctx->sub->gl_context);
      return;
   }

   if (info->mask & PIPE_MASK_Z)
      glmask |= GL_DEPTH_BUFFER_BIT;
   if (info->mask & PIPE_MASK_S)
      glmask |= GL_STENCIL_BUFFER_BIT;
   if (info->mask & PIPE_MASK_RGBA)
      glmask |= GL_COLOR_BUFFER_BIT;

   if (!dst_res->y_0_top) {
      dst_y1 = info->dst.box.y + info->dst.box.height;
      dst_y2 = info->dst.box.y;
   } else {
      dst_y1 = dst_res->base.height0 - info->dst.box.y - info->dst.box.height;
      dst_y2 = dst_res->base.height0 - info->dst.box.y;
   }

   if (!src_res->y_0_top) {
      src_y1 = info->src.box.y + info->src.box.height;
      src_y2 = info->src.box.y;
   } else {
      src_y1 = src_res->base.height0 - info->src.box.y - info->src.box.height;
      src_y2 = src_res->base.height0 - info->src.box.y;
   }

   if (info->scissor_enable) {
      glScissor(info->scissor.minx, info->scissor.miny, info->scissor.maxx - info->scissor.minx, info->scissor.maxy - info->scissor.miny);
      ctx->sub->scissor_state_dirty = (1 << 0);
      glEnable(GL_SCISSOR_TEST);
   } else
      glDisable(GL_SCISSOR_TEST);

   glBindFramebuffer(GL_FRAMEBUFFER_EXT, ctx->sub->blit_fb_ids[0]);
   if (info->mask & PIPE_MASK_RGBA)
      glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT,
                                GL_TEXTURE_2D, 0, 0);
   else
      glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_2D, 0, 0);
   glBindFramebuffer(GL_FRAMEBUFFER_EXT, ctx->sub->blit_fb_ids[1]);
   if (info->mask & PIPE_MASK_RGBA)
      glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT,
                                GL_TEXTURE_2D, 0, 0);
   else if (info->mask & (PIPE_MASK_Z | PIPE_MASK_S))
      glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_2D, 0, 0);
   if (info->src.box.depth == info->dst.box.depth)
      n_layers = info->dst.box.depth;
   for (i = 0; i < n_layers; i++) {
      glBindFramebuffer(GL_FRAMEBUFFER_EXT, ctx->sub->blit_fb_ids[0]);
      glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT,
                                GL_TEXTURE_2D, 0, 0);
      vrend_fb_bind_texture(src_res, 0, info->src.level, info->src.box.z + i);

      glBindFramebuffer(GL_FRAMEBUFFER_EXT, ctx->sub->blit_fb_ids[1]);

      vrend_fb_bind_texture(dst_res, 0, info->dst.level, info->dst.box.z + i);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ctx->sub->blit_fb_ids[1]);

      glBindFramebuffer(GL_READ_FRAMEBUFFER, ctx->sub->blit_fb_ids[0]);

      glBlitFramebuffer(info->src.box.x,
                        src_y1,
                        info->src.box.x + info->src.box.width,
                        src_y2,
                        info->dst.box.x,
                        dst_y1,
                        info->dst.box.x + info->dst.box.width,
                        dst_y2,
                        glmask, filter);
   }

}

void vrend_renderer_blit(struct vrend_context *ctx,
                         uint32_t dst_handle, uint32_t src_handle,
                         const struct pipe_blit_info *info)
{
   struct vrend_resource *src_res, *dst_res;
   src_res = vrend_renderer_ctx_res_lookup(ctx, src_handle);
   dst_res = vrend_renderer_ctx_res_lookup(ctx, dst_handle);

   if (!src_res) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_RESOURCE, src_handle);
      return;
   }
   if (!dst_res) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_RESOURCE, dst_handle);
      return;
   }

   if (ctx->in_error)
      return;

   if (info->render_condition_enable == false)
      vrend_pause_render_condition(ctx, true);

   vrend_renderer_blit_int(ctx, src_res, dst_res, info);

   if (info->render_condition_enable == false)
      vrend_pause_render_condition(ctx, false);
}

int vrend_renderer_create_fence(int client_fence_id, uint32_t ctx_id)
{
   struct vrend_fence *fence;

   fence = malloc(sizeof(struct vrend_fence));
   if (!fence)
      return ENOMEM;

   fence->ctx_id = ctx_id;
   fence->fence_id = client_fence_id;
   fence->syncobj = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

   if (fence->syncobj == NULL)
      goto fail;

   if (vrend_state.sync_thread) {
      pipe_mutex_lock(vrend_state.fence_mutex);
      list_addtail(&fence->fences, &vrend_state.fence_wait_list);
      pipe_mutex_unlock(vrend_state.fence_mutex);
      pipe_condvar_signal(vrend_state.fence_cond);
   } else
      list_addtail(&fence->fences, &vrend_state.fence_list);
   return 0;

 fail:
   fprintf(stderr, "failed to create fence sync object\n");
   free(fence);
   return ENOMEM;
}

static void free_fence_locked(struct vrend_fence *fence)
{
   list_del(&fence->fences);
   glDeleteSync(fence->syncobj);
   free(fence);
}

static void flush_eventfd(int fd)
{
    ssize_t len;
    uint64_t value;
    do {
       len = read(fd, &value, sizeof(value));
    } while ((len == -1 && errno == EINTR) || len == sizeof(value));
}

void vrend_renderer_check_fences(void)
{
   struct vrend_fence *fence, *stor;
   uint32_t latest_id = 0;
   GLenum glret;

   if (!vrend_state.inited)
      return;

   if (vrend_state.sync_thread) {
      flush_eventfd(vrend_state.eventfd);
      pipe_mutex_lock(vrend_state.fence_mutex);
      LIST_FOR_EACH_ENTRY_SAFE(fence, stor, &vrend_state.fence_list, fences) {
         if (fence->fence_id > latest_id)
            latest_id = fence->fence_id;
         free_fence_locked(fence);
      }
      pipe_mutex_unlock(vrend_state.fence_mutex);
   } else {
      vrend_renderer_force_ctx_0();

      LIST_FOR_EACH_ENTRY_SAFE(fence, stor, &vrend_state.fence_list, fences) {
         glret = glClientWaitSync(fence->syncobj, 0, 0);
         if (glret == GL_ALREADY_SIGNALED){
            latest_id = fence->fence_id;
            free_fence_locked(fence);
         }
         /* don't bother checking any subsequent ones */
         else if (glret == GL_TIMEOUT_EXPIRED) {
            break;
         }
      }
   }

   if (latest_id == 0)
      return;
   vrend_clicbs->write_fence(latest_id);
}

static bool vrend_get_one_query_result(GLuint query_id, bool use_64, uint64_t *result)
{
   GLuint ready;
   GLuint passed;
   GLuint64 pass64;

   glGetQueryObjectuiv(query_id, GL_QUERY_RESULT_AVAILABLE_ARB, &ready);

   if (!ready)
      return false;

   if (use_64) {
      glGetQueryObjectui64v(query_id, GL_QUERY_RESULT_ARB, &pass64);
      *result = pass64;
   } else {
      glGetQueryObjectuiv(query_id, GL_QUERY_RESULT_ARB, &passed);
      *result = passed;
   }
   return true;
}

static bool vrend_check_query(struct vrend_query *query)
{
   uint64_t result;
   struct virgl_host_query_state *state;
   bool ret;

   ret = vrend_get_one_query_result(query->id, false, &result);
   if (ret == false)
      return false;

   state = (struct virgl_host_query_state *)query->res->ptr;
   state->result = result;
   state->query_state = VIRGL_QUERY_STATE_DONE;
   return true;
}

void vrend_renderer_check_queries(void)
{
   struct vrend_query *query, *stor;

   if (!vrend_state.inited)
      return;

   LIST_FOR_EACH_ENTRY_SAFE(query, stor, &vrend_state.waiting_query_list, waiting_queries) {
      vrend_hw_switch_context(vrend_lookup_renderer_ctx(query->ctx_id), true);
      if (vrend_check_query(query))
         list_delinit(&query->waiting_queries);
   }
}

bool vrend_hw_switch_context(struct vrend_context *ctx, bool now)
{
   if (ctx == vrend_state.current_ctx && ctx->ctx_switch_pending == false)
      return true;

   if (ctx->ctx_id != 0 && ctx->in_error) {
      return false;
   }

   ctx->ctx_switch_pending = true;
   if (now == true) {
      vrend_finish_context_switch(ctx);
   }
   vrend_state.current_ctx = ctx;
   return true;
}

static void vrend_finish_context_switch(struct vrend_context *ctx)
{
   if (ctx->ctx_switch_pending == false)
      return;
   ctx->ctx_switch_pending = false;

   if (vrend_state.current_hw_ctx == ctx)
      return;

   vrend_state.current_hw_ctx = ctx;

   vrend_clicbs->make_current(0, ctx->sub->gl_context);
}

void
vrend_renderer_object_destroy(struct vrend_context *ctx, uint32_t handle)
{
   vrend_object_remove(ctx->sub->object_hash, handle, 0);
}

uint32_t vrend_renderer_object_insert(struct vrend_context *ctx, void *data,
                                      uint32_t size, uint32_t handle, enum virgl_object_type type)
{
   return vrend_object_insert(ctx->sub->object_hash, data, size, handle, type);
}

int vrend_create_query(struct vrend_context *ctx, uint32_t handle,
                       uint32_t query_type, uint32_t query_index,
                       uint32_t res_handle, uint32_t offset)
{
   struct vrend_query *q;
   struct vrend_resource *res;
   uint32_t ret_handle;
   res = vrend_renderer_ctx_res_lookup(ctx, res_handle);
   if (!res) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_RESOURCE, res_handle);
      return EINVAL;
   }

   q = CALLOC_STRUCT(vrend_query);
   if (!q)
      return ENOMEM;

   list_inithead(&q->waiting_queries);
   q->type = query_type;
   q->index = query_index;
   q->ctx_id = ctx->ctx_id;

   vrend_resource_reference(&q->res, res);

   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      q->gltype = GL_SAMPLES_PASSED_ARB;
      break;
   case PIPE_QUERY_OCCLUSION_PREDICATE:
      q->gltype = GL_ANY_SAMPLES_PASSED;
      break;
   case PIPE_QUERY_TIMESTAMP:
      q->gltype = GL_TIMESTAMP;
      break;
   case PIPE_QUERY_TIME_ELAPSED:
      q->gltype = GL_TIME_ELAPSED;
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      q->gltype = GL_PRIMITIVES_GENERATED;
      break;
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      q->gltype = GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN;
      break;
   default:
      fprintf(stderr,"unknown query object received %d\n", q->type);
      break;
   }

   glGenQueries(1, &q->id);

   ret_handle = vrend_renderer_object_insert(ctx, q, sizeof(struct vrend_query), handle,
                                             VIRGL_OBJECT_QUERY);
   if (!ret_handle) {
      FREE(q);
      return ENOMEM;
   }
   return 0;
}

static void vrend_destroy_query(struct vrend_query *query)
{
   vrend_resource_reference(&query->res, NULL);
   list_del(&query->waiting_queries);
   glDeleteQueries(1, &query->id);
   free(query);
}

static void vrend_destroy_query_object(void *obj_ptr)
{
   struct vrend_query *query = obj_ptr;
   vrend_destroy_query(query);
}

void vrend_begin_query(struct vrend_context *ctx, uint32_t handle)
{
   struct vrend_query *q;

   q = vrend_object_lookup(ctx->sub->object_hash, handle, VIRGL_OBJECT_QUERY);
   if (!q)
      return;

   if (q->gltype == GL_TIMESTAMP)
      return;

   glBeginQuery(q->gltype, q->id);
}

void vrend_end_query(struct vrend_context *ctx, uint32_t handle)
{
   struct vrend_query *q;
   q = vrend_object_lookup(ctx->sub->object_hash, handle, VIRGL_OBJECT_QUERY);
   if (!q)
      return;

   if (vrend_is_timer_query(q->gltype)) {
      if (vrend_state.use_gles && q->gltype == GL_TIMESTAMP) {
         report_gles_warn(ctx, GLES_WARN_TIMESTAMP, 0);
      } else if (q->gltype == GL_TIMESTAMP) {
         glQueryCounter(q->id, q->gltype);
      } else {
         /* remove from active query list for this context */
         glEndQuery(q->gltype);
      }
      return;
   }

   glEndQuery(q->gltype);
}

void vrend_get_query_result(struct vrend_context *ctx, uint32_t handle,
                            uint32_t wait)
{
   struct vrend_query *q;
   bool ret;

   q = vrend_object_lookup(ctx->sub->object_hash, handle, VIRGL_OBJECT_QUERY);
   if (!q)
      return;

   ret = vrend_check_query(q);
   if (ret == false)
      list_addtail(&q->waiting_queries, &vrend_state.waiting_query_list);
}

static void vrend_pause_render_condition(struct vrend_context *ctx, bool pause)
{
   if (pause) {
      if (ctx->sub->cond_render_q_id)
         glEndConditionalRenderNV();
   } else {
      if (ctx->sub->cond_render_q_id)
         glBeginConditionalRender(ctx->sub->cond_render_q_id,
                                  ctx->sub->cond_render_gl_mode);
   }
}

void vrend_render_condition(struct vrend_context *ctx,
                            uint32_t handle,
                            bool condition,
                            uint mode)
{
   struct vrend_query *q;
   GLenum glmode = 0;

   if (handle == 0) {
      glEndConditionalRenderNV();
      ctx->sub->cond_render_q_id = 0;
      ctx->sub->cond_render_gl_mode = 0;
      return;
   }

   q = vrend_object_lookup(ctx->sub->object_hash, handle, VIRGL_OBJECT_QUERY);
   if (!q)
      return;

   switch (mode) {
   case PIPE_RENDER_COND_WAIT:
      glmode = GL_QUERY_WAIT;
      break;
   case PIPE_RENDER_COND_NO_WAIT:
      glmode = GL_QUERY_NO_WAIT;
      break;
   case PIPE_RENDER_COND_BY_REGION_WAIT:
      glmode = GL_QUERY_BY_REGION_WAIT;
      break;
   case PIPE_RENDER_COND_BY_REGION_NO_WAIT:
      glmode = GL_QUERY_BY_REGION_NO_WAIT;
      break;
   default:
      fprintf(stderr, "unhandled condition %x\n", mode);
   }

   ctx->sub->cond_render_q_id = q->id;
   ctx->sub->cond_render_gl_mode = glmode;
   glBeginConditionalRender(q->id, glmode);

}

int vrend_create_so_target(struct vrend_context *ctx,
                           uint32_t handle,
                           uint32_t res_handle,
                           uint32_t buffer_offset,
                           uint32_t buffer_size)
{
   struct vrend_so_target *target;
   struct vrend_resource *res;
   int ret_handle;
   res = vrend_renderer_ctx_res_lookup(ctx, res_handle);
   if (!res) {
      report_context_error(ctx, VIRGL_ERROR_CTX_ILLEGAL_RESOURCE, res_handle);
      return EINVAL;
   }

   target = CALLOC_STRUCT(vrend_so_target);
   if (!target)
      return ENOMEM;

   pipe_reference_init(&target->reference, 1);
   target->res_handle = res_handle;
   target->buffer_offset = buffer_offset;
   target->buffer_size = buffer_size;
   target->sub_ctx = ctx->sub;
   vrend_resource_reference(&target->buffer, res);

   ret_handle = vrend_renderer_object_insert(ctx, target, sizeof(*target), handle,
                                             VIRGL_OBJECT_STREAMOUT_TARGET);
   if (ret_handle == 0) {
      FREE(target);
      return ENOMEM;
   }
   return 0;
}

static void vrender_get_glsl_version(int *glsl_version)
{
   int major_local, minor_local;
   const GLubyte *version_str;
   int c;
   int version;

   version_str = glGetString(GL_SHADING_LANGUAGE_VERSION);
   if (vrend_state.use_gles) {
      char tmp[20];
      c = sscanf((const char *)version_str, "%s %s %s %s %i.%i",
                  tmp, tmp, tmp, tmp, &major_local, &minor_local);
      assert(c == 6);
   } else {
      c = sscanf((const char *)version_str, "%i.%i",
                  &major_local, &minor_local);
      assert(c == 2);
   }

   version = (major_local * 100) + minor_local;
   if (glsl_version)
      *glsl_version = version;
}

void vrend_renderer_fill_caps(uint32_t set, uint32_t version,
                              union virgl_caps *caps)
{
   int i;
   GLint max;
   GLfloat range[2];
   int gl_ver = 31;
   bool fill_capset2 = false;
   if (!caps)
      return;

   if (set > 2) {
      caps->max_version = 0;
      return;
   }

   if (set == 1) {
      memset(caps, 0, sizeof(struct virgl_caps_v1));
      caps->max_version = 1;
   } else if (set == 2) {
      fill_capset2 = true;
      memset(caps, 0, sizeof(*caps));
      caps->max_version = 2;
   }

   caps->v1.bset.occlusion_query = 1;
   if (gl_ver >= 30) {
      caps->v1.bset.indep_blend_enable = 1;
      caps->v1.bset.conditional_render = 1;
   } else {
      if (epoxy_has_gl_extension("GL_EXT_draw_buffers2"))
         caps->v1.bset.indep_blend_enable = 1;
      if (epoxy_has_gl_extension("GL_NV_conditional_render"))
         caps->v1.bset.conditional_render = 1;
   }

   if (vrend_state.use_core_profile) {
      caps->v1.bset.poly_stipple = 0;
      caps->v1.bset.color_clamping = 0;
   } else {
      caps->v1.bset.poly_stipple = 1;
      caps->v1.bset.color_clamping = 1;
   }
   if (gl_ver >= 31) {
      caps->v1.bset.instanceid = 1;
      glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &max);
      vrend_state.max_uniform_blocks = max;
      caps->v1.max_uniform_blocks = max + 1;
   } else {
      if (epoxy_has_gl_extension("GL_ARB_draw_instanced"))
         caps->v1.bset.instanceid = 1;
   }

   if (vrend_state.have_nv_prim_restart || vrend_state.have_gl_prim_restart)
      caps->v1.bset.primitive_restart = 1;

   if (gl_ver >= 32) {
      caps->v1.bset.fragment_coord_conventions = 1;
      caps->v1.bset.depth_clip_disable = 1;
      caps->v1.bset.seamless_cube_map = 1;
   } else {
      if (epoxy_has_gl_extension("GL_ARB_fragment_coord_conventions"))
         caps->v1.bset.fragment_coord_conventions = 1;
      if (epoxy_has_gl_extension("GL_ARB_seamless_cube_map"))
         caps->v1.bset.seamless_cube_map = 1;
   }

   if (epoxy_has_gl_extension("GL_AMD_seamless_cube_map_per_texture"))
      caps->v1.bset.seamless_cube_map_per_texture = 1;

   if (epoxy_has_gl_extension("GL_ARB_texture_multisample")) {
      /* disable multisample until developed */
      caps->v1.bset.texture_multisample = 1;
   }
   if (gl_ver >= 40) {
      caps->v1.bset.indep_blend_func = 1;
      caps->v1.bset.cube_map_array = 1;
      caps->v1.bset.texture_query_lod = 1;
      caps->v1.bset.has_indirect_draw = 1;
      caps->v1.bset.has_sample_shading = 1;
   } else {
      if (epoxy_has_gl_extension("GL_ARB_draw_buffers_blend"))
         caps->v1.bset.indep_blend_func = 1;
      if (epoxy_has_gl_extension("GL_ARB_texture_cube_map_array"))
         caps->v1.bset.cube_map_array = 1;
      if (epoxy_has_gl_extension("GL_ARB_texture_query_lod"))
         caps->v1.bset.texture_query_lod = 1;
      if (epoxy_has_gl_extension("GL_ARB_indirect_draw"))
         caps->v1.bset.has_indirect_draw = 1;
      if (epoxy_has_gl_extension("GL_ARB_sample_shading"))
         caps->v1.bset.has_sample_shading = 1;
   }

   if (gl_ver >= 42) {
      caps->v1.bset.start_instance = 1;
   } else {
      if (epoxy_has_gl_extension("GL_ARB_base_instance"))
         caps->v1.bset.start_instance = 1;
   }
   if (epoxy_has_gl_extension("GL_ARB_shader_stencil_export"))
      caps->v1.bset.shader_stencil_export = 1;

   /* we only support up to GLSL 1.40 features now */
   caps->v1.glsl_level = 130;
   if (vrend_state.use_core_profile) {
      if (gl_ver == 31)
         caps->v1.glsl_level = 140;
      else if (gl_ver == 32)
         caps->v1.glsl_level = 150;
      else if (gl_ver >= 33)
         caps->v1.glsl_level = 330;
   }

   if (epoxy_has_gl_extension("GL_EXT_texture_mirror_clamp"))
      caps->v1.bset.mirror_clamp = true;

   if (epoxy_has_gl_extension("GL_EXT_texture_array")) {
      glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max);
      caps->v1.max_texture_array_layers = max;
   }

   /* we need tf3 so we can do gallium skip buffers */
   if (epoxy_has_gl_extension("GL_ARB_transform_feedback2")) {
      caps->v1.bset.streamout_pause_resume = 1;
   }

   if (epoxy_has_gl_extension("GL_ARB_transform_feedback3")) {
      glGetIntegerv(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, &max);
      caps->v1.max_streamout_buffers = max;
   } else if (epoxy_has_gl_extension("GL_EXT_transform_feedback"))
      caps->v1.max_streamout_buffers = 4;
   if (epoxy_has_gl_extension("GL_ARB_blend_func_extended")) {
      glGetIntegerv(GL_MAX_DUAL_SOURCE_DRAW_BUFFERS, &max);
      caps->v1.max_dual_source_render_targets = max;
   } else
      caps->v1.max_dual_source_render_targets = 0;

   glGetIntegerv(GL_MAX_DRAW_BUFFERS, &max);
   caps->v1.max_render_targets = max;

   glGetIntegerv(GL_MAX_SAMPLES, &max);
   caps->v1.max_samples = max;

   if (epoxy_has_gl_extension("GL_ARB_texture_buffer_object")) {
      glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &max);
      caps->v1.max_tbo_size = max;
   }

   if (epoxy_has_gl_extension("GL_ARB_texture_gather")) {
      glGetIntegerv(GL_MAX_PROGRAM_TEXTURE_GATHER_COMPONENTS_ARB, &max);
      caps->v1.max_texture_gather_components = max;
   }

   if (epoxy_has_gl_extension("GL_ARB_viewport_array")) {
      glGetIntegerv(GL_MAX_VIEWPORTS, &max);
      caps->v1.max_viewports = max;
   } else
      caps->v1.max_viewports = 1;

   caps->v1.prim_mask = (1 << PIPE_PRIM_POINTS) | (1 << PIPE_PRIM_LINES) | (1 << PIPE_PRIM_LINE_STRIP) | (1 << PIPE_PRIM_LINE_LOOP) | (1 << PIPE_PRIM_TRIANGLES) | (1 << PIPE_PRIM_TRIANGLE_STRIP) | (1 << PIPE_PRIM_TRIANGLE_FAN);
   if (vrend_state.use_core_profile == false) {
      caps->v1.prim_mask |= (1 << PIPE_PRIM_QUADS) | (1 << PIPE_PRIM_QUAD_STRIP) | (1 << PIPE_PRIM_POLYGON);
   }
   if (caps->v1.glsl_level >= 150)
      caps->v1.prim_mask |= (1 << PIPE_PRIM_LINES_ADJACENCY) |
         (1 << PIPE_PRIM_LINE_STRIP_ADJACENCY) |
         (1 << PIPE_PRIM_TRIANGLES_ADJACENCY) |
         (1 << PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY);


   if (epoxy_has_gl_extension("GL_ARB_vertex_type_10f_11f_11f_rev")) {
      int val = VIRGL_FORMAT_R11G11B10_FLOAT;
      uint32_t offset = val / 32;
      uint32_t index = val % 32;

      caps->v1.vertexbuffer.bitmask[offset] |= (1 << index);
   }

   for (i = 0; i < VIRGL_FORMAT_MAX; i++) {
      uint32_t offset = i / 32;
      uint32_t index = i % 32;

      if (tex_conv_table[i].internalformat != 0) {
         if (vrend_format_can_sample(i)) {
            caps->v1.sampler.bitmask[offset] |= (1 << index);
            if (vrend_format_can_render(i))
               caps->v1.render.bitmask[offset] |= (1 << index);
         }
      }
   }

   if (!fill_capset2)
      return;

   glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, range);
   caps->v2.min_aliased_point_size = range[0];
   caps->v2.max_aliased_point_size = range[1];

   glGetFloatv(GL_SMOOTH_POINT_SIZE_RANGE, range);
   caps->v2.min_smooth_point_size = range[0];
   caps->v2.max_smooth_point_size = range[1];

   glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, range);
   caps->v2.min_aliased_line_width = range[0];
   caps->v2.max_aliased_line_width = range[1];

   glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, range);
   caps->v2.min_smooth_line_width = range[0];
   caps->v2.max_smooth_line_width = range[1];

   glGetFloatv(GL_MAX_TEXTURE_LOD_BIAS, &caps->v2.max_texture_lod_bias);
   glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &caps->v2.max_vertex_attribs);
   glGetIntegerv(GL_MAX_VERTEX_OUTPUT_COMPONENTS, &max);
   caps->v2.max_vertex_outputs = max / 4;

   if (gl_ver >= 32) {
      glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &caps->v2.max_geom_output_vertices);
      glGetIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &caps->v2.max_geom_total_output_components);
   }
   caps->v2.max_shader_patch_varyings = 0; // until we do tess.

   if (epoxy_has_gl_extension("GL_ARB_texture_gather")) {
       glGetIntegerv(GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET, &caps->v2.min_texture_gather_offset);
       glGetIntegerv(GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET, &caps->v2.max_texture_gather_offset);
   }
   glGetIntegerv(GL_MIN_PROGRAM_TEXEL_OFFSET, &caps->v2.min_texel_offset);
   glGetIntegerv(GL_MAX_PROGRAM_TEXEL_OFFSET, &caps->v2.max_texel_offset);

   glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &caps->v2.uniform_buffer_offset_alignment);

   if (gl_ver >= 43)
      glGetIntegerv(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT, &caps->v2.texture_buffer_offset_alignment);
}

GLint64 vrend_renderer_get_timestamp(void)
{
   GLint64 v;
   glGetInteger64v(GL_TIMESTAMP, &v);
   return v;
}

void *vrend_renderer_get_cursor_contents(uint32_t res_handle, uint32_t *width, uint32_t *height)
{
   GLenum format, type;
   struct vrend_resource *res;
   int blsize;
   char *data, *data2;
   int size;
   int h;

   res = vrend_resource_lookup(res_handle, 0);
   if (!res)
      return NULL;

   if (res->base.width0 > 128 || res->base.height0 > 128)
      return NULL;

   if (res->target != GL_TEXTURE_2D)
      return NULL;

   if (width)
      *width = res->base.width0;
   if (height)
      *height = res->base.height0;
   format = tex_conv_table[res->base.format].glformat;
   type = tex_conv_table[res->base.format].gltype;
   blsize = util_format_get_blocksize(res->base.format);
   size = util_format_get_nblocks(res->base.format, res->base.width0, res->base.height0) * blsize;
   data = malloc(size);
   data2 = malloc(size);

   if (!data || !data2) {
      free(data);
      free(data2);
      return NULL;
   }

   if (vrend_state.have_arb_robustness) {
      glBindTexture(res->target, res->id);
      glGetnTexImageARB(res->target, 0, format, type, size, data);
   } else if (vrend_state.use_gles) {
      GLuint fb_id;

      if (res->readback_fb_id == 0 || res->readback_fb_level != 0 || res->readback_fb_z != 0) {

         if (res->readback_fb_id)
            glDeleteFramebuffers(1, &res->readback_fb_id);

         glGenFramebuffers(1, &fb_id);
         glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb_id);

         vrend_fb_bind_texture(res, 0, 0, 0);

         res->readback_fb_id = fb_id;
         res->readback_fb_level = 0;
         res->readback_fb_z = 0;
      } else {
         glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, res->readback_fb_id);
      }

      if (vrend_state.have_arb_robustness) {
         glReadnPixelsARB(0, 0, *width, *height, format, type, size, data);
      } else if (vrend_state.have_gles_khr_robustness) {
         glReadPixels(0, 0, *width, *height, format, type, data);
      } else {
         glReadPixels(0, 0, *width, *height, format, type, data);
      }

   } else {
      glBindTexture(res->target, res->id);
      glGetTexImage(res->target, 0, format, type, data);
   }

   for (h = 0; h < res->base.height0; h++) {
      uint32_t doff = (res->base.height0 - h - 1) * res->base.width0 * blsize;
      uint32_t soff = h * res->base.width0 * blsize;

      memcpy(data2 + doff, data + soff, res->base.width0 * blsize);
   }
   free(data);

   return data2;
}

void vrend_renderer_force_ctx_0(void)
{
   struct vrend_context *ctx0 = vrend_lookup_renderer_ctx(0);
   vrend_state.current_ctx = NULL;
   vrend_state.current_hw_ctx = NULL;
   vrend_hw_switch_context(ctx0, true);
   vrend_clicbs->make_current(0, ctx0->sub->gl_context);
}

void vrend_renderer_get_rect(int res_handle, struct iovec *iov, unsigned int num_iovs,
                             uint32_t offset, int x, int y, int width, int height)
{
   struct vrend_resource *res = vrend_resource_lookup(res_handle, 0);
   struct vrend_transfer_info transfer_info;
   struct pipe_box box;
   int elsize;

   memset(&transfer_info, 0, sizeof(transfer_info));

   elsize = util_format_get_blocksize(res->base.format);
   box.x = x;
   box.y = y;
   box.z = 0;
   box.width = width;
   box.height = height;
   box.depth = 1;

   transfer_info.box = &box;

   transfer_info.stride = util_format_get_nblocksx(res->base.format, res->base.width0) * elsize;
   transfer_info.offset = offset;
   transfer_info.handle = res->handle;
   transfer_info.iovec = iov;
   transfer_info.iovec_cnt = num_iovs;
   vrend_renderer_transfer_iov(&transfer_info, VREND_TRANSFER_READ);
}

void vrend_renderer_attach_res_ctx(int ctx_id, int resource_id)
{
   struct vrend_context *ctx = vrend_lookup_renderer_ctx(ctx_id);
   struct vrend_resource *res;

   if (!ctx)
      return;

   res = vrend_resource_lookup(resource_id, 0);
   if (!res)
      return;

   vrend_object_insert_nofree(ctx->res_hash, res, sizeof(*res), resource_id, 1, false);
}

static void vrend_renderer_detach_res_ctx_p(struct vrend_context *ctx, int res_handle)
{
   struct vrend_resource *res;
   res = vrend_object_lookup(ctx->res_hash, res_handle, 1);
   if (!res)
      return;

   vrend_object_remove(ctx->res_hash, res_handle, 1);
}

void vrend_renderer_detach_res_ctx(int ctx_id, int res_handle)
{
   struct vrend_context *ctx = vrend_lookup_renderer_ctx(ctx_id);
   if (!ctx)
      return;
   vrend_renderer_detach_res_ctx_p(ctx, res_handle);
}

static struct vrend_resource *vrend_renderer_ctx_res_lookup(struct vrend_context *ctx, int res_handle)
{
   struct vrend_resource *res = vrend_object_lookup(ctx->res_hash, res_handle, 1);

   return res;
}

int vrend_renderer_resource_get_info(int res_handle,
                                     struct vrend_renderer_resource_info *info)
{
   struct vrend_resource *res;
   int elsize;

   if (!info)
      return EINVAL;
   res = vrend_resource_lookup(res_handle, 0);
   if (!res)
      return EINVAL;

   elsize = util_format_get_blocksize(res->base.format);

   info->handle = res_handle;
   info->tex_id = res->id;
   info->width = res->base.width0;
   info->height = res->base.height0;
   info->depth = res->base.depth0;
   info->format = res->base.format;
   info->flags = res->y_0_top ? VIRGL_RESOURCE_Y_0_TOP : 0;
   info->stride = util_format_get_nblocksx(res->base.format, u_minify(res->base.width0, 0)) * elsize;

   return 0;
}

void vrend_renderer_get_cap_set(uint32_t cap_set, uint32_t *max_ver,
                                uint32_t *max_size)
{
   switch (cap_set) {
   case VREND_CAP_SET:
      *max_ver = 1;
      *max_size = sizeof(struct virgl_caps_v1);
      break;
   case VREND_CAP_SET2:
      /* we should never need to increase this - it should be possible to just grow virgl_caps */
      *max_ver = 2;
      *max_size = sizeof(struct virgl_caps_v2);
      break;
   default:
      *max_ver = 0;
      *max_size = 0;
      break;
   }
}

void vrend_renderer_create_sub_ctx(struct vrend_context *ctx, int sub_ctx_id)
{
   struct vrend_sub_context *sub;
   struct virgl_gl_ctx_param ctx_params;

   LIST_FOR_EACH_ENTRY(sub, &ctx->sub_ctxs, head) {
      if (sub->sub_ctx_id == sub_ctx_id) {
         return;
      }
   }

   sub = CALLOC_STRUCT(vrend_sub_context);
   if (!sub)
      return;

   ctx_params.shared = (ctx->ctx_id == 0 && sub_ctx_id == 0) ? false : true;
   ctx_params.major_ver = vrend_state.gl_major_ver;
   ctx_params.minor_ver = vrend_state.gl_minor_ver;
   sub->gl_context = vrend_clicbs->create_gl_context(0, &ctx_params);
   vrend_clicbs->make_current(0, sub->gl_context);

   sub->sub_ctx_id = sub_ctx_id;

   if (!vrend_state.have_vertex_attrib_binding) {
      glGenVertexArrays(1, &sub->vaoid);
      glBindVertexArray(sub->vaoid);
   }

   glGenFramebuffers(1, &sub->fb_id);
   glGenFramebuffers(2, sub->blit_fb_ids);

   list_inithead(&sub->programs);
   list_inithead(&sub->streamout_list);

   sub->object_hash = vrend_object_init_ctx_table();

   ctx->sub = sub;
   list_add(&sub->head, &ctx->sub_ctxs);
   if (sub_ctx_id == 0)
      ctx->sub0 = sub;
}

void vrend_renderer_destroy_sub_ctx(struct vrend_context *ctx, int sub_ctx_id)
{
   struct vrend_sub_context *sub, *tofree = NULL;

   /* never destroy sub context id 0 */
   if (sub_ctx_id == 0)
      return;

   LIST_FOR_EACH_ENTRY(sub, &ctx->sub_ctxs, head) {
      if (sub->sub_ctx_id == sub_ctx_id) {
         tofree = sub;
      }
   }

   if (tofree) {
      if (ctx->sub == tofree) {
         ctx->sub = ctx->sub0;
         vrend_clicbs->make_current(0, ctx->sub->gl_context);
      }
      vrend_destroy_sub_context(tofree);
   }
}

void vrend_renderer_set_sub_ctx(struct vrend_context *ctx, int sub_ctx_id)
{
   struct vrend_sub_context *sub;
   /* find the sub ctx */

   if (ctx->sub && ctx->sub->sub_ctx_id == sub_ctx_id)
      return;

   LIST_FOR_EACH_ENTRY(sub, &ctx->sub_ctxs, head) {
      if (sub->sub_ctx_id == sub_ctx_id) {
         ctx->sub = sub;
         vrend_clicbs->make_current(0, sub->gl_context);
         break;
      }
   }
}

static void vrend_reset_fences(void)
{
   struct vrend_fence *fence, *stor;

   if (vrend_state.sync_thread)
      pipe_mutex_lock(vrend_state.fence_mutex);

   LIST_FOR_EACH_ENTRY_SAFE(fence, stor, &vrend_state.fence_list, fences) {
      free_fence_locked(fence);
   }

   if (vrend_state.sync_thread)
      pipe_mutex_unlock(vrend_state.fence_mutex);
}

void vrend_renderer_reset(void)
{
   if (vrend_state.sync_thread) {
      vrend_free_sync_thread();
      vrend_state.stop_sync_thread = false;
   }
   vrend_reset_fences();
   vrend_decode_reset(false);
   vrend_object_fini_resource_table();
   vrend_decode_reset(true);
   vrend_object_init_resource_table();
   vrend_renderer_context_create_internal(0, 0, NULL);
}

int vrend_renderer_get_poll_fd(void)
{
   if (!vrend_state.inited)
      return -1;

   return vrend_state.eventfd;
}
