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
typedef int (*fb_post_t)(void* device, buffer_handle_t handle);

struct GrallocDispatch {
    gralloc_open_device_t open_device;
    gralloc_alloc_t alloc;
    gralloc_free_t free;
    gralloc_register_t register_buffer;
    gralloc_unregister_t unregister_buffer;
    gralloc_lock_t lock;
    gralloc_unlock_t unlock;
    fb_post_t fbpost;
};

void init_gralloc_dispatch(const char* libName, GrallocDispatch* dispatch_out, void* libOut);
