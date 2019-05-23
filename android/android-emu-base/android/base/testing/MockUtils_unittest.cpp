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

#include "android/base/testing/MockUtils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(ReplaceMock, Simple) {
    std::function<void()> mock;

    bool called = false;
    auto handle = replaceMock(&mock, [&called]() { called = true; });

    mock();
    EXPECT_TRUE(called);
}

TEST(ReplaceMock, Restore) {
    int which = 0;
    std::function<void()> mock = [&which]() { which = 1; };

    {
        auto handle = replaceMock(&mock, [&which]() { which = 2; });
        mock();
        EXPECT_EQ(which, 2);
    }

    mock();
    EXPECT_EQ(which, 1);
}

TEST(ReplaceMock, Pointer) {
    int* mock = nullptr;

    {
        int replacement = 123;
        auto handle = replaceMock(&mock, &replacement);
        EXPECT_EQ(mock, &replacement);
    }

    EXPECT_EQ(mock, nullptr);
}

TEST(ReplaceMock, Move) {
    int which = 0;
    std::function<void()> mock = [&which]() { which = 1; };
    MockReplacementHandle<decltype(mock)> handle;

    {
        auto handle2 = replaceMock(&mock, [&which]() { which = 2; });
        handle = std::move(handle2);
    }

    mock();
    EXPECT_EQ(which, 2);
}