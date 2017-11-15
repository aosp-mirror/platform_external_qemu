#pragma once

#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv1Dispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

namespace ThreadedDispatchImpl {

void registerEGLDispatch(EGLDispatch* table);
void registerGLESv1Dispatch(GLESv1Dispatch* table);
void registerGLESv2Dispatch(GLESv2Dispatch* table);

#define RENDER_EGL_DECLARE_IMPL(return_type, function_name, signature) \
    return_type function_name signature;

#define GLES1_DECLARE_IMPL(return_type,func_name,signature,callargs) \
    return_type gles1_##func_name signature;

#define GLES2_DECLARE_IMPL(return_type,func_name,signature,callargs) \
    return_type gles2_##func_name signature;

    LIST_RENDER_EGL_FUNCTIONS(RENDER_EGL_DECLARE_IMPL)
    LIST_RENDER_EGL_EXTENSIONS_FUNCTIONS(RENDER_EGL_DECLARE_IMPL)
    LIST_RENDER_EGL_SNAPSHOT_FUNCTIONS(RENDER_EGL_DECLARE_IMPL)

    LIST_GLES1_FUNCTIONS(GLES1_DECLARE_IMPL, GLES1_DECLARE_IMPL)
    LIST_GLES2_FUNCTIONS(GLES2_DECLARE_IMPL, GLES2_DECLARE_IMPL)

}; // namespace ThreadedDispatchImpl
