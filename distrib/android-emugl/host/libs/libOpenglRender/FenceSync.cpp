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

#include "FenceSync.h"

#include "DispatchTables.h"
#include "FrameBuffer.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "RenderThreadInfo.h"

#include "android/base/containers/Lookup.h"

FenceSync::FenceSync(bool hasNativeFence,
                     bool destroyWhenSignaled) :
    mDestroyWhenSignaled(destroyWhenSignaled) {

    assert(mCount == 1);
    if (hasNativeFence) incRef();

    // assumes that there is a valid + current OpenGL context
    assert(RenderThreadInfo::get());
    assert(RenderThreadInfo::get()->currContext.get());

    mDisplay = FrameBuffer::getFB()->getDisplay();
    mSync = s_egl.eglCreateSyncKHR(mDisplay,
                                   EGL_SYNC_FENCE_KHR,
                                   NULL);
}

EGLint FenceSync::wait(uint64_t timeout) {
    incRef();
    EGLint wait_res =
        s_egl.eglClientWaitSyncKHR(mDisplay, mSync,
                                   EGL_SYNC_FLUSH_COMMANDS_BIT_KHR,
                                   timeout);
    decRef();
    return wait_res;
}

void FenceSync::signaledNativeFd() {
    if (!decRef() && mDestroyWhenSignaled) {
        decRef();
    }
    // NOTE: Do not use anything like this construction:
    // decRef();
    // if (mDestroyWhenSignaled) ...
    // If the object was originally constructed with
    // mDestroyWhenSignaled == false,
    // the first decRef() will have destroyed this object,
    // which would make subsequent access to
    // |mDestroyWhenSignaled| corrupt memory immediately.
    // Please keep this in mind.
}

void FenceSync::destroy() {
    s_egl.eglDestroySyncKHR(mDisplay, mSync);
}
