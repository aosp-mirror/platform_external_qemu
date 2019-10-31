// Copyright 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "hw/misc/goldfish_pipe.h"
#include "hw/virtio/virtio-goldfish-pipe.h"

extern "C" {

static int pipe_virgl_renderer_init(
    void *cookie, int flags, struct virgl_renderer_callbacks *cb) {
    return 0;
}

static void pipe_virgl_renderer_poll(void) { }

static void* pipe_virgl_renderer_get_cursor_data(
    uint32_t resource_id, uint32_t *width, uint32_t *height) {
    return 0;
}

static int pipe_virgl_renderer_resource_create(
    struct virgl_renderer_resource_create_args *args,
    struct iovec *iov, uint32_t num_iovs) {
    return 0;
}

static void pipe_virgl_renderer_resource_unref(uint32_t res_handle) {
}

static int pipe_virgl_renderer_context_create(
    uint32_t handle, uint32_t nlen, const char *name) {
    return 0;
}

static void pipe_virgl_renderer_context_destroy(uint32_t handle) {
}

static int pipe_virgl_renderer_submit_cmd(void *buffer,
                                           int ctx_id,
                                           int ndw) {
    return 0;
}

static int pipe_virgl_renderer_transfer_read_iov(
    uint32_t handle, uint32_t ctx_id,
    uint32_t level, uint32_t stride,
    uint32_t layer_stride,
    struct virgl_box *box,
    uint64_t offset, struct iovec *iov,
    int iovec_cnt) {
    return 0;
}

static int pipe_virgl_renderer_transfer_write_iov(
    uint32_t handle,
    uint32_t ctx_id,
    int level,
    uint32_t stride,
    uint32_t layer_stride,
    struct virgl_box *box,
    uint64_t offset,
    struct iovec *iovec,
    unsigned int iovec_cnt) {
    return 0;
}

static void pipe_virgl_renderer_get_cap_set(uint32_t set, uint32_t *max_ver,
                                             uint32_t *max_size) {
}

static void pipe_virgl_renderer_fill_caps(uint32_t set, uint32_t version,
                                           void *caps) {
}

static int pipe_virgl_renderer_resource_attach_iov(
    int res_handle, struct iovec *iov,
    int num_iovs) {
    return 0;
}

static void pipe_virgl_renderer_resource_detach_iov(
    int res_handle, struct iovec **iov, int *num_iovs) {
}

static int pipe_virgl_renderer_create_fence(
    int client_fence_id, uint32_t ctx_id) {
    return 0;
}

static void pipe_virgl_renderer_force_ctx_0(void) {
}

static void pipe_virgl_renderer_ctx_attach_resource(
    int ctx_id, int res_handle) {

}

static void pipe_virgl_renderer_ctx_detach_resource(
    int ctx_id, int res_handle) {
}

static int pipe_virgl_renderer_resource_get_info(
    int res_handle,
    struct virgl_renderer_resource_info *info) {
    return 0;
}

#define VIRGLRENDERER_API_PIPE_STRUCT_DEF(api) pipe_##api,

static struct virgl_renderer_virtio_interface s_virtio_interface = {
    LIST_VIRGLRENDERER_API(VIRGLRENDERER_API_PIPE_STRUCT_DEF)
};

static void pipe_virgl_write_fence(void *opaque, uint32_t fence)
{
    virgl_write_fence(opaque, fence);
}

static virgl_renderer_gl_context
pipe_virgl_create_context(void *opaque, int scanout_idx,
                     struct virgl_renderer_gl_ctx_param *params)
{
    return (virgl_renderer_gl_context)0;
}

static void pipe_virgl_destroy_context(
    void *opaque, virgl_renderer_gl_context ctx)
{ }

static int pipe_virgl_make_context_current(
    void *opaque, int scanout_idx, virgl_renderer_gl_context ctx)
{ return 0; }

static struct virgl_renderer_callbacks s_pipe_3d_cbs = {
    .version             = 1,
    .write_fence         = pipe_virgl_write_fence,
    .create_gl_context   = pipe_virgl_create_context,
    .destroy_gl_context  = pipe_virgl_destroy_context,
    .make_current        = pipe_virgl_make_context_current,
};

struct virgl_renderer_virtio_interface*
get_goldfish_pipe_virgl_renderer_virtio_interface(void) {
    return &s_virtio_interface;
}

struct virgl_renderer_callbacks*
get_goldfish_pipe_virgl_renderer_callbacks(void) {
    return &s_pipe_3d_cbs;
}

} // extern "C"
