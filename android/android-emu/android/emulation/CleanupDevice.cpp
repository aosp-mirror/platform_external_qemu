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
#include "android/emulation/CleanupDevice.h"

#include "android/base/synchronization/Lock.h"

using android::base::AutoLock;

namespace android {
namespace emulation {

static android::base::StaticLock sCleanupLock;

CleanupDevice::CleanupDevice() = default;
CleanupDevice::~CleanupDevice() {
    AutoLock lock(sCleanupLock);
    for (auto it : mCallbacks) {
        it.second();
    }
}

void CleanupDevice::addCallback(void* key, CleanupDevice::Callback cb) {
    AutoLock lock(sCleanupLock);
    mCallbacks[key] = cb;
}

void CleanupDevice::removeCallback(void* key) {
    AutoLock lock(sCleanupLock);
    mCallbacks.erase(key);
}

} // namespace emulation
} // namespace android