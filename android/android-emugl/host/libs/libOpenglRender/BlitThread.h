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

#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"

#include "emugl/common/thread.h"

#include "FenceSync.h"
#include "SyncThread.h"
#include "WindowSurface.h"

struct RenderThreadInfo;
class BlitThread : public emugl::Thread {
public:
    BlitThread();
    void flush(uint32_t windowSurface);
    void post(uint32_t colorBuffer);
    void set(uint32_t windowSurface, uint32_t colorBuffer);

    int openColorBuffer(uint32_t colorBuffer);
    void closeColorBuffer(uint32_t colorBuffer);

    uint32_t createWindowSurface(int config, int width, int height);
    void destroyWindowSurface(uint32_t surface);

    uint64_t createSync(bool hasNativeFence, bool destroyWhenSignaled);
    uint64_t getSyncThread();
    void exit();

private:
    enum BlitThreadOpCode {
        Blit = 0,
        Post = 1,
        Set = 2,
        OpenColorBuffer = 3,
        CloseColorBuffer = 4,
        CreateWindowSurface = 5,
        DestroyWindowSurface = 6,
        CreateSync = 7,
        Exit = 8,
    };
   
    struct WindowSurfaceCreateInfo {
        int config;
        int width;
        int height;
    };

    struct FenceSyncCreateInfo {
        bool hasNativeFence;
        bool destroyWhenSignaled;
    };

    struct BlitThreadCmd {
        BlitThreadOpCode opCode = Exit;
        HandleType windowSurface = 0;
        HandleType colorBuffer = 0;
        union {
            WindowSurfaceCreateInfo windowSurfaceCreateInfo;
            FenceSyncCreateInfo fenceSyncCreateInfo;
        };
    };

    virtual intptr_t main() override final;
    android::base::MessageChannel<BlitThreadCmd, 16> mInput;
    android::base::MessageChannel<uint64_t, 16> mOutput;

    // internal context members
    RenderThreadInfo* mTLS = nullptr;

    // Sync thread just for this blit thread
    SyncThread* mSyncThread = nullptr;

    // current command
    BlitThreadCmd mCmd = {};
    // current command return code
    uint64_t mCmdRes = 0;
    // lock to set current command, we expect
    // 2 threads at least to be sending
    android::base::Lock mCmdLock;
};
