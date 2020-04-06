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
#include "OpenGLESDispatch/GLESv1Dispatch.h"

#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/StaticDispatch.h"

#include "android/utils/debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "emugl/common/shared_library.h"

#define DEBUG 0

#if DEBUG
#define DPRINT(...) do { \
    if (!VERBOSE_CHECK(gles1emu)) VERBOSE_ENABLE(gles1emu); \
    VERBOSE_PRINT(gles1emu, __VA_ARGS__); \
} while (0)
#else
#define DPRINT(...)
#endif

static emugl::SharedLibrary *s_gles1_lib = NULL;

// An unimplemented function which prints out an error message.
// To make it consistent with the guest, all GLES1 functions not supported by
// the guest driver should be redirected to this function.

void gles1_unimplemented() {
    fprintf(stderr, "Called unimplemented GLESv1 API\n");
}

#define DEFAULT_GLES_CM_LIB EMUGL_LIBNAME("GLES_CM_translator")
#define DEFAULT_UNDERLYING_GLES_V2_LIB EMUGL_LIBNAME("GLES_V2_translator")

// This section of code (also in GLDispatch.cpp)
// initializes all GLESv1 functions to dummy ones;
// that is, in case we are using a library that doesn't
// have GLESv1, we will still have stubs available to
// signal that they are unsupported on the host.

#define RETURN_void return
#define RETURN_GLboolean return GL_FALSE
#define RETURN_GLint return 0
#define RETURN_GLuint return 0U
#define RETURN_GLenum return 0
#define RETURN_int return 0
#define RETURN_GLconstubyteptr return NULL
#define RETURN_voidptr return NULL

#define RETURN_(x)  RETURN_ ## x

#define DUMMY_MSG "Call to %s: host OpenGL driver does not support OpenGL ES v1. Skipping."

#ifdef _WIN32

#define DEFINE_DUMMY_FUNCTION(return_type, func_name, signature, args) \
static return_type  __stdcall gles1_dummy_##func_name signature { \
    fprintf(stderr, DUMMY_MSG, #func_name); \
    RETURN_(return_type); \
}

#define DEFINE_DUMMY_EXTENSION_FUNCTION(return_type, func_name, signature, args) \
static return_type __stdcall gles1_dummy_##func_name signature { \
    fprintf(stderr, DUMMY_MSG, #func_name); \
    RETURN_(return_type); \
}

#else

#define DEFINE_DUMMY_FUNCTION(return_type, func_name, signature, args) \
static return_type gles1_dummy_##func_name signature { \
    fprintf(stderr, DUMMY_MSG, #func_name); \
    RETURN_(return_type); \
}

#define DEFINE_DUMMY_EXTENSION_FUNCTION(return_type, func_name, signature, args) \
static return_type gles1_dummy_##func_name signature { \
    fprintf(stderr, DUMMY_MSG, #func_name); \
    RETURN_(return_type); \
}

#endif

LIST_GLES1_FUNCTIONS(DEFINE_DUMMY_FUNCTION, DEFINE_DUMMY_EXTENSION_FUNCTION);

//
// This function is called only once during initialiation before
// any thread has been created - hence it should NOT be thread safe.
//

//
// init dummy GLESv1 dispatch table
//
#define ASSIGN_DUMMY(return_type,function_name,signature,callargs) do { \
        dispatch_table-> function_name = gles1_dummy_##function_name; \
        } while(0);

// macro to assign from static library
#define ASSIGN_GLES1_STATIC(return_type,function_name,signature,callargs)\
    dispatch_table-> function_name = reinterpret_cast< function_name ## _t >( \
            translator::gles1::function_name); \
        if ((!dispatch_table-> function_name) && s_egl.eglGetProcAddress) \
        dispatch_table-> function_name = reinterpret_cast< function_name ## _t >( \
            s_egl.eglGetProcAddress(#function_name)); \

bool gles1_dispatch_init_from_static(GLESv1Dispatch* dispatch_table) {
    if (dispatch_table->initialized) return true;

    LIST_GLES1_FUNCTIONS(ASSIGN_GLES1_STATIC, ASSIGN_GLES1_STATIC);

    dispatch_table->initialized = true;
    return true;
}

bool gles1_dispatch_init(GLESv1Dispatch* dispatch_table) {
    if (dispatch_table->initialized) return true;

    const char* useStatic = getenv("ANDROID_EMU_STATIC_TRANSLATOR");
    if (useStatic) {
        return gles1_dispatch_init_from_static(dispatch_table);
    }

    const char* libName = getenv("ANDROID_GLESv1_LIB");
    if (!libName) {
        libName = DEFAULT_GLES_CM_LIB;
    }

    // If emugl_config has detected specifically a backend
    // that supports only GLESv2, set GLESv1 entry points
    // to the dummy functions.
    if (!strcmp(libName, "<gles2_only_backend>")) {

        LIST_GLES1_FUNCTIONS(ASSIGN_DUMMY,ASSIGN_DUMMY)

        DPRINT("assigning dummies because <gles2_only_backend>");
        dispatch_table->initialized = true;
        return true;
    } else {

        char error[256];
        s_gles1_lib = emugl::SharedLibrary::open(libName, error, sizeof(error));
        if (!s_gles1_lib) {
            return gles1_dispatch_init_from_static(dispatch_table);
        }

        //
        // init the GLES dispatch table
        //
#define LOOKUP_SYMBOL(return_type,function_name,signature,callargs) do { \
        dispatch_table-> function_name = reinterpret_cast< function_name ## _t >( \
                s_gles1_lib->findSymbol(#function_name)); \
            if ((!dispatch_table-> function_name) && s_egl.eglGetProcAddress) \
            dispatch_table-> function_name = reinterpret_cast< function_name ## _t >( \
                s_egl.eglGetProcAddress(#function_name)); \
        } while(0); \

        LIST_GLES1_FUNCTIONS(LOOKUP_SYMBOL,LOOKUP_SYMBOL)

        DPRINT("successful");


       dispatch_table->initialized = true;
       return true;
    }
}

//
// This function is called only during initialization of the decoder before
// any thread has been created - hence it should NOT be thread safe.
//
void *gles1_dispatch_get_proc_func(const char *name, void *userData)
{
    void* func = NULL;
    if (s_gles1_lib) {
        func = (void *)s_gles1_lib->findSymbol(name);
    }

    // Check the static functions
    if (!func) {
        func = gles1_dispatch_get_proc_func_static(name);
    }

    // To make it consistent with the guest, redirect any unsupported functions
    // to gles1_unimplemented.
    if (!func) {
        func = (void *)gles1_unimplemented;
    }
    return func;
}
