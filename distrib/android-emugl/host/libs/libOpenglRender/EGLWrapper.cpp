// Copyright 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "EGLWrapper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Technical note:
//
// Minimal wrapper for EGL functions. The main purpose is to publish
// support for GLESv1 APIs, even though the underlying EGL implementation
// only supports GLESv2. This is done in the following way:
//
// - EGLConfig objects are reported as-is (they are not wrapped by custom
//   objects).
//
// - eglGetConfigAttrib() is modified to report EGL_OPENGL_ES_BIT if
//   EGL_OPENGL_ES2_BIT is set, both for EGL_RENDERABLE_TYPE and
//   EGL_CONFORMANT.
//
// - eglChooseConfig() is modified to rewrite the attribute list to ensure
//   that EGL_OPENGL_ES2_BIT is always specified when calling the
//   host implementation.
//
// - eglCreateContext() also rewrites the attribute list to ensure that
//   EGL_CONTEXT_CLIENT_VERSION is set to EGL_OPENGL_ES2_API before
//   calling the underlying function.
//
// - EGLContext objects are reported as-is too, but an out-of-band map
//   is used to 'tag' the ones used to implement GLESv1. Essentially, these
//   are regular host GLESv2 context objects, but map[context] is not NULL
//   and will contain data that holds the additional state required to
//   implement GLESv1 features.
//
// - The GLESv2 dispatch table is a super-thin wrapper around the host one,
//   since there is no config/translation to perform.
//
// - The GLESv1 dispatch table is different. The ES1x or ARC library will
//   be used to implement GLESv1 features on top of GLESv2.
//

// The underlying dispatch table to the host EGL implementation wrapped here.
static const EGLDispatch* sHostEgl;

static EGLint EGLAPIENTRY wrap_eglGetError() {
    return sHostEgl->eglGetError();
}

static EGLDisplay EGLAPIENTRY wrap_eglGetDisplay(EGLNativeDisplayType dpy) {
    // TODO(digit): Initialize sHostEgl->here!!
    return sHostEgl->eglGetDisplay(dpy);
}

static EGLBoolean EGLAPIENTRY wrap_eglInitialize(EGLDisplay display,
                                                 EGLint* major,
                                                 EGLint* minor) {
    // TODO(digit): Extra initialization here?
    return sHostEgl->eglInitialize(display, major, minor);
}

static char* EGLAPIENTRY wrap_eglQueryString(EGLDisplay display, EGLint id) {
    // Only EGL_CLIENT_APIS requires special treatment.
    if (id != EGL_CLIENT_APIS) {
        return sHostEgl->eglQueryString(display, id);
    }
    const char kGLESv1Api[] = "EGL_OPENGL_ES_API";
    char* apis = sHostEgl->eglQueryString(display, id);
    if (!apis || strstr(apis, kGLESv1Api)) {
        return apis;
    }
    // Ok, the string doesn't contain EGL_OPENGL_ES_API, so append it
    // to the result. Store it in a static variable that will be leaked
    static char sNewApis[256];
    if (!sNewApis[0]) {
        size_t apisLen = strlen(apis);
        char* newApis = (char*)::malloc(apisLen + sizeof(kGLESv1Api) + 1);
        memcpy(newApis, apis, apisLen);
        if (apisLen > 0) {
            newApis[apisLen++] = ' ';
        }
        memcpy(newApis + apisLen, kGLESv1Api, sizeof(kGLESv1Api));
        snprintf(sNewApis, sizeof(sNewApis), "%s", newApis);
        sNewApis[sizeof(sNewApis) - 1U] = '\0';  // broken Windows snprintf().
    }
    return sNewApis;
}

static EGLBoolean EGLAPIENTRY wrap_eglGetConfigs(EGLDisplay display,
                                                 EGLConfig* configs,
                                                 EGLint config_size,
                                                 EGLint* num_config) {
    // Just call the host version. There is no point in filtering the list
    // here anyway.
    return sHostEgl->eglGetConfigs(display, configs, config_size, num_config);
}

static EGLBoolean EGLAPIENTRY wrap_eglChooseConfig(EGLDisplay display,
                                                   const EGLint* attribs,
                                                   EGLConfig* configs,
                                                   EGLint config_size,
                                                   EGLint* num_config) {
    // Rewriting the |attribs| list is necessary to deal with the following
    // cases:
    //
    //   - If EGL_RENDERABLE_TYPE is not specified, because its default value
    //     is EGL_OPENGL_ES_BIT, a new attribute with value EGL_OPENGL_ES2_BIT
    //     must be added.
    //
    //   - If EGL_RENDERABLE_TYPE is specified with EGL_OPENGL_ES_BIT, the
    //     value must be changed to EGL_OPENGL_ES2_BIT.
    //
    //   - If EGL_CONFORMANT is specified with EGL_OPENGL_ES_BIT set,
    //     it must be rewritten to EGL_OPENGL_ES2_BIT.
    //

    // First parse |attribs| to check its content.
    int n;
    bool hasRenderableType = false;
    for (n = 0; attribs[n] != EGL_NONE; n += 2) {
        if (attribs[n] == EGL_RENDERABLE_TYPE) {
            hasRenderableType = true;
        }
    }

    // Allocate new attribs list.
    int newSize = n;
    if (!hasRenderableType) {
        // Add room to add a new EGL_RENDERABLE_TYPE filter.
        newSize += 2;
    }
    EGLint* newAttribs = new EGLint[newSize + 1];
    if (!newAttribs) {
        // TODO(digit): Update error?
        return EGL_FALSE;
    }

    // Copy attribute list, patching values on the go if necessary.
    for (n = 0; attribs[n] != EGL_NONE; n += 2) {
        newAttribs[n] = attribs[n];
        newAttribs[n + 1] = attribs[n + 1];
        if (attribs[n] == EGL_RENDERABLE_TYPE ||
            attribs[n] == EGL_CONFORMANT) {
            if ((attribs[n + 1] & EGL_OPENGL_ES_BIT)) {
                newAttribs[n + 1] &= ~EGL_OPENGL_ES_BIT;
                newAttribs[n + 1] |= EGL_OPENGL_ES2_BIT;
            }
        }
    }
    if (!hasRenderableType) {
        newAttribs[n] = EGL_RENDERABLE_TYPE;
        newAttribs[n + 1] = EGL_OPENGL_ES2_BIT;
        n += 2;
    }
    newAttribs[n] = EGL_NONE;

    EGLBoolean result = sHostEgl->eglChooseConfig(
            display, newAttribs, configs, config_size, num_config);

    delete [] newAttribs;

    return result;
}

static EGLBoolean EGLAPIENTRY wrap_eglGetConfigAttrib(EGLDisplay display,
                                                      EGLConfig config,
                                                      EGLint attribute,
                                                      EGLint* value) {
    if (!sHostEgl->eglGetConfigAttrib(display, config, attribute, value)) {
        return EGL_FALSE;
    }
    if (attribute == EGL_RENDERABLE_TYPE || attribute == EGL_CONFORMANT) {
        // Any GLESv2 config can also target GLESv1 due to our wrapper.
        // Any GLESv2-conformat config also supports GLESv1.
        if ((*value & EGL_OPENGL_ES2_BIT) != 0) {
            *value |= EGL_OPENGL_ES_BIT;
        }
    }
    return EGL_TRUE;
}

static EGLSurface EGLAPIENTRY wrap_eglCreateWindowSurface(
        EGLDisplay display,
        EGLConfig config,
        EGLNativeWindowType window,
        const EGLint* attribs) {
    // Nothing to do here.
    return sHostEgl->eglCreateWindowSurface(display, config, window, attribs);
}

static EGLSurface EGLAPIENTRY wrap_eglCreatePbufferSurface(
        EGLDisplay display,
        EGLConfig config,
        const EGLint* attribs) {
    // Nothing to do here.
    return sHostEgl->eglCreatePbufferSurface(display, config, attribs);
}

static EGLBoolean EGLAPIENTRY wrap_eglDestroySurface(EGLDisplay display,
                                                     EGLSurface surface) {
    // Nothing to do here.
    return sHostEgl->eglDestroySurface(display, surface);
}

static EGLBoolean EGLAPIENTRY wrap_eglBindAPI(EGLenum api) {
    // This function only takes EGL_OPENGL_ES_API, even for GLESv2
    // so there is nothing to do here.
    return sHostEgl->eglBindAPI(api);
}

static EGLenum EGLAPIENTRY wrap_eglQueryAPI(void) {
    return sHostEgl->eglQueryAPI();
}

static EGLBoolean EGLAPIENTRY wrap_eglReleaseThread(void) {
    // TODO(digit): Add support for releasing GLESv1 out-of-band data for
    // the current thread.
    return sHostEgl->eglReleaseThread();
}

static EGLContext EGLAPIENTRY wrap_eglCreateContext(EGLDisplay display,
                                                    EGLConfig config,
                                                    EGLContext shareContext,
                                                    const EGLint* attribs) {
    // TODO(digit)
    return sHostEgl->eglCreateContext(display, config, shareContext, attribs);
}

static EGLBoolean EGLAPIENTRY wrap_eglDestroyContext(EGLDisplay display,
                                                     EGLContext context) {
    // TODO(digit)
    return sHostEgl->eglDestroyContext(display, context);
}

static EGLBoolean EGLAPIENTRY wrap_eglMakeCurrent(EGLDisplay display,
                                                  EGLSurface draw,
                                                  EGLSurface read,
                                                  EGLContext context) {
    // TODO(digit): Nothing to do here. Really?
    return sHostEgl->eglMakeCurrent(display, draw, read, context);
}

static EGLContext EGLAPIENTRY wrap_eglGetCurrentContext(void) {
    // Nothing to do here.
    return sHostEgl->eglGetCurrentContext();
}

static EGLSurface EGLAPIENTRY wrap_eglGetCurrentSurface(EGLint readdraw) {
    // Nothing to do here.
    return sHostEgl->eglGetCurrentSurface(readdraw);
}

static EGLBoolean EGLAPIENTRY wrap_eglSwapBuffers(EGLDisplay display,
                                                  EGLSurface surface) {
    return sHostEgl->eglSwapBuffers(display, surface);
}

static void* EGLAPIENTRY wrap_eglGetProcAddress(const char* function_name) {
    // TODO(digit)
    return sHostEgl->eglGetProcAddress(function_name);
}

// Extensions

static EGLImageKHR EGLAPIENTRY wrap_eglCreateImageKHR(EGLDisplay display,
                                                      EGLContext context,
                                                      EGLenum target,
                                                      EGLClientBuffer buffer,
                                                      const EGLint* attribs) {
    // Nothing to do here. Since all contexts are GLESv2, sharing EGLImage
    // objects is automatic!
    return sHostEgl->eglCreateImageKHR(
            display, context, target, buffer, attribs);
}

static EGLBoolean EGLAPIENTRY wrap_eglDestroyImageKHR(EGLDisplay display,
                                                      EGLImageKHR image) {
    // Nothing to do here.
    return sHostEgl->eglDestroyImageKHR(display, image);
}

#define DEFINE_WRAPPED_FUNCTION(return_type, function_name, signature) \
    & wrap_ ## function_name,

static const EGLDispatch kWrapperEglDispatch = {
    LIST_RENDER_EGL_FUNCTIONS(DEFINE_WRAPPED_FUNCTION)
    LIST_RENDER_EGL_EXTENSIONS_FUNCTIONS(DEFINE_WRAPPED_FUNCTION)
};

const EGLDispatch* initEglWrapper(const EGLDispatch* hostDispatch) {
    sHostEgl = hostDispatch;
    return &kWrapperEglDispatch;
}
