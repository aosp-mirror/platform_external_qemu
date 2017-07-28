/*
* Copyright (C) 2016 The Android Open Source Project
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

#include "GLESVersionDetector.h"

#include "EGLDispatch.h"

#include "android/base/system/System.h"
#include "emugl/common/feature_control.h"
#include "emugl/common/misc.h"

#include <algorithm>

using android::base::System;
using android::base::OsType;
// Config + context attributes to query the underlying OpenGL if it is
// a OpenGL ES backend. Only try for OpenGL ES 3, and assume OpenGL ES 2
// exists (if it doesnt, this is the least of our problems).
static const EGLint gles3ConfigAttribs[] =
    { EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT | EGL_OPENGL_ES3_BIT, EGL_NONE };

static const EGLint pbufAttribs[] =
    { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };

static const EGLint gles31Attribs[] =
   { EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
     EGL_CONTEXT_MINOR_VERSION_KHR, 1, EGL_NONE };

static const EGLint gles30Attribs[] =
   { EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
     EGL_CONTEXT_MINOR_VERSION_KHR, 0, EGL_NONE };

static bool sTryContextCreation(EGLDisplay dpy, GLESDispatchMaxVersion ver) {
    EGLConfig config;
    EGLSurface surface;

    const EGLint* contextAttribs = nullptr;

    // Assume ES2 capable.
    if (ver == GLES_DISPATCH_MAX_VERSION_2) return true;

    switch (ver) {
    case GLES_DISPATCH_MAX_VERSION_3_0:
        contextAttribs = gles30Attribs;
        break;
    case GLES_DISPATCH_MAX_VERSION_3_1:
        contextAttribs = gles31Attribs;
        break;
    default:
        break;
    }

    if (!contextAttribs) return false;

    int numConfigs;
    if (!s_egl.eglChooseConfig(
            dpy, gles3ConfigAttribs, &config, 1, &numConfigs) ||
        numConfigs == 0) {
        return false;
    }

    surface = s_egl.eglCreatePbufferSurface(dpy, config, pbufAttribs);
    if (surface == EGL_NO_SURFACE) {
        return false;
    }

    EGLContext ctx = s_egl.eglCreateContext(dpy, config, EGL_NO_CONTEXT,
                                            contextAttribs);

    if (ctx == EGL_NO_CONTEXT) {
        s_egl.eglDestroySurface(dpy, surface);
        return false;
    } else {
        s_egl.eglDestroyContext(dpy, ctx);
        s_egl.eglDestroySurface(dpy, surface);
        return true;
    }
}

GLESDispatchMaxVersion calcMaxVersionFromDispatch(EGLDisplay dpy) {
    // Don't try to detect anything if GLESDynamicVersion is disabled.
    if (!emugl_feature_is_enabled(
            android::featurecontrol::GLESDynamicVersion)) {
        return GLES_DISPATCH_MAX_VERSION_2;
    }

    // TODO: 3.1 is the highest
    GLESDispatchMaxVersion maxVersion =
       GLES_DISPATCH_MAX_VERSION_3_1;

    if (emugl::getRenderer() == SELECTED_RENDERER_HOST) {
        if (s_egl.eglGetMaxGLESVersion) {
            maxVersion =
                (GLESDispatchMaxVersion)s_egl.eglGetMaxGLESVersion(dpy);
        }
    } else {
        if (!sTryContextCreation(dpy, GLES_DISPATCH_MAX_VERSION_3_1)) {
            maxVersion = GLES_DISPATCH_MAX_VERSION_3_0;
            if (!sTryContextCreation(dpy, GLES_DISPATCH_MAX_VERSION_3_0)) {
                maxVersion = GLES_DISPATCH_MAX_VERSION_2;
            }
        }
    }

    // TODO: CTS conformance for OpenGL ES 3.1
    if (emugl_feature_is_enabled(
                android::featurecontrol::PlayStoreImage)) {
        maxVersion =
            std::min(maxVersion,
                     GLES_DISPATCH_MAX_VERSION_3_0);
    }

    int maj = 2; int min = 0;
    switch (maxVersion) {
        case GLES_DISPATCH_MAX_VERSION_2:
            maj = 2; min = 0; break;
        case GLES_DISPATCH_MAX_VERSION_3_0:
            maj = 3; min = 0; break;
        case GLES_DISPATCH_MAX_VERSION_3_1:
            maj = 3; min = 1; break;
        case GLES_DISPATCH_MAX_VERSION_3_2:
            maj = 3; min = 2; break;
        default:
            break;
    }

    emugl::setGlesVersion(maj, min);

    return maxVersion;
}

// For determining whether or not to use core profile OpenGL.
// (Note: This does not affect the detection of possible core profile configs,
// just whether to use them)
bool shouldEnableCoreProfile() {

    // TODO: Remove once CTS conformant
    if (System::get()->getOsType() == OsType::Linux &&
        emugl_feature_is_enabled(android::featurecontrol::PlayStoreImage)) {
        return false;
    }

    return emugl::getRenderer() == SELECTED_RENDERER_HOST &&
           emugl_feature_is_enabled(android::featurecontrol::GLESDynamicVersion);
}
