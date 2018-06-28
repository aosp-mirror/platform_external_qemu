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
#include "OpenGLESDispatch/EGLDispatch.h"

#include "emugl/common/shared_library.h"

#include <stdio.h>
#include <stdlib.h>

EGLDispatch s_egl;
emugl::SharedLibrary* s_egl_lib = nullptr;

#define DEFAULT_EGL_LIB EMUGL_LIBNAME("EGL_translator")

#define RENDER_EGL_LOAD_FIELD(return_type, function_name, signature) \
    egl_dispatch-> function_name = (function_name ## _t) lib->findSymbol(#function_name); \

#define RENDER_EGL_LOAD_FIELD_WITH_EGL(return_type, function_name, signature) \
    if ((!egl_dispatch-> function_name) && egl_dispatch->eglGetProcAddress) egl_dispatch-> function_name = \
            (function_name ## _t) egl_dispatch->eglGetProcAddress(#function_name); \

#define RENDER_EGL_LOAD_OPTIONAL_FIELD(return_type, function_name, signature) \
    if (egl_dispatch->eglGetProcAddress) egl_dispatch-> function_name = \
            (function_name ## _t) egl_dispatch->eglGetProcAddress(#function_name); \
    if (!egl_dispatch->function_name || !egl_dispatch->eglGetProcAddress) \
            RENDER_EGL_LOAD_FIELD(return_type, function_name, signature)

bool init_egl_dispatch() {
    const char *libName = getenv("ANDROID_EGL_LIB");
    if (!libName) libName = DEFAULT_EGL_LIB;
    return init_egl_dispatch_from(libName, &s_egl, &s_egl_lib);
}

bool init_egl_dispatch_from(const char* libPath, EGLDispatch* egl_dispatch, void* library_out) {
    char error[256];
    emugl::SharedLibrary *lib = emugl::SharedLibrary::open(libPath, error, sizeof(error));
    if (!lib) {
        printf("Failed to open %s: [%s]\n", libPath, error);
        return false;
    }

    *(emugl::SharedLibrary**)library_out = lib;

    LIST_RENDER_EGL_FUNCTIONS(RENDER_EGL_LOAD_FIELD)
    LIST_RENDER_EGL_FUNCTIONS(RENDER_EGL_LOAD_FIELD_WITH_EGL)
    LIST_RENDER_EGL_EXTENSIONS_FUNCTIONS(RENDER_EGL_LOAD_OPTIONAL_FIELD)
    LIST_RENDER_EGL_SNAPSHOT_FUNCTIONS(RENDER_EGL_LOAD_FIELD)

    return true;
}
