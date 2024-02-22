// Copyright 2024 The Android Open Source Project
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
#include "android/crashreport/detectors/TimeoutMonitor.h"

namespace android {
namespace crashreport {

TimeoutMonitor::TimeoutMonitor(std::chrono::milliseconds timeout,
                               std::function<void()> callback)
    : mTimeoutMs(timeout), mThreadRunning(true), mTimeoutFn(callback) {
    mWatcherThread = std::thread([this]() {
        std::unique_lock<std::mutex> lock(mMutex);
        if (!mCv.wait_for(lock, mTimeoutMs,
                          [this]() { return !mThreadRunning; })) {
            if (mThreadRunning && mTimeoutFn) {
                mTimeoutFn();
            }
        }
    });
}

TimeoutMonitor::~TimeoutMonitor() {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mThreadRunning = false;
    }
    mCv.notify_one();
    mWatcherThread.join();
}

}  // namespace crashreport
}  // namespace android