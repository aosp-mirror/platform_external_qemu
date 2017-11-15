#include "ThreadedDispatchImpl.h"

#include "DriverThread.h"

#include <vector>

#include <string.h>

namespace ThreadedDispatchImpl {

static EGLDispatch* egl_dispatch = nullptr;
static GLESv1Dispatch* gles1_dispatch = nullptr;
static GLESv2Dispatch* gles2_dispatch = nullptr;

void registerEGLDispatch(EGLDispatch* table) {
    egl_dispatch = table;
    DriverThread::initWorker();
    DriverThread::registerContextQueryCallback([]() {
        return DriverThread::getCallingThreadContextInfo();
    });
    DriverThread::registerContextChangeCallback([](const DriverThread::ContextInfo& info) {
        EGLDisplay dpy = egl_dispatch->eglGetDisplay_underlying(EGL_DEFAULT_DISPLAY);
        return egl_dispatch->eglMakeCurrent_underlying(dpy, (EGLSurface)info.read, (EGLSurface)info.draw, (EGLContext)info.context);
    });
}

void registerGLESv1Dispatch(GLESv1Dispatch* table) {
    gles1_dispatch = table;
}

void registerGLESv2Dispatch(GLESv2Dispatch* table) {
    gles2_dispatch = table;
}

// EGL
EGLint eglGetError() {
    return DriverThread::callOnDriverThread<int>([]() {
        return egl_dispatch->eglGetError();
    });
}

EGLDisplay eglGetDisplay(EGLNativeDisplayType dpy) {
    return (EGLDisplay)DriverThread::callOnDriverThread<void*>([dpy]() {
        return (void*)egl_dispatch->eglGetDisplay_underlying(dpy);
    }, false /* don't change context */);
}

EGLBoolean eglTerminate(EGLDisplay dpy) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglTerminate_underlying(dpy);
    }, false /* don't change context */);
}

EGLBoolean eglInitialize(EGLDisplay dpy, EGLint* major, EGLint* minor) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglInitialize_underlying(dpy, major, minor);
    }, false /* don't change context */);
}

char* eglQueryString(EGLDisplay dpy, EGLint id) {
    return (char*)DriverThread::callOnDriverThread<void*>([&]() {
        return (void*)egl_dispatch->eglQueryString_underlying(dpy, id);
    }, false /* don't change context */);
}

EGLBoolean eglGetConfigs(EGLDisplay display, EGLConfig* configs, EGLint config_size, EGLint* num_config) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglGetConfigs_underlying(display, configs, config_size, num_config);
    }, false /* don't change context */);
}

EGLBoolean eglChooseConfig(EGLDisplay display, const EGLint* attribs, EGLConfig* configs, EGLint config_size, EGLint* num_config) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglChooseConfig_underlying(display, attribs, configs, config_size, num_config);
    }, false /* don't change context */);
}

EGLBoolean eglGetConfigAttrib(EGLDisplay display, EGLConfig config, EGLint attribute, EGLint* value) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglGetConfigAttrib_underlying(display, config, attribute, value);
    }, false /* don't change context */);
}

EGLSurface eglCreateWindowSurface(EGLDisplay display, EGLConfig config, EGLNativeWindowType native_window, const EGLint* attrib_list) {
    return (EGLSurface)DriverThread::callOnDriverThread<void*>([&]() {
        return (void*)egl_dispatch->eglCreateWindowSurface_underlying(display, config, native_window, attrib_list);
    }, false /* don't change context */);
}

EGLSurface eglCreatePbufferSurface(EGLDisplay display, EGLConfig config, const EGLint* attrib_list) {
    return (EGLSurface)DriverThread::callOnDriverThread<void*>([&]() {
        return (void*)egl_dispatch->eglCreatePbufferSurface_underlying(display, config, attrib_list);
    }, false /* don't change context */);
}

EGLBoolean eglDestroySurface(EGLDisplay display, EGLSurface surface) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglDestroySurface_underlying(display, surface);
    });
}

EGLBoolean eglBindAPI(EGLenum api) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglBindAPI_underlying(api);
    }, false /* don't change context */);
}

EGLenum eglQueryAPI(void) {
    return (EGLenum)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglQueryAPI_underlying();
    }, false /* don't change context */);
}

EGLBoolean eglReleaseThread(void) {
    DriverThread::makeCurrentToCallingThread({ 0, 0, 0 });
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglReleaseThread_underlying();
    });
}

EGLContext eglCreateContext(EGLDisplay display, EGLConfig config, EGLContext share_context, const EGLint* attrib_list) {
    return (EGLContext)DriverThread::callOnDriverThread<void*>([&]() {
        return (void*)egl_dispatch->eglCreateContext_underlying(display, config, share_context, attrib_list);
    }, false /* don't change context */);
}

EGLBoolean eglDestroyContext(EGLDisplay display, EGLContext context) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglDestroyContext_underlying(display, context);
    });
}

EGLBoolean eglMakeCurrent(EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context) {
    EGLBoolean res = (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglMakeCurrent_underlying(display, draw, read, context);
    });
    if (res == EGL_TRUE) {
        DriverThread::makeCurrentToCallingThread({ (uintptr_t)draw, (uintptr_t)read, (uintptr_t)context });
    }
    return res;
}

EGLContext eglGetCurrentContext(void) {
    DriverThread::ContextInfo info = DriverThread::getCallingThreadContextInfo();
    return (EGLContext)info.context;
}

EGLSurface eglGetCurrentSurface(EGLint readdraw) {
    DriverThread::ContextInfo info = DriverThread::getCallingThreadContextInfo();
    if (readdraw == EGL_READ) {
        return (EGLSurface)info.read;
    } else {
        return (EGLSurface)info.draw;
    }
}

EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglSwapBuffers_underlying(display, surface);
    });
}

void* eglGetProcAddress(const char* function_name) {
    return egl_dispatch->eglGetProcAddress_underlying(function_name);
}

EGLImageKHR eglCreateImageKHR(EGLDisplay display, EGLContext context, EGLenum target, EGLClientBuffer buffer, const EGLint* attrib_list) {
    return (EGLImageKHR)DriverThread::callOnDriverThread<void*>([&]() {
        return (void*)egl_dispatch->eglCreateImageKHR_underlying(display, context, target, buffer, attrib_list);
    });
}

EGLBoolean eglDestroyImageKHR(EGLDisplay display, EGLImageKHR image) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglDestroyImageKHR_underlying(display, image);
    });
}

EGLSyncKHR eglCreateSyncKHR(EGLDisplay display, EGLenum type, const EGLint* attribs) {
    return (EGLSyncKHR)DriverThread::callOnDriverThread<void*>([&]() {
        return (void*)egl_dispatch->eglCreateSyncKHR_underlying(display, type, attribs);
    });
}

EGLint eglClientWaitSyncKHR(EGLDisplay display, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout) {
    return (EGLint)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglClientWaitSyncKHR_underlying(display, sync, flags, timeout);
    });
}

EGLint eglWaitSyncKHR(EGLDisplay display, EGLSyncKHR sync, EGLint flags) {
    return (EGLint)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglWaitSyncKHR_underlying(display, sync, flags);
    });
}

EGLBoolean eglDestroySyncKHR(EGLDisplay display, EGLSyncKHR sync) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglDestroySyncKHR_underlying(display, sync);
    });
}

EGLint eglGetMaxGLESVersion(EGLDisplay display) {
    return (EGLint)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglGetMaxGLESVersion_underlying(display);
    }, false /* don't change context */);
}

void eglBlitFromCurrentReadBufferANDROID(EGLDisplay display, EGLImageKHR image) {
    DriverThread::callOnDriverThreadSync([&]() {
        egl_dispatch->eglBlitFromCurrentReadBufferANDROID_underlying(display, image);
    });
}

void* eglSetImageFenceANDROID(EGLDisplay display, EGLImageKHR image) {
    return (void*)DriverThread::callOnDriverThread<void*>([&]() {
        return (void*)egl_dispatch->eglSetImageFenceANDROID_underlying(display, image);
    });
}

void eglWaitImageFenceANDROID(EGLDisplay display, void* fence) {
    DriverThread::callOnDriverThreadSync([=]() {
        egl_dispatch->eglWaitImageFenceANDROID(display, fence);
    });
}

EGLConfig eglLoadConfig(EGLDisplay display, EGLStream stream) {
    return (EGLConfig)DriverThread::callOnDriverThread<void*>([&]() {
            return (void*)egl_dispatch->eglLoadConfig_underlying(display, stream);
    });
}

EGLContext eglLoadContext(EGLDisplay display, const EGLint *attrib_list, EGLStream stream) {
    return (EGLContext)DriverThread::callOnDriverThread<void*>([&]() {
            return (void*)egl_dispatch->eglLoadContext_underlying(display, attrib_list, stream);
    });
}

EGLBoolean eglLoadAllImages(EGLDisplay display, EGLStream stream, const void* textureLoader) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (int)egl_dispatch->eglLoadAllImages_underlying(display, stream, textureLoader);
    });
}

EGLBoolean eglSaveConfig(EGLDisplay display, EGLConfig config, EGLStream stream) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (EGLBoolean)egl_dispatch->eglSaveConfig_underlying(display, config, stream);
    });
}

EGLBoolean eglSaveContext(EGLDisplay display, EGLContext context, EGLStream stream) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (EGLBoolean)egl_dispatch->eglSaveConfig_underlying(display, context, stream);
    });
}

EGLBoolean eglSaveAllImages(EGLDisplay display, EGLStream stream, const void* textureSaver) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (EGLBoolean)egl_dispatch->eglSaveAllImages_underlying(display, stream, textureSaver);
    });
}

EGLBoolean eglPreSaveContext(EGLDisplay display, EGLContext context, EGLStream stream) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (EGLBoolean)egl_dispatch->eglPreSaveContext_underlying(display, context, stream);
    });
}

EGLBoolean eglPostLoadAllImages(EGLDisplay display, EGLStream stream) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (EGLBoolean)egl_dispatch->eglPostLoadAllImages_underlying(display, stream);
    });
}

EGLBoolean eglPostSaveContext(EGLDisplay display, EGLConfig config, EGLStream stream) {
    return (EGLBoolean)DriverThread::callOnDriverThread<int>([&]() {
        return (EGLBoolean)egl_dispatch->eglPostSaveContext_underlying(display, config, stream);
    });
}

void eglUseOsEglApi(EGLBoolean enable) {
    DriverThread::callOnDriverThreadSync([&]() {
        egl_dispatch->eglUseOsEglApi_underlying(enable);
    }, false /* don't change context */);
}

void eglSetMaxGLESVersion(EGLint glesVersion) {
    DriverThread::callOnDriverThreadSync([&]() {
        egl_dispatch->eglSetMaxGLESVersion_underlying(glesVersion);
    }, false /* don't change context */);
}

void eglFillUsages(void* usages) {
    DriverThread::callOnDriverThreadSync([&]() {
        egl_dispatch->eglFillUsages_underlying(usages);
    }, false /* don't change context */);
}

////////////////////////////////////////////////////////////////////////////////
// GLES1 ///////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void gles1_glBufferData(GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage) {
    std::vector<unsigned char> copiedBuf(data ? size : 0);
    if (data) {
        memcpy(copiedBuf.data(), data, size);
    }
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glBufferData_underlying(target, size, data ? copiedBuf.data() : nullptr, usage);
    });
}

void gles1_glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data) {
    std::vector<unsigned char> copiedBuf(data ? size : 0);
    if (data) {
        memcpy(copiedBuf.data(), data, size);
    }
    memcpy(copiedBuf.data(), data, size);
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glBufferSubData_underlying(target, offset, size, data ? copiedBuf.data() : nullptr);
    });
}

////////////////////////////////////////////////////////////////////////////////
// GLES2 ///////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void gles2_glBufferData(GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage) {
    std::vector<unsigned char> copiedBuf(data ? size : 0);
    if (data) {
        memcpy(copiedBuf.data(), data, size);
    }
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBufferData_underlying(target, size, data ? copiedBuf.data() : nullptr, usage);
    });
}

void gles2_glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data) {
    std::vector<unsigned char> copiedBuf(data ? size : 0);
    if (data) {
        memcpy(copiedBuf.data(), data, size);
    }
    memcpy(copiedBuf.data(), data, size);
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBufferSubData_underlying(target, offset, size, data ? copiedBuf.data() : nullptr);
    });
}

void gles2_glVertexAttrib1fv(GLuint indx, const GLfloat* values) {
    std::vector<GLfloat> vals(1); vals[0] = values[0];
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glVertexAttrib1fv_underlying(indx, vals.data());
    });
}

void gles2_glVertexAttrib2fv(GLuint indx, const GLfloat* values) {
    size_t sz = 2;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), values, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glVertexAttrib2fv_underlying(indx, vals.data());
    });
}

void gles2_glVertexAttrib3fv(GLuint indx, const GLfloat* values) {
    size_t sz = 3;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), values, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glVertexAttrib3fv_underlying(indx, vals.data());
    });
}

void gles2_glVertexAttrib4fv(GLuint indx, const GLfloat* values) {
    size_t sz = 4;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), values, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glVertexAttrib4fv_underlying(indx, vals.data());
    });
}

void gles2_glUniform1fv(GLint location, GLsizei count, const GLfloat* v) {
    size_t sz = 1 * count;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), v, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform1fv_underlying(location, count, vals.data());
    });
}

void gles2_glUniform2fv(GLint location, GLsizei count, const GLfloat* v) {
    size_t sz = 2 * count;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), v, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform2fv_underlying(location, count, vals.data());
    });
}

void gles2_glUniform3fv(GLint location, GLsizei count, const GLfloat* v) {
    size_t sz = 3 * count;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), v, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform3fv_underlying(location, count, vals.data());
    });
}

void gles2_glUniform4fv(GLint location, GLsizei count, const GLfloat* v) {
    size_t sz = 4 * count;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), v, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform4fv_underlying(location, count, vals.data());
    });
}

void gles2_glUniform1iv(GLint location, GLsizei count, const GLint* v) {
    size_t sz = 1 * count;
    std::vector<GLint> vals(sz); memcpy(vals.data(), v, sz * sizeof(GLint));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform1iv_underlying(location, count, vals.data());
    });
}

void gles2_glUniform2iv(GLint location, GLsizei count, const GLint* v) {
    size_t sz = 2 * count;
    std::vector<GLint> vals(sz); memcpy(vals.data(), v, sz * sizeof(GLint));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform2iv_underlying(location, count, vals.data());
    });
}

void gles2_glUniform3iv(GLint location, GLsizei count, const GLint* v) {
    size_t sz = 3 * count;
    std::vector<GLint> vals(sz); memcpy(vals.data(), v, sz * sizeof(GLint));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform3iv_underlying(location, count, vals.data());
    });
}

void gles2_glUniform4iv(GLint location, GLsizei count, const GLint* v) {
    size_t sz = 4 * count;
    std::vector<GLint> vals(sz); memcpy(vals.data(), v, sz * sizeof(GLint));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform4iv_underlying(location, count, vals.data());
    });
}

void gles2_glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    size_t sz = 4 * count;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), value, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniformMatrix2fv_underlying(location, count, transpose, vals.data());
    });
}

void gles2_glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    size_t sz = 9 * count;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), value, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniformMatrix3fv_underlying(location, count, transpose, vals.data());
    });
}

void gles2_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    size_t sz = 16 * count;
    std::vector<GLfloat> vals(sz); memcpy(vals.data(), value, sz * sizeof(GLfloat));
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniformMatrix4fv_underlying(location, count, transpose, vals.data());
    });
}

#include <ThreadedGLESv1.cpp>
#include <ThreadedGLESv2.cpp>

}; // namespace ThreadedDispatchImpl
