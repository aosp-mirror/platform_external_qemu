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

#ifndef VREND_RENDERER_H
#define VREND_RENDERER_H

#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "virgl_protocol.h"
#include "vrend_iov.h"
#include "virgl_hw.h"

typedef void *virgl_gl_context;
typedef void *virgl_gl_drawable;

struct virgl_gl_ctx_param {
   int major_ver;
   int minor_ver;
   bool shared;
};

extern int vrend_dump_shaders;
struct vrend_context;

struct vrend_resource {
   struct pipe_resource base;
   GLuint id;
   GLenum target;
   /* fb id if we need to readback this resource */
   GLuint readback_fb_id;
   GLuint readback_fb_level;
   GLuint readback_fb_z;

   GLuint tbo_tex_id;/* tbos have two ids to track */
   bool y_0_top;

   GLuint handle;

   char *ptr;
   struct iovec *iov;
   uint32_t num_iovs;
};

/* assume every format is sampler friendly */
#define VREND_BIND_SAMPLER (1 << 0)
#define VREND_BIND_RENDER (1 << 1)
#define VREND_BIND_DEPTHSTENCIL (1 << 2)

#define VREND_BIND_NEED_SWIZZLE (1 << 28)

struct vrend_format_table {
   enum virgl_formats format;
   GLenum internalformat;
   GLenum glformat;
   GLenum gltype;
   uint32_t bindings;
   int flags;
   uint8_t swizzle[4];
};

struct vrend_transfer_info {
   uint32_t handle;
   uint32_t ctx_id;
   int level;
   uint32_t stride;
   uint32_t layer_stride;
   unsigned int iovec_cnt;
   struct iovec *iovec;
   uint64_t offset;
   struct pipe_box *box;
};

struct vrend_if_cbs {
   void (*write_fence)(unsigned fence_id);

   virgl_gl_context (*create_gl_context)(int scanout, struct virgl_gl_ctx_param *params);
   void (*destroy_gl_context)(virgl_gl_context ctx);
   int (*make_current)(int scanout, virgl_gl_context ctx);
};

#define VREND_USE_THREAD_SYNC 1

int vrend_renderer_init(struct vrend_if_cbs *cbs, uint32_t flags);

void vrend_insert_format(struct vrend_format_table *entry, uint32_t bindings);
void vrend_insert_format_swizzle(int override_format, struct vrend_format_table *entry, uint32_t bindings, uint8_t swizzle[4]);
int vrend_create_shader(struct vrend_context *ctx,
                        uint32_t handle,
                        const struct pipe_stream_output_info *stream_output,
                        const char *shd_text, uint32_t offlen, uint32_t num_tokens,
                        uint32_t type, uint32_t pkt_length);

void vrend_bind_shader(struct vrend_context *ctx,
                       uint32_t type,
                       uint32_t handle);

void vrend_bind_vs_so(struct vrend_context *ctx,
                      uint32_t handle);
void vrend_clear(struct vrend_context *ctx,
                 unsigned buffers,
                 const union pipe_color_union *color,
                 double depth, unsigned stencil);

void vrend_draw_vbo(struct vrend_context *ctx,
                    const struct pipe_draw_info *info,
                    uint32_t cso, uint32_t indirect_handle, uint32_t indirect_draw_count_handle);

void vrend_set_framebuffer_state(struct vrend_context *ctx,
                                 uint32_t nr_cbufs, uint32_t surf_handle[PIPE_MAX_COLOR_BUFS],
                                 uint32_t zsurf_handle);

struct vrend_context *vrend_create_context(int id, uint32_t nlen, const char *debug_name);
bool vrend_destroy_context(struct vrend_context *ctx);
int vrend_renderer_context_create(uint32_t handle, uint32_t nlen, const char *name);
void vrend_renderer_context_create_internal(uint32_t handle, uint32_t nlen, const char *name);
void vrend_renderer_context_destroy(uint32_t handle);

/* virgl bind flags - these are compatible with mesa 10.5 gallium.
   but are fixed, no other should be passed to virgl either. */
#define VREND_RES_BIND_DEPTH_STENCIL (1 << 0)
#define VREND_RES_BIND_RENDER_TARGET (1 << 1)
#define VREND_RES_BIND_SAMPLER_VIEW  (1 << 3)
#define VREND_RES_BIND_VERTEX_BUFFER (1 << 4)
#define VREND_RES_BIND_INDEX_BUFFER  (1 << 5)
#define VREND_RES_BIND_CONSTANT_BUFFER (1 << 6)
#define VREND_RES_BIND_STREAM_OUTPUT (1 << 11)
#define VREND_RES_BIND_CURSOR        (1 << 16)
#define VREND_RES_BIND_CUSTOM        (1 << 17)

struct vrend_renderer_resource_create_args {
   uint32_t handle;
   enum pipe_texture_target target;
   uint32_t format;
   uint32_t bind;
   uint32_t width;
   uint32_t height;
   uint32_t depth;
   uint32_t array_size;
   uint32_t last_level;
   uint32_t nr_samples;
   uint32_t flags;
};

int vrend_renderer_resource_create(struct vrend_renderer_resource_create_args *args, struct iovec *iov, uint32_t num_iovs);

void vrend_renderer_resource_unref(uint32_t handle);

int vrend_create_surface(struct vrend_context *ctx,
                         uint32_t handle,
                         uint32_t res_handle, uint32_t format,
                         uint32_t val0, uint32_t val1);
int vrend_create_sampler_view(struct vrend_context *ctx,
                              uint32_t handle,
                              uint32_t res_handle, uint32_t format,
                              uint32_t val0, uint32_t val1, uint32_t swizzle_packed);

int vrend_create_sampler_state(struct vrend_context *ctx,
                               uint32_t handle,
                               struct pipe_sampler_state *templ);

int vrend_create_so_target(struct vrend_context *ctx,
                           uint32_t handle,
                           uint32_t res_handle,
                           uint32_t buffer_offset,
                           uint32_t buffer_size);

void vrend_set_streamout_targets(struct vrend_context *ctx,
                                 uint32_t append_bitmask,
                                 uint32_t num_targets,
                                 uint32_t *handles);

int vrend_create_vertex_elements_state(struct vrend_context *ctx,
                                       uint32_t handle,
                                       unsigned num_elements,
                                       const struct pipe_vertex_element *elements);
void vrend_bind_vertex_elements_state(struct vrend_context *ctx,
                                      uint32_t handle);

void vrend_set_single_vbo(struct vrend_context *ctx,
                          int index,
                          uint32_t stride,
                          uint32_t buffer_offset,
                          uint32_t res_handle);
void vrend_set_num_vbo(struct vrend_context *ctx,
                       int num_vbo);

int vrend_transfer_inline_write(struct vrend_context *ctx,
                                struct vrend_transfer_info *info,
                                unsigned usage);

void vrend_set_viewport_states(struct vrend_context *ctx,
                               uint32_t start_slot, uint32_t num_viewports,
                               const struct pipe_viewport_state *state);
void vrend_set_num_sampler_views(struct vrend_context *ctx,
                                 uint32_t shader_type,
                                 uint32_t start_slot,
                                 int num_sampler_views);
void vrend_set_single_sampler_view(struct vrend_context *ctx,
                                   uint32_t shader_type,
                                   int index,
                                   uint32_t res_handle);

void vrend_object_bind_blend(struct vrend_context *ctx,
                             uint32_t handle);
void vrend_object_bind_dsa(struct vrend_context *ctx,
                           uint32_t handle);
void vrend_object_bind_rasterizer(struct vrend_context *ctx,
                                  uint32_t handle);

void vrend_bind_sampler_states(struct vrend_context *ctx,
                               uint32_t shader_type,
                               uint32_t start_slot,
                               uint32_t num_states,
                               uint32_t *handles);
void vrend_set_index_buffer(struct vrend_context *ctx,
                            uint32_t res_handle,
                            uint32_t index_size,
                            uint32_t offset);

#define VREND_TRANSFER_WRITE 1
#define VREND_TRANSFER_READ 2
int vrend_renderer_transfer_iov(const struct vrend_transfer_info *info, int transfer_mode);

void vrend_renderer_resource_copy_region(struct vrend_context *ctx,
                                         uint32_t dst_handle, uint32_t dst_level,
                                         uint32_t dstx, uint32_t dsty, uint32_t dstz,
                                         uint32_t src_handle, uint32_t src_level,
                                         const struct pipe_box *src_box);

void vrend_renderer_blit(struct vrend_context *ctx,
                         uint32_t dst_handle, uint32_t src_handle,
                         const struct pipe_blit_info *info);

void vrend_set_stencil_ref(struct vrend_context *ctx, struct pipe_stencil_ref *ref);
void vrend_set_blend_color(struct vrend_context *ctx, struct pipe_blend_color *color);
void vrend_set_scissor_state(struct vrend_context *ctx,
                             uint32_t start_slot,
                             uint32_t num_scissor,
                             struct pipe_scissor_state *ss);

void vrend_set_polygon_stipple(struct vrend_context *ctx, struct pipe_poly_stipple *ps);

void vrend_set_clip_state(struct vrend_context *ctx, struct pipe_clip_state *ucp);
void vrend_set_sample_mask(struct vrend_context *ctx, unsigned sample_mask);

void vrend_set_constants(struct vrend_context *ctx,
                         uint32_t shader,
                         uint32_t index,
                         uint32_t num_constant,
                         float *data);

void vrend_set_uniform_buffer(struct vrend_context *ctx, uint32_t shader,
                              uint32_t index, uint32_t offset, uint32_t length,
                              uint32_t res_handle);

void vrend_renderer_fini(void);

int vrend_decode_block(uint32_t ctx_id, uint32_t *block, int ndw);
struct vrend_context *vrend_lookup_renderer_ctx(uint32_t ctx_id);

int vrend_renderer_create_fence(int client_fence_id, uint32_t ctx_id);

void vrend_renderer_check_fences(void);
void vrend_renderer_check_queries(void);

bool vrend_hw_switch_context(struct vrend_context *ctx, bool now);
uint32_t vrend_renderer_object_insert(struct vrend_context *ctx, void *data,
                                      uint32_t size, uint32_t handle, enum virgl_object_type type);
void vrend_renderer_object_destroy(struct vrend_context *ctx, uint32_t handle);

int vrend_create_query(struct vrend_context *ctx, uint32_t handle,
                       uint32_t query_type, uint32_t query_index,
                       uint32_t res_handle, uint32_t offset);

void vrend_begin_query(struct vrend_context *ctx, uint32_t handle);
void vrend_end_query(struct vrend_context *ctx, uint32_t handle);
void vrend_get_query_result(struct vrend_context *ctx, uint32_t handle,
                            uint32_t wait);
void vrend_render_condition(struct vrend_context *ctx,
                            uint32_t handle,
                            bool condtion,
                            uint mode);
void *vrend_renderer_get_cursor_contents(uint32_t res_handle, uint32_t *width, uint32_t *height);
void vrend_bind_va(GLuint vaoid);
int vrend_renderer_flush_buffer_res(struct vrend_resource *res,
                                    struct pipe_box *box);

void vrend_renderer_fill_caps(uint32_t set, uint32_t version,
                              union virgl_caps *caps);

GLint64 vrend_renderer_get_timestamp(void);

void vrend_build_format_list(void);
void vrend_build_format_list_gles(void);

int vrend_renderer_resource_attach_iov(int res_handle, struct iovec *iov,
                                       int num_iovs);
void vrend_renderer_resource_detach_iov(int res_handle,
                                        struct iovec **iov_p,
                                        int *num_iovs_p);
void vrend_renderer_resource_destroy(struct vrend_resource *res, bool remove);

static inline void
vrend_resource_reference(struct vrend_resource **ptr, struct vrend_resource *tex)
{
   struct vrend_resource *old_tex = *ptr;

   if (pipe_reference(&(*ptr)->base.reference, &tex->base.reference))
      vrend_renderer_resource_destroy(old_tex, true);
   *ptr = tex;
}

void vrend_renderer_force_ctx_0(void);

void vrend_renderer_get_rect(int resource_id, struct iovec *iov, unsigned int num_iovs,
                             uint32_t offset, int x, int y, int width, int height);
void vrend_renderer_attach_res_ctx(int ctx_id, int resource_id);
void vrend_renderer_detach_res_ctx(int ctx_id, int resource_id);

struct vrend_renderer_resource_info {
   uint32_t handle;
   uint32_t format;
   uint32_t width;
   uint32_t height;
   uint32_t depth;
   uint32_t flags;
   uint32_t tex_id;
   uint32_t stride;
};

int vrend_renderer_resource_get_info(int res_handle,
                                     struct vrend_renderer_resource_info *info);

#define VREND_CAP_SET 1
#define VREND_CAP_SET2 2

void vrend_renderer_get_cap_set(uint32_t cap_set, uint32_t *max_ver,
                                uint32_t *max_size);

void vrend_renderer_create_sub_ctx(struct vrend_context *ctx, int sub_ctx_id);
void vrend_renderer_destroy_sub_ctx(struct vrend_context *ctx, int sub_ctx_id);
void vrend_renderer_set_sub_ctx(struct vrend_context *ctx, int sub_ctx_id);
void vrend_report_buffer_error(struct vrend_context *ctx, int cmd);

void vrend_fb_bind_texture(struct vrend_resource *res,
                           int idx,
                           uint32_t level, uint32_t layer);
bool vrend_is_ds_format(int format);
bool vrend_format_is_emulated_alpha(int format);
/* blitter interface */
void vrend_renderer_blit_gl(struct vrend_context *ctx,
                            struct vrend_resource *src_res,
                            struct vrend_resource *dst_res,
                            const struct pipe_blit_info *info);

void vrend_renderer_reset(void);
int vrend_renderer_get_poll_fd(void);
void vrend_decode_reset(bool ctx_0_only);

struct gl_version {
   uint32_t major;
   uint32_t minor;
};

static const struct gl_version gl_versions[] = { {4,5}, {4,4}, {4,3}, {4,2}, {4,1}, {4,0},
                                                 {3,3}, {3,2}, {3,1}, {3,0} };

extern struct vrend_if_cbs *vrend_clicbs;
#endif
