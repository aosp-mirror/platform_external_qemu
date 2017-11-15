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

#include "GLcommon/DriverThread.h"
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

typedef GlLibrary::GlFunctionPointer GL_FUNC_PTR;

static GL_FUNC_PTR getGLFuncAddress(const char *funcName, GlLibrary* glLib) {
    return glLib->findSymbol(funcName);
}

#define LOAD_GL_FUNC(return_type, func_name, signature, args)  do { \
        if (!func_name##Underlying) { \
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
            if (address) { \
                func_name##Underlying = (__typeof__(func_name))(address); \
            } else { \
                GL_LOG("%s not found", #func_name); \
                func_name##Underlying = (__typeof__(func_name))(dummy_##func_name); \
            } \
        } \
    } while (0);

#define LOAD_GLEXT_FUNC(return_type, func_name, signature, args) do { \
        if (!func_name) { \
            void* address = (void *)getGLFuncAddress(#func_name, glLib); \
            if (address) { \
                func_name##Underlying = (__typeof__(func_name))(address); \
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
    GL_APICALL return_type (GL_APIENTRY *GLDispatch::function_name) signature = NULL; \
    GL_APICALL return_type (GL_APIENTRY *GLDispatch::function_name##Underlying) signature = NULL;

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

#define DEFINE_API_FUNC(return_type, func_name, signature, args) \
    if (func_name##Underlying) { \
        func_name = func_name##Driverthread; \
    } else { \
        func_name = nullptr; \
    } \

    LIST_GLES_FUNCTIONS(DEFINE_API_FUNC, DEFINE_API_FUNC);

#define DEFINE_ASYNC_API_FUNC(return_type, func_name, signature, args) \
    if (func_name##Underlying) { \
        func_name = func_name##DriverthreadAsync; \
    } else { \
        func_name = nullptr; \
    } \

    LIST_ASYNC_GL_FUNCTIONS(DEFINE_ASYNC_API_FUNC)
    LIST_CUSTOM_ASYNC_GL_FUNCTIONS(DEFINE_ASYNC_API_FUNC)
}

#define _Args(...) __VA_ARGS__
#define STRIP_PARENS(X) X
#define PASS_PARAMETERS(X) STRIP_PARENS( _Args X )

#define DEFINE_DRIVERTHREAD_FUNC(return_type, func_name, signature, args) \
    DEFINE_DRIVERTHREAD_FUNC##return_type(return_type, func_name, signature, args) \

#define DEFINE_DRIVERTHREAD_FUNCvoid(return_type, func_name, signature, args) \
    return_type GLDispatch::func_name##Driverthread signature { \
        DriverThread::callOnDriverThread<return_type>([&]() { \
            func_name##Underlying args; \
        }); } \

#define DEFINE_DRIVERTHREAD_FUNC_RET(return_type, func_name, signature, args) \
    return_type GLDispatch::func_name##Driverthread signature { \
        return DriverThread::callOnDriverThread<return_type>([PASS_PARAMETERS(args)]() { \
            return func_name##Underlying args; \
        }); } \

#define DEFINE_DRIVERTHREAD_FUNCGLboolean(return_type, func_name, signature, args) \
    DEFINE_DRIVERTHREAD_FUNC_RET(return_type, func_name, signature, args) \

#define DEFINE_DRIVERTHREAD_FUNCint(return_type, func_name, signature, args) \
    DEFINE_DRIVERTHREAD_FUNC_RET(return_type, func_name, signature, args) \

#define DEFINE_DRIVERTHREAD_FUNCGLint(return_type, func_name, signature, args) \
    DEFINE_DRIVERTHREAD_FUNC_RET(return_type, func_name, signature, args) \

#define DEFINE_DRIVERTHREAD_FUNCGLuint(return_type, func_name, signature, args) \
    DEFINE_DRIVERTHREAD_FUNC_RET(return_type, func_name, signature, args) \

#define DEFINE_DRIVERTHREAD_FUNCGLenum(return_type, func_name, signature, args) \
    DEFINE_DRIVERTHREAD_FUNC_RET(return_type, func_name, signature, args) \

#define DEFINE_DRIVERTHREAD_FUNCGLsync(return_type, func_name, signature, args) \
    DEFINE_DRIVERTHREAD_FUNC_RET(return_type, func_name, signature, args) \

#define DEFINE_DRIVERTHREAD_FUNCGLconstubyteptr(return_type, func_name, signature, args) \
    DEFINE_DRIVERTHREAD_FUNC_RET(return_type, func_name, signature, args) \

#define DEFINE_DRIVERTHREAD_FUNCvoidptr(return_type, func_name, signature, args) \
    return_type GLDispatch::func_name##Driverthread signature { \
        return DriverThread::callOnDriverThread<void*>([PASS_PARAMETERS(args)]() { \
            return func_name##Underlying args; \
        }); } \

LIST_GLES_FUNCTIONS(DEFINE_DRIVERTHREAD_FUNC, DEFINE_DRIVERTHREAD_FUNC)


#define DEFINE_DRIVERTHREAD_FUNCvoidAsync(return_type, func_name, signature, args) \
    return_type GLDispatch::func_name##DriverthreadAsync signature { \
        DriverThread::callOnDriverThreadAsync<return_type>([=]() { \
            func_name##Underlying args; \
        }); } \

LIST_ASYNC_GL_FUNCTIONS(DEFINE_DRIVERTHREAD_FUNCvoidAsync)

// Custom implementations
void GLDispatch::glBufferDataDriverthreadAsync(GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage) {
    std::vector<unsigned char> copiedBuf(data ? size : 0);
    if (data) {
        memcpy(copiedBuf.data(), data, size);
    }
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        glBufferDataUnderlying(target, size, data ? copiedBuf.data() : nullptr, usage);
    });
}

void GLDispatch::glBufferSubDataDriverthreadAsync(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data) {
    std::vector<unsigned char> copiedBuf(data ? size : 0);
    if (data) {
        memcpy(copiedBuf.data(), data, size);
    }
    memcpy(copiedBuf.data(), data, size);
    DriverThread::callOnDriverThreadAsync<void>([=]() {
            glBufferSubDataUnderlying(target, offset, size, data ? copiedBuf.data() : nullptr);
    });
}

// void GLDispatch::glVertexAttribPointerWithDataSizeDriverthreadAsync(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr, GLsizei dataSize) {
//     std::vector<unsigned char> copiedBuf(dataSize);
//     if (dataSize) {
//         memcpy(copiedBuf.data(), ptr, dataSize);
//     }
//     DriverThread::callOnDriverThreadAsync<void>([=]() {
//         glVertexAttribPointerWithDataSizeUnderlying(indx, size, type, normalized, stride, dataSize ? copiedBuf.data() : nullptr, dataSize);
//     });
// }
// 
// void GLDispatch::glBindAttribLocationDriverthreadAsync(GLuint program, GLuint index, const GLchar* name) {
//     std::vector<unsigned char> copiedBuf(strlen(name) + 1);
//     memcpy(copiedBuf.data(), name, strlen(name) + 1);
//     DriverThread::callOnDriverThreadAsync<void>([=]() {
//         glBindAttribLocationUnderlying(program, index, name);
//     });
// }

void GLDispatch::glVertexAttrib1fvDriverthreadAsync(GLuint indx, const GLfloat* values) {
    std::vector<GLfloat> vals(1); vals[0] = values[0];
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        glVertexAttrib1fvUnderlying(indx, vals.data());
    });
}

void GLDispatch::glVertexAttrib2fvDriverthreadAsync(GLuint indx, const GLfloat* values) {
    size_t sz = 2;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), values, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        glVertexAttrib2fvUnderlying(indx, vals.data());
    });
}

void GLDispatch::glVertexAttrib3fvDriverthreadAsync(GLuint indx, const GLfloat* values) {
    size_t sz = 3;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), values, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        glVertexAttrib3fvUnderlying(indx, vals.data());
    });
}

void GLDispatch::glVertexAttrib4fvDriverthreadAsync(GLuint indx, const GLfloat* values) {
    size_t sz = 4;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), values, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        glVertexAttrib4fvUnderlying(indx, vals.data());
    });
}

void GLDispatch::glUniform1fvDriverthreadAsync(GLint location, GLsizei count, const GLfloat* v) {
    size_t sz = 1;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), v, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        glUniform1fvUnderlying(location, count, vals.data());
    });
}

void GLDispatch::glUniform2fvDriverthreadAsync(GLint location, GLsizei count, const GLfloat* v) {
    size_t sz = 2;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), v, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        glUniform2fvUnderlying(location, count, vals.data());
    });
}

void GLDispatch::glUniform3fvDriverthreadAsync(GLint location, GLsizei count, const GLfloat* v) {
    size_t sz = 3;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), v, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        glUniform3fvUnderlying(location, count, vals.data());
    });
}

void GLDispatch::glUniform4fvDriverthreadAsync(GLint location, GLsizei count, const GLfloat* v) {
    size_t sz = 4;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), v, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        glUniform4fvUnderlying(location, count, vals.data());
    });
}

void GLDispatch::glUniform1ivDriverthreadAsync(GLint location, GLsizei count, const GLint* v) {
    size_t sz = 1;
    std::vector<GLint> vals(sz); memcpy(vals.data(), v, sz * sizeof(GLint));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        glUniform1ivUnderlying(location, count, vals.data());
    });
}

void GLDispatch::glUniform2ivDriverthreadAsync(GLint location, GLsizei count, const GLint* v) {
    size_t sz = 2;
    std::vector<GLint> vals(sz); memcpy(vals.data(), v, sz * sizeof(GLint));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        glUniform2ivUnderlying(location, count, vals.data());
    });
}

void GLDispatch::glUniform3ivDriverthreadAsync(GLint location, GLsizei count, const GLint* v) {
    size_t sz = 3;
    std::vector<GLint> vals(sz); memcpy(vals.data(), v, sz * sizeof(GLint));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        glUniform3ivUnderlying(location, count, vals.data());
    });
}

void GLDispatch::glUniform4ivDriverthreadAsync(GLint location, GLsizei count, const GLint* v) {
    size_t sz = 4;
    std::vector<GLint> vals(sz); memcpy(vals.data(), v, sz * sizeof(GLint));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        glUniform4ivUnderlying(location, count, vals.data());
    });
}

void GLDispatch::glUniformMatrix2fvDriverthreadAsync(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    size_t sz = 4;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), value, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        glUniformMatrix2fvUnderlying(location, count, transpose, vals.data());
    });
}

void GLDispatch::glUniformMatrix3fvDriverthreadAsync(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    size_t sz = 9;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), value, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        glUniformMatrix3fvUnderlying(location, count, transpose, vals.data());
    });
}

void GLDispatch::glUniformMatrix4fvDriverthreadAsync(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    size_t sz = 16;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), value, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        glUniformMatrix4fvUnderlying(location, count, transpose, vals.data());
    });
}
