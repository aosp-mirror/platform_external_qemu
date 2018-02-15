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

#include "android/emulation/control/AdbInterface.h"

#include "android/base/ArraySize.h"
#include "android/base/StringView.h"
#include "android/base/Uuid.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/Log.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketWaiter.h"
#include "android/base/system/System.h"
#include "android/base/misc/FileUtils.h"
#include "android/emulation/ComponentVersion.h"
#include "android/emulation/ConfigDirs.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"
#include "android/utils/sockets.h"

#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <utility>


using android::base::AutoLock;
using android::base::Looper;
using android::base::ParallelTask;
using android::base::PathUtils;
using android::base::RunOptions;
using android::base::System;
using android::base::Uuid;
using android::base::Version;
using android::base::ScopedPtr;
using android::base::ScopedCPtr;
using android::base::ScopedSocket;
using android::base::SocketWaiter;

namespace android {
namespace emulation {

class AdbInterfaceImpl final : public AdbInterface {
public:
    explicit AdbInterfaceImpl(Looper* looper);

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

private:
    void configureAdbPaths();

    Looper* mLooper;
    std::string mAutoAdbPath;
    std::string mCustomAdbPath;
    std::string mSerialString;
    std::map<int, std::string> mAdbVersions;
    int mProtocol; // Protocol version of mAutoAdbPath
    bool mAdbVersionCurrent = false;
};

std::unique_ptr<AdbInterface> AdbInterface::create(Looper* looper) {
    return std::unique_ptr<AdbInterface>{new AdbInterfaceImpl(looper)};
}

static const int kAdbQueryTimeoutMs = 300;
static const int kAdbDefaultPort = 5037;

// Gets the reported adb protocol version from the given executable
// This is the last digit in the adb version string.
static int getAdbVersion(const std::string& adb_path) {
    const std::vector<std::string> adbVersion = { adb_path, "version" };
    int major, minor, protocol = 0;
    std::string tmp_dir = System::get()->getTempDir();

    // Get temporary file path
    constexpr base::StringView temp_filename_pattern = "adbversion_XXXXXX";
    std::string temp_file_path =
            PathUtils::join(tmp_dir, temp_filename_pattern);

    auto tmpfd = android::base::ScopedFd(mkstemp((char*)temp_file_path.data()));
    if (!tmpfd.valid()) {
        return protocol;
    }
    temp_file_path.resize(strlen(temp_file_path.c_str()));
    auto tmpFileDeleter = base::makeCustomScopedPtr(
            &temp_file_path,
            [](const std::string* name) { remove(name->c_str()); });

    // Retrieve the adb version.
    if (!System::get()->runCommand(adbVersion,
                                   RunOptions::WaitForCompletion |
                                           RunOptions::TerminateOnTimeout |
                                           RunOptions::DumpOutputToFile,
                                   kAdbQueryTimeoutMs, nullptr, nullptr,
                                   temp_file_path)) {
        LOG(VERBOSE) << "Failue to invoke: " << adb_path;
        return 0;
    }
    std::string read = android::readFileIntoString(temp_file_path).valueOr({});
    sscanf(read.c_str(),"Android Debug Bridge version %d.%d.%d", &major, &minor, &protocol);
    LOG(VERBOSE) << "Discovered " << adb_path << " : " << protocol;
    return protocol;
}

// Helper function, checks if the version of adb in the given SDK is
// fresh enough.
static bool checkAdbVersion(const std::string& sdk_root_directory,
                            const std::string& adb_path) {
    static const int kMinAdbVersionMajor = 23;
    static const int kMinAdbVersionMinor = 1;
    static const Version kMinAdbVersion(kMinAdbVersionMajor,
                                        kMinAdbVersionMinor, 0);
    if (sdk_root_directory.empty()) {
        return false;
    }

    if (!System::get()->pathCanExec(adb_path)) {
        return false;
    }

    Version version = android::getCurrentSdkVersion(
            sdk_root_directory, android::SdkComponentType::PlatformTools);

    if (version.isValid()) {
        return !(version < kMinAdbVersion);

    } else {
        // If the version is invalid, assume the tools directory is broken in
        // some way, and updating should fix the problem.
        return false;
    }
}

// Attempts to read a response from the given socket fd
// Worst time: len * kAdbQueryTimeoutMs
static std::string readResponse(int fd, SocketWaiter* waiter, int len) {
    std::string buffer(len, '\0');
    unsigned int idx = 0;
    while (waiter->wait(kAdbQueryTimeoutMs) && len > 0) {
        int ret = socket_recv(fd, (void*) &buffer[idx], len);
        if (ret <= 0) {
            break;
        }
        len -= ret;
        idx += ret;
    }
    LOG(VERBOSE) << "Read: [" << buffer << "]" << ", idx: " << idx;
    return buffer;
}

// Best effort to figure out if an adb server is running, and if so which
// protocol version it is using.
static int getAdbDaemonVersion() {
    std::string server_host = System::get()->envGet("ANDROID_ADB_SERVER_ADDRESS");
    std::string server_port = System::get()->envGet("ANDROID_ADB_SERVER_PORT");
    // Protocol = 4 bytes, indicating length of message (0x000c = 12 chars)
    // "host:version".size() == 12
    std::string version_info = "000chost:version";
    int running = 0;
    int port = kAdbDefaultPort;
    ScopedPtr<SocketWaiter> waiter(SocketWaiter::create());

    // First we figure out where we think we can find ADB.
    sscanf(server_port.c_str(), "%d", &port);
    if (server_host.empty()) {
        server_host = "localhost";
    }

    // Okay. let's see if we can connect to this adb server.
    ScopedSocket fd = socket_network_client(server_host.c_str(), port, SOCKET_STREAM);
    if (!fd.valid()) {
        LOG(VERBOSE) << "Unable to connect to " << server_host << ":" << port;
        return running;
    }
    waiter->update(fd.get(), SocketWaiter::kEventRead);

    // And ask it for its protocol version.
    if (socket_send(fd.get(), version_info.c_str(), version_info.size()) != version_info.size()) {
        return running;
    }

    // The server should respond with OKAY, after which it will send the
    // version.
    if (readResponse(fd.get(), waiter.get(), 4).compare("OKAY") == 0) {
        std::string version_result = readResponse(fd.get(), waiter.get(), 8);
        // We expect the hex response.
        sscanf(version_result.c_str(), "0004%04x", &running);
    }

    LOG(VERBOSE) << "Extracted version: " << running;
    return running;
}

#ifdef _WIN32
static const char kAdbExe[] = "adb.exe";
#else
static const char kAdbExe[] = "adb";
#endif

// We consider adb shipped with 23.1.0 or later to be modern enough
// to not notify people to upgrade adb. 23.1.0 ships with adb protocol 32
static const int kMinAdbProtocol = 32;

// Configures the adb paths
void AdbInterfaceImpl::configureAdbPaths() {
    // First see if we can get the running protocol version of adb
    int protocol = getAdbDaemonVersion();
    std::vector<std::pair<std::string, std::string>> options;

    // First try finding ADB by the environment variable.
    auto sdk_root_by_env = android::ConfigDirs::getSdkRootDirectoryByEnv();
    if (!sdk_root_by_env.empty()) {
        // If ANDROID_SDK_ROOT is defined, the user most likely wanted to use
        // that ADB. Store it for later - if the second potential ADB path
        // also fails, we'll warn the user about this one.
        auto adb_path = PathUtils::join(sdk_root_by_env, "platform-tools",
                                        PathUtils::toExecutableName("adb"));
        options.push_back(std::make_pair(sdk_root_by_env, adb_path));
        mAdbVersions[getAdbVersion(adb_path)] = adb_path;
    }

    // Try to infer the path based on the emulator executable location.
    auto sdk_root_by_path = android::ConfigDirs::getSdkRootDirectoryByPath();
    if (sdk_root_by_path != sdk_root_by_env && !sdk_root_by_path.empty()) {
        auto adb_path = PathUtils::join(sdk_root_by_path, "platform-tools",
                                        PathUtils::toExecutableName("adb"));
        options.push_back(std::make_pair(sdk_root_by_path, adb_path));
        mAdbVersions[getAdbVersion(adb_path)] = adb_path;
    }

    // Well, maybe there's an adb executable on the path..
    ScopedCPtr<char> adb_exe(path_search_exec(kAdbExe));
    if (adb_exe && System::get()->pathCanExec(adb_exe.get())) {
        auto adb_path = std::string(adb_exe.get());
        options.push_back(std::make_pair("", adb_path));
        mAdbVersions[getAdbVersion(adb_path)] = adb_path;
    }

    // We now have several adb choices, we prefer the one that has the same
    // protocol version as the running one (if any). If we
    // don't pick the same executable as the running version it will be
    // terminated which can cause all sorts of havoc.
    if (protocol && mAdbVersions.find(protocol) != mAdbVersions.end()) {
        mAutoAdbPath = mAdbVersions[protocol];
        mAdbVersionCurrent =  protocol >= kMinAdbProtocol;
        mProtocol = protocol;
        return;
    }
    LOG(VERBOSE) << "No matching protocol: " << protocol <<  " found.";

    // Okay, we were unable to extract the protocol version from the running adb
    // or we have no matching executable.. Let's just use our old preference
    // ordering, we will use the first adb that has a proper version.
    for (auto option : options) {
        auto root = option.first;
        mAutoAdbPath = option.second;
        mProtocol = getAdbVersion(mAutoAdbPath);

        if (checkAdbVersion(root, mAutoAdbPath)) {
            mAdbVersionCurrent = true;
            LOG(VERBOSE) << "No matching protocol, path: " << mAutoAdbPath << " root: " << root;
            return;
        }
    }

    // Oh, oh. We either have no valid adb executable, or an
    // outdated/unverifiable one.
    LOG(VERBOSE) << "No matching protocol or verified version, using path: " << mAutoAdbPath;
    return;
}

const std::string& AdbInterfaceImpl::adbPath() {
    // User knows best..
    if (!mCustomAdbPath.empty()) {
        return mCustomAdbPath;
    }

    // Let's see if our detected protocol version matches
    // the running one..
    int daemon = getAdbDaemonVersion();
    if (mProtocol != daemon && mAdbVersions.find(daemon) != mAdbVersions.end()) {
        LOG(VERBOSE) << "Switching to protocol version: " << daemon;
        mAutoAdbPath = mAdbVersions[daemon];
        mProtocol = daemon;
    }

    return mAutoAdbPath;
}


AdbInterfaceImpl::AdbInterfaceImpl(Looper* looper)
    : mLooper(looper) {
    configureAdbPaths();
}

AdbCommandPtr AdbInterfaceImpl::runAdbCommand(
        const std::vector<std::string>& args,
        ResultCallback&& result_callback,
        System::Duration timeout_ms,
        bool want_output) {
    auto command = std::shared_ptr<AdbCommand>(new AdbCommand(
            mLooper, adbPath(),
            mSerialString, args, want_output, timeout_ms,
            std::move(result_callback)));
    command->start();
    return command;
}

AdbCommand::AdbCommand(Looper* looper,
                       const std::string& adb_path,
                       const std::string& serial_string,
                       const std::vector<std::string>& command,
                       bool want_output,
                       System::Duration timeoutMs,
                       ResultCallback&& callback)
    : mLooper(looper),
      mResultCallback(std::move(callback)),
      mOutputFilePath(PathUtils::join(
              System::get()->getTempDir(),
              std::string("adbcommand").append(Uuid::generate().toString()))),
      mWantOutput(want_output),
      mTimeoutMs(timeoutMs) {
    mCommand.push_back(adb_path);

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

void AdbCommand::start(int checkTimeoutMs) {
    if (!mTask && !mFinished) {
        auto shared = shared_from_this();
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
    }
}

void AdbCommand::taskDoneFunction(const OptionalAdbCommandResult& result) {
    if (!mCancelled) {
        mResultCallback(result);
    }
    AutoLock lock(mLock);
    mFinished = true;
    mCv.broadcastAndUnlock(&lock);
    // This may invalidate this object and clean it up.
    // DO NOT reference any internal state from this class after this
    // point.
    mTask.reset();
}

bool AdbCommand::wait(System::Duration timeoutMs) {
    if (!mTask) {
        return true;
    }

    AutoLock lock(mLock);
    if (timeoutMs < 0) {
        mCv.wait(&lock, [this]() { return mFinished; });
        return true;
    }

    const auto deadlineUs = System::get()->getUnixTimeUs() + timeoutMs * 1000;
    while (!mFinished && System::get()->getUnixTimeUs() < deadlineUs) {
        mCv.timedWait(&mLock, deadlineUs);
    }

    return mFinished;
}

void AdbCommand::taskFunction(OptionalAdbCommandResult* result) {
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
    }
}

AdbCommandResult::AdbCommandResult(System::ProcessExitCode exitCode,
                                   const std::string& outputName)
    : exit_code(exitCode),
      output(outputName.empty() ? nullptr
                                : new std::ifstream(outputName.c_str())),
      output_name(outputName) {}

AdbCommandResult::~AdbCommandResult() {
    output.reset();
    if (!output_name.empty()) {
        path_delete_file(output_name.c_str());
    }
}

AdbCommandResult::AdbCommandResult(AdbCommandResult&& other)
    : exit_code(other.exit_code),
      output(std::move(other.output)),
      output_name(std::move(other.output_name)) {
    other.output_name.clear();  // make sure |other| won't delete it
}

AdbCommandResult& AdbCommandResult::operator=(AdbCommandResult&& other) {
    exit_code = other.exit_code;
    output = std::move(other.output);
    output_name = std::move(other.output_name);
    other.output_name.clear();
    return *this;
}

}  // namespace emulation
}  // namespace android
