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

#include "RenderThreadInfo.h"

#include "android/base/containers/Lookup.h"
#include "android/base/files/StreamSerializing.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"

#include "emugl/common/lazy_instance.h"
#include "emugl/common/thread_store.h"
#include "FrameBuffer.h"

#include <unordered_map>

using android::base::Stream;

namespace {

class ThreadInfoStore : public ::emugl::ThreadStore {
public:
    ThreadInfoStore() : ::emugl::ThreadStore(NULL) {}
};

}  // namespace

static ::emugl::LazyInstance<ThreadInfoStore> s_tls = LAZY_INSTANCE_INIT;

RenderThreadInfo::RenderThreadInfo() {
    s_tls->set(this);
}

RenderThreadInfo::~RenderThreadInfo() {
    s_tls->set(NULL);
}

RenderThreadInfo* RenderThreadInfo::get() {
    return static_cast<RenderThreadInfo*>(s_tls->get());
}

// Each RenderThreadInfo may have 1 SyncThread associated with it.
// These SyncThreads must be handled carefully for snapshots:
// 1. The guest might ask for a SyncThread handle that doesn't
// exist on the host anymore.
// 2. The host might allocate a new SyncThread that has the same
// uint64_t handle as a stale SyncThread on the guest.
//
// This problem has some convenient properties:
//
// 1. Since RenderThread and SyncThread 1:1, and rcTriggerWait +
// rcCreateSyncKHR are in 1:1, and rcTriggerWait is the only API call
// that has a SyncThread handle as argument, we only have one
// inconsistent call to rcTriggerWait per sync thread,
// then the rest will update when rcClientWaitSyncKHR is called next
// (and reads the new sync thread handle).
// 2. Only ~1 such rcTriggerWait is pending total, I believe this
// is because there is usually not much eglSwapBuffers activity
// going on at any time.
//
// So we maintain:
// |sCurrSyncThreads|, a set of sync thread handles that were
// definitely created in this snapshot session.
static android::base::LazyInstance<
           std::unordered_set<uint64_t> > sCurrSyncThreads =
    LAZY_INSTANCE_INIT;
// |sPrevSyncThreads|, a map of old sync thread handles to new sync thrads,
// that are used only to address the lone rcTriggerWait's.
static android::base::LazyInstance<
           std::unordered_map<uint64_t, SyncThread*> > sPrevSyncThreads =
    LAZY_INSTANCE_INIT;
// |sSyncThreadSetLock| protecting r/w access to both |sCurrSyncThreads|
// and |sPrevSyncThreads|.
static android::base::Lock sSyncThreadSetLock;

// createSyncThread() is only called when the guest makes
// a new context with sync enabled after the last snapshot.
// So, we track it in |sCurrSyncThreads|, but only if there
// is reason to (i.e., the same thread exists in sPrevSyncThreads).
void RenderThreadInfo::createSyncThread() {
    syncThread.reset(new SyncThread(currContext->getEGLContext()));
    uint64_t handle = (uint64_t)(uintptr_t)syncThread.get();

    android::base::AutoLock lock(sSyncThreadSetLock);
    if (sPrevSyncThreads.get().find(handle) == sPrevSyncThreads.get().end()) return;
    sCurrSyncThreads.get().insert((uint64_t)(uintptr_t)syncThread.get());
}

// destroySyncThread() should untrack.
void RenderThreadInfo::destroySyncThread() {
    android::base::AutoLock lock(sSyncThreadSetLock);
    uint64_t handle = (uint64_t)(uintptr_t)syncThread.get();
    sCurrSyncThreads.get().erase(handle);
    sPrevSyncThreads.get().erase(handle);
    syncThread.reset(nullptr);
}


void RenderThreadInfo::onSave(Stream* stream) {
    if (currContext) {
        stream->putBe32(currContext->getHndl());
    } else {
        stream->putBe32(0);
    }
    if (currDrawSurf) {
        stream->putBe32(currDrawSurf->getHndl());
    } else {
        stream->putBe32(0);
    }
    if (currReadSurf) {
        stream->putBe32(currReadSurf->getHndl());
    } else {
        stream->putBe32(0);
    }

    saveCollection(stream, m_contextSet, [](Stream* stream, HandleType val) {
        stream->putBe32(val);
    });
    saveCollection(stream, m_windowSet, [](Stream* stream, HandleType val) {
        stream->putBe32(val);
    });

    if (syncThread.get()) {
        uint64_t virtualSyncThreadId = (uint64_t)(uintptr_t)syncThread.get();
        fprintf(stderr, "%s: creating temp virtual syncthread 0x%llx\n", __func__,
                (unsigned long long)virtualSyncThreadId);
        // We don't care about saving |sCurrSyncThreads| or
        // |sPrevSyncThreads|, since all we need to know when
        // the snapshot is restored, is all |syncThreadAlias|'es,
        // which are used to reconstruct |sPrevSyncThreads|.
        syncThreadAlias = virtualSyncThreadId;
    }
    stream->putBe64(syncThreadAlias);

    // TODO: save the remaining members.
}

bool RenderThreadInfo::onLoad(Stream* stream) {
    FrameBuffer* fb = FrameBuffer::getFB();
    assert(fb);
    HandleType ctxHndl = stream->getBe32();
    HandleType drawSurf = stream->getBe32();
    HandleType readSurf = stream->getBe32();
    fb->lock();
    currContext = fb->getContext_locked(ctxHndl);
    currDrawSurf = fb->getWindowSurface_locked(drawSurf);
    currReadSurf = fb->getWindowSurface_locked(readSurf);
    fb->unlock();

    loadCollection(stream, &m_contextSet, [](Stream* stream) {
        return stream->getBe32();
    });
    loadCollection(stream, &m_windowSet, [](Stream* stream) {
        return stream->getBe32();
    });

    syncThreadAlias = stream->getBe64();

    if (syncThreadAlias) {

        // We need to create a new sync thread super early if we are restoring
        // renderthreadinfo, since there could be a pending rcTriggerWait.
        // Additionally, that rcTriggerWait most likely uses the old pointer
        // value, so we need to remap it to the new one temporarily.  If not,
        // rcTriggerWait refers to a garbage pointer and undefined behavior
        // results.

        syncThread.reset(new SyncThread(currContext->getEGLContext()));
        sPrevSyncThreads.get()[syncThreadAlias] = syncThread.get();
        fprintf(stderr, "%s: the temp alias for syncthread 0x%llx -> %p\n",
                __func__, (unsigned long long)syncThreadAlias,
                syncThread.get());

        // Note that the values in sPrevSyncThreads will only be used for a
        // very short time, because after the snapshot is restored, the guest
        // will know about the new SyncThread* pointers and we will most likely
        // not have to use the map anymore.  However, the map still needs to
        // stay around in case we restored from snapshot, waited a while, and
        // then woke up a guest rendering thread that still had the old value
        // of rcTriggerWait (but that is unlikely).
    }

    // TODO: load the remaining members.

    return true;
}

SyncThread* getSyncThreadFromAlias(uint64_t alias) {
    // We only use |sCurrSyncThreads| if there is a possible conflict.
    // If there is, then that's because a new syncthread has the same
    // uint64_t handle as a key in sPrevSyncThreads. To get around it,
    // if |sPrevSyncThreads| is nonempty, we will track all sync thread
    // creation in |sCurrSyncThreads|.
    if (sCurrSyncThreads.get().find(alias) != sCurrSyncThreads.get().end()) {
        return reinterpret_cast<SyncThread*>(alias);
    } else if (auto elt = android::base::find(sPrevSyncThreads.get(), alias)) {
        // This path is taken for just one rcTriggerWait per syncthread
        // (at most) during snapshot restore.
        sPrevSyncThreads.get().erase(alias);
        // If we have no possibility of name collision anymore,
        // clean up |sCurrSyncThreads| as well.
        if (sPrevSyncThreads.get().empty()) {
            android::base::AutoLock lock(sSyncThreadSetLock);
            sCurrSyncThreads.get().clear();
        }
        return *elt;
    } else {
        // This is the fast path and is taken the rest of the time.
        return reinterpret_cast<SyncThread*>(alias);
    }
}
