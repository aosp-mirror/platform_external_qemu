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

#include <stdio.h>
#include <stdlib.h>

#include "emugl/common/shared_library.h"

static emugl::SharedLibrary *s_gles1_lib = NULL;

//
// This function is called only once during initialiation before
// any thread has been created - hence it should NOT be thread safe.
//

#define DEFAULT_GLES_CM_LIB EMUGL_LIBNAME("GLES_CM_translator")

#define RETURN_void return
#define RETURN_GLboolean return GL_FALSE
#define RETURN_GLint return 0
#define RETURN_GLuint return 0U
#define RETURN_GLenum return 0
#define RETURN_int return 0
#define RETURN_GLconstubyteptr return NULL

#define RETURN_(x)  RETURN_ ## x

#define DEFINE_DUMMY_FUNCTION(return_type, func_name, signature, args) \
static return_type gles1_dummy_##func_name signature { \
    fprintf(stderr, "dummygles1!%s\n", #func_name); \
    RETURN_(return_type); \
}

#define DEFINE_DUMMY_EXTENSION_FUNCTION(return_type, func_name, signature, args) \
static return_type gles1_dummy_##func_name signature { \
    fprintf(stderr, "dummygles1_ext!%s\n", #func_name); \
    RETURN_(return_type); \
}

LIST_GLES1_FUNCTIONS(DEFINE_DUMMY_FUNCTION, DEFINE_DUMMY_EXTENSION_FUNCTION)

bool gles1_dispatch_init(GLESv1Dispatch* dispatch_table) {
    //
    // init the GLES dispatch table
    //
#define LOOKUP_SYMBOL(return_type,function_name,signature,callargs) \
    dispatch_table-> function_name = gles1_dummy_##function_name;

    LIST_GLES1_FUNCTIONS(LOOKUP_SYMBOL,LOOKUP_SYMBOL)

    return true;
}

//
// This function is called only during initialization of the decoder before
// any thread has been created - hence it should NOT be thread safe.
//
void *gles1_dispatch_get_proc_func(const char *name, void *userData)
{
    if (!s_gles1_lib) {
        return NULL;
    }
    return (void *)s_gles1_lib->findSymbol(name);
}
