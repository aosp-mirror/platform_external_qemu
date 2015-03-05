/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "GLESv1Dispatch.h"

#include <stdio.h>
#include <stdlib.h>

#include "emugl/common/shared_library.h"

gles1_decoder_context_t s_gles1;

static emugl::SharedLibrary *s_gles1_lib = NULL;

//
// This function is called only once during initialiation before
// any thread has been created - hence it should NOT be thread safe.
//

#define DEFAULT_GLES_CM_LIB EMUGL_LIBNAME("GLES_CM_translator")

bool init_gles1_dispatch()
{
    const char *libName = getenv("ANDROID_GLESv1_LIB");
    if (!libName) libName = DEFAULT_GLES_CM_LIB;

    s_gles1_lib = emugl::SharedLibrary::open(libName);
    if (!s_gles1_lib) return false;

    //
    // init the GLES dispatch table
    //
    s_gles1.initDispatchByName(gles1_dispatch_get_proc_func, NULL);
    return true;
}

//
// This function is called only during initialiation before
// any thread has been created - hence it should NOT be thread safe.
//
void *gles1_dispatch_get_proc_func(const char *name, void *userData)
{
    if (!s_gles1_lib) {
        return NULL;
    }
    return (void *)s_gles1_lib->findSymbol(name);
}
