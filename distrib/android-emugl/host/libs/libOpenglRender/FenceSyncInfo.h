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

#include <map>

class RcSync {
public:
    RcSync();
    RcSync(GLsync sync);
    GLsync sync;
    uint64_t handle;
};

class RcSyncInfo {
public:
    RcSyncInfo();

    void addSync(RcSync* rcsync);
    RcSync* lookupSync(uint64_t handle);
    void removeSync(uint64_t handle);

    void setSignaled(uint64_t handle);
    bool isSignaled(uint64_t handle);

    static RcSyncInfo* get();
private:
    std::map<uint64_t, RcSync*> mSyncMap;
    std::map<uint64_t, bool> mSignaledMap;
    android::base::ReadWriteLock mRWLock;
    DISALLOW_COPY_ASSIGN_AND_MOVE(RcSyncInfo);
};
