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

/* library interface from QEMU to virglrenderer */

#ifndef VIRGLRENDERER_H
#define VIRGLRENDERER_H

#include <stdint.h>
#include <stdbool.h>

struct virgl_box;
struct iovec;

#define VIRGL_EXPORT  __attribute__((visibility("default")))

typedef void *virgl_renderer_gl_context;

struct virgl_renderer_gl_ctx_param {
   int version;
   bool shared;
   int major_ver;
   int minor_ver;
};

struct virgl_renderer_callbacks {
   int version;
   void (*write_fence)(void *cookie, uint32_t fence);

   /* interact with GL implementation */
   virgl_renderer_gl_context (*create_gl_context)(void *cookie, int scanout_idx, struct virgl_renderer_gl_ctx_param *param);
   void (*destroy_gl_context)(void *cookie, virgl_renderer_gl_context ctx);
   int (*make_current)(void *cookie, int scanout_idx, virgl_renderer_gl_context ctx);
};

/* virtio-gpu compatible interface */
#define VIRGL_RENDERER_USE_EGL 1
/*
 * Wait for sync objects in thread rather than polling
 * need to use virgl_renderer_get_poll_fd to know if this feature is in effect.
 */
#define VIRGL_RENDERER_THREAD_SYNC 2
#define VIRGL_RENDERER_USE_GLX (1 << 2)

VIRGL_EXPORT int virgl_renderer_init(void *cookie, int flags, struct virgl_renderer_callbacks *cb);
VIRGL_EXPORT void virgl_renderer_poll(void); /* force fences */

/* we need to give qemu the cursor resource contents */
VIRGL_EXPORT void *virgl_renderer_get_cursor_data(uint32_t resource_id, uint32_t *width, uint32_t *height);

VIRGL_EXPORT void virgl_renderer_get_rect(int resource_id, struct iovec *iov, unsigned int num_iovs,
                                          uint32_t offset, int x, int y, int width, int height);

/* virgl bind flags - these are compatible with mesa 10.5 gallium.
   but are fixed, no other should be passed to virgl either. */
#define VIRGL_RES_BIND_DEPTH_STENCIL (1 << 0)
#define VIRGL_RES_BIND_RENDER_TARGET (1 << 1)
#define VIRGL_RES_BIND_SAMPLER_VIEW  (1 << 3)
#define VIRGL_RES_BIND_VERTEX_BUFFER (1 << 4)
#define VIRGL_RES_BIND_INDEX_BUFFER  (1 << 5)
#define VIRGL_RES_BIND_CONSTANT_BUFFER (1 << 6)
#define VIRGL_RES_BIND_STREAM_OUTPUT (1 << 11)
#define VIRGL_RES_BIND_CURSOR        (1 << 16)
#define VIRGL_RES_BIND_CUSTOM        (1 << 17)

struct virgl_renderer_resource_create_args {
   uint32_t handle;
   uint32_t target;
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

/* new API */

VIRGL_EXPORT int virgl_renderer_resource_create(struct virgl_renderer_resource_create_args *args, struct iovec *iov, uint32_t num_iovs);
VIRGL_EXPORT void virgl_renderer_resource_unref(uint32_t res_handle);

VIRGL_EXPORT int virgl_renderer_context_create(uint32_t handle, uint32_t nlen, const char *name);
VIRGL_EXPORT void virgl_renderer_context_destroy(uint32_t handle);

VIRGL_EXPORT int virgl_renderer_submit_cmd(void *buffer,
                                           int ctx_id,
                                           int ndw);

VIRGL_EXPORT int virgl_renderer_transfer_read_iov(uint32_t handle, uint32_t ctx_id,
                                                  uint32_t level, uint32_t stride,
                                                  uint32_t layer_stride,
                                                  struct virgl_box *box,
                                                  uint64_t offset, struct iovec *iov,
                                                  int iovec_cnt);

VIRGL_EXPORT int virgl_renderer_transfer_write_iov(uint32_t handle,
                                                   uint32_t ctx_id,
                                                   int level,
                                                   uint32_t stride,
                                                   uint32_t layer_stride,
                                                   struct virgl_box *box,
                                                   uint64_t offset,
                                                   struct iovec *iovec,
                                                   unsigned int iovec_cnt);

VIRGL_EXPORT void virgl_renderer_get_cap_set(uint32_t set, uint32_t *max_ver,
                                             uint32_t *max_size);

VIRGL_EXPORT void virgl_renderer_fill_caps(uint32_t set, uint32_t version,
                                           void *caps);

VIRGL_EXPORT int virgl_renderer_resource_attach_iov(int res_handle, struct iovec *iov,
                                                    int num_iovs);
VIRGL_EXPORT void virgl_renderer_resource_detach_iov(int res_handle, struct iovec **iov, int *num_iovs);

VIRGL_EXPORT int virgl_renderer_create_fence(int client_fence_id, uint32_t ctx_id);

VIRGL_EXPORT void virgl_renderer_force_ctx_0(void);

VIRGL_EXPORT void virgl_renderer_ctx_attach_resource(int ctx_id, int res_handle);
VIRGL_EXPORT void virgl_renderer_ctx_detach_resource(int ctx_id, int res_handle);

/* return information about a resource */

struct virgl_renderer_resource_info {
   uint32_t handle;
   uint32_t virgl_format;
   uint32_t width;
   uint32_t height;
   uint32_t depth;
   uint32_t flags;
   uint32_t tex_id;
   uint32_t stride;
   int drm_fourcc;
};

VIRGL_EXPORT int virgl_renderer_resource_get_info(int res_handle,
                                                  struct virgl_renderer_resource_info *info);

VIRGL_EXPORT void virgl_renderer_cleanup(void *cookie);

/* reset the rendererer - destroy all contexts and resource */
VIRGL_EXPORT void virgl_renderer_reset(void);

VIRGL_EXPORT int virgl_renderer_get_poll_fd(void);

#endif
