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

#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <atomic>
#include <memory>
#include <unordered_map>

// The FenceSync class wraps actual EGLSyncKHR objects,
// issuing calls to eglCreateSyncKHR/eglClientWaitSyncKHR/
// eglDestroySyncKHR.
//
// We need this class because we need to track, on the host,
// the EGL sync objects created by the guest and realized
// in the host OpenGL driver.
//
// Because this class is used for tracking, it must also mirror
// the construction/destruction behavior of real OpenGL sync
// objects. As such, there are subtleties, described below,
// regarding when to destroy FenceSync objects.
class FenceSync {
public:
    // The constructor wraps eglCreateSyncKHR on the
    // host OpenGL driver.
    // |hasNativeFence| specifies whether this sync object
    // is associated with a native fence FD object
    // in the Android sync framework. In this case, we will
    // need to be more careful about when we free this object.
    // |destroyWhenSignaled| specifies whether to mark this
    // object for destruction when its native fence FD
    // becomes signaled (in our case, roughly when the
    // EGL fence object becomes signaled). This is used
    // for the case where the goldfish opengl driver in the guest
    // wants to create EGL sync objects on its own.
    // Otherwise, we will rely on the guest's calls to
    // eglDestroySyncKHR / host's rcDestroySyncKHR to
    // clean up this object.
    FenceSync(bool hasNativeFence,
              bool destroyWhenSignaled);

    // wait() wraps eglClientWaitSyncKHR. To be extra conservative,
    // it also manipulates the reference count in case we ever
    // call wait() outside of a context that uses the
    // FenceDestroyer class, described below.
    EGLint wait(uint64_t timeout);

    // signaledNativeFd(): upon when the native fence fd
    // is signaled due to the sync object being signaled,
    // this method does the following:
    // - adjusts the reference count
    // - destroy()s the sync object if |mDestroyWhenSignaled|.
    void signaledNativeFd();

    // destroy() wraps eglDestroySyncKHR. If used in conjunction
    // with a FenceDestroyer object, the FenceSync object is marked
    // for deletion and will be deleted when the FenceDestroyer
    // object goes out of scope.
    void destroy();

    // Ref counts for dealing with correct sync object destruction
    // in the presence of concurrent waits / destroys.
    // This is a simple reference counting implementation
    // that is pretty much just the kref.
    //
    // We do not use shared_ptr or anything like that here
    // because we need to explicitly manipulate the reference count
    // in the case of sync objects that use native fences:
    //
    // When a sync object is of native fence nature, we need to
    // keep the sync object around long enough to complete the
    // wait and signal the fence fd through the goldfish sync
    // virtual device. Otherwise, we will either signal the
    // fence fd too early or end up freeing the FenceSync object
    // too early.
    void getRef() { assert(mCount > 0); ++mCount; }
    bool putRef() {
        assert(mCount > 0);

        if (mCount == 1 || --mCount == 0) {
            return true;
        }
        return false;
    }
private:
    bool mDestroyWhenSignaled;
    std::atomic<int> mCount;
    EGLDisplay mDisplay;
    EGLSyncKHR mGLSync;
};

// FenceDestroyer is to be used in any situation where
// we might potentially end up freeing a FenceSync object,
// which is any wait() or destroy() call, plus any
// signaledNativeFd() call on a sync object generated
// by the goldfish opengl driver itself, in which case
// the guest will not even notice that sync object, and
// we need to clean it up on the host by ourselves.
class FenceDestroyer {
public:
    explicit FenceDestroyer(FenceSync* f) :
        sync(f) {
        assert(sync);
        sync->getRef();
    }
    ~FenceDestroyer();
    FenceSync* sync;
};
