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

#include "android/base/Compiler.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"

#include "FenceSyncInfo.h"

using android::base::ReadWriteLock;
using android::base::AutoReadLock;
using android::base::AutoWriteLock;

RcSync::RcSync(GLsync _sync, void* cxt = nullptr) : sync(_sync), gl_cxt(cxt) {
    handle = reinterpret_cast<uint64_t>(sync);
}

RcSyncInfo::RcSyncInfo() { }

RcSyncInfo::~RcSyncInfo() {
    for (const auto& pair : mSyncMap) {
        delete pair.second.rc_sync;
    }
}

void RcSyncInfo::addSync(RcSync* rcsync) {
    AutoWriteLock lock(mRWLock);
    if (!rcsync) {
        SyncHandledInfo& info = mSyncMap[0];
        info.signaled = true;
        info.rc_sync = nullptr;
    } else {
        SyncHandledInfo& info = mSyncMap[rcsync->handle];
        info.signaled = false;
        info.rc_sync = rcsync;
    }
}

RcSync* RcSyncInfo::lookupSync(uint64_t handle) {
    AutoReadLock lock(mRWLock);
    return mSyncMap[handle].rc_sync;
}

void RcSyncInfo::setSignaled(uint64_t handle) {
    AutoWriteLock lock(mRWLock);
    SyncHandledInfo& info = mSyncMap[handle];
    info.signaled = true;
    delete info.rc_sync;
    info.rc_sync = nullptr;
}

bool RcSyncInfo::isSignaled(uint64_t handle) {
    bool res;
    std::unordered_map<uint64_t, SyncHandledInfo>::iterator it;

    AutoReadLock lock(mRWLock);
    it = mSyncMap.find(handle);
    if (it != mSyncMap.end()) {
        res = it->second.signaled;
    } else {
        res = false;
    }
    return res;
}

::android::base::LazyInstance<RcSyncInfo> sRcSyncInfo = LAZY_INSTANCE_INIT;

RcSyncInfo* RcSyncInfo::get() {
    return sRcSyncInfo.ptr();
}
