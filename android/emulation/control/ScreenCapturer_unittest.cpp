// Copyright (C) 2016 The Android Open Source Project
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

#include "android/emulation/control/ScreenCapturer.h"

#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/Compiler.h"
#include "android/base/system/System.h"
#include "android/base/StringView.h"
#include "android/base/testing/TestSystem.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

using android::base::StringView;
using android::base::System;
using android::emulation::ScreenCapturer;
using std::shared_ptr;
using std::string;
using std::vector;

class ScreenCapturerTest : public ::testing::Test {
public:
    // Convenience typedef.
    using Result = ScreenCapturer::Result;

    ScreenCapturerTest()
        : mTestSystem("/progdir",
                      System::kProgramBitness,
                      "/homedir",
                      "/appdir") {}

    void SetUp() override {
        mShellMayProceed = false;
        mTestSystem.setShellCommand(&ScreenCapturerTest::obsequiousShellCommand,
                                    this);
        mLooper = android::base::ThreadLooper::get();
        mCapturer = ScreenCapturer::create(
                mLooper, {"adb"}, mTestSystem.getTempDir(),
                [this](Result result) { this->resultSaver(result); }, 10);
        mCapturerWeak = mCapturer;
    }

    void TearDown() override {
        // Ensure that no hanging thread makes the object undead at the end of
        // the test.
        mCapturer.reset();
        expectCapturerAlive(false);
    }

    void looperAdvance() { mLooper->runWithTimeoutMs(100); }

    // It is an error to call this when no shell is waiting to proceed.
    void shellCommandAdvance() {
        // Default initialization of values set by the shell.
        // You may check that these values are changed in your
        // post-expectations.
        mShellCommandLine.clear();

        mShellMayProceed = true;
        // This has the potential to leave the test hanging, but in that case
        // the
        // test _is failing_.
        while (mShellMayProceed) {
            System::get()->sleepMs(10);
        }
    }

    void expectCapturerAlive(bool alive) {
        EXPECT_EQ(!alive, mCapturerWeak.expired());
    }

    void expectStringInVector(const string& item,
                              const vector<string>& vec) {
        EXPECT_TRUE(std::any_of(vec.begin(), vec.end(),
                                [item](const string& x) { return x == item; }))
                << "Failed to find \"" << item << "\" in \""
                << vectorToString(vec) << "\"";
    }

    string vectorToString(const vector<string>& vec) {
        string res;
        for (const auto& item : vec) {
            if (!res.empty()) {
                res += " ";
            }
            res += item;
        }
        return res;
    }

    // This function will be called on a separate thread (than the test
    // main-thread).
    // A shell function that busy loops on the atomic variable
    // |mShellMayProceed|.
    // Once unblocked, it reads the values of |mShell*| variables to dictate the
    // result. Finally, it resets |mShellMayProceed|.
    // It is not safe to have two instances of |obsequiousShellCommand| running
    // concurrently.
    static bool obsequiousShellCommand(void* opaque,
                                       const vector<string>& commandLine,
                                       System::Duration timeoutMs,
                                       System::ProcessExitCode* outExitCode,
                                       System::Pid* outChildPid,
                                       const string& outputFile) {
        return static_cast<ScreenCapturerTest*>(opaque)
                ->obsequiousShellCommandImpl(commandLine, timeoutMs,
                                             outExitCode, outChildPid,
                                             outputFile);
    }

    bool obsequiousShellCommandImpl(const vector<string>& commandLine,
                                    System::Duration,
                                    System::ProcessExitCode* outExitCode,
                                    System::Pid*,
                                    const string&) {
        while (!mShellMayProceed) {
            System::get()->sleepMs(20);
        }

        mShellCommandLine = commandLine;
        // Always play nice with the exit code.
        if (outExitCode) {
            *outExitCode = 0;
        }
        auto myResult = mShellResult;

        mShellMayProceed = false;
        return myResult;
    }

    void resultSaver(Result result) { mResult = result; }

protected:
    android::base::TestSystem mTestSystem;

    android::base::Looper* mLooper;
    shared_ptr<ScreenCapturer> mCapturer;
    std::weak_ptr<ScreenCapturer> mCapturerWeak;

    // Control the behaviour of shell commmands.
    std::atomic_bool mShellMayProceed;
    bool mShellResult = false;
    // Set by the shell command on a separate thread.
    vector<string> mShellCommandLine;

    // Result from the ScreenCapturer callback is saved here.
    Result mResult = Result::kUnknownError;

    DISALLOW_COPY_AND_ASSIGN(ScreenCapturerTest);
};

TEST_F(ScreenCapturerTest, screenshotCaptureFailure) {
    mCapturer->start();
    EXPECT_TRUE(mCapturer->inFlight());

    // Screencapture failes.
    mShellResult = false;
    shellCommandAdvance();
    looperAdvance();
    expectStringInVector("screencap", mShellCommandLine);
    EXPECT_EQ(Result::kCaptureFailed, mResult);
    EXPECT_FALSE(mCapturer->inFlight());
}

TEST_F(ScreenCapturerTest, adbPullFailure) {
    mCapturer->start();
    EXPECT_TRUE(mCapturer->inFlight());

    // Screencapture succeeds.
    mShellResult = true;
    shellCommandAdvance();
    // Don't |looperAdvance| as the screencapture thread is still in the process
    // of screen-capturing. ;)
    // Calling looperAdvance here will simply timeout.
    expectStringInVector("screencap", mShellCommandLine);
    EXPECT_TRUE(mCapturer->inFlight());

    // Adb pull fails.
    mShellResult = false;
    shellCommandAdvance();
    looperAdvance();
    // EXPECT_NE(mShellCommandLine.end(), mShellCommandLine.find("pull"));
    EXPECT_EQ(Result::kPullFailed, mResult);
    EXPECT_FALSE(mCapturer->inFlight());
}

TEST_F(ScreenCapturerTest, success) {
    mCapturer->start();
    EXPECT_TRUE(mCapturer->inFlight());

    // Screencapture succeeds.
    mShellResult = true;
    shellCommandAdvance();
    // Don't |looperAdvance| as the screencapture thread is still in the process
    // of screen-capturing. ;)
    // Calling looperAdvance here will simply timeout.
    expectStringInVector("screencap", mShellCommandLine);
    EXPECT_TRUE(mCapturer->inFlight());

    // Adb pull succeeds.
    mShellResult = true;
    shellCommandAdvance();
    looperAdvance();
    expectStringInVector("pull", mShellCommandLine);
    EXPECT_EQ(Result::kSuccess, mResult);
    EXPECT_FALSE(mCapturer->inFlight());
}

TEST_F(ScreenCapturerTest, synchronousFailures) {
    mCapturer->start();
    EXPECT_TRUE(mCapturer->inFlight());

    // Stating again should fail synchronously.
    mCapturer->start();
    EXPECT_EQ(Result::kOperationInProgress, mResult);

    // Unblock the command we'd blocked at first.
    mShellResult = false;
    shellCommandAdvance();
    looperAdvance();
    EXPECT_FALSE(mCapturer->inFlight());
}

TEST_F(ScreenCapturerTest, cancelCancelsCallback) {
    mCapturer->start();
    EXPECT_TRUE(mCapturer->inFlight());

    mCapturer->cancel();
    EXPECT_TRUE(mCapturer->inFlight());

    // Screencapture failes.
    mShellResult = false;
    shellCommandAdvance();
    looperAdvance();
    // Since the callback is not called, the result is not updated.
    EXPECT_EQ(Result::kUnknownError, mResult);
    EXPECT_FALSE(mCapturer->inFlight());
}

TEST_F(ScreenCapturerTest, objectOutlivesCallingThread) {
    mCapturer->start();
    EXPECT_TRUE(mCapturer->inFlight());

    mCapturer->cancel();
    mCapturer.reset();
    // Worker thread is still blocked, so the object must remain alive.
    expectCapturerAlive(true);

    mShellResult = false;
    shellCommandAdvance();
    looperAdvance();

    // Worker thread has finished, so the object must be cleaned up.
    expectCapturerAlive(false);
}
