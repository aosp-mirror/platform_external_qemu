// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/update-check/UpdateChecker.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/Version.h"

#include <gtest/gtest.h>

namespace android {
namespace update_check {

using android::base::Version;

class MockVersionExtractor : public IVersionExtractor {
public:
    MockVersionExtractor()
        : mExtractVersionResult(0, 0, 0),
          mExtractVersionCallCount(0),
          mGetCurrentVersionResult(0, 0, 0),
          mGetCurrentVersionCallCount(0) {}

    virtual Version extractVersion(const std::string& data) const {
        ++mExtractVersionCallCount;
        EXPECT_EQ(mExtractVersionDataParam, data);
        return mExtractVersionResult;
    }

    virtual Version getCurrentVersion() const {
        ++mGetCurrentVersionCallCount;
        return mGetCurrentVersionResult;
    }

    std::string mExtractVersionDataParam;
    Version mExtractVersionResult;
    mutable int mExtractVersionCallCount;
    Version mGetCurrentVersionResult;
    mutable int mGetCurrentVersionCallCount;
};

class MockDataLoader : public IDataLoader {
public:
    MockDataLoader() : mLoadCallCount(0) {}

    virtual std::string load() {
        ++mLoadCallCount;
        return mLoadResult;
    }

    std::string mLoadResult;
    int mLoadCallCount;
};

class MockTimeStorage : public ITimeStorage {
public:
    MockTimeStorage()
        : mLockCallCount(),
          mGetTimeCallCount(),
          mSetTimeCallCount(),
          mLockResult(),
          mGetTimeResult(),
          mSetTimeParam() {}

    virtual bool lock() {
        ++mLockCallCount;
        return mLockResult;
    }

    virtual time_t getTime() {
        ++mGetTimeCallCount;
        return mGetTimeResult;
    }

    virtual void setTime(time_t time) {
        ++mSetTimeCallCount;
        EXPECT_EQ(mSetTimeParam, time);
    }

    int mLockCallCount;
    int mGetTimeCallCount;
    int mSetTimeCallCount;
    bool mLockResult;
    time_t mGetTimeResult;
    time_t mSetTimeParam;
};

class MockNewerVersionReporter : public INewerVersionReporter {
public:
    MockNewerVersionReporter()
        : mReportNewerVersionExistingParam(0, 0, 0),
          mReportNewerVersionNewerParam(0, 0, 0),
          mReportNewerVersionCallCount(0) {}

    virtual void reportNewerVersion(const Version& existing,
                                    const Version& newer) {
        EXPECT_EQ(mReportNewerVersionExistingParam, existing);
        EXPECT_EQ(mReportNewerVersionNewerParam, newer);
        ++mReportNewerVersionCallCount;
    }

    Version mReportNewerVersionExistingParam;
    Version mReportNewerVersionNewerParam;
    int mReportNewerVersionCallCount;
};

class TestUpdateChecker : public UpdateChecker {
public:
    TestUpdateChecker(IVersionExtractor* extractor,
                      IDataLoader* loader,
                      ITimeStorage* storage,
                      INewerVersionReporter* reporter)
        : UpdateChecker(extractor, loader, storage, reporter) {}

    // make it public for testing
    using UpdateChecker::asyncWorker;
};

class TestData {
public:
    TestData() : mSystem("", 32) {
        mDataLoader = new MockDataLoader();
        mReporter = new MockNewerVersionReporter();
        mTimeStorage = new MockTimeStorage();
        mVersionExtractor = new MockVersionExtractor();
        mUC.reset(new TestUpdateChecker(mVersionExtractor, mDataLoader,
                                        mTimeStorage, mReporter));
    }

    std::auto_ptr<TestUpdateChecker> mUC;
    MockDataLoader* mDataLoader;
    MockNewerVersionReporter* mReporter;
    MockTimeStorage* mTimeStorage;
    MockVersionExtractor* mVersionExtractor;
    android::base::TestSystem mSystem;
};

TEST(UpdateChecker, init) {
    TestData test;
    test.mTimeStorage->mLockResult = false;
    EXPECT_FALSE(test.mUC->init());
    EXPECT_EQ(1, test.mTimeStorage->mLockCallCount);

    /////////////////////////////////////////////////////////////////////
    test.mTimeStorage->mLockResult = true;
    EXPECT_TRUE(test.mUC->init());
    EXPECT_EQ(2, test.mTimeStorage->mLockCallCount);
}

TEST(UpdateChecker, needsCheck) {
    TestData test;
    // set last check time and current time to be close enough -
    // no new check is needed
    test.mTimeStorage->mGetTimeResult = time_t(1);
    test.mSystem.setUnixTime(time_t(2));

    EXPECT_FALSE(test.mUC->needsCheck());
    EXPECT_EQ(1, test.mTimeStorage->mGetTimeCallCount);

    /////////////////////////////////////////////////////////////////////
    // now set the current time to require the check (+one year from the epoch)
    test.mSystem.setUnixTime(time_t(365 * 24 * 60 * 60));
    EXPECT_TRUE(test.mUC->needsCheck());
    EXPECT_EQ(2, test.mTimeStorage->mGetTimeCallCount);
}

TEST(UpdateChecker, getVersion) {
    TestData test;

    // Set the version number
    test.mVersionExtractor->mGetCurrentVersionResult = Version(4, 5, 6);

    // Get the version as a string
    Version returnedVersion = test.mVersionExtractor->getCurrentVersion();

    EXPECT_TRUE( returnedVersion.toString().equals("4.5.6") );
}

TEST(UpdateChecker, asyncWorker) {
    TestData test;

    // first set version to be the newest
    test.mDataLoader->mLoadResult =
            test.mVersionExtractor->mExtractVersionDataParam = "test";
    test.mVersionExtractor->mExtractVersionResult = Version(1, 0, 0);
    test.mVersionExtractor->mGetCurrentVersionResult = Version(1, 0, 0);
    test.mSystem.setUnixTime(time_t(1));
    test.mTimeStorage->mSetTimeParam = time_t(1);

    test.mUC->asyncWorker();
    EXPECT_EQ(1, test.mDataLoader->mLoadCallCount);
    EXPECT_EQ(1, test.mVersionExtractor->mExtractVersionCallCount);
    EXPECT_EQ(1, test.mVersionExtractor->mGetCurrentVersionCallCount);
    EXPECT_EQ(1, test.mTimeStorage->mSetTimeCallCount);

    // didn't report the newer version
    EXPECT_EQ(0, test.mReporter->mReportNewerVersionCallCount);

    /////////////////////////////////////////////////////////////////////
    // now there's a newer version available
    test.mVersionExtractor->mExtractVersionResult = Version(2, 0, 0);
    test.mReporter->mReportNewerVersionExistingParam = Version(1, 0, 0);
    test.mReporter->mReportNewerVersionNewerParam = Version(2, 0, 0);

    test.mUC->asyncWorker();
    EXPECT_EQ(2, test.mDataLoader->mLoadCallCount);
    EXPECT_EQ(2, test.mVersionExtractor->mExtractVersionCallCount);
    EXPECT_EQ(2, test.mVersionExtractor->mGetCurrentVersionCallCount);
    EXPECT_EQ(2, test.mTimeStorage->mSetTimeCallCount);

    // reported the newer version
    EXPECT_EQ(1, test.mReporter->mReportNewerVersionCallCount);

    /////////////////////////////////////////////////////////////////////
    // failed to get the last version
    test.mVersionExtractor->mExtractVersionResult = Version::Invalid();

    test.mUC->asyncWorker();
    EXPECT_EQ(3, test.mDataLoader->mLoadCallCount);
    EXPECT_EQ(3, test.mVersionExtractor->mExtractVersionCallCount);
    EXPECT_EQ(3, test.mVersionExtractor->mGetCurrentVersionCallCount);

    // didn't store the last check time
    EXPECT_EQ(2, test.mTimeStorage->mSetTimeCallCount);
    // didn't report the newer version
    EXPECT_EQ(1, test.mReporter->mReportNewerVersionCallCount);
}

}  // namespace update_check
}  // namespace android
