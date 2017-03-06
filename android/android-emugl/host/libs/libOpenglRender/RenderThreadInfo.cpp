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

static android::base::LazyInstance<
           std::unordered_map<uint64_t, SyncThread*> > sSyncThreadMap =
    LAZY_INSTANCE_INIT;

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
        // We don't care about saving |sSyncThreadMap|
        // since all the entries will come back
        // when we load (from saved |syncThreadAlias|)
        syncThreadAlias = virtualSyncThreadId;
    }
    stream->putBe64(syncThreadAlias);

    // TODO: save the remaining members.
}

void RenderThreadInfo::allocUnusedSyncThreadPtr() {
    syncThread.reset(new SyncThread(currContext->getEGLContext()));
    while (sSyncThreadMap.get().find(
               (uint64_t)(uintptr_t)syncThread.get()) !=
           sSyncThreadMap.get().end()) {
        syncThread->cleanup();
        syncThread.reset(nullptr);
        syncThread.reset(new SyncThread(currContext->getEGLContext()));
    }
}

bool RenderThreadInfo::onLoad(Stream* stream) {
    FrameBuffer* fb = FrameBuffer::getFB();
    assert(fb);
    HandleType ctxHndl = stream->getBe32();
    HandleType drawSurf = stream->getBe32();
    HandleType readSurf = stream->getBe32();
    currContext = fb->getContext(ctxHndl);
    currDrawSurf = fb->getWindowSurface(drawSurf);
    currReadSurf = fb->getWindowSurface(readSurf);

    loadCollection(stream, &m_contextSet, [](Stream* stream) {
        return stream->getBe32();
    });
    loadCollection(stream, &m_windowSet, [](Stream* stream) {
        return stream->getBe32();
    });

    syncThreadAlias = stream->getBe64();

    if (syncThreadAlias) {
        // We need to create a new sync thread super early if
        // we are restoring renderthreadinfo, since there could be a pending
        // rcTriggerWait.
        // Additionally, that rcTriggerWait most likely uses the old pointer value,
        // so we need to remap it to the new one temporarily.
        // If not, rcTriggerWait refers to a garbage pointer and undefined
        // behavior results.
        allocUnusedSyncThreadPtr();
        sSyncThreadMap.get()[syncThreadAlias] = syncThread.get();
        fprintf(stderr, "%s: the temp alias for syncthread 0x%llx -> %p\n", __func__,
                (unsigned long long)syncThreadAlias,
                syncThread.get());
        // Note that the values in sSyncThreadMap will only be used for a very short time,
        // because after the snapshot is restored, the guest will know about the new
        // SyncThread* pointers and we will most likely not have to use the map anymore.
        // However, the map still needs to stay around in case we restored from snapshot,
        // waited a while, and then woke up a guest rendering thread that still
        // had the old value of rcTriggerWait (but that is unlikely).
    }

    // TODO: load the remaining members.

    return true;
}

SyncThread* getSyncThreadFromAlias(uint64_t alias) {
    if (auto elt = android::base::find(sSyncThreadMap.get(), alias)) {
        // This path is taken for just one rcTriggerWait per syncthread
        // (at most) during snapshot restore.
        sSyncThreadMap.get().erase(alias);
        return *elt;
    } else {
        // This is the fast path and is taken the rest of the time.
        return reinterpret_cast<SyncThread*>(alias);
    }
}
