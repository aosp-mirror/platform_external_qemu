
#include "ThreadedDispatch.h"
#include "ThreadedDispatchImpl.h"

#include "DriverThread.h"

#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv1Dispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

#define RENDER_EGL_DEFINE_THREADED(return_type, function_name, signature) \
    if (table-> function_name##_underlying) { \
        table-> function_name = ThreadedDispatchImpl::function_name; \
    } else { \
        table-> function_name = nullptr; \
    }

void init_egl_threaded_dispatch(EGLDispatch* table) {
    ThreadedDispatchImpl::registerEGLDispatch(table);

    LIST_RENDER_EGL_FUNCTIONS(RENDER_EGL_DEFINE_THREADED)
    LIST_RENDER_EGL_EXTENSIONS_FUNCTIONS(RENDER_EGL_DEFINE_THREADED)
    LIST_RENDER_EGL_SNAPSHOT_FUNCTIONS(RENDER_EGL_DEFINE_THREADED)
}

void init_gles1_threaded_dispatch(GLESv1Dispatch* table) {
    ThreadedDispatchImpl::registerGLESv1Dispatch(table);

#define GLES1_DEFINE_THREADED(return_type,function_name,signature, callargs) \
    if (table->function_name##_underlying) { \
        table->function_name = ThreadedDispatchImpl::gles1_##function_name; \
    }

    LIST_GLES1_FUNCTIONS(GLES1_DEFINE_THREADED, GLES1_DEFINE_THREADED)
}

void init_gles2_threaded_dispatch(GLESv2Dispatch* table) {
    ThreadedDispatchImpl::registerGLESv2Dispatch(table);

#define GLES2_DEFINE_THREADED(return_type,function_name,signature, callargs) \
    if (table->function_name##_underlying) { \
        table->function_name = ThreadedDispatchImpl::gles2_##function_name; \
    }

    LIST_GLES2_FUNCTIONS(GLES2_DEFINE_THREADED, GLES2_DEFINE_THREADED)
}


