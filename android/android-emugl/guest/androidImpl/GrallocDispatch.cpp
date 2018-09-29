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
#include <hardware/fb.h>
#include <hardware/gralloc.h>

#include <dlfcn.h>
#include <stdio.h>

#include "AndroidHostCommon.h"

extern "C" {

EXPORT void load_gralloc_module(
    const char* path,
    struct framebuffer_device_t** fb0Dev,
    struct alloc_device_t** gpu0Dev,
    struct gralloc_module_t** fb0Module,
    struct gralloc_module_t** gpu0Module) {
    void* lib = dlopen(path, RTLD_NOW);

    if (!lib) {
        fprintf(stderr, "%s: failed to load gralloc module from %s\n",
                __func__, lib);
        *fb0Module = nullptr;
        *gpu0Module = nullptr;
        return;
    }

    struct hw_module_t* hw_module =
        (struct hw_module_t*)dlsym(lib, HAL_MODULE_INFO_SYM_AS_STR);

    framebuffer_open(hw_module, fb0Dev);
    gralloc_open(hw_module, gpu0Dev);

    *fb0Module = reinterpret_cast<gralloc_module_t*>((*fb0Dev)->common.module);
    *gpu0Module = reinterpret_cast<gralloc_module_t*>((*gpu0Dev)->common.module);
}

} // extern "C"
