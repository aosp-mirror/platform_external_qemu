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

#ifndef ANDROID_TESTING_TEST_SYSTEM_H
#define ANDROID_TESTING_TEST_SYSTEM_H

#include "android/base/files/PathUtils.h"
#include "android/base/Log.h"
#include "android/base/system/System.h"
#include "android/base/String.h"
#include "android/base/testing/TestTempDir.h"

namespace android {
namespace base {

class TestSystem : public System {
public:
    TestSystem(const char* programDir, int hostBitness) :
            mProgramDir(programDir),
            mHostBitness(hostBitness),
            mTempDir(NULL),
            mTempRootPrefix(),
            mPrevSystem(System::setForTesting(this)) {}

    virtual ~TestSystem() {
        System::setForTesting(mPrevSystem);
        delete mTempDir;
    }

    virtual const String& getProgramDirectory() const {
        if (mProgramDir.size()) {
            return mProgramDir;
        } else {
            return mTempDir->pathString();
        }
    }

    virtual int getHostBitness() const {
        return mHostBitness;
    }

    virtual bool pathExists(const char* path) {
        return pathExistsInternal(toTempRoot(path).c_str());
    }

    virtual bool pathIsFile(const char* path) {
        return pathIsFileInternal(toTempRoot(path).c_str());
    }

    virtual bool pathIsDir(const char* path) {
        return pathIsDirInternal(toTempRoot(path).c_str());
    }

    virtual StringVector scanDirEntries(const char* dirPath,
                                        bool fullPath = false) {
        if (!mTempDir) {
            // Nothing to return for now.
            LOG(ERROR) << "No temp root yet!";
            return StringVector();
        }
        String newPath = toTempRoot(dirPath);
        StringVector result = scanDirInternal(newPath.c_str());
        if (fullPath) {
            String prefix = PathUtils::addTrailingDirSeparator(
                    String(dirPath));
            size_t prefixLen = prefix.size();
            for (size_t n = 0; n < result.size(); ++n) {
                result[n] = String(result[n].c_str() + prefixLen);
            }
        }
        return result;
    }

    virtual TestTempDir* getTempRoot() const {
        if (!mTempDir) {
            mTempDir = new TestTempDir("TestSystem");
            mTempRootPrefix = PathUtils::addTrailingDirSeparator(
                    String(mTempDir->path()));
        }
        return mTempDir;
    }

private:
    String toTempRoot(const char* path) {
        String result = mTempRootPrefix;
        result += path;
        return result;
    }

    String fromTempRoot(const char* path) {
        String result = path;
        if (result.size() > mTempRootPrefix.size()) {
            result = path + mTempRootPrefix.size();
        }
        return result;
    }

    String mProgramDir;
    int mHostBitness;
    mutable TestTempDir* mTempDir;
    mutable String mTempRootPrefix;
    System* mPrevSystem;
};

}  // namespace base
}  // namespace android

#endif  // ANDROID_TESTING_TEST_SYSTEM_H
