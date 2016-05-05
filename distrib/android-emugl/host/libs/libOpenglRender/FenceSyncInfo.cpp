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

void RcSyncInfo::addSync(RcSync* rcsync) {
    AutoWriteLock lock(mRWLock);
    if (!rcsync) {
        SyncHandledInfo& info = mSyncMap[0];
        info.signaled = true;
        info.rc_sync.reset();
    } else {
        SyncHandledInfo& info = mSyncMap[rcsync->handle];
        info.signaled = false;
        info.rc_sync = std::unique_ptr<RcSync>(rcsync);
    }
}

void RcSyncInfo::setSignaled(uint64_t handle) {
    AutoWriteLock lock(mRWLock);
    SyncHandledInfo& info = mSyncMap[handle];
    info.signaled = true;
}

RcSync* RcSyncInfo::findNonSignaledSync(uint64_t handle) {
    AutoReadLock lock(mRWLock);
    bool signaled;
    auto it = mSyncMap.find(handle);
    if (it != mSyncMap.end()) {
        signaled = it->second.signaled;
    } else {
        signaled = false;
        return nullptr;
    }

    // If the sync has already been signaled,
    // we represent this by returning nullptr.
    if (signaled) { return nullptr; }
    // If the sync has not been signaled yet,
    // return pointer to the actual RcSync pointer
    // for further processing.
    return it->second.rc_sync.get();
}

::android::base::LazyInstance<RcSyncInfo> sRcSyncInfo = LAZY_INSTANCE_INIT;

RcSyncInfo* RcSyncInfo::get() {
    return sRcSyncInfo.ptr();
}
