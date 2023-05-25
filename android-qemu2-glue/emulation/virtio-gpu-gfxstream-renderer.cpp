
// Copyright 2023 The Android Open Source Project
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

#include <functional>

extern "C" {
#include "android/utils/dll.h"
#include "qemu/osdep.h"
#include "host-common/goldfish_pipe.h"
#include "render-utils/virtio-gpu-gfxstream-renderer.h"
#include "render-utils/virtio-gpu-gfxstream-renderer-unstable.h"
}  // extern "C"

#define RENDERER_LIB_NAME "libgfxstream_backend"
#ifdef VIRTIO_GOLDFISH_EXPORT_API
#ifdef _WIN32
#define VG_EXPORT __declspec(dllexport)
#else
#define VG_EXPORT __attribute__((visibility("default")))
#endif
#else
#define VG_EXPORT
#endif // !VIRTIO_GOLDFISH_EXPORT_API

#define LIST_STREAM_RENDERER_API(f) \
f(stream_renderer_init) \
f(stream_renderer_teardown) \
f(stream_renderer_resource_create) \
f(stream_renderer_resource_unref) \
f(stream_renderer_context_destroy) \
f(stream_renderer_submit_cmd) \
f(stream_renderer_transfer_read_iov) \
f(stream_renderer_transfer_write_iov) \
f(stream_renderer_get_cap_set) \
f(stream_renderer_fill_caps) \
f(stream_renderer_resource_attach_iov) \
f(stream_renderer_resource_detach_iov) \
f(stream_renderer_ctx_attach_resource) \
f(stream_renderer_ctx_detach_resource) \
f(stream_renderer_resource_get_info) \
f(stream_renderer_flush) \
f(stream_renderer_create_blob) \
f(stream_renderer_export_blob) \
f(stream_renderer_resource_map) \
f(stream_renderer_resource_unmap) \
f(stream_renderer_context_create) \
f(stream_renderer_create_fence) \
f(stream_renderer_platform_import_resource) \
f(stream_renderer_platform_resource_info) \
f(stream_renderer_platform_create_shared_egl_context) \
f(stream_renderer_platform_destroy_shared_egl_context) \
f(stream_renderer_resource_map_info) \
f(stream_renderer_vulkan_info) \
f(stream_renderer_set_service_ops) \

#define FUNCTION_(api) \
        decltype(api)* api;
struct stream_renderer_funcs {
    LIST_STREAM_RENDERER_API(FUNCTION_)
};
#undef FUNCTION_
static struct stream_renderer_funcs s_render;

extern "C" {
int goldfish_virtio_init(void) {
    char* error = NULL;
    int ret = -1;
    ADynamicLibrary* rendererSo = adynamicLibrary_open(RENDERER_LIB_NAME, &error);
    if (rendererSo == NULL) {
        fprintf(stderr, "Could not open %s\n", RENDERER_LIB_NAME);
        goto BAD_EXIT;
    }
    void* symbol;
#define FUNCTION_(api) \
    symbol = adynamicLibrary_findSymbol(rendererSo, #api, &error); \
    if (symbol != NULL) { \
        using type = decltype(api); \
        s_render.api = reinterpret_cast<type*>(symbol); \
    } else { \
        fprintf(stderr, "Could not find required symbol (%s): %s\n", #api, error); \
        free(error); \
        goto BAD_EXIT; \
    }
    LIST_STREAM_RENDERER_API(FUNCTION_)
#undef FUNCTION_
    ret = 0;
    s_render.stream_renderer_set_service_ops(goldfish_pipe_get_service_ops());
BAD_EXIT:
    adynamicLibrary_close(rendererSo);
    return ret;
}

VG_EXPORT int stream_renderer_resource_create(struct stream_renderer_resource_create_args* args,
                                              struct iovec* iov, uint32_t num_iovs) {
    return s_render.stream_renderer_resource_create(args, iov, num_iovs);
}

VG_EXPORT void stream_renderer_resource_unref(uint32_t res_handle) {
    s_render.stream_renderer_resource_unref(res_handle);
}

VG_EXPORT void stream_renderer_context_destroy(uint32_t handle) {
    s_render.stream_renderer_context_destroy(handle);
}

VG_EXPORT int stream_renderer_submit_cmd(void* buffer, int ctx_id, int dwordCount) {
    return s_render.stream_renderer_submit_cmd(buffer, ctx_id, dwordCount);
}

VG_EXPORT int stream_renderer_transfer_read_iov(uint32_t handle, uint32_t ctx_id, uint32_t level,
                                                uint32_t stride, uint32_t layer_stride,
                                                struct stream_renderer_box* box, uint64_t offset,
                                                struct iovec* iov, int iovec_cnt) {
    return s_render.stream_renderer_transfer_read_iov(
            handle, ctx_id, level, stride, layer_stride, box, offset, iov, iovec_cnt);
}

VG_EXPORT int stream_renderer_transfer_write_iov(uint32_t handle, uint32_t ctx_id, int level,
                                                 uint32_t stride, uint32_t layer_stride,
                                                 struct stream_renderer_box* box, uint64_t offset,
                                                 struct iovec* iovec, unsigned int iovec_cnt) {
    return s_render.stream_renderer_transfer_write_iov(
            handle, ctx_id, level, stride, layer_stride, box, offset, iovec, iovec_cnt);
}

VG_EXPORT void stream_renderer_get_cap_set(uint32_t set, uint32_t* max_ver, uint32_t* max_size) {
    return s_render.stream_renderer_get_cap_set(set, max_ver, max_size);
}

VG_EXPORT void stream_renderer_fill_caps(uint32_t set, uint32_t version, void* caps) {
    return s_render.stream_renderer_fill_caps(set, version, caps);
}

VG_EXPORT int stream_renderer_resource_attach_iov(int res_handle, struct iovec* iov, int num_iovs) {
    return s_render.stream_renderer_resource_attach_iov(res_handle, iov, num_iovs);
}

VG_EXPORT void stream_renderer_resource_detach_iov(int res_handle, struct iovec** iov,
                                                   int* num_iovs) {
    return s_render.stream_renderer_resource_detach_iov(res_handle, iov, num_iovs);
}

VG_EXPORT void stream_renderer_ctx_attach_resource(int ctx_id, int res_handle) {
    s_render.stream_renderer_ctx_attach_resource(ctx_id, res_handle);
}

VG_EXPORT void stream_renderer_ctx_detach_resource(int ctx_id, int res_handle) {
    s_render.stream_renderer_ctx_detach_resource(ctx_id, res_handle);
}

VG_EXPORT int stream_renderer_resource_get_info(int res_handle,
                                                struct stream_renderer_resource_info* info) {
    return s_render.stream_renderer_resource_get_info(res_handle, info);
}

VG_EXPORT void stream_renderer_flush(uint32_t res_handle) {
    s_render.stream_renderer_flush(res_handle);
}

VG_EXPORT int stream_renderer_create_blob(uint32_t ctx_id, uint32_t res_handle,
                                          const struct stream_renderer_create_blob* create_blob,
                                          const struct iovec* iovecs, uint32_t num_iovs,
                                          const struct stream_renderer_handle* handle) {
    return s_render.stream_renderer_create_blob(
            ctx_id, res_handle, create_blob, iovecs, num_iovs, handle);
}

VG_EXPORT int stream_renderer_export_blob(uint32_t res_handle,
                                          struct stream_renderer_handle* handle) {
    return s_render.stream_renderer_export_blob(res_handle, handle);
}

VG_EXPORT int stream_renderer_resource_map(uint32_t res_handle, void** hvaOut, uint64_t* sizeOut) {
    return s_render.stream_renderer_resource_map(res_handle, hvaOut, sizeOut);
}

VG_EXPORT int stream_renderer_resource_unmap(uint32_t res_handle) {
    return s_render.stream_renderer_resource_unmap(res_handle);
}

VG_EXPORT int stream_renderer_context_create(uint32_t ctx_id, uint32_t nlen, const char* name,
                                             uint32_t context_init) {
    return s_render.stream_renderer_context_create(ctx_id, nlen, name, context_init);
}

VG_EXPORT int stream_renderer_create_fence(const struct stream_renderer_fence* fence) {
    return s_render.stream_renderer_create_fence(fence);
}

VG_EXPORT int stream_renderer_platform_import_resource(int res_handle, int res_info,
                                                       void* resource) {
    return s_render.stream_renderer_platform_import_resource(res_handle, res_info, resource);
}

VG_EXPORT int stream_renderer_platform_resource_info(int res_handle, int* width, int* height,
                                                     int* internal_format) {
    return s_render.stream_renderer_platform_resource_info(
            res_handle, width, height, internal_format);
}

VG_EXPORT void* stream_renderer_platform_create_shared_egl_context() {
    return s_render.stream_renderer_platform_create_shared_egl_context();
}

VG_EXPORT int stream_renderer_platform_destroy_shared_egl_context(void* context) {
    return s_render.stream_renderer_platform_destroy_shared_egl_context(context);
}

VG_EXPORT int stream_renderer_resource_map_info(uint32_t res_handle, uint32_t* map_info) {
    return s_render.stream_renderer_resource_map_info(res_handle, map_info);
}

VG_EXPORT int stream_renderer_vulkan_info(uint32_t res_handle,
                                          struct stream_renderer_vulkan_info* vulkan_info) {
    return s_render.stream_renderer_vulkan_info(res_handle, vulkan_info);
}

VG_EXPORT int stream_renderer_init(struct stream_renderer_param* stream_renderer_params,
                                   uint64_t num_params) {
    return s_render.stream_renderer_init(stream_renderer_params, num_params);
}

VG_EXPORT void stream_renderer_teardown() {
    s_render.stream_renderer_teardown();
}

VG_EXPORT void stream_renderer_set_service_ops(const GoldfishPipeServiceOps* ops) {
    s_render.stream_renderer_set_service_ops(ops);
}

}  // extern "C"
