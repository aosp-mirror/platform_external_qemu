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
// limitations under the License.#pragma once
#pragma once

#include <hardware/hardware.h>
#include <hardware/gralloc.h>
#include <cutils/native_handle.h>

typedef void* (*gralloc_open_device_t)(const char* name);
typedef int (*gralloc_alloc_t)(void* device, int w, int h, int format, int usage, buffer_handle_t* pHandle, int* stride);
typedef int (*gralloc_free_t)(void* device, buffer_handle_t handle);
typedef int (*gralloc_register_t)(void* module, buffer_handle_t handle);
typedef int (*gralloc_unregister_t)(void* module, buffer_handle_t handle);
typedef int (*gralloc_lock_t)(void* module, buffer_handle_t handle, int usage, int l, int t, int w, int h, void** vaddr);
typedef int (*gralloc_unlock_t)(void* module, buffer_handle_t handle);

struct GrallocDispatch {
    gralloc_open_device_t open_device;
    gralloc_alloc_t alloc;
    gralloc_free_t free;
    gralloc_register_t register_buffer;
    gralloc_unregister_t unregister_buffer;
    gralloc_lock_t lock;
    gralloc_unlock_t unlock;
};

namespace aemu {

void openGrallocLib(const char* libName, GrallocDispatch* dispatch_out);

} // namespace aemu
