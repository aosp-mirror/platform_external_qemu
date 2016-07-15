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

#include "FenceSyncInfo.h"

#include "android/base/Compiler.h"
#include "android/base/synchronization/Lock.h"

#include "android/utils/assert.h"

using android::base::AutoReadLock;
using android::base::AutoWriteLock;
using android::base::ReadWriteLock;

void FenceSyncInfo::addSync(FenceSync* fencesync) {
    AutoWriteLock lock(mRWLock);
    if (fencesync) {
        mSyncMap[fencesync->getHandle()].reset(fencesync);
    }
}

void FenceSyncInfo::setSignaled(uint64_t handle) {
    AutoWriteLock lock(mRWLock);
    mSyncMap[handle].reset();
    mSyncMap.erase(handle);
}

FenceSync* FenceSyncInfo::findNonSignaledSync(uint64_t handle) {
    AutoReadLock lock(mRWLock);
    auto it = mSyncMap.find(handle);

    if (it == mSyncMap.end()) {
        // The sync has either been signaled
        // or wasn't there at all.
        // This function is to return
        // non-signaled syncs only.
        return nullptr;
    }

    return it->second.get();
}

::android::base::LazyInstance<FenceSyncInfo>
FenceSyncInfo::sFenceSyncInfo =
    LAZY_INSTANCE_INIT;

/* static */
FenceSyncInfo* FenceSyncInfo::get() {
    return sFenceSyncInfo.ptr();
}
