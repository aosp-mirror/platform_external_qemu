// Copyright 2018 The Android Open Source Project
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
#pragma once

#include <hardware/fb.h>
#include <hardware/gralloc.h>

// Consolidates all framebuffer device, alloc device,
// and gralloc module operations into one convenient struct.
struct gralloc_implementation {
    void* lib;

    struct alloc_device_t* alloc_dev;
    struct gralloc_module_t* alloc_module;

// Allocator device=============================================================

    int alloc(int w,
              int h,
              int format,
              int usage,
              buffer_handle_t* handle,
              int* stride) {
        return alloc_dev->alloc(alloc_dev, w, h, format, usage, handle, stride);
    }

    int free(buffer_handle_t handle) {
        return alloc_dev->free(alloc_dev, handle);
    }

    void dumpAlloc(char* buffer, int buff_len) {
        if (!alloc_dev->dump) return;
        alloc_dev->dump(alloc_dev, buffer, buff_len);
    }

// Gralloc module===============================================================

    int registerBuffer(buffer_handle_t handle) {
        return alloc_module->registerBuffer(alloc_module, handle);
    }

    int unregisterBuffer(buffer_handle_t handle) {
        return alloc_module->unregisterBuffer(alloc_module, handle);
    }

    int lock(buffer_handle_t handle, int usage,
             int l, int t, int w, int h,
             void** vaddr) {
        return alloc_module->lock(alloc_module, handle, usage, l, t, w, h,
                                  vaddr);
    }

    int unlock(buffer_handle_t handle) {
        return alloc_module->unlock(alloc_module, handle);
    }

    int lock_ycbcr(buffer_handle_t handle,
                   int usage,
                   int l,
                   int t,
                   int w,
                   int h,
                   struct android_ycbcr* ycbcr) {
        return alloc_module->lock_ycbcr(alloc_module, handle, usage, l, t, w, h,
                                        ycbcr);
    }

    int lockAsync(buffer_handle_t handle,
                  int usage,
                  int l,
                  int t,
                  int w,
                  int h,
                  void** vaddr,
                  int fenceFd) {
        return alloc_module->lockAsync(alloc_module, handle, usage, l, t, w, h,
                                       vaddr, fenceFd);
    }

    int unlockAsync(buffer_handle_t handle, int* fenceFd) {
        return alloc_module->unlockAsync(alloc_module, handle, fenceFd);
    }

    int lockAsync_ycbcr(buffer_handle_t handle,
                        int usage,
                        int l,
                        int t,
                        int w,
                        int h,
                        struct android_ycbcr* ycbcr,
                        int fenceFd) {
        return alloc_module->lockAsync_ycbcr(alloc_module, handle, usage, l, t,
                                             w, h, ycbcr, fenceFd);
    }
};

extern "C" {

void load_gralloc_module(
    const char* path,
    struct gralloc_implementation* impl_out);

void unload_gralloc_module(
    const struct gralloc_implementation* impl);

void set_global_gralloc_module(struct gralloc_implementation* impl);

struct gralloc_implementation*
get_global_gralloc_module(void);

} // extern "C"
