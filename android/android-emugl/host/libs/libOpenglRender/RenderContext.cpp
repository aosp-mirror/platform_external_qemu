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

extern GLESv1Dispatch s_gles1;

static int MAGICKEST_NUMBER = 111111;

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

    bool shouldEmulateGLES1 = s_gles1.underlying_gles2_api != NULL;

    GLESApi clientVersion = version;
    int majorVersion = clientVersion;
    int minorVersion = 0;

    if (version == GLESApi_CM && shouldEmulateGLES1) {
        clientVersion = GLESApi_2;
        majorVersion = 2;
    } else if (version == GLESApi_3_0) {
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
        fprintf(stderr, "%s Loading EglContext.\n", __FILE__);
        context = s_egl.eglLoadContext(display, &contextAttribs[0], stream);
    } else {
        fprintf(stderr, "%s Creating EglContext.\n", __FILE__);
        context = s_egl.eglCreateContext(
            display, config, sharedContext, &contextAttribs[0]);
    }

    if (stream) {
        fprintf(stderr, "%s loaded MAGICKEST_NUMBER %d: %d\n", __FILE__, MAGICKEST_NUMBER, stream->getBe32());
    }

    if (context == EGL_NO_CONTEXT) {
        fprintf(stderr, "%s: failed to create context (EGL_NO_CONTEXT result)\n", __func__);
        return NULL;
    }

    if (shouldEmulateGLES1) {
        DBG("%s: should be creating a emulated gles1 context here\n", __FUNCTION__);
        if (sharedContext == EGL_NO_CONTEXT) {
            DBG("%s: thankfully, this context isn't sharing anything :)\n", __FUNCTION__);
        } else {
            DBG("%s: this context is shared. need to maintain a sort of map I guess\n", __FUNCTION__);
        }
        emulatedGLES1Context = s_gles1.create_gles1_context(NULL, s_gles1.underlying_gles2_api);
        DBG("%s: created a emulated gles1 context @ %p\n", __FUNCTION__, emulatedGLES1Context);
        return new RenderContext(display, context, hndl, clientVersion, emulatedGLES1Context);
    } else {
        return new RenderContext(display, context, hndl, clientVersion, NULL);
    }
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
        mEmulatedGLES1Context(emulatedGLES1Context),
        mContextData() { }

RenderContext::~RenderContext() {
    if (mContext != EGL_NO_CONTEXT) {
        s_egl.eglDestroyContext(mDisplay, mContext);
    }
    if (mEmulatedGLES1Context != NULL) {
        s_gles1.destroy_gles1_context(mEmulatedGLES1Context);
        mEmulatedGLES1Context = NULL;
    }
}

void RenderContext::onSave(android::base::Stream* stream) {
    fprintf(stderr, "\n\n%s %s %d ^^^^^^^^^^^ rendercontext onsave\n", __FILE__, __func__, __LINE__);
    fprintf(stderr, "rctx hdl: %d, ver: %d\n", mHndl, static_cast<uint32_t>(mVersion));
    stream->putBe32(mHndl);
    stream->putBe32(static_cast<uint32_t>(mVersion));
    assert(s_egl.eglCreateContext);
    if (s_egl.eglSaveContext) {
        fprintf(stderr, "%s %s %d saving context in rendercontext onsave\n", __FILE__, __func__, __LINE__);
        s_egl.eglSaveContext(mDisplay, mContext, static_cast<EGLStream>(stream));
    }
    else {
        fprintf(stderr, "%s %s %d no context save in rendercontext onsave \n", __FILE__, __func__, __LINE__);
    }
    stream->putBe32(MAGICKEST_NUMBER);
    fprintf(stderr, "saved MAGICKEST_NUMBER %d\n", MAGICKEST_NUMBER);
}

RenderContext *RenderContext::onLoad(android::base::Stream* stream,
            EGLDisplay display) {
    HandleType hndl = static_cast<HandleType>(stream->getBe32());
    GLESApi version = static_cast<GLESApi>(stream->getBe32());
    fprintf(stderr, "\n\n%s %s %d vvvvvvvvvvvvvvv rendercontext onload\n", __FILE__, __func__, __LINE__);
    fprintf(stderr, "rctx hdl: %d, ver: %d\n", hndl, static_cast<uint32_t>(version));
    return createImpl(display, (EGLConfig)0, EGL_NO_CONTEXT, hndl, version,
                      stream);
}

GLESApi RenderContext::clientVersion() const {
    if (getEmulatedGLES1Context()) {
        return GLESApi_CM;
    }
    return mVersion;
}
