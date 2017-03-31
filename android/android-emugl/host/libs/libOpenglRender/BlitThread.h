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
#include "WindowSurface.h"

enum BlitThreadOpCode {
    BLIT_THREAD_INIT = 0,
    BLIT_THREAD_BLIT = 1,
    BLIT_THREAD_POST = 2,
    BLIT_THREAD_SETWSCB = 3,
    BLIT_THREAD_EXIT = 4,
};

struct BlitThreadCmd {
    BlitThreadOpCode opCode;
    FenceSync* sync = nullptr;
    HandleType windowSurface;
    HandleType colorBuffer;
};

struct RenderThreadInfo;
class BlitThread : public emugl::Thread {
public:
    BlitThread();
    void blit2(FenceSync* sync, uint32_t windowSurface);
    void post(uint32_t colorBuffer);
    void setWsCb(uint32_t ws, uint32_t cb);
private:
    virtual intptr_t main() override final;
    android::base::MessageChannel<BlitThreadCmd, 16> mInput;
    android::base::MessageChannel<BlitThreadCmd, 16> mOutput;
    EGLDisplay mDisplay;
    RenderThreadInfo* mTLS;
    uint32_t mContext;
    uint32_t mSurface;
};
