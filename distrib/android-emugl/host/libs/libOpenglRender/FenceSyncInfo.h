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

#include "android/base/synchronization/Lock.h"

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
    explicit FenceSync(EGLSyncKHR eglsync, void* cxt = nullptr) :
        sync(eglsync), gl_cxt(cxt) { }
    uint32_t getHandle() const { return reinterpret_cast<uint64_t>(sync); }
    EGLSyncKHR sync;
    void* gl_cxt;
};

// SyncHandledInfo caches information about
// FenceSync objects that have already been signaled.
struct SyncHandledInfo {
    std::unique_ptr<FenceSync> fence_sync;
    bool signaled = false;
};

// The FenceSyncInfo class maps handles to SyncHandledInfo objects.
// When eglClientWaitSyncKHR is called, we look up the
// SyncHandledInfo object corresponding to the input handle,
// and check whether it is already signaled.
//
// If it is already signaled, there is no need to incur
// the cost of calling out to the host OpenGL driver again;
// we just return. Otherwise, we will call glClientWaitSync.
class FenceSyncInfo {
public:
    FenceSyncInfo() = default;

    void addSync(FenceSync* fencesync);
    void removeSync(uint64_t handle);

    void setSignaled(uint64_t handle);
    FenceSync* findNonSignaledSync(uint64_t handle);

    static FenceSyncInfo* get();
private:
    std::unordered_map<uint64_t, SyncHandledInfo> mSyncMap;
    android::base::ReadWriteLock mRWLock;
    DISALLOW_COPY_ASSIGN_AND_MOVE(FenceSyncInfo);
};
