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

#pragma once

#include "android/base/Compiler.h"
#include "android/base/async/Looper.h"
#include "android/base/threads/Thread.h"
#include "android/base/threads/Types.h"

#include <memory>

namespace android {
namespace base {
namespace internal {

// Base class containing template free implemenatation details of LooperThread
// to minimize template specialization blowup.
//
// This is an implementation detail. DO NOT use this class directly.
class LooperThreadBase {
public:
    virtual ~LooperThreadBase() = default;

protected:
    LooperThreadBase(android::base::Looper* looper,
                     android::base::Looper::Duration checkTimeoutMs,
                     ThreadFlags flags);

    // API functions.
    bool start();
    bool inFlight();
    static bool fireAndForget(std::unique_ptr<LooperThreadBase> thread);

    // |LooperThread<T>| implements these hooks.
    virtual void mainImpl() = 0;
    virtual void onJoinedImpl() = 0;

private:
    class ManagedThread : public ::android::base::Thread {
    public:
        ManagedThread(LooperThreadBase* manager, ThreadFlags flags)
            : Thread(flags), mManager(manager) {}

        intptr_t main() {
            mManager->mainImpl();
            // This return value is ignored.
            return 0;
        }

    private:
        LooperThreadBase* mManager;
    };

    // Called prediodically to |tryJoin| the launched thread.
    static void tryWaitTillJoined(void* opaqueThis,
                                  android::base::Looper::Timer* timer);
    void cleanup();

    android::base::Looper* mLooper;
    android::base::Looper::Duration mCheckTimeoutMs;
    std::unique_ptr<ManagedThread> mManagedThread;

    bool isRunning = false;
    void* mResultBuffer = nullptr;
    bool shouldGarbageCollect = false;
    std::unique_ptr<android::base::Looper::Timer> mTimer;

    DISALLOW_COPY_AND_ASSIGN(LooperThreadBase);
};

}  // namespace internal
}  // namespace base
}  // namespace android
