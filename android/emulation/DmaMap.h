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

#pragma once

#include "android/base/Compiler.h"

#include <inttypes.h>

namespace android {

class DmaMap {
public:
    // Constructor.
    DmaMap() = default;

    // Destructor.
    virtual ~DmaMap() = default;

    virtual void addBuffer(uint64_t addr, uint64_t sz) { }
    virtual void removeBuffer(uint64_t addr) { }
    virtual uint64_t getHostAddr(uint64_t addr) { return 0; }
    virtual void invalidateHostMappings(void) { }

    static DmaMap* get();
    static DmaMap* set(DmaMap* dmaMap);

    DISALLOW_COPY_ASSIGN_AND_MOVE(DmaMap);
};

// // Convenience class to perform scoped VM locking.
// class ScopedVmLock {
// public:
//     ScopedVmLock(VmLock* vmLock = VmLock::get()) : mVmLock(vmLock) {
//         mVmLock->lock();
//     }
// 
//     ~ScopedVmLock() {
//         mVmLock->unlock();
//     }
// 
//     DISALLOW_COPY_ASSIGN_AND_MOVE(ScopedVmLock);
// 
// private:
//     VmLock* mVmLock;
// };

}  // namespace android

