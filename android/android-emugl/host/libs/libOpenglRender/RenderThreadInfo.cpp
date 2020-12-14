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
#include "android/base/synchronization/Lock.h"

#include "emugl/common/lazy_instance.h"
#include "FrameBuffer.h"

#include <unordered_map>
#include <unordered_set>

using android::base::AutoLock;
using android::base::Stream;
using android::base::Lock;

static thread_local RenderThreadInfo* s_threadInfoPtr;

struct RenderThreadRegistry {
    Lock lock;
    std::unordered_set<RenderThreadInfo*> threadInfos;
};

static RenderThreadRegistry sRegistry;

RenderThreadInfo::RenderThreadInfo() {
    s_threadInfoPtr = this;
    AutoLock lock(sRegistry.lock);
    sRegistry.threadInfos.insert(this);
}

RenderThreadInfo::~RenderThreadInfo() {
    s_threadInfoPtr = nullptr;
    AutoLock lock(sRegistry.lock);
    sRegistry.threadInfos.erase(this);
}

RenderThreadInfo* RenderThreadInfo::get() {
    return s_threadInfoPtr;
}

// Loop over all active render thread infos. Takes the global render thread info lock.
void RenderThreadInfo::forAllRenderThreadInfos(std::function<void(RenderThreadInfo*)> f) {
    AutoLock lock(sRegistry.lock);
    for (auto info: sRegistry.threadInfos) {
        f(info);
    }
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

    stream->putBe64(m_puid);

    // No need to associate render threads with sync threads
    // if there is a global sync thread. This is only needed
    // to maintain backward compatibility with snapshot file format.
    // (Used to be: stream->putBe64(syncThreadAlias))
    stream->putBe64(0);
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

    m_puid = stream->getBe64();

    // (Used to be: syncThreadAlias = stream->getBe64())
    stream->getBe64();

    return true;
}
