// Copyright 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/emulation/DmaMap.h"

#include "android/base/containers/Lookup.h"
#include "android/base/memory/LazyInstance.h"

#include <type_traits>
#include <inttypes.h>
#include <stdio.h>

#define DEBUG 0

#if DEBUG >= 1
#define D(fmt,...) fprintf(stderr, "DmaMap: %s: " fmt "\n", __func__, ##__VA_ARGS__);
#else
#define D(...) (void)0
#endif

#if DEBUG >= 2
#define DD(fmt,...) fprintf(stderr, "DmaMap: %s: " fmt "\n", __func__, ##__VA_ARGS__);
#else
#define DD(...) (void)0
#endif

#define E(fmt,...) fprintf(stderr, "DmaMap: ERROR: %s: " fmt "\n", __func__, ##__VA_ARGS__);

namespace android {

static DmaMap* sInstance = nullptr;

DmaMap* DmaMap::get() {
    // NOT thread-safe, but we don't expect multiple threads to call this
    // concurrently at init time, and the worst that can happen is to leak
    // a single instance.
    if (!sInstance) {
        sInstance = new DmaMap();
    }
    return sInstance;
}

DmaMap* DmaMap::set(DmaMap* dmaMap) {
    DmaMap* result = sInstance;
    sInstance = dmaMap;
    return result;
}

void DmaMap::addBuffer(uint64_t guest_paddr,
                       uint64_t sz) {
    D("guest paddr 0x%llx sz %llu",
      (unsigned long long)guest_paddr,
      (unsigned long long)sz);
    android::base::AutoWriteLock lock(mLock);
    DmaBufferInfo info;
    info.guestAddr = guest_paddr; // guest address
    info.sz = sz; // size of buffer
    info.currHostAddr = 0; // no current host address
    info.hostAddrValid = false; // host address NOT valid (needs (re)mapping)
    mDmaBuffers[guest_paddr] = info;
}

void DmaMap::removeBuffer(uint64_t guest_paddr) {
    D("guest paddr 0x%llx", (unsigned long long)guest_paddr);
    android::base::AutoWriteLock lock(mLock);
    if (auto info = android::base::find(mDmaBuffers, guest_paddr)) {
        removeMappingLocked(info);
        mDmaBuffers.erase(guest_paddr);
    } else {
        E("guest addr 0x%llx not alloced!",
          (unsigned long long)guest_paddr);
    }
}

uint64_t DmaMap::getHostAddr(uint64_t guest_paddr) {
    DD("guest paddr 0x%llx", (unsigned long long)guest_paddr);
    android::base::AutoReadLock rlock(mLock);
    if (auto info = android::base::find(mDmaBuffers, guest_paddr)) {
        if (info->hostAddrValid) {
            DD("guest paddr 0x%llx -> host 0x%llx valid",
              (unsigned long long)guest_paddr,
              (unsigned long long)info->currHostAddr);
            return info->currHostAddr;
        } else {
            rlock.unlockRead();
            android::base::AutoWriteLock wlock(mLock);
            createMappingLocked(info);
            D("guest paddr 0x%llx -> host 0x%llx valid (new)",
              (unsigned long long)guest_paddr,
              (unsigned long long)info->currHostAddr);
            return info->currHostAddr;
        }
    } else {
        E("guest paddr 0x%llx not alloced!",
          (unsigned long long)guest_paddr);
        return 0;
    }
}

void DmaMap::invalidateHostMappings() {
    android::base::AutoWriteLock lock(mLock);
    for (auto& it : mDmaBuffers) {
        removeMappingLocked(&it.second);
    }
}

void DmaMap::createMappingLocked(DmaBufferInfo* info) {
    info->currHostAddr = doMap(info->guestAddr, info->sz);
    info->hostAddrValid = true;
}

void DmaMap::removeMappingLocked(DmaBufferInfo* info ) {
    if (info->hostAddrValid) {
        doUnmap(info->currHostAddr, info->sz);
        info->hostAddrValid = false;
        D("guest addr 0x%llx host mapping 0x%llx removed.",
          (unsigned long long)info->guestAddr,
          (unsigned long long)info->currHostAddr);
    } else {
        D("guest addr 0x%llx has no host mapping. don't remove.",
          (unsigned long long)info->guestAddr);
    }
}

}  // namespace android


