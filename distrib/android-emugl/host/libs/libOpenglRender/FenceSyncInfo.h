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

#include "RenderContext.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <memory>
#include <unordered_map>

// The purpose of FenceSyncInfo is to track the progress
// of OpenGL fence commands issued from the guest,
// and to have lower host driver overhead when
// waiting on fence commands to be completed.
//
// Tracking is necessary because fence objects on the host
// are just opaque pointers and don't correspond to anything
// real on the guest. When the guest calls eglCreateSyncKHR,
// the guest receives a handle that corresponds to the
// actual EGLSyncKHR object on the host.
//
// When the EGLSyncKHR object is signaled / when the guest
// calls eglClientWaitSyncKHR, the host processes
// the EGLSyncKHR object directly, while communicating only
// the result of the wait command to the guest.

// The FenceSync class wraps actual EGLSyncKHR objects.
// The handle is included with the EGLSyncKHR object.
// The OpenGL context that created the EGLSyncKHR object is also
// added as a class member, so that it is easier to
// create sync contexts when waiting on fence commands
// from other threads (contexts).
class FenceSync {
public:
    explicit FenceSync(EGLSyncKHR eglsync,
                       RenderContext* cxt = nullptr) :
        mGLSync(eglsync), mContext(cxt) { }
    uint64_t getHandle() const { return reinterpret_cast<uint64_t>(mGLSync); }
    EGLSyncKHR mGLSync;
    RenderContext* mContext;
};

// The FenceSyncInfo class maps handles to FenceSync object pointers,
// and adds functionality for tracking the signaled status
// of the underlying OpenGL sync object.
class FenceSyncInfo {
public:
    FenceSyncInfo() = default;

    // Workflow for any OpenGL create sync -> wait sync -> signaled
    // scenario:
    // 1. After creating a EGLSyncKHR object,
    // we wrap it with a FenceSync object and pass it to
    // |addSync|.
    void addSync(FenceSync* fencesync);

    // 2. When waiting on a EGLSyncKHR object from the guest,
    // we pass the |handle| field of that object to
    // |findNonSignaledSync|. Two things can then happen:
    // - The object is not signaled yet:
    //     - Then a non-null FenceSync object is returned.
    // - The object is signaled:
    //     - A null FenceSync* is returned to represent the
    //       signaled status.
    FenceSync* findNonSignaledSync(uint64_t handle);

    // 3. After we complete waiting, we can notify the
    // FenceSyncInfo struct that the corresponding
    // EGLSyncKHR object has been signaled by calling
    // |setSignaled|. This will end up removing
    // the corresponding FenceSync* object from the map
    // and make further queries return NULL to reflect
    // the signaled status.
    void setSignaled(uint64_t handle);

    // We interact with a global FenceSyncInfo object,
    // and use |get| to obtain it (or create it, if it
    // does not exist already).
    static FenceSyncInfo* get();
private:
    // The global FenceSyncInfo object.
    static ::android::base::LazyInstance<FenceSyncInfo> sFenceSyncInfo;

    // |mSyncMap| maintains all non-signaled EGLSyncKHR objects.
    std::unordered_map<uint64_t, std::unique_ptr<FenceSync> > mSyncMap;

    // A read-write lock is used to protect mSyncMap.
    // There can be an asymmetry between writes and reads.
    // There are many scenarios where the same sync object is
    // waited on in multiple threads, which only requires writing
    // for the first successful wait. Otherwise, only reads
    // would be used.
    android::base::ReadWriteLock mRWLock;
    DISALLOW_COPY_ASSIGN_AND_MOVE(FenceSyncInfo);
};
