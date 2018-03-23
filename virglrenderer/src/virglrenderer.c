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

#include <stdio.h>
#include <time.h>

#include "igl.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "pipe/p_state.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "vrend_renderer.h"

#include "virglrenderer.h"

#ifdef HAVE_EPOXY_EGL_H
#include "virgl_egl.h"
static struct virgl_egl *egl_info;
#endif

#ifdef HAVE_EPOXY_GLX_H
#include "virgl_glx.h"
static struct virgl_glx *glx_info;
#endif

enum {
   CONTEXT_NONE,
   CONTEXT_EGL,
   CONTEXT_GLX
};

static int use_context = CONTEXT_NONE;

/* new API - just wrap internal API for now */

int virgl_renderer_resource_create(struct virgl_renderer_resource_create_args *args, struct iovec *iov, uint32_t num_iovs)
{
   return vrend_renderer_resource_create((struct vrend_renderer_resource_create_args *)args, iov, num_iovs);
}

void virgl_renderer_resource_unref(uint32_t res_handle)
{
   vrend_renderer_resource_unref(res_handle);
}

void virgl_renderer_fill_caps(uint32_t set, uint32_t version,
                              void *caps)
{
   vrend_renderer_fill_caps(set, version, (union virgl_caps *)caps);
}

int virgl_renderer_context_create(uint32_t handle, uint32_t nlen, const char *name)
{
   return vrend_renderer_context_create(handle, nlen, name);
}

void virgl_renderer_context_destroy(uint32_t handle)
{
   vrend_renderer_context_destroy(handle);
}

int virgl_renderer_submit_cmd(void *buffer,
                              int ctx_id,
                              int ndw)
{
   return vrend_decode_block(ctx_id, buffer, ndw);
}

int virgl_renderer_transfer_write_iov(uint32_t handle,
                                      uint32_t ctx_id,
                                      int level,
                                      uint32_t stride,
                                      uint32_t layer_stride,
                                      struct virgl_box *box,
                                      uint64_t offset,
                                      struct iovec *iovec,
                                      unsigned int iovec_cnt)
{
   struct vrend_transfer_info transfer_info;

   transfer_info.handle = handle;
   transfer_info.ctx_id = ctx_id;
   transfer_info.level = level;
   transfer_info.stride = stride;
   transfer_info.layer_stride = layer_stride;
   transfer_info.box = (struct pipe_box *)box;
   transfer_info.offset = offset;
   transfer_info.iovec = iovec;
   transfer_info.iovec_cnt = iovec_cnt;

   return vrend_renderer_transfer_iov(&transfer_info, VREND_TRANSFER_WRITE);
}

int virgl_renderer_transfer_read_iov(uint32_t handle, uint32_t ctx_id,
                                     uint32_t level, uint32_t stride,
                                     uint32_t layer_stride,
                                     struct virgl_box *box,
                                     uint64_t offset, struct iovec *iovec,
                                     int iovec_cnt)
{
   struct vrend_transfer_info transfer_info;

   transfer_info.handle = handle;
   transfer_info.ctx_id = ctx_id;
   transfer_info.level = level;
   transfer_info.stride = stride;
   transfer_info.layer_stride = layer_stride;
   transfer_info.box = (struct pipe_box *)box;
   transfer_info.offset = offset;
   transfer_info.iovec = iovec;
   transfer_info.iovec_cnt = iovec_cnt;

   return vrend_renderer_transfer_iov(&transfer_info, VREND_TRANSFER_READ);
}

int virgl_renderer_resource_attach_iov(int res_handle, struct iovec *iov,
                                       int num_iovs)
{
   return vrend_renderer_resource_attach_iov(res_handle, iov, num_iovs);
}

void virgl_renderer_resource_detach_iov(int res_handle, struct iovec **iov_p, int *num_iovs_p)
{
   return vrend_renderer_resource_detach_iov(res_handle, iov_p, num_iovs_p);
}

int virgl_renderer_create_fence(int client_fence_id, uint32_t ctx_id)
{
   return vrend_renderer_create_fence(client_fence_id, ctx_id);
}

void virgl_renderer_force_ctx_0(void)
{
   vrend_renderer_force_ctx_0();
}

void virgl_renderer_ctx_attach_resource(int ctx_id, int res_handle)
{
   vrend_renderer_attach_res_ctx(ctx_id, res_handle);
}

void virgl_renderer_ctx_detach_resource(int ctx_id, int res_handle)
{
   vrend_renderer_detach_res_ctx(ctx_id, res_handle);
}

int virgl_renderer_resource_get_info(int res_handle,
                                     struct virgl_renderer_resource_info *info)
{
   int ret;
   ret = vrend_renderer_resource_get_info(res_handle, (struct vrend_renderer_resource_info *)info);
#ifdef HAVE_EPOXY_EGL_H
   if (ret == 0 && use_context == CONTEXT_EGL)
      return virgl_egl_get_fourcc_for_texture(egl_info, info->tex_id, info->virgl_format, &info->drm_fourcc);
#endif

   return ret;
}

void virgl_renderer_get_cap_set(uint32_t cap_set, uint32_t *max_ver,
                                uint32_t *max_size)
{
   vrend_renderer_get_cap_set(cap_set, max_ver, max_size);
}

void virgl_renderer_get_rect(int resource_id, struct iovec *iov, unsigned int num_iovs,
                             uint32_t offset, int x, int y, int width, int height)
{
   vrend_renderer_get_rect(resource_id, iov, num_iovs, offset, x, y, width, height);
}


static struct virgl_renderer_callbacks *rcbs;

static void *dev_cookie;

static struct vrend_if_cbs virgl_cbs;

static void virgl_write_fence(uint32_t fence_id)
{
   rcbs->write_fence(dev_cookie, fence_id);
}

static virgl_renderer_gl_context create_gl_context(int scanout_idx, struct virgl_gl_ctx_param *param)
{
   struct virgl_renderer_gl_ctx_param vparam;

#ifdef HAVE_EPOXY_EGL_H
   if (use_context == CONTEXT_EGL)
      return virgl_egl_create_context(egl_info, param);
#endif
#ifdef HAVE_EPOXY_GLX_H
   if (use_context == CONTEXT_GLX)
      return virgl_glx_create_context(glx_info, param);
#endif
   vparam.version = 1;
   vparam.shared = param->shared;
   vparam.major_ver = param->major_ver;
   vparam.minor_ver = param->minor_ver;
   return rcbs->create_gl_context(dev_cookie, scanout_idx, &vparam);
}

static void destroy_gl_context(virgl_renderer_gl_context ctx)
{
#ifdef HAVE_EPOXY_EGL_H
   if (use_context == CONTEXT_EGL)
      return virgl_egl_destroy_context(egl_info, ctx);
#endif
#ifdef HAVE_EPOXY_GLX_H
   if (use_context == CONTEXT_GLX)
      return virgl_glx_destroy_context(glx_info, ctx);
#endif
   return rcbs->destroy_gl_context(dev_cookie, ctx);
}

static int make_current(int scanout_idx, virgl_renderer_gl_context ctx)
{
#ifdef HAVE_EPOXY_EGL_H
   if (use_context == CONTEXT_EGL)
      return virgl_egl_make_context_current(egl_info, ctx);
#endif
#ifdef HAVE_EPOXY_GLX_H
   if (use_context == CONTEXT_GLX)
      return virgl_glx_make_context_current(glx_info, ctx);
#endif
   return rcbs->make_current(dev_cookie, scanout_idx, ctx);
}

static struct vrend_if_cbs virgl_cbs = {
   virgl_write_fence,
   create_gl_context,
   destroy_gl_context,
   make_current,
};

void *virgl_renderer_get_cursor_data(uint32_t resource_id, uint32_t *width, uint32_t *height)
{
   return vrend_renderer_get_cursor_contents(resource_id, width, height);
}

void virgl_renderer_poll(void)
{
   vrend_renderer_check_queries();
   vrend_renderer_check_fences();
}

void virgl_renderer_cleanup(void *cookie)
{
   vrend_renderer_fini();
#ifdef HAVE_EPOXY_EGL_H
   if (use_context == CONTEXT_EGL) {
      virgl_egl_destroy(egl_info);
      egl_info = NULL;
      use_context = CONTEXT_NONE;
   }
#endif
#ifdef HAVE_EPOXY_GLX_H
   if (use_context == CONTEXT_GLX) {
      virgl_glx_destroy(glx_info);
      glx_info = NULL;
      use_context = CONTEXT_NONE;
   }
#endif
}

int virgl_renderer_init(void *cookie, int flags, struct virgl_renderer_callbacks *cbs)
{
   uint32_t renderer_flags = 0;
   if (!cookie || !cbs)
      return -1;

   if (cbs->version != 1)
      return -1;

   dev_cookie = cookie;
   rcbs = cbs;

   if (flags & VIRGL_RENDERER_USE_EGL) {
#ifdef HAVE_EPOXY_EGL_H
      egl_info = virgl_egl_init();
      if (!egl_info)
         return -1;
      use_context = CONTEXT_EGL;
#else
      fprintf(stderr, "EGL is not supported on this platform\n");
      return -1;
#endif
   } else if (flags & VIRGL_RENDERER_USE_GLX) {
#ifdef HAVE_EPOXY_GLX_H
      glx_info = virgl_glx_init();
      if (!glx_info)
         return -1;
      use_context = CONTEXT_GLX;
#else
      fprintf(stderr, "GLX is not supported on this platform\n");
      return -1;
#endif
   }

   if (flags & VIRGL_RENDERER_THREAD_SYNC)
      renderer_flags |= VREND_USE_THREAD_SYNC;

   return vrend_renderer_init(&virgl_cbs, renderer_flags);
}

int virgl_renderer_get_fd_for_texture(uint32_t tex_id, int *fd)
{
   return -1;
}

void virgl_renderer_reset(void)
{
   vrend_renderer_reset();
}

int virgl_renderer_get_poll_fd(void)
{
   return vrend_renderer_get_poll_fd();
}
