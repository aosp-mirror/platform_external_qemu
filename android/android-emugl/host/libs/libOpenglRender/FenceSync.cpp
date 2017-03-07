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
#include "android/base/files/StreamSerializing.h"
#include "android/base/synchronization/Lock.h"

#include <unordered_set>

FenceSync::FenceSync(bool hasNativeFence,
                     bool destroyWhenSignaled) :
    mDestroyWhenSignaled(destroyWhenSignaled) {
    FenceSync::addFence(this);

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

// Snapshots for FenceSync//////////////////////////////////////////////////////
// It's possible, though it does not happen often, that a fence 
// can be created but not yet waited on by the guest, which
// needs careful handling:
//
// 1. Avoid manipulating garbage memory on snapshot restore;
// rcCreateSyncKHR *creates new fence in valid memory*
// --snapshot--
// rcClientWaitSyncKHR *refers to uninitialized memory*
// rcDestroySyncKHR *refers to uninitialized memory*
// 2. Make rcCreateSyncKHR/rcDestroySyncKHR implementations return
// the "signaled" status if referring to previous snapshot fences. It's
// assumed that the GPU is long done with them.
// 3. Avoid name collisions where a new FenceSync object is created
// that has the same uint64_t casting as a FenceSync object from a previous
// snapshot.

// |sActiveFences|: always tracks set of fences not been destroyed yet.
static android::base::LazyInstance<
           std::unordered_set<uint64_t> > sActiveFences = LAZY_INSTANCE_INIT;
// |sPrevSnapshotFences|: always tracks set of stale fences that the guest
// may refer to. Must track them separately, in case we created a FenceSync
// in our current session that had the same name
// as a fence in |sPrevSnapshotFences|.
static android::base::LazyInstance<
           std::unordered_set<uint64_t> > sPrevSnapshotFences = LAZY_INSTANCE_INIT;
// |sFenceSetLock| protects all read/write access
static android::base::ReadWriteLock sFenceSetLock;

// static
void FenceSync::addFence(FenceSync* fence) {
    android::base::AutoWriteLock lock(sFenceSetLock);
    sActiveFences.get().insert((uint64_t)(uintptr_t)fence);
}
// static
void FenceSync::removeFence(FenceSync* fence) {
    android::base::AutoWriteLock lock(sFenceSetLock);
    uint64_t handle = (uint64_t)(uintptr_t)fence;
    sActiveFences.get().erase(handle);
    sPrevSnapshotFences.get().erase(handle);
}
// static
void FenceSync::onSave(android::base::Stream* stream) {
    android::base::AutoWriteLock lock(sFenceSetLock);
    sPrevSnapshotFences.get().insert(
        sActiveFences.get().begin(),
        sActiveFences.get().end());
    sActiveFences.get().clear();
    saveCollection(
            stream, sPrevSnapshotFences.get(),
            [](android::base::Stream* stream, uint64_t val) {
                stream->putBe64(val); });
}

// static
bool FenceSync::onLoad(android::base::Stream* stream) {
    android::base::AutoWriteLock lock(sFenceSetLock);
    loadCollection(stream, sPrevSnapshotFences.ptr(),
        [](android::base::Stream* stream) {
        return stream->getBe64(); });
    return true;
}

// static
FenceSync* FenceSync::getFenceSync(uint64_t handle) {
    android::base::AutoReadLock lock(sFenceSetLock);
    if (sActiveFences.get().find(handle) !=
        sActiveFences.get().end()) {
        return reinterpret_cast<FenceSync*>(handle);
    } else if (sPrevSnapshotFences.get().find(handle) !=
               sPrevSnapshotFences.get().end()) {
        return nullptr;
    } else {
        // TODO: should throw error here
        return nullptr;
    }
}
