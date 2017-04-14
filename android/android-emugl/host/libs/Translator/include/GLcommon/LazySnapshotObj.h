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

#include "emugl/common/mutex.h"

class LazySnapshotObj {
public:
    LazySnapshotObj() = default;
    // Snapshot loader
    LazySnapshotObj(android::base::Stream* stream) {
        mNeedRestore = true;
    }
    ~LazySnapshotObj() = default;
    void touch() {
        emugl::Mutex::AutoLock lock(mMutex);
        if (!mNeedRestore) return;
        restore();
        mNeedRestore = false;
    }
    bool needRestore() {
        return mNeedRestore;
    }
protected:
    virtual void restore() = 0;
    bool mNeedRestore = false;
private:
    emugl::Mutex mMutex;
};


