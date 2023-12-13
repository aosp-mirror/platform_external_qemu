// Copyright 2023 The Android Open Source Project
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
#include "android/hw-sensors.h"

#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <ratio>
#include <thread>
#include <vector>

namespace android {

void register_and_unregister() {
    std::unique_ptr<int> i = std::make_unique<int>(1);
    HwSensorChangedCallback callback = [](void*) { /* ignored */ };
    android_hw_sensors_register_callback(callback, i.get());
    std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 10)));
    android_hw_sensors_unregister_callback(i.get());
}

TEST(android_hw_test, register_callback_thread_safe) {
    constexpr int NR_THREADS = 500;
    std::vector<std::thread> test_threads;
    test_threads.reserve(NR_THREADS);
    for (int i = 0; i < NR_THREADS; i++) {
        test_threads.emplace_back(std::thread(&register_and_unregister));
    }

    for (auto& thread : test_threads) {
        thread.join();
    }
}
}  // namespace android
