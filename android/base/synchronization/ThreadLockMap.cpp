// Copyright (C) 2014 The Android Open Source Project
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

#include "android/base/synchronization/ThreadLockMap.h"
#include <unordered_map>

namespace android {
namespace base {

static ThreadLockMapStore sTLLockMap;

ThreadLockMap& getLockMap() {
    if (!sTLLockMap.get()) {
        sTLLockMap.set(new ThreadLockMap);
    }
    return *((ThreadLockMap*)sTLLockMap.get());
}

void addLockMapEntry(void* obj) {
    getLockMap()[obj] = false;
}

void removeLockMapEntry(void* obj) {
    getLockMap().erase(obj);
    if (getLockMap().size() == 0) {
        ThreadLockMap* to_delete = (ThreadLockMap*)sTLLockMap.get();
        // Apparently this is a double free?
        // delete to_delete;
        sTLLockMap.set(nullptr);
    }
}

} // namespace base
} // namespace android


