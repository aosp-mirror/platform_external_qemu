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

#include "FenceSyncInfo.h"

#include "DispatchTables.h"
#include "FrameBuffer.h"
#include "OpenGLESDispatch/EGLDispatch.h"

#include "android/base/Compiler.h"
#include "android/base/containers/Lookup.h"
#include "android/base/synchronization/Lock.h"

#include "android/utils/assert.h"

using android::base::AutoReadLock;
using android::base::AutoWriteLock;
using android::base::ReadWriteLock;

FenceSync::FenceSync(bool hasNativeFence,
                     bool destroyWhenSignaled) :
    mDestroyWhenSignaled(destroyWhenSignaled),
    mCount(hasNativeFence ? 2 : 1) {

    mDisplay = FrameBuffer::getFB()->getDisplay();
    mGLSync = s_egl.eglCreateSyncKHR(mDisplay,
                                     EGL_SYNC_FENCE_KHR,
                                     NULL);
}

EGLint FenceSync::wait(uint64_t timeout) {
    getRef();
    EGLint wait_res =
        s_egl.eglClientWaitSyncKHR(mDisplay, mGLSync,
                                   EGL_SYNC_FLUSH_COMMANDS_BIT_KHR,
                                   timeout);
    putRef();
    return wait_res;
}

void FenceSync::signaledNativeFd() {
    putRef();
    if (mDestroyWhenSignaled) {
        putRef();
    }
}

void FenceSync::destroy() {
    s_egl.eglDestroySyncKHR(mDisplay, mGLSync);
}

FenceDestroyer::~FenceDestroyer() {
    if (sync->putRef()) {
        sync->destroy();
        delete sync;
    }
}
