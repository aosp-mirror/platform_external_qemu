// Copyright (C) 2020 The Android Open Source Project
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
#ifndef ANDROID_VIRTIO_GPU_OPS
#define ANDROID_VIRTIO_GPU_OPS

/* virtio-gpu interface for color buffers
 * (triggered by minigbm/egl calling virtio-gpu ioctls) */
typedef void (*create_color_buffer_with_handle_t)(
    uint32_t width,
    uint32_t height,
    uint32_t format,
    uint32_t fwkFormat,
    uint32_t handle);

/* create YUV textures with given width and height
   type: FRAMEWORK_FORMAT_NV12 or FRAMEWORK_FORMAT_YUV_420_888
   count: how many set of textures will be created
   output: caller allocated storage to hold results: size should
   be 2*count for NV12 or 3*cout for YUV_420_888
 */
typedef void (*create_yuv_textures_t)(uint32_t type,
                                      uint32_t count,
                                      int width,
                                      int height,
                                      uint32_t* output);

/* destroy YUV textures */
typedef void (*destroy_yuv_textures_t)(uint32_t type,
                                       uint32_t count,
                                       uint32_t* textures);
/* call the user provided func with privData to update the content
   of textures.
   type: FRAMEWORK_FORMAT_NV12 or FRAMEWORK_FORMAT_YUV_420_888
   textures: size 2 (NV12) or 3 (YUV420)
 */
typedef void (*update_yuv_textures_t)(uint32_t type,
                                      uint32_t* textures,
                                      void* privData,
                                      void* func);

/* swap out the yuv textures of the colorbuffer, and use the new yuv textures
 * to update colorbuffer content; on return, textures will have the retired
 * yuv textures that are free to hold new data
 */
typedef void (*swap_textures_and_update_color_buffer_t)(
        uint32_t colorbufferhandle,
        int x,
        int y,
        int width,
        int height,
        uint32_t format,
        uint32_t type,
        uint32_t texture_type,
        uint32_t* textures);

typedef void (*open_color_buffer_t)(uint32_t handle);
typedef void (*close_color_buffer_t)(uint32_t handle);
typedef void (*update_color_buffer_t)(
    uint32_t handle, int x, int y, int width, int height,
    uint32_t format, uint32_t type, void* pixels);
typedef void (*read_color_buffer_t)(
    uint32_t handle, int x, int y, int width, int height,
    uint32_t format, uint32_t type, void* pixels);
typedef void (*read_color_buffer_yuv_t)(
    uint32_t handle, int x, int y, int width, int height,
    void* pixels, uint32_t pixels_size);
typedef void (*post_color_buffer_t)(uint32_t handle);
typedef void (*repost_t)(void);
typedef uint32_t (*get_last_posted_color_buffer_t)(void);
typedef void (*bind_color_buffer_to_texture_t)(uint32_t);
typedef void* (*get_global_egl_context_t)(void);
typedef void (*wait_for_gpu_t)(uint64_t eglsync);
typedef void (*wait_for_gpu_vulkan_t)(uint64_t device, uint64_t fence);

struct AndroidVirtioGpuOps {
    create_color_buffer_with_handle_t create_color_buffer_with_handle;
    open_color_buffer_t open_color_buffer;
    close_color_buffer_t close_color_buffer;
    update_color_buffer_t update_color_buffer;
    read_color_buffer_t read_color_buffer;
    read_color_buffer_yuv_t read_color_buffer_yuv;
    post_color_buffer_t post_color_buffer;
    repost_t repost;
    /* yuv texture related */
    create_yuv_textures_t create_yuv_textures;
    destroy_yuv_textures_t destroy_yuv_textures;
    update_yuv_textures_t update_yuv_textures;
    swap_textures_and_update_color_buffer_t
            swap_textures_and_update_color_buffer;
    /* virtual device widget related */
    get_last_posted_color_buffer_t get_last_posted_color_buffer;
    bind_color_buffer_to_texture_t bind_color_buffer_to_texture;
    get_global_egl_context_t get_global_egl_context;
    wait_for_gpu_t wait_for_gpu;
    wait_for_gpu_vulkan_t wait_for_gpu_vulkan;
};

#endif // ANDROID_VIRTIO_GPU_OPS
