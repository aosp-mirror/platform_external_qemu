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

using android::base::Lock;
using android::base::AutoLock;

RcSync::RcSync(GLsync _sync) : sync(_sync) {
    handle = reinterpret_cast<uint64_t>(sync);
}

RcSyncInfo::RcSyncInfo() { }

void RcSyncInfo::addSync(RcSync* rcsync) {
    mRWLock.lockWrite();
    mSyncMap[rcsync->handle] = rcsync;
    mSignaledMap[rcsync->handle] = false;
    mRWLock.unlockWrite();
}

RcSync* RcSyncInfo::lookupSync(uint64_t handle) {
    RcSync* res;

    mRWLock.lockRead();
    res = mSyncMap[handle];
    mRWLock.lockRead();
    
    return res;
}

void RcSyncInfo::removeSync(uint64_t handle) {
    std::map<uint64_t, RcSync*>::iterator it;
    mRWLock.lockWrite();
    it = mSyncMap.find(handle);
    mSyncMap.erase(it);
    mRWLock.unlockWrite();
}

void RcSyncInfo::setSignaled(uint64_t handle) {
    RcSync* to_delete = lookupSync(handle);
    removeSync(handle);
    mRWLock.lockWrite();
    delete to_delete;
    mSignaledMap[handle] = true;
    mRWLock.unlockWrite();
}

bool RcSyncInfo::isSignaled(uint64_t handle) {
    bool res;
    std::map<uint64_t, bool>::iterator it;

    mRWLock.lockRead();
    it = mSignaledMap.find(handle);
    if (it != mSignaledMap.end()) {
        res = it->second;
    } else {
        res = false;
    }
    mRWLock.lockRead();

    return res;
}

::android::base::LazyInstance<RcSyncInfo> sRcSyncInfo = LAZY_INSTANCE_INIT;

RcSyncInfo* RcSyncInfo::get() {
    return sRcSyncInfo.ptr();
}
