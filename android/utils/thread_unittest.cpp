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

#include "android/utils/thread.h"

#include <gtest/gtest.h>

namespace {

class CThreadTest : public testing::Test {
public:
    static intptr_t mainFuncImpl() {
        mainFuncCalled = true;
        return 108;
    }

    static void onExitFuncImpl() { onExitFuncCalled = true; }

    void SetUp() {
        mainFuncCalled = false;
        onExitFuncCalled = false;
    }

    static bool mainFuncCalled;
    static bool onExitFuncCalled;
};

bool CThreadTest::mainFuncCalled = false;
bool CThreadTest::onExitFuncCalled = false;

TEST_F(CThreadTest, simpleMain) {
    CThread* thread = thread_new(&CThreadTest::mainFuncImpl, NULL);
    EXPECT_FALSE(CThreadTest::mainFuncCalled);
    EXPECT_FALSE(CThreadTest::onExitFuncCalled);
    EXPECT_TRUE(thread_start(thread));
    intptr_t status;
    EXPECT_TRUE(thread_wait(thread, &status));
    EXPECT_EQ(108, status);
    EXPECT_TRUE(CThreadTest::mainFuncCalled);
    EXPECT_FALSE(CThreadTest::onExitFuncCalled);
}

TEST_F(CThreadTest, simpleMainAndExit) {
    CThread* thread = thread_new(&CThreadTest::mainFuncImpl,
                                 &CThreadTest::onExitFuncImpl);
    EXPECT_FALSE(CThreadTest::mainFuncCalled);
    EXPECT_FALSE(CThreadTest::onExitFuncCalled);
    EXPECT_TRUE(thread_start(thread));
    intptr_t status;
    EXPECT_TRUE(thread_wait(thread, &status));
    EXPECT_EQ(108, status);
    EXPECT_TRUE(CThreadTest::mainFuncCalled);
    EXPECT_TRUE(CThreadTest::onExitFuncCalled);
}

}  // namespace
