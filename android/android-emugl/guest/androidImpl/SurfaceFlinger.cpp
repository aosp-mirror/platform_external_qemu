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
#include "SurfaceFlinger.h"

#include "AndroidWindow.h"

#include <stdio.h>

#define E(fmt,...) \
    fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__);

using android::base::AutoLock;
using android::base::FunctionView;
using android::base::Lock;

namespace aemu {

SurfaceFlinger::SurfaceFlinger(
        int refreshRate,
        AndroidWindow* composeWindow,
        std::vector<ANativeWindowBuffer*> appBuffers,
        SurfaceFlinger::ComposerConstructFunc&& composerFunc,
        Vsync::Callback vsyncFunc)
    : mComposeWindow(composeWindow),
      mComposerImpl(composerFunc(mComposeWindow, &mApp2Sf, &mSf2App)),
      mVsync(refreshRate, std::move(vsyncFunc)) {

    for (auto buffer : appBuffers) {
        mSf2App.queueBuffer(AndroidBufferQueue::Item(buffer));
    }
}

SurfaceFlinger::~SurfaceFlinger() {
    join();
}

void SurfaceFlinger::start() {
    mVsync.start();
}

void SurfaceFlinger::join() {
    mVsync.join();
}

void SurfaceFlinger::connectWindow(AndroidWindow* window) {
    AutoLock lock(mLock);

    disconnectWindow();

    if (!window) return;

    window->setProducer(&mSf2App, &mApp2Sf);

    // and prime the queue again
    AndroidBufferQueue::Item item;
    while (mApp2Sf.try_dequeueBuffer(&item)) {
        mSf2App.queueBuffer(item);
    }

    mCurrentWindow = window;
}

void SurfaceFlinger::advanceFrame() {
    mComposerImpl->advanceFrame();
}

void SurfaceFlinger::disconnectWindow() {
    if (!mCurrentWindow) return;

    mCurrentWindow->setProducer(nullptr, nullptr);

    mCurrentWindow = nullptr;
}

} // namespace aemu
