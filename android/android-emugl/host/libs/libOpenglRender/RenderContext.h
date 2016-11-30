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
#ifndef _LIBRENDER_RENDER_CONTEXT_H
#define _LIBRENDER_RENDER_CONTEXT_H

#include "emugl/common/smart_ptr.h"
#include "GLDecoderContextData.h"

#include <EGL/egl.h>

// A class used to model a guest EGLContext. This simply wraps a host
// EGLContext, associated with an GLDecoderContextData instance that is
// used to store copies of guest-side arrays.
class RenderContext {
public:
    // Create a new RenderContext instance.
    // |display| is the host EGLDisplay handle.
    // |config| is the host EGLConfig to use.
    // |sharedContext| is either EGL_NO_CONTEXT of a host EGLContext handle.
    // |version| specifies the GLES version ({1,2,3})
    static RenderContext *create(EGLDisplay display,
                                 EGLConfig config,
                                 EGLContext sharedContext,
                                 int version = 1);

    // Destructor.
    ~RenderContext();

    // Retrieve host EGLContext value.
    EGLContext getEGLContext() const { return mContext; }

    // Retrieve emulated GLES1 context.
    void* getEmulatedGLES1Context() const { return mEmulatedGLES1Context; }

    // Return the actual underlying version used in this context.
    int version() const { return mVersion; }

    // Retrieve GLDecoderContextData instance reference for this
    // RenderContext instance.
    GLDecoderContextData& decoderContextData() { return mContextData; }

private:
    RenderContext();

    RenderContext(EGLDisplay display,
                  EGLContext context,
                  int version,
                  void* emulatedGLES1Context);

private:
    EGLDisplay mDisplay;
    EGLContext mContext;
    int mVersion;
    void* mEmulatedGLES1Context;
    GLDecoderContextData mContextData;
};

typedef emugl::SmartPtr<RenderContext> RenderContextPtr;

#endif  // _LIBRENDER_RENDER_CONTEXT_H
