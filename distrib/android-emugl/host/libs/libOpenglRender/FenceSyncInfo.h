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

#pragma once

#include "android/base/synchronization/Lock.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>

#include <memory>
#include <unordered_map>

class RcSync {
public:
    explicit RcSync(GLsync glsync, void* cxt = nullptr) :
        sync(glsync), gl_cxt(cxt),
        handle(reinterpret_cast<uint64_t>(glsync)) { }
    GLsync sync;
    void* gl_cxt;
    uint64_t handle;
};

struct SyncHandledInfo {
    std::unique_ptr<RcSync> rc_sync;
    bool signaled = false;
};

class RcSyncInfo {
public:
    RcSyncInfo() { }

    void addSync(RcSync* rcsync);
    void removeSync(uint64_t handle);

    void setSignaled(uint64_t handle);
    RcSync* findNonSignaledSync(uint64_t handle);

    static RcSyncInfo* get();
private:
    std::unordered_map<uint64_t, SyncHandledInfo> mSyncMap;
    android::base::ReadWriteLock mRWLock;
    DISALLOW_COPY_ASSIGN_AND_MOVE(RcSyncInfo);
};
