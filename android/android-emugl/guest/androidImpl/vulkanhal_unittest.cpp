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

#include "GoldfishOpenglTestEnv.h"
#include "VulkanDispatch.h"

#include <vulkan/vulkan.h>

#include "android/base/memory/ScopedPtr.h"
#include "android/base/system/System.h"
#include "android/opengles.h"

#include <atomic>
#include <memory>
#include <vector>

using android::base::System;

namespace aemu {

class VulkanHalTest : public ::testing::Test {
protected:

    static GoldfishOpenglTestEnv* testEnv;

    static void SetUpTestCase() {
        testEnv = new GoldfishOpenglTestEnv;
    }

    static void TearDownTestCase() {
        delete testEnv;
        testEnv = nullptr;
    }

    void SetUp() override {
    }

    void TearDown() override {
        // Cancel all host threads as well
        android_finishOpenglesRenderer();
    }
};

// static
GoldfishOpenglTestEnv* VulkanHalTest::testEnv = nullptr;

TEST_F(VulkanHalTest, Basic) {
    uint32_t extCount = 0;
    std::vector<VkExtensionProperties> exts;
    EXPECT_EQ(VK_SUCCESS, vkEnumerateInstanceExtensionProperties(
                                  nullptr, &extCount, nullptr));
    exts.resize(extCount);
    EXPECT_EQ(VK_SUCCESS, vkEnumerateInstanceExtensionProperties(
                                  nullptr, &extCount, exts.data()));

    for (uint32_t i = 0; i < extCount; ++i) {
        printf("Available extension: %s\n", exts[i].extensionName);
    }
}

}  // namespace aemu
