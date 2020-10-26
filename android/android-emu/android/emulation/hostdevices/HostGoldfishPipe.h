// Copyright (C) 2018 The Android Open Source Project
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

#include "android/base/Result.h"
#include "android/base/files/Stream.h"
#include "android/emulation/android_pipe_base.h"

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

#include <sys/types.h>

// Fake pipe driver and device for testing goldfish pipe on host without
// running a VM.
namespace android {

class HostGoldfishPipeDevice {
public:
    static constexpr int kNoFd = -1;

    using ReadResult = android::base::Result<std::vector<uint8_t>, int>;
    using WriteResult = android::base::Result<ssize_t, int>;

    HostGoldfishPipeDevice();
    ~HostGoldfishPipeDevice();

    int getErrno() const;

    // Opens/closes pipe services.
    int connect(const char* name);
    void close(int fd);

    // Read/write for a particular pipe, along with C++ versions.
    ssize_t read(int fd, void* buffer, size_t len);
    ssize_t write(int fd, const void* buffer, size_t len);
    ReadResult read(int fd, size_t maxLength);
    WriteResult write(int fd, const std::vector<uint8_t>& data);
    unsigned poll(int fd) const;

    // Sets a callback that will be invoked when the pipe is signaled by the
    // host, accepts an "int wakes" parameter that matches contains PIPE_WAKE_*
    // flags.
    void setWakeCallback(int fd, std::function<void(int)> callback);

    // Gets the host side pipe object corresponding to hwpipe.
    // Not persistent across snapshot save/load.
    void* getHostPipe(int fd) const;

    // Saves/loads the entire set of pipes including pre/post save/load.
    void saveSnapshot(base::Stream* stream);
    void loadSnapshot(base::Stream* stream);

    // Save/load the device, but only for a particular hwpipe.
    void saveSnapshot(base::Stream* stream, int fd);
    int loadSnapshotSinglePipe(base::Stream* stream);

    // Closes all pipes and resets to initial state.
    void clear();

    static HostGoldfishPipeDevice* get();

private:
    void initialize();

    struct InternalPipe {};

    struct HostHwPipe {
        explicit HostHwPipe(int fd);
        ~HostHwPipe();

        int getFd() const { return mFd; }

        static std::unique_ptr<HostHwPipe> create(int fd);

    private:
        static const AndroidPipeHwFuncs vtbl;

        const AndroidPipeHwFuncs* const vtblPtr;
        int mFd;
    };

    friend HostHwPipe;

    struct FdInfo {
        std::unique_ptr<HostHwPipe> hwPipe;
        InternalPipe* hostPipe = nullptr;
        std::function<void(int)> wakeCallback = [](int){};
    };

    void clearLocked();

    FdInfo* lookupFdInfo(int fd);
    const FdInfo* lookupFdInfo(int fd) const;

    HostHwPipe* associatePipes(std::unique_ptr<HostHwPipe> hwPipe,
                               InternalPipe* hostPipe);
    bool eraseFdInfo(int fd);

    ssize_t writeInternal(InternalPipe** pipe, const void* buffer, size_t len);

    void setErrno(ssize_t res);

    // Wake a hwpipe.
    void signalWake(int fd, int wakes);

    // Callbacks for AndroidPipeHwFuncs.
    static void closeFromHostCallback(void* hwpipe);
    static void signalWakeCallback(void* hwpipe, unsigned wakes);
    static int getPipeIdCallback(void* hwpipe);

    bool mInitialized = false;
    int mFdGenerator = 0;
    int mErrno = 0;

    std::unordered_map<InternalPipe*, HostHwPipe*> mPipeToHwPipe;
    std::unordered_map<int, FdInfo> mFdInfo;
};

// Initializes a global pipe instance for the current process.
HostGoldfishPipeDevice* getHostPipeInstance();

} // namespace android

