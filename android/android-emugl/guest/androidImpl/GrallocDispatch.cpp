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
#include "GrallocDispatch.h"

#include "AndroidHostCommon.h"
#include "Ashmem.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

static struct gralloc_implementation* s_global_gralloc_impl = 0;

extern "C" {

EXPORT void load_gralloc_module(
    const char* path,
    struct gralloc_implementation* impl_out) {

    memset(impl_out, 0x0, sizeof(*impl_out));

    impl_out->lib = dlopen(path, RTLD_NOW);

    if (!impl_out->lib) {
        fprintf(stderr, "%s: failed to load gralloc module from %s\n",
                __func__, impl_out->lib);
        return;
    }

    struct hw_module_t* hw_module = (struct hw_module_t*)dlsym(
            impl_out->lib, HAL_MODULE_INFO_SYM_AS_STR);

    framebuffer_open(hw_module, &impl_out->fb_dev);

    if (!impl_out->fb_dev) {
        fprintf(stderr, "%s: Failed to load fb device from %p\n",
                __func__, impl_out->lib);
        return;
    }

    gralloc_open(hw_module, &impl_out->alloc_dev);

    if (!impl_out->alloc_dev) {
        fprintf(stderr, "%s: Failed to load alloc device from %p\n",
                __func__, impl_out->lib);
        return;
    }

    impl_out->fb_module = reinterpret_cast<gralloc_module_t*>(
        impl_out->fb_dev->common.module);
    impl_out->alloc_module = reinterpret_cast<gralloc_module_t*>(
        impl_out->alloc_dev->common.module);
}

EXPORT void unload_gralloc_module(
    const struct gralloc_implementation* impl) {

    if (!impl->lib) return;

    gralloc_close(impl->alloc_dev);
    framebuffer_close(impl->fb_dev);

    dlclose(impl->lib);

    ashmem_teardown();

    if (s_global_gralloc_impl == impl) {
        s_global_gralloc_impl = 0;
    }
}

EXPORT void set_global_gralloc_module(
    struct gralloc_implementation* impl) {
    s_global_gralloc_impl = impl;
}

EXPORT struct gralloc_implementation*
get_global_gralloc_module(void) {
    return s_global_gralloc_impl;
}

} // extern "C"
