// Copyright (C) 2018 The Android Open Source Project
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

#include "GoldfishOpenGLDispatch.h"
#include "Standalone.h"
#include "VirtualDisplay.h"

#include "android/base/memory/LazyInstance.h"

#include <EGL/egl.h>

#include <assert.h>
#include <string.h>

#include <system/window.h>

using android::base::LazyInstance;

namespace emugl {

class EGLGlobals {
public:
    EGLGlobals() {
        mGoldfishTables = emugl::loadGoldfishOpenGL();
        aemu::initVirtualDisplay();
        
        assert(mGoldfishTables);
        
        auto& egl = mGoldfishTables->egl;

        EGLDisplay display = egl.eglGetDisplay(EGL_DEFAULT_DISPLAY);

        EGLint maj, min;
        mGoldfishTables->egl.eglInitialize(display, &maj, &min);
    }

    __eglMustCastToProperFunctionPointerType getProcAddress(const char* name) {
        return (__eglMustCastToProperFunctionPointerType)mGoldfishTables->egl.eglGetProcAddress(name);

    }

    const EGLDispatch& egl() const { return mGoldfishTables->egl; }

private:
    const DispatchTables* mGoldfishTables;
};

static LazyInstance<EGLGlobals> sEGLGlobals = LAZY_INSTANCE_INIT;

} // namespace emugl

using emugl::sEGLGlobals;

extern "C" {

EGLAPI EGLDisplay EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType id) {
    return sEGLGlobals->egl().eglGetDisplay(id);
}

EGLAPI EGLBoolean EGLAPIENTRY eglInitialize(EGLDisplay dpy, EGLint* maj, EGLint* min) {
    return sEGLGlobals->egl().eglInitialize(dpy, maj, min);
}

EGLAPI EGLBoolean EGLAPI eglBindAPI(EGLenum api) {
    return emugl::sEGLGlobals->egl().eglBindAPI(api);
}

EGLAPI EGLBoolean EGLAPIENTRY eglBindTexImage (EGLDisplay dpy, EGLSurface surface, EGLint buffer) {
    return EGL_FALSE;
    // return emugl::sEGLGlobals->egl().eglBindTexImage(dpy, surface, buffer);
}

EGLAPI EGLBoolean EGLAPIENTRY eglChooseConfig (EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
    return emugl::sEGLGlobals->egl().eglChooseConfig(dpy, attrib_list, configs, config_size, num_config);
}

EGLAPI EGLint EGLAPIENTRY eglClientWaitSync (EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTime timeout) {
    return emugl::sEGLGlobals->egl().eglClientWaitSyncKHR(dpy, sync, flags, timeout);
}

EGLAPI EGLBoolean EGLAPIENTRY eglCopyBuffers (EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target) {
    return EGL_FALSE;
    // return emugl::sEGLGlobals->egl().eglCopyBuffers(dpy, surface, target);
}

EGLAPI EGLContext EGLAPIENTRY eglCreateContext (EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list) {
    return emugl::sEGLGlobals->egl().eglCreateContext(dpy, config, share_context, attrib_list);
}

EGLAPI EGLImage EGLAPIENTRY eglCreateImage (EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLAttrib *attrib_list) {
    return emugl::sEGLGlobals->egl().eglCreateImageKHR(dpy, ctx, target, buffer, (const EGLint*)attrib_list);
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePbufferFromClientBuffer (EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list) {
    return EGL_NO_SURFACE;
    // return emugl::sEGLGlobals->egl().eglCreatePbufferFromClientBuffer(dpy, buftype, buffer, config, attrib_list);
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePbufferSurface (EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list) {
    return emugl::sEGLGlobals->egl().eglCreatePbufferSurface(dpy, config, attrib_list);
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePixmapSurface (EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list) {
    fprintf(stderr, "%s: unimpl\n", __func__);
    return EGL_NO_SURFACE;
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePlatformPixmapSurface (EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLAttrib *attrib_list) {
    fprintf(stderr, "%s: unimpl\n", __func__);
    return EGL_NO_SURFACE;
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePlatformWindowSurface (EGLDisplay dpy, EGLConfig config, void *native_window, const EGLAttrib *attrib_list) {
    fprintf(stderr, "%s: unimpl\n", __func__);
    return EGL_NO_SURFACE;
}

EGLAPI EGLSync EGLAPIENTRY eglCreateSync (EGLDisplay dpy, EGLenum type, const EGLAttrib *attrib_list) {
    return emugl::sEGLGlobals->egl().eglCreateSyncKHR(dpy, type, (const EGLint*)attrib_list);
}

EGLAPI EGLSurface EGLAPIENTRY eglCreateWindowSurface (EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list) {
    fprintf(stderr, "%s: call (combo)\n", __func__);
    return emugl::sEGLGlobals->egl().eglCreateWindowSurface(dpy, config, win, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroyContext (EGLDisplay dpy, EGLContext ctx) {
    return emugl::sEGLGlobals->egl().eglDestroyContext(dpy, ctx);
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroySurface (EGLDisplay dpy, EGLSurface surface) {
    return emugl::sEGLGlobals->egl().eglDestroySurface(dpy, surface);
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroySync (EGLDisplay dpy, EGLSync sync) {
    return emugl::sEGLGlobals->egl().eglDestroySyncKHR(dpy, sync);
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroyImage (EGLDisplay dpy, EGLImage image) {
    return emugl::sEGLGlobals->egl().eglDestroyImageKHR(dpy, image);
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigAttrib (EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value) {
    EGLBoolean result = emugl::sEGLGlobals->egl().eglGetConfigAttrib(dpy, config, attribute, value);
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigs (EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
    return emugl::sEGLGlobals->egl().eglGetConfigs(dpy, configs, config_size, num_config);
}

EGLAPI EGLContext EGLAPIENTRY eglGetCurrentContext (void) {
    return emugl::sEGLGlobals->egl().eglGetCurrentContext();
}

EGLAPI EGLDisplay EGLAPIENTRY eglGetCurrentDisplay (void) {
    return emugl::sEGLGlobals->egl().eglGetCurrentDisplay();
}

EGLAPI EGLSurface EGLAPIENTRY eglGetCurrentSurface (EGLint readdraw) {
    return emugl::sEGLGlobals->egl().eglGetCurrentSurface(readdraw);
}

EGLAPI EGLint EGLAPIENTRY eglGetError (void) {
    return emugl::sEGLGlobals->egl().eglGetError();
}

EGLAPI EGLDisplay EGLAPIENTRY eglGetPlatformDisplay (EGLenum platform, void *native_display, const EGLAttrib *attrib_list) {
    return EGL_NO_DISPLAY;
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetSyncAttrib (EGLDisplay dpy, EGLSync sync, EGLint attribute, EGLAttrib *value) {
    fprintf(stderr, "%s: unimpl\n", __func__);
    return EGL_FALSE;
    // return emugl::sEGLGlobals->egl().eglGetSyncAttribKHR(dpy, sync, attribute, value);
}

EGLAPI EGLBoolean EGLAPIENTRY eglMakeCurrent (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx) {
    return emugl::sEGLGlobals->egl().eglMakeCurrent(dpy, draw, read, ctx);
}

EGLAPI EGLenum EGLAPIENTRY eglQueryAPI (void) {
    return emugl::sEGLGlobals->egl().eglQueryAPI();
}

EGLAPI EGLBoolean EGLAPIENTRY eglQueryContext (EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value) {
    return emugl::sEGLGlobals->egl().eglQueryContext(dpy, ctx, attribute, value);
}

EGLAPI const char *EGLAPIENTRY eglQueryString (EGLDisplay dpy, EGLint name) {
    return emugl::sEGLGlobals->egl().eglQueryString(dpy, name);
}

EGLAPI EGLBoolean EGLAPIENTRY eglQuerySurface (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value) {
    return emugl::sEGLGlobals->egl().eglQuerySurface(dpy, surface, attribute, value);
}

EGLAPI EGLBoolean EGLAPIENTRY eglReleaseTexImage (EGLDisplay dpy, EGLSurface surface, EGLint buffer) {
    fprintf(stderr, "%s: unimpl\n", __func__);
    return EGL_FALSE;
    // return emugl::sEGLGlobals->egl().eglReleaseTexImage(dpy, surface, buffer);
}

EGLAPI EGLBoolean EGLAPIENTRY eglReleaseThread (void) {
    return emugl::sEGLGlobals->egl().eglReleaseThread();
}

EGLAPI EGLBoolean EGLAPIENTRY eglSurfaceAttrib (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value) {
    fprintf(stderr, "%s: unimpl\n", __func__);
    return EGL_FALSE;
    // return emugl::sEGLGlobals->egl().eglSurfaceAttrib(dpy, surface, attribute, value);
}

EGLAPI EGLBoolean EGLAPIENTRY eglSwapBuffers (EGLDisplay dpy, EGLSurface surface) {
    return emugl::sEGLGlobals->egl().eglSwapBuffers(dpy, surface);
}

EGLAPI EGLBoolean EGLAPIENTRY eglSwapInterval (EGLDisplay dpy, EGLint interval) {
    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglTerminate (EGLDisplay dpy) {
    return emugl::sEGLGlobals->egl().eglTerminate(dpy);
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitClient (void) {
    fprintf(stderr, "%s: unimpl\n", __func__);
    return EGL_TRUE;
    // return emugl::sEGLGlobals->egl().eglWaitClient();
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitGL (void) {
    return EGL_TRUE;
    // return emugl::sEGLGlobals->egl().eglWaitGL();
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitNative (EGLint engine) {
    return EGL_TRUE;
    // return emugl::sEGLGlobals->egl.eglWaitNative();
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitSync (EGLDisplay dpy, EGLSync sync, EGLint flags) {
    return emugl::sEGLGlobals->egl().eglWaitSyncKHR(dpy, sync, flags);
}

EGLAPI __eglMustCastToProperFunctionPointerType EGLAPIENTRY eglGetProcAddress(const char *procname)
{
    if (!strcmp(procname, "eglGetDisplay")) { return (__eglMustCastToProperFunctionPointerType)eglGetDisplay; }
    if (!strcmp(procname, "eglInitialize")) { return (__eglMustCastToProperFunctionPointerType)eglInitialize; }

    // search in goldfish-opengl
    return emugl::sEGLGlobals->getProcAddress(procname);
}

EGLAPI void* EGLAPIENTRY aemuCreateWindow(int width, int height) {
    aemu::initVirtualDisplay();
    return aemu::createWindow();
}

EGLAPI void EGLAPIENTRY aemuDestroyWindow(void* window) {
    fprintf(stderr, "%s: destroy %p\n", __func__, window);
    // struct ANativeWindow* res = (ANativeWindow*)window;
    // delete res;
}

} // extern "C"
