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
// The purpose of this class:
// - We need to track, on the host, the EGL sync objects
//   created by the guest and realized in the host OpenGL driver.
// - As an immediate consequence, we need to mirror
//   creation/destruction semantics EGL sync objects in the
//   host OpenGL driver, which, according to spec, would require
//   tracking of eglClientWaitSyncKHR calls that are in flight
//   during a call to eglDestroySyncKHR.
// - We need to deal with EGL_SYNC_NATIVE_FENCE_ANDROID-type
//   EGL sync objects which will need different destruction
//   semantics on the host side. A common pattern is to first
//   eglCreateSyncKHR a EGL_SYNC_NATIVE_FENCE_ANDROID sync object,
//   dup the FD with eglDupNativeFenceFDANDROID, and immediately
//   eglDestroySyncKHR the sync object in the same thread.
//   We cannot directly mirror the call to eglDestroySyncKHR
//   because we will utilize the original EGL sync object on
//   the host to signal the native fence FD. As such, we need
//   to delay the destruction of the host-side sync object
//   until the native fence FD is signaled.
// - The goldfish sync-equipped OpenGL driver on the guest creates
//   a EGL sync object by itself in the implementation of
//   eglSwapBuffers. It is cumbersome to eglDestroySyncKHR it
//   on the guest, as that would require starting up another thread
//   and OpenGL context (complete with host connection) to destroy it
//   when it is not needed by SurfaceFlinger anymore.
//   Thus, FenceSync also includes a field |mDestroyWhenSignaled|
//   to track such emulator-specific "one-shot" sync objects,
//   and tell SyncThread to delete them when the FD becomes signaled.
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
    // becomes signaled.
    // This is used for the case where the goldfish opengl driver
    // in the guest wants to create EGL sync objects on its own.
    // Otherwise, we will rely on the guest's calls to
    // eglDestroySyncKHR -> host's rcDestroySyncKHR to
    // clean up this object.
    FenceSync(bool hasNativeFence,
              bool destroyWhenSignaled);

    // wait() wraps eglClientWaitSyncKHR. During such a wait,
    // in order to follow spec, we need to increment
    // the reference count while the wait is active,
    // in case there is a concurrent call to
    // eglDestroySyncKHR.
    EGLint wait(uint64_t timeout);

    // signaledNativeFd(): upon when the native fence fd
    // is signaled due to the sync object being signaled,
    // this method does the following:
    // - adjusts the reference count down by 1. There is a
    //   corresponding +1 to the count added at the beginning
    //   if this fence object is of
    //   EGL_SYNC_NATIVE_FENCE_ANDROID nature.
    // - if |mDestroyWhenSignaled| is true, decrements the
    //   reference count again properly destroy sync objects that
    //   arise purely from goldfish driver and are only waited on
    //   in one place.
    void signaledNativeFd();

    // destroy() wraps eglDestroySyncKHR and is called when
    // the reference count reaches 0.
    void destroy();

    // Ref counts for dealing with correct sync object destruction
    // in the presence of concurrent waits / destroys.
    // This is a simple reference counting implementation
    // that is almost literally the kref() in the Linux kernel.
    //
    // We do not use shared_ptr or anything like that here
    // because we need to explicitly manipulate the reference count
    // to fulfill the purposes of this class.
    void getRef() { assert(mCount > 0); ++mCount; }
    bool putRef() {
        assert(mCount > 0);
        if (mCount == 1 || --mCount == 0) {
            destroy();
            // This delete-then-return seems OK.
            delete this;
            return true;
        }
        return false;
    }
private:
    bool mDestroyWhenSignaled;
    std::atomic<int> mCount;

    // EGL state needed for calling sync functions in the host driver.
    EGLDisplay mDisplay;
    EGLSyncKHR mGLSync;
};
