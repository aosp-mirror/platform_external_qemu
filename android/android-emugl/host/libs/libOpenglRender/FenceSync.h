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

#pragma once

#include "aemu/base/Compiler.h"
#include "aemu/base/files/Stream.h"
#include "aemu/base/memory/LazyInstance.h"
#include "aemu/base/synchronization/Lock.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <atomic>

// The FenceSync class wraps actual EGLSyncKHR objects
// and issues calls to eglCreateSyncKHR, eglClientWaitSyncKHR,
// and eglDestroySyncKHR.
//
// The purpose of this class:
// - We need to track EGL sync objects created by the guest and
//   realized in the host OpenGL driver. They are passed between
//   guest and host all the time.
// - In particular, we also need to destroy EGL sync objects at
//   the proper time. There are at least 3 issues (referenced below
//   in spec comments):
//   1 According to spec, we would need to allow concurrent
//     eglClientWaitSyncKHR and eglDestroySyncKHR to all finish
//     properly.
//   2 If the EGL sync object is of EGL_SYNC_NATIVE_FENCE_ANDROID
//     nature, we cannot mirror the guest's call to eglDestroySyncKHR
//     by calling the same function on the host, because Goldfish
//     sync device can only know when native fence FD's are signaled
//     when a host-side EGL sync object is signaled. Thus, we would
//     need to delete such sync objects after both the guest and
//     the Goldfish sync device are done with them.
//   3 We sometimes create sync objects that are only seen by
//     the Goldfish OpenGL driver in the guest, such as for
//     implementing eglSwapBuffers() in a way that avoids
//     out of order frames. It is cumbersome to eglDestroySyncKHR
//     those on the guest, as that would require starting up another guest
//     thread and OpenGL context (complete with host connection)
//     to destroy it.
class FenceSync {
public:
    // The constructor wraps eglCreateSyncKHR on the host OpenGL driver.
    // |hasNativeFence| specifies whether this sync object
    // is of EGL_SYNC_NATIVE_FENCE_ANDROID nature (2), and
    // |destroyWhenSignaled| specifies whether or not to destroy
    // the sync object when the native fence FD becomes signaled (3).
    FenceSync(bool hasNativeFence,
              bool destroyWhenSignaled);
    ~FenceSync();

    // wait() wraps eglClientWaitSyncKHR. During such a wait, we need
    // to increment the reference count while the wait is active,
    // in case there is a concurrent call to eglDestroySyncKHR (1).
    EGLint wait(uint64_t timeout);

    // waitAsync wraps eglWaitSyncKHR.
    void waitAsync();

    // isSignaled wraps eglGetSyncAttribKHR.
    bool isSignaled();

    bool shouldDestroyWhenSignaled() const {
        return mDestroyWhenSignaled;
    }

    // When a native fence gets signaled, this function is called to update the
    // timeline counter in the FenceSync internal timeline and delete old
    // fences.
    static void incrementTimelineAndDeleteOldFences();

    // incRef() / decRef() increment/decrement refence counts in order
    // to deal with sync object destruction. This is a simple reference
    // counting implementation that is almost literally the kref() in
    // the Linux kernel.
    //
    // We do not use shared_ptr or anything like that here because
    // we need to explicitly manipulate the reference count in order to
    // satisfy (1,2,3) above.
    void incRef() { assert(mCount > 0); ++mCount; }
    bool decRef() {
        assert(mCount > 0);
        if (mCount == 1 || --mCount == 0) {
            // destroy() here would delay calls to eglDestroySyncKHR
            // in the host driver until all waits have completed,
            // which is a bit different from simply allowing concurrent calls.
            // But, from the guest's perspective, the contract of allowing
            // everything to finish is still fulfilled, and there is
            // no reason to think (theoretically or practically) that
            // is undesirable to destroy the underlying EGL sync object
            // a tiny bit later. We could have put in extra logic to allow
            // concurrent destruction, but this would have made the code
            // undesirably less simple.
            destroy();
            // This delete-then-return seems OK.
            delete this;
            return true;
        }
        return false;
    }

    void setIsCompositionFence(bool isComposition) {
        mIsCompositionFence = isComposition;
    }

    bool isCompositionFence() const {
        return mIsCompositionFence;
    }

    // Tracks current active set of fences. Useful for snapshotting.
    void addToRegistry();
    void removeFromRegistry();

    static FenceSync* getFromHandle(uint64_t handle);

    // Functions for snapshotting all fence state at once
    static void onSave(android::base::Stream* stream);
    static void onLoad(android::base::Stream* stream);
private:
    bool mDestroyWhenSignaled;
    std::atomic<int> mCount {1};

    // EGL state needed for calling OpenGL sync operations.
    EGLDisplay mDisplay;
    EGLSyncKHR mSync;

    // Whether this fence was against composition, in which case
    // we should make this wait till next vsync.
    bool mIsCompositionFence = false;

    // destroy() wraps eglDestroySyncKHR. This is private, because we need
    // careful control of when eglDestroySyncKHR is actually called.
    void destroy();

    DISALLOW_COPY_AND_ASSIGN(FenceSync);
};
