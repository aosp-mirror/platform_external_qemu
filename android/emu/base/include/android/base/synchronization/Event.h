// Copyright 2020 The Android Open Source Project
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

#include "android/base/synchronization/MessageChannel.h"

// super minimal wrapper around MessageChannel to make it less verbose to
// specify simple events and waiting on them
//
// WARNING: This does not behave like most 'event' classes; if two threads call
// wait(), then signal() will only unblock one of the threads (there's no
// 'broadcast' feature). Not to be used widely.
//
// Once you need broadcasting or anything more complicated than a single waiter
// and single signaler, please use a different method (such as plain condition
// variables with mutexes)
namespace android {
namespace base {

class Event {
public:
    void wait() {
        int res;
        mChannel.receive(&res);
    }

    bool timedWait(System::Duration wallTimeUs) {
        auto res = mChannel.timedReceive(wallTimeUs);
        if (res) return true;
        return false;
    }

    void signal() {
        mChannel.trySend(0);
    }

private:
    MessageChannel<int, 1> mChannel;
};

}  // namespace base
}  // namespace android
