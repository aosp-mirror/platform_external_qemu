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

#include <utility>
#include "android/base/Log.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestTempDir.h"
#include "android/base/threads/Thread.h"
#include "android/utils/path.h"

namespace android {
namespace base {

// The TestSystem class provides a mock implementation that of the System that
// can be used by UnitTests. Instantiation will result in tempory replacement of
// the System singleton with this version, the prevous setting will be restored
// in the destructor of this object
//
// Some things to be aware of:
//
// Interaction with the filesystem will result in the creation of
// a temporary directory that will be used to interact with the
// filesystem. This temporary directory is deleted in the destructor.
//
// Path resolution is done as follows:
//   - Relative paths are resolved starting from current directory
//     which by default is: "/home"
//   - The construction of this object does not create the launcherDir,
//     appDataDir or home dir. If you need these directories to exist you will
//     have to create them as follows: getTempRoot()->makeSubDir("home").
//   - Path resolution can result in switching / into \ when running under
//      Win32. If you are doing anything with paths
//     it is best to include "android/utils/path.h" and use the PATH_SEP
//     and PATH_SEP_C macros to indicate path separators in your strings.
//
class TestSystem : public System {
public:
    TestSystem(const StringView& launcherDir,
               int hostBitness,
               const StringView& homeDir = "/home",
               const StringView& appDataDir = "")
        : mProgramDir(launcherDir),
          mProgramSubdir(""),
          mLauncherDir(launcherDir),
          mHomeDir(homeDir),
          mAppDataDir(appDataDir),
          mCurrentDir(homeDir),
          mHostBitness(hostBitness),
          mIsRemoteSession(false),
          mRemoteSessionType(),
          mTempDir(nullptr),
          mTempRootPrefix(),
          mEnvPairs(),
          mPrevSystem(System::setForTesting(this)),
          mTimes(),
          mShellFunc(nullptr),
          mShellOpaque(nullptr),
          mUnixTime() {}

    ~TestSystem() override {
        System::setForTesting(mPrevSystem);
        delete mTempDir;
    }

    const std::string& getProgramDirectory() const override {
        return mProgramDir;
    }

    // Set directory of currently executing binary.  This must be a subdirectory
    // of mLauncherDir and specified relative to mLauncherDir
    void setProgramSubDir(const StringView& programSubDir) {
        mProgramSubdir = programSubDir;
        if (programSubDir.empty()) {
            mProgramDir = getLauncherDirectory();
        } else {
            mProgramDir =
                    PathUtils::join(getLauncherDirectory(), programSubDir);
        }
    }

    const std::string& getLauncherDirectory() const override {
        if (mLauncherDir.size()) {
            return mLauncherDir;
        } else {
            return getTempRoot()->pathString();
        }
    }

    void setLauncherDirectory(const StringView& launcherDir) {
        mLauncherDir = launcherDir;
        // Update directories that are suffixes of |mLauncherDir|.
        setProgramSubDir(mProgramSubdir);
    }

    const std::string& getHomeDirectory() const override { return mHomeDir; }

    void setHomeDirectory(const StringView& homeDir) { mHomeDir = homeDir; }

    const std::string& getAppDataDirectory() const override {
        return mAppDataDir;
    }

    void setAppDataDirectory(const StringView& appDataDir) {
        mAppDataDir = appDataDir;
    }

    std::string getCurrentDirectory() const override { return mCurrentDir; }

    // Set current directory during unit-testing.
    void setCurrentDirectoryForTesting(const StringView& path) {
        mCurrentDir = path;
    }

    int getHostBitness() const override { return mHostBitness; }

    OsType getOsType() const override { return mOsType; }

    std::string getOsName() override { return mOsName; }

    bool isRunningUnderWine() const override { return mUnderWine; }

    void setRunningUnderWine(bool underWine) { mUnderWine = underWine; }

    Pid getCurrentProcessId() const override { return mPid; }

    WaitExitResult waitForProcessExit(int pid,
                                      Duration timeoutMs) const override {
        return WaitExitResult::Exited;
    }

    int getCpuCoreCount() const override { return mCoreCount; }

    void setCpuCoreCount(int count) { mCoreCount = count; }

    MemUsage getMemUsage() const override {
        MemUsage res;
        res.resident = 4294967295ULL;
        res.resident_max = 4294967295ULL * 2;
        res.virt = 4294967295ULL * 4;
        res.virt_max = 4294967295ULL * 8;
        res.total_phys_memory = 4294967295ULL * 16;
        res.total_page_file = 4294967295ULL * 32;
        return res;
    }

    void setCurrentProcessId(Pid pid) { mPid = pid; }

    void setOsType(OsType type) { mOsType = type; }

    std::string envGet(StringView varname) const override {
        for (size_t n = 0; n < mEnvPairs.size(); n += 2) {
            const std::string& name = mEnvPairs[n];
            if (name == varname) {
                return mEnvPairs[n + 1];
            }
        }
        return std::string();
    }

    std::vector<std::string> envGetAll() const override {
        std::vector<std::string> res;
        for (size_t i = 0; i < mEnvPairs.size(); i += 2) {
            const std::string& name = mEnvPairs[i];
            const std::string& val = mEnvPairs[i + 1];
            res.push_back(name + "=" + val);
        }
        return res;
    }

    void envSet(StringView varname, StringView varvalue) override {
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
                mEnvPairs.erase(mEnvPairs.begin() + index,
                                mEnvPairs.begin() + index + 2);
            }
        } else {
            if (index >= 0) {
                // Replacement.
                mEnvPairs[index + 1] = varvalue;
            } else {
                // Addition.
                mEnvPairs.push_back(varname);
                mEnvPairs.push_back(varvalue);
            }
        }
    }

    bool envTest(StringView varname) const override {
        for (size_t n = 0; n < mEnvPairs.size(); n += 2) {
            const std::string& name = mEnvPairs[n];
            if (name == varname) {
                return true;
            }
        }
        return false;
    }

    bool pathExists(StringView path) const override {
        return pathExistsInternal(toTempRoot(path));
    }

    bool pathIsFile(StringView path) const override {
        return pathIsFileInternal(toTempRoot(path));
    }

    bool pathIsDir(StringView path) const override {
        return pathIsDirInternal(toTempRoot(path));
    }

    bool pathIsLink(StringView path) const override {
        return pathIsLinkInternal(toTempRoot(path));
    }

    bool pathCanRead(StringView path) const override {
        return pathCanReadInternal(toTempRoot(path));
    }

    bool pathCanWrite(StringView path) const override {
        return pathCanWriteInternal(toTempRoot(path));
    }

    bool pathCanExec(StringView path) const override {
        return pathCanExecInternal(toTempRoot(path));
    }

    bool deleteFile(StringView path) const override {
        return deleteFileInternal(toTempRoot(path));
    }

    bool pathFileSize(StringView path, FileSize* outFileSize) const override {
        return pathFileSizeInternal(toTempRoot(path), outFileSize);
    }

    FileSize recursiveSize(StringView path) const override {
        return recursiveSizeInternal(toTempRoot(path));
    }

    bool pathFreeSpace(StringView path, FileSize* sizeInBytes) const override {
        return pathFreeSpaceInternal(toTempRoot(path), sizeInBytes);
    }

    bool fileSize(int fd, FileSize* outFileSize) const override {
        return fileSizeInternal(fd, outFileSize);
    }

    Optional<std::string> which(StringView executable) const override {
        return mWhich;
    }

    void setWhich(Optional<std::string> which) { mWhich = std::move(which); }

    Optional<Duration> pathCreationTime(StringView path) const override {
        return pathCreationTimeInternal(toTempRoot(path));
    }

    Optional<Duration> pathModificationTime(StringView path) const override {
        return pathModificationTimeInternal(toTempRoot(path));
    }

    Optional<DiskKind> pathDiskKind(StringView path) override {
        return diskKindInternal(toTempRoot(path));
    }
    Optional<DiskKind> diskKind(int fd) override {
        return diskKindInternal(fd);
    }

    std::vector<std::string> scanDirEntries(
            StringView dirPath,
            bool fullPath = false) const override {
        getTempRoot();  // make sure we have a temp root;

        std::string newPath = toTempRoot(dirPath);
        std::vector<std::string> result = scanDirInternal(newPath);
        if (fullPath) {
            std::string prefix = PathUtils::addTrailingDirSeparator(dirPath);
            for (auto& n : result) {
                n = prefix + n;
            }
        }
        return result;
    }

    virtual TestTempDir* getTempRoot() const {
        if (!mTempDir) {
            mTempDir = new TestTempDir("TestSystem");
            mTempRootPrefix = PathUtils::addTrailingDirSeparator(
                    std::string(mTempDir->path()));
        }
        return mTempDir;
    }

    bool isRemoteSession(std::string* sessionType) const override {
        if (!mIsRemoteSession) {
            return false;
        }
        *sessionType = mRemoteSessionType;
        return true;
    }

    // Force the remote session type. If |sessionType| is NULL or empty,
    // this sets the session as local. Otherwise, |*sessionType| must be
    // a session type.
    void setRemoteSessionType(const StringView& sessionType) {
        mIsRemoteSession = !sessionType.empty();
        if (mIsRemoteSession) {
            mRemoteSessionType = sessionType;
        }
    }

    Times getProcessTimes() const override { return mTimes; }

    void setProcessTimes(const Times& times) { mTimes = times; }

    // Type of a helper function that can be used during unit-testing to
    // receive the parameters of a runCommand() call. Register it
    // with setShellCommand().
    using ShellCommand = bool(void*,
                              const std::vector<std::string>&,
                              System::Duration,
                              System::ProcessExitCode*,
                              System::Pid*,
                              const std::string&);

    // Register a silent shell function. |shell| is the function callback,
    // and |shellOpaque| a user-provided pointer passed as its first parameter.
    void setShellCommand(ShellCommand* shell, void* shellOpaque) {
        mShellFunc = shell;
        mShellOpaque = shellOpaque;
    }

    bool runCommand(const std::vector<std::string>& commandLine,
                    RunOptions options,
                    System::Duration timeoutMs,
                    System::ProcessExitCode* outExitCode,
                    System::Pid* outChildPid,
                    const std::string& outputFile) override {
        if (!commandLine.size()) {
            return false;
        }
        // If a silent shell function was registered, invoke it, otherwise
        // ignore the command completely.
        bool result = true;

        if (mShellFunc) {
            result = (*mShellFunc)(mShellOpaque, commandLine, timeoutMs,
                                   outExitCode, outChildPid, outputFile);
        }

        return result;
    }

    Optional<std::string> runCommandWithResult(
            const std::vector<std::string>& commandLine,
            System::Duration timeoutMs = kInfinite,
            System::ProcessExitCode* outExitCode = nullptr) override {
        return {};
    }

    std::string getTempDir() const override { return "/tmp"; }

    time_t getUnixTime() const override { return getUnixTimeUs() / 1000000; }

    Duration getUnixTimeUs() const override { return getHighResTimeUs(); }

    WallDuration getHighResTimeUs() const override {
        if (mUnixTimeLive) {
            auto now = hostSystem()->getHighResTimeUs();
            mUnixTime += now - mUnixTimeLastQueried;
            mUnixTimeLastQueried = now;
        }
        return mUnixTime;
    }

    void setUnixTime(time_t time) { setUnixTimeUs(time * 1000000LL); }

    void setUnixTimeUs(Duration time) {
        mUnixTimeLastQueried = mUnixTime = time;
    }

    void setLiveUnixTime(bool enable) {
        mUnixTimeLive = enable;
        if (enable) {
            mUnixTimeLastQueried = hostSystem()->getHighResTimeUs();
        }
    }

    void sleepMs(unsigned n) const override {
        // Don't sleep in tests, use the static functions from Thread class
        // if you need a delay (you don't!).
        Thread::yield();  // Add a small delay to mimic the intended behavior.
    }

    void sleepUs(unsigned n) const override { sleepMs(n / 1000); }

    void yield() const override { Thread::yield(); }

private:
    std::string toTempRoot(const StringView& pathView) const {
        std::string path = pathView;
        if (!PathUtils::isAbsolute(path)) {
            auto currdir = getCurrentDirectory();
            path = currdir + PATH_SEP + path;
        }

        // mTempRootPrefix ends with a dir separator, ignore it for comparison.
        StringView prefix(mTempRootPrefix.c_str(), mTempRootPrefix.size() - 1);
        if (prefix.size() <= path.size() &&
            prefix == StringView(path.c_str(), prefix.size()) &&
            (prefix.size() == path.size() ||
             PathUtils::isDirSeparator(path[prefix.size()]))) {
            // Avoid prepending prefix if it's already there.
            return path;
        } else {
            // Resolve ., .. and replacing \ or / with PATH_SEP
            auto parts = PathUtils::decompose(mTempRootPrefix + path);
            PathUtils::simplifyComponents(&parts);
            auto res = PathUtils::recompose(parts);
            return res;
        }
    }

    std::string mProgramDir;
    std::string mProgramSubdir;
    std::string mLauncherDir;
    std::string mHomeDir;
    std::string mAppDataDir;
    std::string mCurrentDir;
    int mHostBitness;
    bool mIsRemoteSession;
    std::string mRemoteSessionType;
    mutable TestTempDir* mTempDir;
    mutable std::string mTempRootPrefix;
    std::vector<std::string> mEnvPairs;
    System* mPrevSystem;
    Times mTimes;
    ShellCommand* mShellFunc;
    void* mShellOpaque;
    mutable Duration mUnixTime;
    mutable Duration mUnixTimeLastQueried = 0;
    bool mUnixTimeLive = false;
    OsType mOsType = OsType::Windows;
    std::string mOsName = "";
    bool mUnderWine = false;
    Pid mPid = 0;
    int mCoreCount = 4;
    Optional<std::string> mWhich;
};

}  // namespace base
}  // namespace android
