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

#include "android/emulation/control/adb/AdbInterface.h"

#include <assert.h>                                        // for assert
#include <inttypes.h>                                      // for PRId64
#include <stdint.h>                                        // for int64_t
#include <cstdio>                                          // for printf
#include <fstream>                                         // for fstream
#include <limits>                                          // for numeric_li...
#include <queue>                                           // for queue
#include <sstream>                                         // for basic_stri...
#include <string>                                          // for string
#include <thread>                                          // for thread
#include <tuple>                                           // for tie, tuple
#include <utility>                                         // for pair, move

#include "android/avd/info.h"                              // for avdInfo_ge...
#include "android/avd/util.h"                              // for ANDROID_AV...
#include "android/base/EnumFlags.h"                        // for operator|
#include "android/base/Log.h"                              // for LogStream
#include "android/base/Optional.h"                         // for Optional
#include "android/base/StringView.h"                       // for StringView
#include "android/base/Uuid.h"                             // for Uuid
#include "android/base/files/PathUtils.h"                  // for PathUtils
#include "android/base/memory/LazyInstance.h"              // for LazyInstance
#include "android/base/memory/ScopedPtr.h"                 // for ScopedCPtr
#include "android/base/synchronization/Lock.h"             // for Lock, Auto...
#include "android/base/system/System.h"                    // for System
#include "android/base/threads/Async.h"                    // for AsyncThrea...
#include "android/base/threads/ParallelTask.h"             // for ParallelTask
#include "android/cmdline-option.h"                        // for android_cm...
#include "android/emulation/AdbHostServer.h"               // for AdbHostServer
#include "android/emulation/ConfigDirs.h"                  // for ConfigDirs
#include "android/emulation/control/adb/AdbConnection.h"   // for AdbConnection
#include "android/emulation/control/adb/AdbShellStream.h"  // for AdbShellSt...
#include "android/globals.h"                               // for android_av...
#include "android/utils/debug.h"                           // for VERBOSE_CHECK
#include "android/utils/path.h"                            // for path_delet...

namespace android {
namespace base {
class Looper;
class ScopedSocket;
class SocketWaiter;
class Version;
}  // namespace base
}  // namespace android

using android::base::AutoLock;
using android::base::Looper;
using android::base::Optional;
using android::base::ParallelTask;
using android::base::PathUtils;
using android::base::RunOptions;
using android::base::ScopedCPtr;
using android::base::ScopedPtr;
using android::base::ScopedSocket;
using android::base::SocketWaiter;
using android::base::System;
using android::base::Uuid;
using android::base::Version;

namespace android {
namespace emulation {

// We consider adb shipped with 23.1.0 or later to be modern enough
// to not notify people to upgrade adb. 23.1.0 ships with adb protocol 32
static const int kMinAdbProtocol = 32;
class AdbInterfaceImpl;

class AdbDaemonImpl : public AdbDaemon {
public:
    virtual Optional<int> getProtocolVersion() override {
        return AdbHostServer::getProtocolVersion();
    }
};

// Executes an adb command by launching the adb executable.
class AdbThroughExe : public AdbCommand {
    friend android::emulation::AdbInterfaceImpl;

public:
    using ResultCallback = AdbInterface::ResultCallback;
    ~AdbThroughExe() {
        if (VERBOSE_CHECK(adb)) {
            for (auto cmd : mCommand) {
                printf("%s ", cmd.c_str());
            }
            printf(" in %" PRId64 " usec\n",
                   System::get()->getUnixTimeUs() - mStart);
        }
    }

    // Returns true if the command is currently in the process of execution.
    bool inFlight() const override { return static_cast<bool>(mTask); }

    // Cancels the running command (has no effect if the command isn't running).
    void cancel() override { mCancelled = true; }

    void start(int checkTimeoutMs = 1000) override;

private:
    AdbThroughExe(android::base::Looper* looper,
                  const std::string& adb_path,
                  const std::string& serial_string,
                  const std::vector<std::string>& command,
                  bool want_output,
                  base::System::Duration timeoutMs,
                  ResultCallback&& callback);
    void taskFunction(OptionalAdbCommandResult* result);
    void taskDoneFunction(const OptionalAdbCommandResult& result);

    android::base::Looper* mLooper;
    std::unique_ptr<base::ParallelTask<OptionalAdbCommandResult>> mTask;
    ResultCallback mResultCallback;
    std::string mOutputFilePath;
    std::vector<std::string> mCommand;
    long mStart;
    bool mWantOutput;
    bool mCancelled = false;
    bool mFinished = false;
    int mTimeoutMs;
};

// Executes an AdbCommand by directly connecting to the ADBD inside the
// emulator. This does not spin up any executables. Currently on supports
// "shell/logcat" interactions.
class AdbDirect : public AdbCommand {
public:
    AdbDirect(const std::vector<std::string>& args,
              ResultCallback&& result_callback,
              bool want_output = true)
        : mResultCallback(std::move(result_callback)),
          mWantOutput(want_output) {
        int idx = 0;
        // We don't want to prefix with shell as we are using the shell
        // service..
        if (args[0] == "shell")
            idx++;
        while (idx < args.size()) {
            mCmd.append(args[idx++]);
            mCmd.append(" ");
        }
    }

    ~AdbDirect() {
        if (VERBOSE_CHECK(adb)) {
            printf("%s in %" PRId64 " usec\n", mCmd.c_str(),
                   System::get()->getUnixTimeUs() - mStart);
        }
    }
    // Returns true if the command is currently in the process of execution.
    bool inFlight() const override { return mRunning; }

    // Cancels the running command (has no effect if the command isn't running).
    void cancel() override { mTimeout = 0; }

    void start(int timeout_ms = 1000) override {
        mTimeout = System::get()->getUnixTimeUs() + timeout_ms * 1000;
        mStart = System::get()->getUnixTimeUs();
        auto shared = shared_from_this();
        std::thread t([this, timeout_ms, shared]() { execute(timeout_ms); });
        t.detach();
    }

private:
    void execute(int timeout_ms) {
        auto connection = AdbConnection::connection(timeout_ms);
        AdbShellStream shell(mCmd, connection);
        auto output =
                std::unique_ptr<std::stringstream>(new std::stringstream());
        std::vector<char> sout;
        std::vector<char> serr;
        int exitcode;
        while (shell.good() && System::get()->getUnixTimeUs() < mTimeout) {
            shell.read(sout, serr, exitcode);
            if (mWantOutput) {
                output->write(sout.data(), sout.size());
                output->write(serr.data(), serr.size());
            }
        }

        if (mResultCallback) {
            OptionalAdbCommandResult result;
            if (System::get()->getUnixTimeUs() < mTimeout) {
                result.emplace(exitcode, output.release());
            }
            mResultCallback(result);
        }
        mRunning = false;
    }

    int64_t mTimeout{std::numeric_limits<int64_t>::max()};
    bool mWantOutput{false};
    bool mRunning{true};
    std::string mCmd;
    long mStart;
    ResultCallback mResultCallback;
    AdbCommandPtr mSelf;
};

class AdbInterfaceImpl final : public AdbInterface {
public:
    friend AdbInterface::Builder;

    bool isAdbVersionCurrent() const final { return mAdbVersionCurrent; }

    void setCustomAdbPath(const std::string& path) final {
        mCustomAdbPath = path;
    }

    const std::string& detectedAdbPath() const final { return mAutoAdbPath; }

    const std::string& adbPath() final;

    virtual void setSerialNumberPort(int port) final {
        mSerialString = std::string("emulator-") + std::to_string(port);
    }

    const std::string& serialString() const final { return mSerialString; }

    AdbCommandPtr runAdbCommand(const std::vector<std::string>& args,
                                ResultCallback&& result_callback,
                                System::Duration timeout_ms,
                                bool want_output = true) final;

    void enqueueCommand(const std::vector<std::string>& args,
                        ResultCallback&& result) final;

private:
    explicit AdbInterfaceImpl(Looper* looper,
                              AdbLocator* locator,
                              AdbDaemon* daemon);
    void discoverAdbInstalls();
    void selectAdbPath();

    Looper* mLooper;
    std::unique_ptr<AdbLocator> mLocator;
    std::unique_ptr<AdbDaemon> mDaemon;
    std::string mAutoAdbPath;
    std::string mCustomAdbPath;
    std::string mSerialString;
    std::vector<std::pair<std::string, Optional<int>>> mAvailableAdbInstalls;
    bool mAdbVersionCurrent = false;
};

// static
std::unique_ptr<AdbInterface> AdbInterface::create(Looper* looper) {
    auto res = AdbInterface::Builder().setLooper(looper).build();
    return res;
}

static AdbInterface* sAdbInterface = nullptr;
static android::base::AsyncThreadWithLooper* sAdbInterfaceThread = nullptr;

// static
AdbInterface* AdbInterface::createGlobal(Looper* looper) {
    if (sAdbInterface)
        return sAdbInterface;

    auto res = AdbInterface::Builder().setLooper(looper).build();
    sAdbInterface = res.get();
    res.release();

    return sAdbInterface;
}

// static
AdbInterface* AdbInterface::createGlobalOwnThread() {
    if (sAdbInterface)
        return sAdbInterface;

    sAdbInterfaceThread = new android::base::AsyncThreadWithLooper();

    return createGlobal(sAdbInterfaceThread->getLooper());
}

// static
AdbInterface* AdbInterface::getGlobal() {
    return sAdbInterface;
}

struct AdbCommandQueueStatics {
    AdbInterface* adbInterface = nullptr;
    int retryCountdown = 0;
    bool didInitialCommand = false;
    android::base::Lock adbLock;

    std::queue<
            std::pair<std::vector<std::string>, AdbInterface::ResultCallback>>
            commandQueue;
};

static base::LazyInstance<AdbCommandQueueStatics> sStatics = LAZY_INSTANCE_INIT;

static constexpr int INITIAL_ADB_RETRY_LIMIT =
        50;  // Max # retries for first ADB request
static constexpr int SUBSEQUENT_ADB_RETRY_LIMIT =
        5;  // Max # retries for subsquent ADB requests

// This call-back is invoked when results are received from
// a queued ADB command.
// If this command succeeded, we send the next queued command.
// If this command has failed a small number of times, we resend
// it. If it has failed a large number of times, we discard it
// and move on.
static void adbResultHandler(const OptionalAdbCommandResult& result) {
    AutoLock lock(sStatics->adbLock);
    if (--(sStatics->retryCountdown) <= 0 ||
        (result && (result->exit_code == 0))) {
        // Either this command was successful or we are
        // ready to give up.
        // Either way, invoke the callback and retire this command.
        auto theCallback = sStatics->commandQueue.front().second;
        if (theCallback) {
            // Give this final result to the caller
            theCallback(result);
        }
        sStatics->commandQueue.pop();
        if (sStatics->commandQueue.size() == 0) {
            // The queue is now empty. We're done.
            return;
        }
        sStatics->retryCountdown = SUBSEQUENT_ADB_RETRY_LIMIT;
    }
    // Send or resend an ADB command
    sStatics->adbInterface->runAdbCommand(sStatics->commandQueue.front().first,
                                          adbResultHandler, 5000);
}

// A locator that will scan the filesystem for available ADB installs
class AdbLocatorImpl : public AdbLocator {
    std::vector<std::string> availableAdb() override;

    Optional<int> getAdbProtocolVersion(StringView adbPath) override;
};

// Construct the platform path with adb executable, or Nothing if it doesn't
// exist.
static Optional<std::string> platformPath(base::StringView root) {
    if (root.empty()) {
        LOG(VERBOSE) << " no root specified: ";
        return Optional<std::string>();
    }

    auto path = PathUtils::join(root, "platform-tools",
                                PathUtils::toExecutableName("adb"));
    return System::get()->pathCanExec(path) ? base::makeOptional(path)
                                            : base::kNullopt;
}

std::vector<std::string> AdbLocatorImpl::availableAdb() {
    std::vector<std::string> available = {};
    // First try finding ADB by the environment variable.
    auto adb = platformPath(android::ConfigDirs::getSdkRootDirectoryByEnv());
    if (adb) {
        available.push_back(std::move(*adb));
    }

    // Try finding it based on the emulator executable
    adb = platformPath(android::ConfigDirs::getSdkRootDirectoryByPath());
    if (adb) {
        available.push_back(std::move(*adb));
    }

    // See if it is on the path.
    adb = System::get()->which(PathUtils::toExecutableName("adb"));
    if (adb) {
        available.push_back(std::move(*adb));
    }

    LOG(VERBOSE) << "Found: " << available.size() << " adb executables";
    for (const auto& install : available) {
        LOG(VERBOSE) << "Adb: " << install;
    }

    return available;
}

// Gets the reported adb protocol version from the given executable
// This is the last digit in the adb version string.
Optional<int> AdbLocatorImpl::getAdbProtocolVersion(StringView adbPath) {
    const std::vector<std::string> adbVersion = {adbPath, "version"};
    int protocol = 0;
    // Retrieve the adb version.
    const int maxAdbRetrievalTimeMs = 500;
    auto read = System::get()->runCommandWithResult(
            adbVersion, maxAdbRetrievalTimeMs, nullptr);
    if (!read ||
        sscanf(read->c_str(), "Android Debug Bridge version %*d.%*d.%d",
               &protocol) != 1) {
        LOG(VERBOSE) << "Failed obtain protocol version from " << adbPath;
        return {};
    }
    LOG(VERBOSE) << "Path:" << adbPath << " protocol version: " << protocol;
    return Optional<int>(protocol);
}

std::unique_ptr<AdbInterface> AdbInterface::Builder::build() {
    if (!mLocator) {
        setAdbLocator(new AdbLocatorImpl());
    }
    if (!mDaemon) {
        setAdbDaemon(new AdbDaemonImpl());
    }

    AdbInterface* adb = new AdbInterfaceImpl(mLooper, mLocator.release(),
                                             mDaemon.release());
    return std::unique_ptr<AdbInterface>(adb);
}

// Discovers which adb executables are available.
void AdbInterfaceImpl::discoverAdbInstalls() {
    auto adbs = mLocator->availableAdb();
    for (const auto& install : adbs) {
        mAvailableAdbInstalls.push_back(std::make_pair(
                install, mLocator->getAdbProtocolVersion(install)));
    }
}

const std::string& AdbInterfaceImpl::adbPath() {
    if (!mCustomAdbPath.empty())
        return mCustomAdbPath;

    selectAdbPath();
    return mAutoAdbPath;
}

void AdbInterfaceImpl::selectAdbPath() {
    // Running without ADB! I too like to live on the edge.
    if (mAvailableAdbInstalls.empty()) {
        return;
    }

    Optional<int> adbVersion;
    std::string adbPath;

    // Maybe we can find one matching the current active version.
    // Even if this might be an ancient adb version.
    Optional<int> daemon = mDaemon->getProtocolVersion();
    if (daemon) {
        for (const auto& choice : mAvailableAdbInstalls) {
            std::tie(adbPath, adbVersion) = choice;
            if (daemon == adbVersion) {
                assert(adbVersion);  // Clearly true..
                mAdbVersionCurrent = *adbVersion >= kMinAdbProtocol;
                mAutoAdbPath = adbPath;
                return;
            }
        }
    }

    // A locator that will scan the filesystem for available ADB installs
    // Just take the first (likely the one from the SDK, our dir)
    std::tie(mAutoAdbPath, adbVersion) = mAvailableAdbInstalls.front();
    mAdbVersionCurrent = adbVersion ? *adbVersion >= kMinAdbProtocol : false;
}

AdbInterfaceImpl::AdbInterfaceImpl(Looper* looper,
                                   AdbLocator* locator,
                                   AdbDaemon* daemon)
    : mLooper(looper), mLocator(locator), mDaemon(daemon) {
    discoverAdbInstalls();
    selectAdbPath();
}

AdbCommandPtr AdbInterfaceImpl::runAdbCommand(
        const std::vector<std::string>& args,
        ResultCallback&& result_callback,
        System::Duration timeout_ms,
        bool want_output) {
    AdbCommandPtr command;
    if (!(android_cmdLineOptions && android_cmdLineOptions->no_direct_adb) && AdbConnection::failed() &&
        (args[0] == "shell" || args[0] == "logcat")) {
        command = std::shared_ptr<AdbDirect>(
                new AdbDirect(args, std::move(result_callback), want_output));
    } else {
        command = std::shared_ptr<AdbThroughExe>(new AdbThroughExe(
                mLooper, adbPath(), mSerialString, args, want_output,
                timeout_ms, std::move(result_callback)));
    }
    command->start();
    return command;
}

void AdbInterfaceImpl::enqueueCommand(const std::vector<std::string>& args,
                                      ResultCallback&& resultCallback) {
    AutoLock lock(sStatics->adbLock);

    if (sStatics->adbInterface == nullptr) {
        sStatics->adbInterface = this;
    }
    sStatics->commandQueue.push(make_pair(args, resultCallback));
    if (sStatics->commandQueue.size() == 1) {
        // We were idle. Start up.
        // Set the retry count for this command.
        // If this is the very first command, use a longer
        // retry, because ADB may still be launching.
        sStatics->retryCountdown = sStatics->didInitialCommand
                                           ? SUBSEQUENT_ADB_RETRY_LIMIT
                                           : INITIAL_ADB_RETRY_LIMIT;
        sStatics->didInitialCommand = true;

        // Send this command (the only one we have)
        sStatics->adbInterface->runAdbCommand(args, adbResultHandler, 5000);
    }
}

AdbThroughExe::AdbThroughExe(Looper* looper,
                             const std::string& adb_path,
                             const std::string& serial_string,
                             const std::vector<std::string>& command,
                             bool want_output,
                             System::Duration timeoutMs,
                             ResultCallback&& callback)
    : mLooper(looper),
      mResultCallback(std::move(callback)),
      mWantOutput(want_output),
      mTimeoutMs(timeoutMs) {
    mCommand.push_back(adb_path);

    const char* avdContentPath = nullptr;

    std::string outputFolder;

    if (android_avdInfo) {
        avdContentPath = avdInfo_getContentPath(android_avdInfo);
    }

    if (avdContentPath) {
        auto avdCmdDir = PathUtils::join(avdContentPath,
                                         ANDROID_AVD_TMP_ADB_COMMAND_DIR);

        if (path_mkdir_if_needed_no_cow(avdCmdDir.c_str(), 0755) < 0) {
            outputFolder = System::get()->getTempDir();
        } else {
            outputFolder = avdCmdDir;
        }
    } else {
        outputFolder = System::get()->getTempDir();
    }

    mOutputFilePath = PathUtils::join(
            outputFolder,
            std::string("adbcommand").append(Uuid::generate().toString()));

    // TODO: when run headless, the serial string won't be properly
    // initialized, so make a best attempt by using -e. This should be updated
    // when the headless emulator is given an AdbInterface reference.
    if (serial_string.empty()) {
        mCommand.push_back("-e");
    } else {
        mCommand.push_back("-s");
        mCommand.push_back(serial_string);
    }

    mCommand.insert(mCommand.end(), command.begin(), command.end());
}

void AdbThroughExe::start(int checkTimeoutMs) {
    if (!mTask && !mFinished) {
        auto shared =
                std::static_pointer_cast<AdbThroughExe>(shared_from_this());
        mTask.reset(new ParallelTask<OptionalAdbCommandResult>(
                mLooper,
                [shared](OptionalAdbCommandResult* result) {
                    shared->taskFunction(result);
                },
                [shared](const OptionalAdbCommandResult& result) {
                    shared->taskDoneFunction(result);
                },
                checkTimeoutMs));
        mTask->start();
        mStart = System::get()->getUnixTimeUs();
    }
}

void AdbThroughExe::taskDoneFunction(const OptionalAdbCommandResult& result) {
    if (!mCancelled) {
        mResultCallback(result);
    }
    mFinished = true;
    // This may invalidate this object and clean it up.
    // DO NOT reference any internal state from this class after this
    // point.
    mTask.reset();
}

void AdbThroughExe::taskFunction(OptionalAdbCommandResult* result) {
    if (mCommand.empty() || mCommand.front().empty()) {
        result->clear();
        return;
    }

    RunOptions output_flag = mWantOutput ? RunOptions::DumpOutputToFile
                                         : RunOptions::HideAllOutput;
    RunOptions run_flags = RunOptions::WaitForCompletion |
                           RunOptions::TerminateOnTimeout | output_flag;
    System::Pid pid;
    System::ProcessExitCode exit_code;

    bool command_ran = System::get()->runCommand(
            mCommand, run_flags, mTimeoutMs, &exit_code, &pid, mOutputFilePath);

    if (command_ran) {
        result->emplace(exit_code,
                        mWantOutput ? mOutputFilePath : std::string());
    } else {
        // Just in case (e.g. if the command got terminated on timeout).
        path_delete_file(mOutputFilePath.c_str());
    }
}

AdbCommandResult::AdbCommandResult(System::ProcessExitCode exitCode,
                                   const std::string& outputName)
    : exit_code(exitCode),
      output(outputName.empty() ? nullptr
                                : new std::ifstream(outputName.c_str())),
      output_name(outputName) {}

AdbCommandResult::AdbCommandResult(
        android::base::System::ProcessExitCode exitCode,
        std::istream* stream)
    : exit_code(exitCode), output(stream), output_name("") {}

AdbCommandResult::~AdbCommandResult() {
    output.reset();
    if (!output_name.empty()) {
        path_delete_file(output_name.c_str());
    }
}

}  // namespace emulation
}  // namespace android
