// Copyright 2016 The Android Open Source Project
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

#include "android/metrics/FileMetricsWriter.h"

#include "android/base/Optional.h"
#include "android/base/StringView.h"
#include "android/base/testing/TestLooper.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"
#include "android/base/Uuid.h"
#include "android/metrics/proto/clientanalytics.pb.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"

#include <gtest/gtest.h>

#include <algorithm>

using namespace android::base;
using namespace android::metrics;

static constexpr StringView kSpoolDir = ".android/metrics/spool";

namespace {

class FileMetricsWriterTest : public ::testing::Test {
public:
    TestSystem mSystem{"/", System::kProgramBitness, "/home"};
    TestTempDir* mRoot = mSystem.getTempRoot();
    std::unique_ptr<TestLooper> mLooper{new TestLooper()};

    FileMetricsWriter::Ptr mWriter;

    void SetUp() override {
        ASSERT_TRUE(mRoot != nullptr);
        ASSERT_TRUE(mLooper != nullptr);
        mSystem.envSet("ANDROID_SDK_HOME", mRoot->pathString());
    }

    std::string spoolDir() { return mRoot->makeSubPath(kSpoolDir.c_str()); }
    static std::string sessionId() { return Uuid::nullUuidStr; }
};

}  // namespace

TEST_F(FileMetricsWriterTest, createDestroy) {
    EXPECT_FALSE(mSystem.pathIsDir(spoolDir()));
    mWriter = FileMetricsWriter::create(spoolDir(), sessionId(), 0, nullptr, 0);
    EXPECT_TRUE(mWriter != nullptr);
    EXPECT_TRUE(mSystem.pathIsDir(spoolDir()));
    auto files = mSystem.scanDirEntries(spoolDir());
    ASSERT_EQ(2, (int)files.size());

    // one of the files is the open metrics file, another one is the lock.
    EXPECT_TRUE(PathUtils::extension(files[0]) == ".lock" ||
                PathUtils::extension(files[1]) == ".lock");
    EXPECT_TRUE(PathUtils::extension(files[0]) == ".open" ||
                PathUtils::extension(files[1]) == ".open");

    mWriter.reset();

    // on destruction it finalizes and unlocks the metrics file.
    files = mSystem.scanDirEntries(spoolDir());
    ASSERT_FALSE(files.empty());
    ASSERT_EQ(1, (int)files.size());
    EXPECT_TRUE(PathUtils::extension(files[0]) == ".trx");
}

TEST_F(FileMetricsWriterTest, createDestroyWithTimer) {
    EXPECT_FALSE(mSystem.pathIsDir(spoolDir()));
    mWriter = FileMetricsWriter::create(spoolDir(), sessionId(), 0,
                                        mLooper.get(), 10);
    EXPECT_TRUE(mWriter != nullptr);
    EXPECT_TRUE(mSystem.pathIsDir(spoolDir()));
    auto files = mSystem.scanDirEntries(spoolDir());
    ASSERT_EQ(2, (int)files.size());

    // make sure we have no timer, as it is only created on the first message
    // write.
    EXPECT_EQ(0, mLooper->timers().size());

    // one of the files is the open metrics file, another one is the lock.
    EXPECT_TRUE(PathUtils::extension(files[0]) == ".lock" ||
                PathUtils::extension(files[1]) == ".lock");
    EXPECT_TRUE(PathUtils::extension(files[0]) == ".open" ||
                PathUtils::extension(files[1]) == ".open");

    mWriter.reset();

    // on destruction it finalizes and unlocks the metrics file.
    files = mSystem.scanDirEntries(spoolDir());
    ASSERT_FALSE(files.empty());
    ASSERT_EQ(1, (int)files.size());
    EXPECT_TRUE(PathUtils::extension(files[0]) == ".trx");

    // we should still have no timers
    EXPECT_EQ(0, mLooper->timers().size());
}

TEST_F(FileMetricsWriterTest, finalizeAbandonedSessionFilesNoDir) {
    // no spool directory - no sessions
    auto sessions =
            FileMetricsWriter::finalizeAbandonedSessionFiles(spoolDir());
    EXPECT_TRUE(sessions.empty());
}

TEST_F(FileMetricsWriterTest, finalizeAbandonedSessionFilesEmptyDir) {
    path_mkdir_if_needed(spoolDir().c_str(), 0700);
    // empty spool directory - no sessions
    auto sessions =
            FileMetricsWriter::finalizeAbandonedSessionFiles(spoolDir());
    EXPECT_TRUE(sessions.empty());
}

TEST_F(FileMetricsWriterTest, finalizeAbandonedSessionFilesManyFiles) {
    // create several session-like files now in the spool directory
    path_mkdir_if_needed(spoolDir().c_str(), 0700);

    std::vector<std::string> testSessions;
    for (int i = 0; i < 10; ++i) {
        testSessions.push_back(Uuid::generateFast().toString());
        ASSERT_TRUE(mRoot->makeSubFile(PathUtils::join(
                kSpoolDir, StringFormat("emulator-metrics-%s-1-1.open",
                                        testSessions.back()))));
        ASSERT_TRUE(mRoot->makeSubFile(PathUtils::join(
                kSpoolDir, StringFormat("emulator-metrics-%s-2-1.open",
                                        testSessions.back()))));
    }

    // now finalize them
    auto sessions =
            FileMetricsWriter::finalizeAbandonedSessionFiles(spoolDir());

    // all test sessions should be present in |sessions|
    EXPECT_EQ(sessions.size(), testSessions.size());
    for (const auto& session : testSessions) {
        EXPECT_NE(sessions.end(), sessions.find(session));
    }

    // make sure the open files are renamed and there are no other files
    auto files = mSystem.scanDirEntries(spoolDir());
    // there were twice as many files as sessions
    EXPECT_EQ(2 * sessions.size(), files.size()) << "Files: \n" <<
                                                    std::accumulate(files.begin(), files.end(), std::string(),
                                                                                            [](const std::string& cur, const std::string& next) { return cur + '\n' + next; });

    for (const auto& file : files) {
        EXPECT_STREQ(PathUtils::extension(file).c_str(), ".trx");

        // make sure all files have a known session ID in the name
        EXPECT_NE(sessions.end(),
                  std::find_if(sessions.begin(), sessions.end(),
                               [&file](const std::string& session) {
                                   return file.find(session) !=
                                          std::string::npos;
                               }));
    }
}

TEST_F(FileMetricsWriterTest, finalizeAbandonedSessionFilesWrongNames) {
    path_mkdir_if_needed(spoolDir().c_str(), 0700);

    // create some non-open-like files
    ASSERT_TRUE(mRoot->makeSubFile(
            PathUtils::join(kSpoolDir, "emulator-metrics.trx")));
    ASSERT_TRUE(
            mRoot->makeSubFile(PathUtils::join(kSpoolDir, "emulator-metrics")));
    ASSERT_TRUE(mRoot->makeSubFile(
            PathUtils::join(kSpoolDir, "emulator-metrics.open.closed")));
    ASSERT_TRUE(mRoot->makeSubFile(
            PathUtils::join(kSpoolDir, "emulator-metrics.lock")));

    // no sessions
    auto sessions =
            FileMetricsWriter::finalizeAbandonedSessionFiles(spoolDir());
    EXPECT_TRUE(sessions.empty());
}

TEST_F(FileMetricsWriterTest, finalizeAbandonedSessionFilesLockedSession) {
    path_mkdir_if_needed(spoolDir().c_str(), 0700);

    // create a locked open session file and make sure it is not finalized
    auto openSessionName = PathUtils::join(
            kSpoolDir,
            StringFormat("emulator-metrics-%s-1-1.open", sessionId()));
    ASSERT_TRUE(mRoot->makeSubFile(openSessionName));

    {
        // lock the file and try finalizing
        auto scopedLock = makeCustomScopedPtr(
                filelock_create(mRoot->makeSubPath(openSessionName).c_str()), filelock_release);
        ASSERT_TRUE(scopedLock);

        auto sessions =
                FileMetricsWriter::finalizeAbandonedSessionFiles(spoolDir());
        // no sessions
        EXPECT_TRUE(sessions.empty());
    }

    // check that now, when there's no lock, the only file left in the directory
    // is the open session file
    auto files = mSystem.scanDirEntries(spoolDir());
    ASSERT_EQ(1, files.size());
    EXPECT_TRUE(openSessionName.find(files[0]) != std::string::npos)
            << "Expected file name '" << files[0] << "' to be a file name from"
               " session file path '" << openSessionName << '\'';
}
