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
#include "RenderContext.h"

#include "GLESVersionDetector.h"

#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv1Dispatch.h"

#include "android/base/containers/SmallVector.h"

#include "emugl/common/feature_control.h"
#include "emugl/common/misc.h"

#include <assert.h>
#include <OpenglCodecCommon/ErrorLog.h>

RenderContext* RenderContext::create(EGLDisplay display,
                                     EGLConfig config,
                                     EGLContext sharedContext,
                                     HandleType hndl,
                                     GLESApi version) {
    return createImpl(display, config, sharedContext, hndl, version, nullptr);
}

RenderContext* RenderContext::createImpl(EGLDisplay display,
                                     EGLConfig config,
                                     EGLContext sharedContext,
                                     HandleType hndl,
                                     GLESApi version,
                                     android::base::Stream *stream) {
    void* emulatedGLES1Context = NULL;

    GLESApi clientVersion = version;
    int majorVersion = clientVersion;
    int minorVersion = 0;

    if (version == GLESApi_3_0) {
        majorVersion = 3;
        minorVersion = 0;
    } else if (version == GLESApi_3_1) {
        majorVersion = 3;
        minorVersion = 1;
    }

    android::base::SmallFixedVector<EGLint, 7> contextAttribs = {
        EGL_CONTEXT_CLIENT_VERSION, majorVersion,
        EGL_CONTEXT_MINOR_VERSION_KHR, minorVersion,
    };

    if (shouldEnableCoreProfile()) {
        contextAttribs.push_back(EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR);
        contextAttribs.push_back(EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR);
    }

    contextAttribs.push_back(EGL_NONE);

    EGLContext context;
    if (stream && s_egl.eglLoadContext) {
        context = s_egl.eglLoadContext(display, &contextAttribs[0], stream);
    } else {
        context = s_egl.eglCreateContext(
            display, config, sharedContext, &contextAttribs[0]);
    }
    if (context == EGL_NO_CONTEXT) {
        fprintf(stderr, "%s: failed to create context (EGL_NO_CONTEXT result)\n", __func__);
        return NULL;
    }

    return new RenderContext(display, context, hndl, clientVersion, NULL);
}

RenderContext::RenderContext(EGLDisplay display,
                             EGLContext context,
                             HandleType hndl,
                             GLESApi version,
                             void* emulatedGLES1Context) :
        mDisplay(display),
        mContext(context),
        mHndl(hndl),
        mVersion(version),
        mContextData() { }

RenderContext::~RenderContext() {
    if (mContext != EGL_NO_CONTEXT) {
        s_egl.eglDestroyContext(mDisplay, mContext);
    }
}

void RenderContext::onSave(android::base::Stream* stream) {
    stream->putBe32(mHndl);
    stream->putBe32(static_cast<uint32_t>(mVersion));
    assert(s_egl.eglCreateContext);
    if (s_egl.eglSaveContext) {
        s_egl.eglSaveContext(mDisplay, mContext, static_cast<EGLStream>(stream));
    }
}

RenderContext *RenderContext::onLoad(android::base::Stream* stream,
            EGLDisplay display) {
    HandleType hndl = static_cast<HandleType>(stream->getBe32());
    GLESApi version = static_cast<GLESApi>(stream->getBe32());

    return createImpl(display, (EGLConfig)0, EGL_NO_CONTEXT, hndl, version,
                      stream);
}

GLESApi RenderContext::clientVersion() const {
    return mVersion;
}
