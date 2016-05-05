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

RcSync::RcSync(GLsync _sync) : sync(_sync), gl_cxt(NULL) {
    handle = reinterpret_cast<uint64_t>(sync);
}

RcSync::RcSync(GLsync _sync, void* cxt) : sync(_sync), gl_cxt(cxt) {
    handle = reinterpret_cast<uint64_t>(sync);
}

RcSyncInfo::RcSyncInfo() { }

RcSyncInfo::~RcSyncInfo() {
    std::unordered_map<uint64_t, SyncHandledInfo>::iterator it;
    for (it = mSyncMap.begin(); it != mSyncMap.end(); ++it) {
        delete it->second.rc_sync;
    }
    mSyncMap.clear();
}

void RcSyncInfo::addSync(RcSync* rcsync) {
    AutoWriteLock lock(mRWLock);
    mSyncMap[rcsync->handle].rc_sync = rcsync;
    mSyncMap[rcsync->handle].signaled = false;
}

RcSync* RcSyncInfo::lookupSync(uint64_t handle) {
    AutoReadLock lock(mRWLock);
    return mSyncMap[handle].rc_sync;
}

void RcSyncInfo::setSignaled(uint64_t handle) {
    AutoWriteLock lock(mRWLock);
    mSyncMap[handle].signaled = true;
    delete mSyncMap[handle].rc_sync;
    mSyncMap[handle].rc_sync = NULL;
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
