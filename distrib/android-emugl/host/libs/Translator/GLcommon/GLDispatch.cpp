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

#include <GLcommon/GLDispatch.h>

#include "emugl/common/shared_library.h"

#ifdef __linux__
#include <GL/glx.h>
#elif defined(WIN32)
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Generic function type.
typedef void (*GL_FUNC_PTR)();

// GL symbol resolver function type.
typedef GL_FUNC_PTR (Resolver)(const char* funcName);

static GL_FUNC_PTR getGLFuncAddress(const char *funcName) {
    GL_FUNC_PTR ret = NULL;
    static emugl::SharedLibrary* s_libGL = NULL;
    static Resolver* s_resolver = NULL;

    if (!s_libGL) {
#if defined(__APPLE__)
        static const char kLibName[] =
                "/System/Library/Frameworks/OpenGL.framework/OpenGL";
        static const char* const kResolverName = NULL;
#elif defined(_WIN32)
        static const char kLibName[] = "opengl32";
        static const char kResolverName[] = "wglGetProcAddress";
#elif defined(__linux__)
        static const char kLibName[] = "libGL";
        static const char kResolverName[] = "glXGetProcAddress";
#else
#error "Unsupported platform"
#endif
        const char* libName = kLibName;
        const char* resolverName = kResolverName;
        const char* env = ::getenv("ANDROID_EGL_ENGINE");
        if (env) {
            if (!strcmp(env, "osmesa")) {
                libName = "libosmesa";
                resolverName = "OSMesaGetProcAddress";
            } else {
                fprintf(stderr,
                        "Ignoring invalid ANDROID_EGL_ENGINE value '%s'\n",
                        env);
            }
        }
        s_libGL = emugl::SharedLibrary::open(libName);
        if (!s_libGL) {
            fprintf(stderr, "Could not load desktop GL library: %s\n",
                    libName);
            return NULL;
        }
        if (resolverName) {
            s_resolver = reinterpret_cast<Resolver*>(
                    s_libGL->findSymbol(resolverName));
            if (!s_resolver) {
                fprintf(
                    stderr,
                    "Could not find desktop GL symbol resolver '%s' in '%s'\n",
                    resolverName, libName);
            }
        }
    }

#if defined(__linux__)
    // HACK ATTACK: Using the resolver loaded through dlsym() doesn't work
    // on Linux with some native system libraries, for reasons to obscure to
    // understand here. We must thus call glXGetProcAddress() directly instead
    // or the functions returned by the resolver will _not_ work correctly and
    // simply return NULL.
    ret = glXGetProcAddress((const GLubyte*)funcName);
#else
    if (s_resolver) {
        ret = (*s_resolver)(funcName);
    }
#endif
    if (!ret && s_libGL) {
        ret = s_libGL->findSymbol(funcName);
    }
    return ret;
}

#define LOAD_GL_FUNC(return_type, func_name, signature)  do { \
        void* address = (void *)getGLFuncAddress(#func_name); \
        if (address) { \
            func_name = (__typeof__(func_name))(address); \
        } else { \
            fprintf(stderr, "Could not load func %s\n", #func_name); \
        } \
    } while (0);

#define LOAD_GLEXT_FUNC(return_type, func_name, signature) do { \
        if (!func_name) { \
            void* address = (void *)getGLFuncAddress(#func_name); \
            if (address) { \
                func_name = (__typeof__(func_name))(address); \
            } \
        } \
    } while (0);


// Define dummy functions, only for non-extensions.

#define RETURN_void return
#define RETURN_GLboolean return GL_FALSE
#define RETURN_GLint return 0
#define RETURN_GLuint return 0U
#define RETURN_GLenum return 0
#define RETURN_int return 0
#define RETURN_GLconstubyteptr return NULL

#define RETURN_(x)  RETURN_ ## x

#define DEFINE_DUMMY_FUNCTION(return_type, func_name, signature) \
static return_type GL_APIENTRY dummy_##func_name signature { \
    RETURN_(return_type); \
}

#define DEFINE_DUMMY_EXTENSION_FUNCTION(return_type, func_name, signature) \
  // nothing here

LIST_GLES_FUNCTIONS(DEFINE_DUMMY_FUNCTION, DEFINE_DUMMY_EXTENSION_FUNCTION)

// Constructor
#define INIT_POINTER(return_type, function_name, signature) \
    function_name(& dummy_ ## function_name),

#define INIT_EXTENSION_POINTER(return_type, function_name, signature) \
    function_name(NULL),

GLDispatch::GLDispatch() :
    LIST_GLES_FUNCTIONS(INIT_POINTER, INIT_EXTENSION_POINTER)
    m_isLoaded(false) {}

// Initialization fo dispatch table.
void GLDispatch::dispatchFuncs(GLESVersion version) {
    if (m_isLoaded) {
        return;
    }

    /* Loading OpenGL functions which are needed for implementing BOTH GLES 1.1 & GLES 2.0*/
    LIST_GLES_COMMON_FUNCTIONS(LOAD_GL_FUNC)
    LIST_GLES_EXTENSIONS_FUNCTIONS(LOAD_GLEXT_FUNC)

    /* Loading OpenGL functions which are needed ONLY for implementing GLES 1.1*/
    if(version == GLES_1_1){
        LIST_GLES1_ONLY_FUNCTIONS(LOAD_GL_FUNC)
        LIST_GLES1_EXTENSIONS_FUNCTIONS(LOAD_GLEXT_FUNC)
    } else if (version == GLES_2_0){
        LIST_GLES2_ONLY_FUNCTIONS(LOAD_GL_FUNC)
        LIST_GLES2_EXTENSIONS_FUNCTIONS(LOAD_GLEXT_FUNC)
    }
    m_isLoaded = true;
}
