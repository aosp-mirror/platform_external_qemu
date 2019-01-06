// Copyright 2019 The Android Open Source Project
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
#include "android/emulation/FaultHandler.h"

#include "android/base/containers/Lookup.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/snapshot/MemoryWatch.h"

#ifdef _WIN32
#include <sys/mman.h>
#define UNPROT_FLAGS (PROT_READ | PROT_WRITE | PROT_EXEC)
#else
#include <Hypervisor/hv.h>
#define UNPROT_FLAGS (HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC)
#endif

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;

namespace android {

FaultHandler::FaultHandler() = default;
FaultHandler::~FaultHandler() = default;

bool FaultHandler::supportsFaultHandling() const {
#ifdef __linux__
    return false;
#else
    return true;
#endif
}

FaultHandler::Id FaultHandler::addFaultHandler(
        uint64_t gpaStart,
        uint64_t size,
        FaultHandler::OnFaultCallback cb) {
    AutoLock lock(mLock);
    HandlerInfo info = {gpaStart, size, cb};
    auto res = mNextId;
    mStorage[res] = info;
    ++mNextId;
    return res;
}

void FaultHandler::removeFaultHandler(FaultHandler::Id id) {
    AutoLock lock(mLock);
    mStorage.erase(id);
}

void FaultHandler::setupFault(FaultHandler::Id id) {
    auto handler = android::base::find(mStorage, id);
    if (!handler) return;
    mGuestMemProtect(handler->gpaStart, handler->size, 0);
}

void FaultHandler::teardownFault(FaultHandler::Id id) {
    auto handler = android::base::find(mStorage, id);
    if (!handler) return;
    mGuestMemProtect(handler->gpaStart, handler->size, UNPROT_FLAGS);
}

static bool sIntervalIntersects(
    uint64_t x, uint64_t xsize,
    uint64_t y, uint64_t ysize) {

    uint64_t xend = x + xsize;
    uint64_t yend = y + ysize;

    return (x >= y && x < yend) ||
           (y >= x && y < xend);
}

void FaultHandler::runHandlersForFault(void* hva, uint64_t faultSize) {
    uint64_t faultGpas[64], faultSizes[64];
    int count = hva2gpa_call(hva, faultSize, 64, faultGpas, faultSizes);

    if (count > 0) {
        AutoLock lock(mLock);
        for (auto it : mStorage) {
            const auto& info = it.second;
            for (int i = 0; i < count; ++i) {
                if (sIntervalIntersects(
                        faultGpas[i], faultSizes[i],
                        info.gpaStart,
                        info.size)) {
                    info.cb();
                    mGuestMemProtect(
                        faultGpas[i],
                        faultSizes[i],
                        UNPROT_FLAGS);
                    break;
                }
            }
        }
    }
}

static LazyInstance<FaultHandler> sFaultHandler =
    LAZY_INSTANCE_INIT;

// static
FaultHandler* FaultHandler::get() { return sFaultHandler.ptr(); }

} // namespace android
