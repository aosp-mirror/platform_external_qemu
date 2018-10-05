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
};

class SurfaceFlinger {
public:
    using ComposerConstructFunc = std::function<Composer*(AndroidWindow*,
                                                          AndroidBufferQueue*,
                                                          AndroidBufferQueue*)>;

    SurfaceFlinger(
            int width,
            int height,
            std::vector<ANativeWindowBuffer*> appBuffers,
            std::vector<ANativeWindowBuffer*> composeBuffers,
            ComposerConstructFunc&& composerFunc);

    ~SurfaceFlinger() = default;

    void connectWindow(AndroidWindow* window);

private:
    void disconnectWindow();

    android::base::Lock mLock;

    AndroidBufferQueue mFromHwc;
    AndroidBufferQueue mToHwc;

    AndroidBufferQueue mApp2Sf;
    AndroidBufferQueue mSf2App;

    std::unique_ptr<AndroidWindow> mComposeWindow;
    std::unique_ptr<Composer> mComposerImpl;

    AndroidWindow* mCurrentWindow = nullptr;
};

} // namespace aemu