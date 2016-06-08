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
#include "OpenGLESDispatch/GLESv2Dispatch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "emugl/common/shared_library.h"

static emugl::SharedLibrary *s_gles1_lib = NULL;
static emugl::SharedLibrary *s_underlying_gles2_lib = NULL;

// An unimplemented function which prints out an error message.
// To make it consistent with the guest, all GLES1 functions not supported by
// the guest driver should be redirected to this function.

static void gles1_unimplemented() {
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
LIST_GLES12_TR_FUNCTIONS(DEFINE_DUMMY_FUNCTION);

//
// This function is called only once during initialiation before
// any thread has been created - hence it should NOT be thread safe.
//

        //
        // init dummy GLESv1 dispatch table
        //
#define ASSIGN_DUMMY(return_type,function_name,signature,callargs) do { \
        fprintf(stderr, "%s: dummying %s\n", __FUNCTION__, #function_name); \
        dispatch_table-> function_name = gles1_dummy_##function_name; \
        } while(0);
bool gles1_dispatch_init(GLESv1Dispatch* dispatch_table) {
    const char* libName = getenv("ANDROID_GLESv1_LIB");
    if (!libName) {
        libName = DEFAULT_GLES_CM_LIB;
    }

    // If emugl_config has detected specifically a backend
    // that supports only GLESv2, set GLESv1 entry points
    // to the dummy functions.
    if (!strcmp(libName, "<gles2_only_backend>")) {

        LIST_GLES1_FUNCTIONS(ASSIGN_DUMMY,ASSIGN_DUMMY)

            fprintf(stderr, "%s: assigning dummies because <gles2_only_backend>\n", __FUNCTION__);
            return true;
    } else {

        char error[256];
        s_gles1_lib = emugl::SharedLibrary::open(libName, error, sizeof(error));
        if (!s_gles1_lib) {
            fprintf(stderr, "%s: Could not load %s [%s]\n", __FUNCTION__,
                    libName, error);
            return false;
        }

        //
        // init the GLES dispatch table
        //
#define LOOKUP_SYMBOL(return_type,function_name,signature,callargs) do { \
        fprintf(stderr, "%s: loading %s...", __FUNCTION__, #function_name); \
        dispatch_table-> function_name = reinterpret_cast< function_name ## _t >( \
                s_gles1_lib->findSymbol(#function_name)); \
        fprintf(stderr, "%p\n", dispatch_table-> function_name); \
        } while(0);

        LIST_GLES1_FUNCTIONS(LOOKUP_SYMBOL,LOOKUP_SYMBOL)

            fprintf(stderr, "%s successful\n", __FUNCTION__);

            LIST_GLES12_TR_FUNCTIONS(ASSIGN_DUMMY);
        // If we are using the translator, additionally list the gles1 translator interface
        //
            if (strstr(libName, "GLES12Translator")) {

                fprintf(stderr, "%s: trying to assign gles12-specific functoins\n", __FUNCTION__);
                LIST_GLES12_TR_FUNCTIONS(LOOKUP_SYMBOL);
                fprintf(stderr, "%s: hopefully, successfully assigned 12tr-specific functions...\n", __FUNCTION__);

                fprintf(stderr, "%s: Now creating the underlying api\n", __FUNCTION__);
                UnderlyingApis* gles2api = (UnderlyingApis*)dispatch_table->create_underlying_api();

#define SET_UNDERLYING_GLES2_FUNC(rett, function_name, sig, callargs) do { \
    fprintf(stderr, "%s: set underlying gles2 %s...", __FUNCTION__, #function_name); \
    gles2api->angle-> function_name = reinterpret_cast< function_name ## _t >(s_underlying_gles2_lib->findSymbol(#function_name)); \
    fprintf(stderr, "%p\n", gles2api->angle-> function_name); \
} while(0);

                fprintf(stderr, "%s: should be using gles12 translator\n", __FUNCTION__);
                fprintf(stderr, "%s: trying to get gles2 lib\n", __FUNCTION__);
                const char* underlying_gles2_lib_name = getenv("ANDROID_GLESv2_LIB");
                if (!underlying_gles2_lib_name) {
                    underlying_gles2_lib_name = DEFAULT_UNDERLYING_GLES_V2_LIB;
                }
                s_underlying_gles2_lib = emugl::SharedLibrary::open(underlying_gles2_lib_name, error, sizeof(error));
                if (!s_underlying_gles2_lib) {
                    fprintf(stderr, "%s: Could not load underlying gles2 lib %s [%s]\n", __FUNCTION__,
                            libName, error);
                    return false;
                }
                fprintf(stderr, "%s: done trying to get gles2 lib\n", __FUNCTION__);

                LIST_GLES2_FUNCTIONS(SET_UNDERLYING_GLES2_FUNC, SET_UNDERLYING_GLES2_FUNC);
                // LIST_GLES12TR_FUNCTIONS(LOOKUP_SYMBOL, LOOKUP_SYMBOL);
            }
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
    // To make it consistent with the guest, redirect any unsupported functions
    // to gles1_unimplemented.
    if (!func) {
        func = (void *)gles1_unimplemented;
    }
    return func;
}
