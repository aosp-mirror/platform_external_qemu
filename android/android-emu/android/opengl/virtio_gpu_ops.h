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

struct AndroidVirtioGpuOps {
    create_color_buffer_with_handle_t create_color_buffer_with_handle;
    open_color_buffer_t open_color_buffer;
    close_color_buffer_t close_color_buffer;
    update_color_buffer_t update_color_buffer;
    read_color_buffer_t read_color_buffer;
    read_color_buffer_yuv_t read_color_buffer_yuv;
    post_color_buffer_t post_color_buffer;
};

#endif // ANDROID_VIRTIO_GPU_OPS
