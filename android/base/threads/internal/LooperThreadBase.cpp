// Copyright (C) 2015 The Android Open Source Project
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

#include "android/base/threads/internal/LooperThreadBase.h"

namespace android {
namespace base {
namespace internal {

using std::unique_ptr;

LooperThreadBase::LooperThreadBase(Looper* looper,
                                   Looper::Duration checkTimeoutMs,
                                   ThreadFlags flags)
    : mLooper(looper),
      mCheckTimeoutMs(checkTimeoutMs),
      mManagedThread(new ManagedThread(this, flags)) {}

bool LooperThreadBase::start() {
    if (!mManagedThread->start()) {
        cleanup();
        return false;
    }
    mTimer.reset(mLooper->createTimer(&tryWaitTillJoined, this));
    mTimer->startRelative(0);
    isRunning = true;
    return true;
}

bool LooperThreadBase::inFlight() {
    return isRunning;
}

// static
bool LooperThreadBase::fireAndForget(unique_ptr<LooperThreadBase> thread) {
    thread->shouldGarbageCollect = true;
    bool result = thread->start();
    // |thread| is responsible for its own cleanup.
    thread.release();
    return result;
}

// static
void LooperThreadBase::tryWaitTillJoined(void* opaqueThis,
                                         Looper::Timer* timer) {
    auto thisPtr = static_cast<LooperThreadBase*>(opaqueThis);
    if (thisPtr->mManagedThread->tryWait(nullptr)) {
        thisPtr->onJoinedImpl();
        thisPtr->isRunning = false;
        // Possibly invalides |this|.
        thisPtr->cleanup();
        return;
    }

    thisPtr->mTimer->startRelative(thisPtr->mCheckTimeoutMs);
}

void LooperThreadBase::cleanup() {
    if (shouldGarbageCollect) {
        delete this;
    }
}

}  // namespace internal
}  // namespace base
}  // namespace android
