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

#include "GLcommon/GLDispatch.h"
#include "GLcommon/GLLibrary.h"

#include "emugl/common/lazy_instance.h"
#include "emugl/common/logging.h"
#include "emugl/common/shared_library.h"

#include "OpenglCodecCommon/ErrorLog.h"

#ifdef __linux__
#include <GL/glx.h>
#elif defined(WIN32)
#include <windows.h>
#endif

#include <stdio.h>
#include <string.h>
#include <unordered_map>

typedef GlLibrary::GlFunctionPointer GL_FUNC_PTR;

static GL_FUNC_PTR getGLFuncAddress(const char *funcName, GlLibrary* glLib) {
    return glLib->findSymbol(funcName);
}

static const std::unordered_map<std::string, std::string> sAliasExtra = {
    {"glDepthRange", "glDepthRangef"},
    {"glDepthRangef", "glDepthRange"},
    {"glClearDepth", "glClearDepthf"},
    {"glClearDepthf", "glClearDepth"},
};

#define LOAD_GL_FUNC(return_type, func_name, signature, args)  do { \
        if (!func_name) { \
            void* address = (void *)getGLFuncAddress(#func_name, glLib); \
            /*Check alias*/ \
            if (!address) { \
                address = (void *)getGLFuncAddress(#func_name "OES", glLib); \
                if (address) GL_LOG("%s not found, using %sOES", #func_name, #func_name); \
            } \
            if (!address) { \
                address = (void *)getGLFuncAddress(#func_name "EXT", glLib); \
                if (address) GL_LOG("%s not found, using %sEXT", #func_name, #func_name); \
            } \
            if (!address) { \
                address = (void *)getGLFuncAddress(#func_name "ARB", glLib); \
                if (address) GL_LOG("%s not found, using %sARB", #func_name, #func_name); \
            } \
            if (!address) { \
                const auto& it = sAliasExtra.find(#func_name); \
                if (it != sAliasExtra.end()) { \
                    address = (void *)getGLFuncAddress(it->second.c_str(), glLib); \
                } \
            } \
            if (address) { \
                func_name = (__typeof__(func_name))(address); \
            } else { \
                GL_LOG("%s not found", #func_name); \
                func_name = (__typeof__(func_name))(dummy_##func_name); \
            } \
        } \
    } while (0);

#define LOAD_GLEXT_FUNC(return_type, func_name, signature, args) do { \
        if (!func_name) { \
            void* address = (void *)getGLFuncAddress(#func_name, glLib); \
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

#define DEFINE_DUMMY_FUNCTION(return_type, func_name, signature, args) \
static return_type dummy_##func_name signature { \
    assert(0); \
    RETURN_(return_type); \
}

#define DEFINE_DUMMY_EXTENSION_FUNCTION(return_type, func_name, signature, args) \
  // nothing here

LIST_GLES_FUNCTIONS(DEFINE_DUMMY_FUNCTION, DEFINE_DUMMY_EXTENSION_FUNCTION)

// Initializing static GLDispatch members*/

emugl::Mutex GLDispatch::s_lock;

#define GL_DISPATCH_DEFINE_POINTER(return_type, function_name, signature, args) \
    GL_APICALL return_type (GL_APIENTRY *GLDispatch::function_name) signature = NULL;

LIST_GLES_FUNCTIONS(GL_DISPATCH_DEFINE_POINTER, GL_DISPATCH_DEFINE_POINTER)

// Constructor.
GLDispatch::GLDispatch() : m_isLoaded(false) {}

bool GLDispatch::isInitialized() const {
    return m_isLoaded;
}

GLESVersion GLDispatch::getGLESVersion() const {
    return m_version;
}

void GLDispatch::dispatchFuncs(GLESVersion version, GlLibrary* glLib) {
    emugl::Mutex::AutoLock mutex(s_lock);
    if(m_isLoaded)
        return;

    /* Loading OpenGL functions which are needed for implementing BOTH GLES 1.1 & GLES 2.0*/
    LIST_GLES_COMMON_FUNCTIONS(LOAD_GL_FUNC)
    LIST_GLES_EXTENSIONS_FUNCTIONS(LOAD_GLEXT_FUNC)

    /* Load both GLES1 and GLES2. On core profile, GLES 1 implementation will
     * require GLES 3 function supports and set version to GLES_3_0. Thus
     * we cannot really tell if the dispatcher is used for GLES1 or GLES2, so
     * let's just load both of them.
     */
    LIST_GLES1_ONLY_FUNCTIONS(LOAD_GL_FUNC)
    LIST_GLES1_EXTENSIONS_FUNCTIONS(LOAD_GLEXT_FUNC)
    LIST_GLES2_ONLY_FUNCTIONS(LOAD_GL_FUNC)
    LIST_GLES2_EXTENSIONS_FUNCTIONS(LOAD_GLEXT_FUNC)

    /* Load OpenGL ES 3.x functions through 3.1. Not all are supported;
     * leave it up to EGL to determine support level. */

    if (version >= GLES_3_0) {
        LIST_GLES3_ONLY_FUNCTIONS(LOAD_GLEXT_FUNC)
        LIST_GLES3_EXTENSIONS_FUNCTIONS(LOAD_GLEXT_FUNC)
    }
    if (version >= GLES_3_1) {
        LIST_GLES31_ONLY_FUNCTIONS(LOAD_GLEXT_FUNC)
    }

    m_isLoaded = true;
    m_version = version;
}
