/*
* Copyright (C) 2017 The Android Open Source Project
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

#pragma once

#include "android/base/Compiler.h"
#include "android/base/synchronization/Lock.h"

namespace android {

namespace base { class Stream; }

namespace snapshot {

// LazySnapshotObj is a base class for objects that use lazy strategy for
// snapshot loading. It separates heavy-weight loading / restoring operations
// and only triggers it when the object needs to be used.
// Please implement heavy-weight loading / restoring operations in restore()
// method and call "touch" before you need to use the object.

// An example is for texture lazy loading. On load it only reads the data from
// disk but does not load them into GPU. On restore it performs the heavy-weight
// GPU data loading.

template <class Derived>
class LazySnapshotObj {
    DISALLOW_COPY_AND_ASSIGN(LazySnapshotObj);
public:
    LazySnapshotObj() = default;
    // Snapshot loader
    LazySnapshotObj(base::Stream*) : mNeedRestore(true) {}

    void touch() {
        base::AutoLock lock(mMutex);
        if (!mNeedRestore) {
            return;
        }
        static_cast<Derived*>(this)->restore();
        mNeedRestore = false;
    }

    bool needRestore() const {
        base::AutoLock lock(mMutex);
        return mNeedRestore;
    }

protected:
    ~LazySnapshotObj() = default;
    bool mNeedRestore = false;

private:
    mutable base::Lock mMutex;
};

} // namespace snapshot
} // namespace android
