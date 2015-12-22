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
    TestSystem(StringView launcherDir,
               int hostBitness,
               StringView homeDir = "/home",
               StringView appDataDir = "")
        : mProgramDir(launcherDir),
          mLauncherDir(launcherDir),
          mHomeDir(homeDir),
          mAppDataDir(appDataDir),
          mCurrentDir(homeDir),
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

    virtual const String& getProgramDirectory() const { return mProgramDir; }

    // Set directory of currently executing binary.  This must be a subdirectory
    // of mLauncherDir and specified relative to mLauncherDir
    void setProgramDir(const String& programDir) {
        if (programDir.empty()) {
            mProgramDir = mLauncherDir;
            return;
        }

        StringVector pathList;
        pathList.push_back(getLauncherDirectory());
        pathList.push_back(programDir);
        mProgramDir = PathUtils::recompose(pathList);
    }

    virtual const String& getLauncherDirectory() const {
        if (mLauncherDir.size()) {
            return mLauncherDir;
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

    virtual String getCurrentDirectory() const { return mCurrentDir; }

    // Set current directory during unit-testing.
    void setCurrentDirectoryForTesting(StringView path) { mCurrentDir = path; }

    virtual int getHostBitness() const {
        return mHostBitness;
    }

    virtual OsType getOsType() const override {
        return mOsType;
    }

    void setOsType(OsType type) {
        mOsType = type;
    }

    virtual String envGet(StringView varname) const {
        for (size_t n = 0; n < mEnvPairs.size(); n += 2) {
            const String& name = mEnvPairs[n];
            if (name == varname) {
                return mEnvPairs[n + 1];
            }
        }
        return String();
    }

    virtual void envSet(StringView varname, StringView varvalue) {
        // First, find if the name is in the array.
        int index = -1;
        for (size_t n = 0; n < mEnvPairs.size(); n += 2) {
            if (mEnvPairs[n] == varname) {
                index = static_cast<int>(n);
                break;
            }
        }
        if (varvalue.empty()) {
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

    virtual bool envTest(StringView varname) const {
        for (size_t n = 0; n < mEnvPairs.size(); n += 2) {
            const String& name = mEnvPairs[n];
            if (name == varname) {
                return true;
            }
        }
        return false;
    }

    virtual bool pathExists(StringView path) const {
        return pathExistsInternal(toTempRoot(path));
    }

    virtual bool pathIsFile(StringView path) const {
        return pathIsFileInternal(toTempRoot(path));
    }

    virtual bool pathIsDir(StringView path) const {
        return pathIsDirInternal(toTempRoot(path));
    }

    virtual bool pathCanRead(StringView path) const override {
        return pathCanReadInternal(toTempRoot(path));
    }

    virtual bool pathCanWrite(StringView path) const override {
        return pathCanWriteInternal(toTempRoot(path));
    }

    virtual bool pathCanExec(StringView path) const override {
        return pathCanExecInternal(toTempRoot(path));
    }

    virtual StringVector scanDirEntries(StringView dirPath,
                                        bool fullPath = false) const {
        if (!mTempDir) {
            // Nothing to return for now.
            LOG(ERROR) << "No temp root yet!";
            return StringVector();
        }
        String newPath = toTempRoot(dirPath);
        StringVector result = scanDirInternal(newPath);
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
    void setRemoteSessionType(StringView sessionType) {
        mIsRemoteSession = !sessionType.empty();
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

    virtual bool runSilentCommand(const StringVector& commandLine,
                                  bool wait = false) {
        return RunFailed != runCommand(commandLine,
                  wait ? RunOptions::WaitForCompletion : RunOptions::Default);
    }

    virtual int runCommand(const StringVector& commandLine,
                           RunOptions options) override {
        if (!commandLine.size()) {
            return RunFailed;
        }
        // If a silent shell function was registered, invoke it, otherwise
        // ignore the command completely.
        if (mSilentShellFunc) {
            return (*mSilentShellFunc)(mSilentShellOpaque, commandLine)
                    ? ((options & RunOptions::ReturnExitCode) != 0)
                        ? 0 : 1
                    : RunFailed;
        } else {
            return ((options & RunOptions::ReturnExitCode) != 0) ? 0 : 1;
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
    String toTempRoot(StringView path) const {
        String result = mTempRootPrefix;
        result += path;
        return result;
    }

    String fromTempRoot(StringView path) {
        if (path.size() > mTempRootPrefix.size()) {
            return path.c_str() + mTempRootPrefix.size();
        }
        return path;
    }

    String mProgramDir;
    String mLauncherDir;
    String mHomeDir;
    String mAppDataDir;
    String mCurrentDir;
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
    OsType mOsType = OsType::Windows;
};

}  // namespace base
}  // namespace android
