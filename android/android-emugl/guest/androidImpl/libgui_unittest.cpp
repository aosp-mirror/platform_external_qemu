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
#include <gtest/gtest.h>

#include "AndroidWindow.h"
#include "Vsync.h"

#include "android/base/memory/ScopedPtr.h"
#include "android/base/system/System.h"

#include <atomic>
#include <memory>
#include <vector>

using android::base::System;

namespace aemu {

TEST(AndroidWindow, Basic) {
    const int kWidth = 1920;
    const int kHeight = 1080;

    auto window = android::base::makeCustomScopedPtr(
            create_host_anativewindow(kWidth, kHeight),
            destroy_host_anativewindow);

    int width = 0;
    int height = 0;

    ANativeWindow_query(window.get(), ANATIVEWINDOW_QUERY_DEFAULT_WIDTH,
                        &width);
    ANativeWindow_query(window.get(), ANATIVEWINDOW_QUERY_DEFAULT_HEIGHT,
                        &height);
    EXPECT_EQ(kWidth, width);
    EXPECT_EQ(kHeight, height);

    int minSwapInterval = 0;
    int maxSwapInterval = 0;
    ANativeWindow_query(window.get(), ANATIVEWINDOW_QUERY_MIN_SWAP_INTERVAL,
                        &minSwapInterval);
    ANativeWindow_query(window.get(), ANATIVEWINDOW_QUERY_MAX_SWAP_INTERVAL,
                        &maxSwapInterval);
    EXPECT_EQ(0, minSwapInterval);
    EXPECT_EQ(1, maxSwapInterval);
}

TEST(AndroidWindow, BufferQueue) {
    auto window = android::base::makeCustomScopedPtr(
            create_host_anativewindow(256, 256), destroy_host_anativewindow);

    AndroidWindow* aWindow = AndroidWindow::getSelf(window.get());

    AndroidBufferQueue toWindow;
    AndroidBufferQueue fromWindow;

    aWindow->setProducer(&toWindow, &fromWindow);

    std::vector<AndroidBufferQueue::Item> itemsForQueueing = {
            {nullptr, 0},
            {nullptr, 1},
            {nullptr, 2},
    };

    const size_t bufferCount = itemsForQueueing.size();

    for (const auto& item : itemsForQueueing) {
        toWindow.queueBuffer(item);
    }

    for (size_t i = 0; i < bufferCount; i++) {
        AndroidBufferQueue::Item item;
        ANativeWindow_dequeueBuffer(window.get(), &item.buffer, &item.fenceFd);
        EXPECT_EQ(itemsForQueueing[i].buffer, item.buffer);
        EXPECT_EQ(itemsForQueueing[i].fenceFd, item.fenceFd);
        ANativeWindow_queueBuffer(window.get(), item.buffer, item.fenceFd);
    }

    for (size_t i = 0; i < bufferCount; i++) {
        AndroidBufferQueue::Item item;
        fromWindow.dequeueBuffer(&item);
        EXPECT_EQ(itemsForQueueing[i].buffer, item.buffer);
        EXPECT_EQ(itemsForQueueing[i].fenceFd, item.fenceFd);
    }
}

// TestSystem will totally mess this up. Disable for now
TEST(Vsync, DISABLED_Basic) {
    std::atomic<int> count { 0 };
    Vsync vsync(1000, [&count]() { ++count; });

    constexpr System::Duration kSleepIntervalMs = 20;
    constexpr System::Duration kSleepLimitMs = 15000;

    System::Duration totalSleepMs = 0;
    while (count.load(std::memory_order_relaxed) == 0) {
        totalSleepMs += kSleepIntervalMs;
        System::get()->sleepMs(kSleepIntervalMs);
        ASSERT_LE(totalSleepMs, kSleepLimitMs);
    }

    vsync.join();
}

}  // namespace aemu
