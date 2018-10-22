// Copyright 2018 The Android Open Source Project
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

#include "AndroidBufferQueue.h"
#include "GrallocDispatch.h"
#include "Vsync.h"

#include <system/window.h>

#include "android/base/FunctionView.h"
#include "android/base/synchronization/Lock.h"

#include <functional>
#include <memory>
#include <vector>

namespace aemu {

class AndroidWindow;

// An abstract class for the composer implementation.
class Composer {
public:
    Composer() = default;
    virtual ~Composer() = default;
    virtual void advanceFrame() = 0;
};

class SurfaceFlinger {
public:
    using ComposerConstructFunc = std::function<Composer*(AndroidWindow*,
                                                          AndroidBufferQueue*,
                                                          AndroidBufferQueue*)>;

    SurfaceFlinger(
            int refreshRate,
            AndroidWindow* composeWindow,
            std::vector<ANativeWindowBuffer*> appBuffers,
            ComposerConstructFunc&& composerFunc,
            Vsync::Callback vsyncFunc);
    ~SurfaceFlinger();

    void start();
    void join();

    void connectWindow(AndroidWindow* window);
    void advanceFrame();

private:
    void disconnectWindow();

    android::base::Lock mLock;

    AndroidBufferQueue mApp2Sf;
    AndroidBufferQueue mSf2App;

    AndroidWindow* mComposeWindow = nullptr;
    std::unique_ptr<Composer> mComposerImpl;
    Vsync mVsync;

    AndroidWindow* mCurrentWindow = nullptr;
};

} // namespace aemu
