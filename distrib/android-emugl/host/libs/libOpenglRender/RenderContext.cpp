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

#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv1Dispatch.h"

extern GLESv1Dispatch s_gles1;

RenderContext* RenderContext::create(EGLDisplay display,
                                     EGLConfig config,
                                     EGLContext sharedContext,
                                     bool isGl2) {
    void* emulatedGLES1Context = NULL;

    if (!isGl2) { fprintf(stderr, "%s: trying to create gles1 context\n",
                          __func__); }
            
    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLContext context = s_egl.eglCreateContext(
            display, config, sharedContext, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        fprintf(stderr, "%s: failed to create context (EGL_NO_CONTEXT result)\n", __func__);
        return NULL;
    }

    fprintf(stderr, "%s: gles12tr ptr: %p\n", __FUNCTION__,
            s_gles1.underlying_gles2_api);
    if (s_gles1.underlying_gles2_api != NULL) {
        fprintf(stderr, "%s: should be creating a emulated gles1 context here\n", __FUNCTION__);
        if (sharedContext == EGL_NO_CONTEXT) {
            fprintf(stderr, "%s: thankfully, this context isn't sharing anything :)\n", __FUNCTION__);
        } else {
            fprintf(stderr, "%s: this context is shared. need to maintain a sort of map I guess\n", __FUNCTION__);
        }
        emulatedGLES1Context = s_gles1.create_gles1_context(NULL, s_gles1.underlying_gles2_api);
    }

    return new RenderContext(display, context, isGl2, emulatedGLES1Context);
}

RenderContext::RenderContext(EGLDisplay display,
                             EGLContext context,
                             bool isGl2,
                             void* emulatedGLES1Context) :
        mDisplay(display),
        mContext(context),
        mIsGl2(isGl2),
        mEmulatedGLES1Context(emulatedGLES1Context),
        mContextData() {}

RenderContext::~RenderContext() {
    if (mContext != EGL_NO_CONTEXT) {
        s_egl.eglDestroyContext(mDisplay, mContext);
    }
}
