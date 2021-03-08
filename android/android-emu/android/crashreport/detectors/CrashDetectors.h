// Copyright 2019 The Android Open Source Project
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
#include "android/crashreport/HangDetector.h"
#include "android/base/system/System.h"

namespace android {
namespace crashreport {

using base::System;

// A hang detector that should check only after a given
// interval has expired.
class TimedHangDetector : public StatefulHangdetector {
public:
    TimedHangDetector(System::Duration intervalMs, StatefulHangdetector* check);
    bool check() override;

private:
    std::unique_ptr<StatefulHangdetector> mInner;
    bool mFailedOnce = false;
    System::Duration mIntervalMs = 0;
    System::Duration mNextCheck = 0;
};

// Checks if the heartbeat of the guest is still going.
// Note: Heartbeats cannot wrap around, so this effectively
// will put an upper bound to your emulator runtime of 2^64
// heartbeats. This should be more than enough :-)
class HeartBeatDetector : public StatefulHangdetector {
public:
    HeartBeatDetector(std::function<int()> getHeartbeat,
                      volatile int* bootComplete);
    bool check() override;

private:
    std::function<int()> mGetHeartbeat;
    volatile int* mBootComplete;
    int64_t mBeatCount = 0;
};

}  // namespace crashreport
}  // namespace android
