/*
 * Copyright 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// WARNING: This file is generated. See ../README.md for instructions.

#include "vk_dispatch_gen.h"

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>

static VkDllDispatch s_vkDll;
static VkGlobalDispatch s_vkGlobal;
static bool initialized = false;

#ifdef _WIN32
#define DLL_SUFFIX ".dll"
#elif defined(__APPLE__)
#define DLL_SUFFIX ".dylib"
#else
#define DLL_SUFFIX ".so"
#endif

#define VULKAN_SDK_PATH "./vulkan-libs"

static const char vkLibPath[] =
    VULKAN_SDK_PATH "/libvulkan" DLL_SUFFIX;

// Initializes DLL and global-level Vulkan functions.
void initVkDispatch() {
    if (initialized) return;

    void* lib = dlopen(vkLibPath, RTLD_NOW);

#define LOAD_VK_DLL_FUNC(funcname) do { \
    void* addr = dlsym(lib, #funcname); \
    if (addr) { \
        s_vkDll. funcname = (funcname##_func_t)addr; \
    } else { \
        fprintf(stderr, "%s: dll level: could not find %s\n", __func__, #funcname); \
    } } while(0); \


    LIST_VK_DLL_FUNCS(LOAD_VK_DLL_FUNC);

#define LOAD_VK_GLOBAL_FUNC(funcname) do { \
    PFN_vkVoidFunction addr = s_vkDll.vkGetInstanceProcAddr(nullptr, #funcname); \
    if (addr) { \
        s_vkGlobal. funcname = (funcname##_func_t)addr; \
    } else { \
    } } while(0); \

    LIST_VK_GLOBAL_FUNCS(LOAD_VK_GLOBAL_FUNC);
    initialized = true;

    fprintf(stderr, "%s: loaded Vulkan dispatch from host driver\n", __func__);
}

VkInstanceDispatch initVkInstanceDispatch(VkInstance instance) {
    VkInstanceDispatch res;
    res.instance = instance;

#define LOAD_VK_INSTANCE_FUNC(funcname) do { \
    void* addr = (void*)s_vkDll.vkGetInstanceProcAddr(res.instance, #funcname); \
    if (addr) { \
        res. funcname = (funcname##_func_t)addr; \
    } else { \
    } } while(0); \

    LIST_VK_INSTANCE_FUNCS(LOAD_VK_INSTANCE_FUNC);

    return res;
}

VkDeviceDispatch initVkDeviceDispatch(const VkInstanceDispatch& vki, VkDevice device) {
    VkDeviceDispatch res;
    res.device = device;
 
#define LOAD_VK_DEVICE_FUNC(funcname) do { \
    void* addr = (void*)vki.vkGetDeviceProcAddr(res.device, #funcname); \
    if (addr) { \
        res. funcname = (funcname##_func_t)addr; \
    } else { \
    } } while(0); \

    LIST_VK_DEVICE_FUNCS(LOAD_VK_DEVICE_FUNC);

    return res;
}

VkDllDispatch* vkDispatcher() {
    return &s_vkDll;
}

VkGlobalDispatch* vkGlobalDispatcher() {
    return &s_vkGlobal;
}
