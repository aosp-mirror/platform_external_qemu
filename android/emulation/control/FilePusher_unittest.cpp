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

#include "android/emulation/control/FilePusher.h"

#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"

#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <utility>
#include <vector>

using android::base::PathUtils;
using android::base::StringView;
using android::base::System;
using android::emulation::FilePusher;
using std::shared_ptr;
using std::string;
using std::vector;

class FilePusherTest : public testing::Test {
public:
    using ResultPair = std::pair<string, FilePusher::Result>;
    using ProgressPair = std::pair<double, bool>;

    void SetUp() override {
        mTestSystem.reset(new android::base::TestSystem(
                "/progdir", System::kProgramBitness, "/homedir", "/appdir"));
        mLooper = android::base::ThreadLooper::get();
        // Speed up test execution by setting progress check timeout to 0.
        mFilePusher = FilePusher::create(mLooper, 0);
        mFilePusher->setAdbCommandArgs({"myadb"});
        mFilePusherWeak = mFilePusher;
        mSubscriptionToken = mFilePusher->subscribeToUpdates(
                [this](StringView filePath, FilePusher::Result result) {
                    this->mResults.push_back({filePath, result});
                },
                [this](double progress, bool done) {
                    this->mProgresses.push_back({progress, done});
                });

        // By default, the 'adb push' command will always block.
        mTestSystem->setShellCommand(&FilePusherTest::fakeShellCommand, this);
        mAtomicNumCommands = 0;
    }

    void TearDown() {
        mSubscriptionToken.reset();
        mFilePusher.reset();
        EXPECT_TRUE(mFilePusherWeak.expired());
        mTestSystem.reset();
    }

    static bool fakeShellCommand(void* opaque,
                                 const vector<string>& command,
                                 System::Duration,
                                 System::ProcessExitCode* outExitCode,
                                 System::Pid*,
                                 const string&) {
        EXPECT_GE(command.size(), 2);
        EXPECT_EQ("myadb", command[0]);
        EXPECT_EQ("push", command[1]);

        auto thisPtr = static_cast<FilePusherTest*>(opaque);

        // Only |mAtomicNumCommands| iterations of fakeShellCommand are allowed
        // to proceed. This way, the test may block the external command
        // launched in the worker thread.
        short noisy = 0;
        while (int(thisPtr->mAtomicNumCommands) <= 0) {
            ++noisy;
            if (noisy == 20) {
                LOG(VERBOSE) << "Fake command waiting to be unblocked";
                fflush(stdout);
                noisy = 0;
            }
            System::get()->sleepMs(2);
        }
        thisPtr->mAtomicNumCommands--;

        if (outExitCode) {
            *outExitCode = thisPtr->mFakeExitCode;
        }
        return thisPtr->mFakeRunCommandResult;
    }

    void looperAdvance(int numCommands) {
        mAtomicNumCommands = numCommands;
        // Increase this timeout if the test is flaky on slow servers.
        mLooper->runWithTimeoutMs(50);
    }

    void createFileAndEnqueue(StringView file) {
        EXPECT_TRUE(mTestSystem->getTempRoot()->makeSubFile(file.c_str()));
        mFilePusher->enqueue(file, "/tmp");
    }

protected:
    std::unique_ptr<android::base::TestSystem> mTestSystem;
    android::base::Looper* mLooper;
    shared_ptr<FilePusher> mFilePusher;
    std::weak_ptr<FilePusher> mFilePusherWeak;
    FilePusher::SubscriptionToken mSubscriptionToken;

    // Fake runCommand support.
    vector<ResultPair> mResults;
    vector<ProgressPair> mProgresses;
    System::ProcessExitCode mFakeExitCode = 0;
    bool mFakeRunCommandResult = true;
    std::atomic_int mAtomicNumCommands;
};

TEST_F(FilePusherTest, success) {
    createFileAndEnqueue("file1");
    createFileAndEnqueue("file2");
    createFileAndEnqueue("file3");
    createFileAndEnqueue("file4");

    const vector<ProgressPair> kExpectedProgress = {
            {0.0, false},  {0.0, false}, {0.0, false},  {0.0, false},
            {0.25, false}, {0.5, false}, {0.75, false}, {1.0, true}};
    const vector<ResultPair> kExpectedResults = {
            {"file1", FilePusher::Result::Success},
            {"file2", FilePusher::Result::Success},
            {"file3", FilePusher::Result::Success},
            {"file4", FilePusher::Result::Success}};

    looperAdvance(4);
    EXPECT_EQ(kExpectedProgress, mProgresses);
    EXPECT_EQ(kExpectedResults, mResults);
}

TEST_F(FilePusherTest, fileNotReadable) {
    mFilePusher->enqueue("file1", "/tmp");
    createFileAndEnqueue("file2");

    const vector<ProgressPair> kExpectedProgress = {
            {0.0, false}, {1.0, true}, {0.0, false}, {1.0, true}};
    const vector<ResultPair> kExpectedResults = {
            {"file1", FilePusher::Result::FileReadError},
            {"file2", FilePusher::Result::Success}};

    // Only file2 will actually be pushed.
    looperAdvance(1);
    EXPECT_EQ(kExpectedProgress, mProgresses);
    EXPECT_EQ(kExpectedResults, mResults);
}

TEST_F(FilePusherTest, commandFailures) {
    createFileAndEnqueue("file1");
    createFileAndEnqueue("file2");
    createFileAndEnqueue("file3");
    createFileAndEnqueue("file4");

    const vector<ProgressPair> kExpectedProgress = {
            {0.0, false},  {0.0, false}, {0.0, false},  {0.0, false},
            {0.25, false}, {0.5, false}, {0.75, false}, {1.0, true}};
    const vector<ResultPair> kExpectedResults = {
            {"file1", FilePusher::Result::Success},
            {"file2", FilePusher::Result::UnknownError},
            {"file3", FilePusher::Result::AdbPushFailure},
            {"file4", FilePusher::Result::Success}};

    looperAdvance(1);
    mFakeRunCommandResult = false;
    looperAdvance(1);
    mFakeRunCommandResult = true;
    mFakeExitCode = 99;
    looperAdvance(1);
    mFakeRunCommandResult = true;
    mFakeExitCode = 0;
    looperAdvance(1);

    EXPECT_EQ(kExpectedProgress, mProgresses);
    EXPECT_EQ(kExpectedResults, mResults);
}

TEST_F(FilePusherTest, enqueueWhilePushing) {
    const vector<ProgressPair> kExpectedProgress = {
            {0.0, false},     {0.0, false},   // First enqueue.
            {0.5, false},                     // First batch of push.
            {1.0 / 3, false}, {0.25, false},  // Second enqueue.
            {0.5, false},     {0.75, false},
            {1.0, true}};  // Final batch of push.
    const vector<ResultPair> kExpectedResults = {
            {"file1", FilePusher::Result::Success},
            {"file2", FilePusher::Result::Success},
            {"file3", FilePusher::Result::Success},
            {"file4", FilePusher::Result::Success}};

    createFileAndEnqueue("file1");
    createFileAndEnqueue("file2");
    looperAdvance(1);

    createFileAndEnqueue("file3");
    createFileAndEnqueue("file4");
    looperAdvance(3);

    EXPECT_EQ(kExpectedProgress, mProgresses);
    EXPECT_EQ(kExpectedResults, mResults);
}

TEST_F(FilePusherTest, cancelWhilePushing) {
    const vector<ProgressPair> kExpectedProgress = {{0.0, false},
                                                    {0.0, false},
                                                    {0.0, false},
                                                    {1.0 / 3, false},
                                                    {2.0 / 3, true}};
    const vector<ResultPair> kExpectedResults = {
            {"file1", FilePusher::Result::Success},
            {"file2", FilePusher::Result::Success},
    };

    createFileAndEnqueue("file1");
    createFileAndEnqueue("file2");
    createFileAndEnqueue("file3");

    looperAdvance(1);
    mFilePusher->cancel();
    // Should finish the push that is already in-flight, but drop the last one.
    looperAdvance(1);

    EXPECT_EQ(kExpectedProgress, mProgresses);
    EXPECT_EQ(kExpectedResults, mResults);
}

TEST_F(FilePusherTest, unsubscribeWhilePushing) {
    const vector<ProgressPair> kExpectedProgress = {
            {0.0, false}, {0.0, false}, {0.0, false}, {1.0 / 3, false}};
    const vector<ResultPair> kExpectedResults = {
            {"file1", FilePusher::Result::Success}};

    createFileAndEnqueue("file1");
    createFileAndEnqueue("file2");
    createFileAndEnqueue("file3");

    looperAdvance(1);
    // We drop our reference of the object, as well as unsubscribe from it.
    mSubscriptionToken.reset();
    mFilePusher.reset();
    // Should still finish all pushes, but we won't see updates.
    looperAdvance(2);

    EXPECT_EQ(kExpectedProgress, mProgresses);
    EXPECT_EQ(kExpectedResults, mResults);
}
