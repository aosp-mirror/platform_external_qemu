// Copyright (C) 2018 The Android Open Source Project
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

#include "android/base/threads/Async.h"

#include "android/base/async/Looper.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

TEST(AsyncTest, Simple) {
    for (int i = 0; i < 10; i++) {
        async([] { });
    }
}

// Tests that we can create and destroy the async thread with looper.
TEST(AsyncThreadWithLooper, Simple) {
    AsyncThreadWithLooper thread;
}

// Tests that we can send a few simple tasks to the async thread with looper.
TEST(AsyncThreadWithLooper, Tasks) {
    AsyncThreadWithLooper thread;
    auto looper = thread.getLooper();
    auto timer1 = looper->createTimer(
        [](void* opaque, Looper::Timer* timer) {
            fprintf(stderr, "%s: timer1\n", __func__);
        }, 0);
    timer1->startAbsolute(0);
    auto timer2 = looper->createTimer(
        [](void* opaque, Looper::Timer* timer) {
            fprintf(stderr, "%s: timer2\n", __func__);
        }, 0);
    timer2->startAbsolute(0);
}

}  // namespace base
}  // namespace android
