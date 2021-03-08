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

#include "android/base/misc/StringUtils.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "emugl/common/feature_control.h"
#include "emugl/common/misc.h"

#include <algorithm>

using android::base::c_str;
using android::base::OsType;
using android::base::StringView;
using android::base::System;

// Config + context attributes to query the underlying OpenGL if it is
// a OpenGL ES backend. Only try for OpenGL ES 3, and assume OpenGL ES 2
// exists (if it doesn't, this is the least of our problems).
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

    // TODO: 3.1 is the highest
    GLESDispatchMaxVersion maxVersion =
       GLES_DISPATCH_MAX_VERSION_3_1;

    // TODO: CTS conformance for OpenGL ES 3.1
    bool playStoreImage = emugl::emugl_feature_is_enabled(
            android::featurecontrol::PlayStoreImage);

    if (emugl::getRenderer() == SELECTED_RENDERER_HOST
        || emugl::getRenderer() == SELECTED_RENDERER_SWIFTSHADER_INDIRECT
        || emugl::getRenderer() == SELECTED_RENDERER_ANGLE_INDIRECT
        || emugl::getRenderer() == SELECTED_RENDERER_ANGLE9_INDIRECT) {
        if (s_egl.eglGetMaxGLESVersion) {
            maxVersion =
                (GLESDispatchMaxVersion)s_egl.eglGetMaxGLESVersion(dpy);
        }
    } else {
        if (playStoreImage ||
            !sTryContextCreation(dpy, GLES_DISPATCH_MAX_VERSION_3_1)) {
            maxVersion = GLES_DISPATCH_MAX_VERSION_3_0;
            if (!sTryContextCreation(dpy, GLES_DISPATCH_MAX_VERSION_3_0)) {
                maxVersion = GLES_DISPATCH_MAX_VERSION_2;
            }
        }
    }

    if (playStoreImage) {
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
    int dispatchMaj, dispatchMin;

    emugl::getGlesVersion(&dispatchMaj, &dispatchMin);
    return emugl::getRenderer() == SELECTED_RENDERER_HOST &&
           dispatchMaj > 2;
}

void sAddExtensionIfSupported(GLESDispatchMaxVersion currVersion,
                              const std::string& from,
                              GLESDispatchMaxVersion extVersion,
                              StringView ext,
                              std::string& to) {
    // If we chose a GLES version less than or equal to
    // the |extVersion| the extension |ext| is tagged with,
    // filter it according to the whitelist.
    if (emugl::hasExtension(from.c_str(), c_str(ext)) &&
        currVersion > extVersion) {
        to += ext;
        to += " ";
    }
}

static bool sWhitelistedExtensionsGLES2(StringView hostExt) {

#define WHITELIST(ext) \
    if (hostExt == #ext) return true; \

WHITELIST(GL_OES_compressed_ETC1_RGB8_texture)
WHITELIST(GL_OES_depth24)
WHITELIST(GL_OES_depth32)
WHITELIST(GL_OES_depth_texture)
WHITELIST(GL_OES_depth_texture_cube_map)
WHITELIST(GL_OES_EGL_image)
WHITELIST(GL_OES_EGL_image_external)
WHITELIST(GL_OES_EGL_sync)
WHITELIST(GL_OES_element_index_uint)
WHITELIST(GL_OES_framebuffer_object)
WHITELIST(GL_OES_packed_depth_stencil)
WHITELIST(GL_OES_rgb8_rgba8)
WHITELIST(GL_OES_standard_derivatives)
WHITELIST(GL_OES_texture_float)
WHITELIST(GL_OES_texture_float_linear)
WHITELIST(GL_OES_texture_half_float)
WHITELIST(GL_OES_texture_half_float_linear)
WHITELIST(GL_OES_texture_npot)
WHITELIST(GL_OES_texture_3D)
WHITELIST(GL_EXT_blend_minmax)
WHITELIST(GL_EXT_color_buffer_half_float)
WHITELIST(GL_EXT_draw_buffers)
WHITELIST(GL_EXT_instanced_arrays)
WHITELIST(GL_EXT_occlusion_query_boolean)
WHITELIST(GL_EXT_read_format_bgra)
WHITELIST(GL_EXT_texture_filter_anisotropic)
WHITELIST(GL_EXT_texture_format_BGRA8888)
WHITELIST(GL_EXT_texture_rg)
WHITELIST(GL_ANGLE_framebuffer_blit)
WHITELIST(GL_ANGLE_framebuffer_multisample)
WHITELIST(GL_ANGLE_instanced_arrays)
WHITELIST(GL_CHROMIUM_texture_filtering_hint)
WHITELIST(GL_NV_fence)
WHITELIST(GL_NV_framebuffer_blit)
WHITELIST(GL_NV_read_depth)

#undef WHITELIST

    return false;
}

std::string filterExtensionsBasedOnMaxVersion(GLESDispatchMaxVersion ver,
                                              const std::string& exts) {
    // We need to advertise ES 2 extensions if:
    // a. the dispatch version on the host is ES 2
    // b. the guest image is not updated for ES 3+
    // (GLESDynamicVersion is disabled)
    if (ver > GLES_DISPATCH_MAX_VERSION_2 &&
        emugl::emugl_feature_is_enabled(
                android::featurecontrol::GLESDynamicVersion)) {
        return exts;
    }

    std::string filteredExtensions;
    filteredExtensions.reserve(4096);
    auto add = [ver, &filteredExtensions](StringView hostExt) {
        if (!hostExt.empty() &&
            sWhitelistedExtensionsGLES2(hostExt)) {
            filteredExtensions += hostExt;
            filteredExtensions += " ";
        }
    };

    android::base::split(exts, " ", add);

    return filteredExtensions;
}
