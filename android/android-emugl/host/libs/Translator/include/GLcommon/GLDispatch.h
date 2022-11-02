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
#ifndef GL_DISPATCHH
#define GL_DISPATCHH

#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>

#include "aemu/base/threads/Thread.h"

#include "OpenGLESDispatch/gldefs.h"
#include "OpenGLESDispatch/gles_functions.h"
#include "GLutils.h"
#include "GLESmacros.h" // for CHECK_ALL_GL_CALLS

#include "emugl/common/mutex.h"

#define GLAPIENTRY GL_APIENTRY
typedef void (*FUNCPTR_NO_ARGS_RET_VOID)();
typedef int (*FUNCPTR_NO_ARGS_RET_INT)();
typedef GLsync (*FUNCPTR_FENCE_SYNC)(GLenum, GLbitfield);
typedef GLenum (*FUNCPTR_CLIENT_WAIT_SYNC)(GLsync, GLbitfield, GLuint64);
typedef void (*FUNCPTR_DELETE_SYNC)(GLsync);
typedef void (*FUNCPTR_WAIT_SYNC)(GLsync, GLbitfield, GLuint64);
typedef void (*FUNCPTR_GET_SYNC_IV)(GLsync, GLenum pname, GLsizei bufsize, GLsizei *length, GLint *values);

class GlLibrary;


#define GLES_DECLARE_PRIV_METHOD(return_type, function_name, signature, args) \
    static GL_APICALL return_type (GL_APIENTRY *function_name##_priv) signature;

#if CHECK_ALL_GL_CALLS

#define GLES_DECLARE_METHOD(return_type, function_name, signature, args) \
    static return_type function_name signature; \

#else

#define GLES_DECLARE_METHOD(return_type, function_name, signature, args) \
    static GL_APICALL return_type (GL_APIENTRY *function_name) signature;

#endif

class GLDispatch {
public:
    // Constructor.
    GLDispatch();

    bool isInitialized() const;
    void dispatchFuncs(GLESVersion version, GlLibrary* glLib);
    GLESVersion getGLESVersion() const;

#if CHECK_ALL_GL_CALLS
    LIST_GLES_FUNCTIONS(GLES_DECLARE_PRIV_METHOD, GLES_DECLARE_PRIV_METHOD)
#endif

    LIST_GLES_FUNCTIONS(GLES_DECLARE_METHOD, GLES_DECLARE_METHOD)

private:
    bool                  m_isLoaded;
    GLESVersion           m_version;
    static emugl::Mutex   s_lock;
};

// Used to query max GLES version support based on what the dispatch mechanism
// has found in the system OpenGL library.
// First, a enum for tracking the detected GLES version based on dispatch.
// We support 2 minimally.
// This is meant to communicate with GLESv2Dispatch to communicate the
// max version up the stack.
enum GLDispatchMaxGLESVersion {
    GL_DISPATCH_MAX_GLES_VERSION_2 = 0,
    GL_DISPATCH_MAX_GLES_VERSION_3_0 = 1,
    GL_DISPATCH_MAX_GLES_VERSION_3_1 = 2,
    GL_DISPATCH_MAX_GLES_VERSION_3_2 = 3,
};

extern "C" GL_APICALL GLDispatchMaxGLESVersion GL_APIENTRY gl_dispatch_get_max_version();

class GLDispatchChecker {
public:
    GLDispatchChecker(const char * funcName):
            mFuncName(funcName) {
        if (strcmp(funcName, "glGetError")) {
            GLuint err = GLDispatch::glGetError();
            if (err != GL_NO_ERROR) {
                fprintf (stderr, "0x%llx: GLDispatch error 0x%x existed before running %s\n", (unsigned long long)android::base::getCurrentThreadId(), err, mFuncName);
            }
            fprintf (stderr, "0x%llx: GLDispatch calling %s (", (unsigned long long)android::base::getCurrentThreadId(), funcName);
        }
    }

    void printArgs() {
        if (strcmp(mFuncName, "glGetError")) {
            fprintf(stderr, ")\n");
        }
    }
    template <typename T, typename...Ts>
    void printArgs(const T &first, const Ts&... rest) {
        const int n = sizeof...(Ts);
        if (strcmp(mFuncName, "glGetError")) {
            const char *fmt = "0x%x%s";
            if(std::is_same<T,GLuint>::value ||
               std::is_same<T,GLint>::value) {
                fmt = "%d%s";
            } else if (std::is_same<T,GLenum>::value) {
                fmt = "%04X%s";
            } else if (std::is_same<T,double>::value) {
                fmt = "%f%s";
            } else if (std::is_same<T,GLfloat>::value) {
                fmt = "%f%s";
            } else if (std::is_same<T,GLvoid>::value) {
                fmt = "%p%s";
            }
            fprintf(stderr, fmt, first, n?", ":"");
            printArgs(rest...);
        }
    }
    ~GLDispatchChecker();
private:
    const char * mFuncName;
};

#endif  // GL_DISPATCH_H
