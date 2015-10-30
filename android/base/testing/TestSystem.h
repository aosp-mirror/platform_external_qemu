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

#pragma once

#include "android/base/containers/StringVector.h"
#include "android/base/files/PathUtils.h"
#include "android/base/Log.h"
#include "android/base/system/System.h"
#include "android/base/String.h"
#include "android/base/testing/TestTempDir.h"

namespace android {
namespace base {

class TestSystem : public System {
public:
    TestSystem(const char* programDir,
               int hostBitness,
               const char* homeDir = "/home",
               const char* appDataDir = "")
        : mProgramDir(programDir),
          mHomeDir(homeDir),
          mAppDataDir(appDataDir),
          mHostBitness(hostBitness),
          mIsRemoteSession(false),
          mRemoteSessionType(),
          mTempDir(NULL),
          mTempRootPrefix(),
          mEnvPairs(),
          mPrevSystem(System::setForTesting(this)),
          mTimes(),
          mSilentShellFunc(NULL),
          mSilentShellOpaque(NULL),
          mUnixTime() {}

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

    virtual const String& getHomeDirectory() const {
        return mHomeDir;
    }

    virtual const String& getAppDataDirectory() const {
        return mAppDataDir;
    }

    virtual int getHostBitness() const {
        return mHostBitness;
    }

    virtual String envGet(const char* varname) const {
        for (size_t n = 0; n < mEnvPairs.size(); n += 2) {
            const String& name = mEnvPairs[n];
            if (name == varname) {
                return mEnvPairs[n + 1];
            }
        }
        return String();
    }

    virtual void envSet(const char* varname, const char* varvalue) {
        // First, find if the name is in the array.
        int index = -1;
        for (size_t n = 0; n < mEnvPairs.size(); n += 2) {
            if (mEnvPairs[n] == varname) {
                index = static_cast<int>(n);
                break;
            }
        }
        if (!varvalue || !varvalue[0]) {
            // Remove definition, if any.
            if (index >= 0) {
                mEnvPairs.remove(index);
                mEnvPairs.remove(index);
            }
        } else {
            if (index >= 0) {
                // Replacement.
                mEnvPairs[index + 1] = String(varvalue);
            } else {
                // Addition.
                mEnvPairs.append(String(varname));
                mEnvPairs.append(String(varvalue));
            }
        }
    }

    virtual bool envTest(const char* varname) const {
        for (size_t n = 0; n < mEnvPairs.size(); n += 2) {
            const String& name = mEnvPairs[n];
            if (name == varname) {
                return true;
            }
        }
        return false;
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

    virtual bool isRemoteSession(String* sessionType) const {
        if (!mIsRemoteSession) {
            return false;
        }
        *sessionType = mRemoteSessionType;
        return true;
    }

    // Force the remote session type. If |sessionType| is NULL or empty,
    // this sets the session as local. Otherwise, |*sessionType| must be
    // a session type.
    void setRemoteSessionType(const char* sessionType) {
        mIsRemoteSession = (sessionType != NULL) && *sessionType;
        if (mIsRemoteSession) {
            mRemoteSessionType = sessionType;
        }
    }

    virtual Times getProcessTimes() const {
        return mTimes;
    }

    void setProcessTimes(const Times& times) {
        mTimes = times;
    }

    // Type of a helper function that can be used during unit-testing to
    // receive the parameters of a runSilentCommand() call. Register it
    // with setSilentCommandShell().
    typedef bool (SilentCommandShell)(void* opaque,
                                      const StringVector& commandLine);

    // Register a silent shell function. |shell| is the function callback,
    // and |shellOpaque| a user-provided pointer passed as its first parameter.
    void setSilentCommandShell(SilentCommandShell* shell, void* shellOpaque) {
        mSilentShellFunc = shell;
        mSilentShellOpaque = shellOpaque;
    }

    virtual bool runSilentCommand(const StringVector& commandLine) {
        if (!commandLine.size()) {
            return false;
        }
        // If a silent shell function was registered, invoke it, otherwise
        // ignore the command completely.
        if (mSilentShellFunc) {
            return (*mSilentShellFunc)(mSilentShellOpaque, commandLine);
        } else {
            return true;
        }
    }

    virtual String getTempDir() const { return String("/tmp"); }

    virtual time_t getUnixTime() const {
        return mUnixTime;
    }

    void setUnixTime(time_t time) {
        mUnixTime = time;
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
    String mHomeDir;
    String mAppDataDir;
    int mHostBitness;
    bool mIsRemoteSession;
    String mRemoteSessionType;
    mutable TestTempDir* mTempDir;
    mutable String mTempRootPrefix;
    StringVector mEnvPairs;
    System* mPrevSystem;
    Times mTimes;
    SilentCommandShell* mSilentShellFunc;
    void* mSilentShellOpaque;
    time_t mUnixTime;
};

}  // namespace base
}  // namespace android
