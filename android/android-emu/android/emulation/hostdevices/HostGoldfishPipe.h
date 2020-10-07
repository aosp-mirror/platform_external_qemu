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
    using ReadResult = android::base::Result<std::vector<uint8_t>, int>;
    using WriteResult = android::base::Result<ssize_t, int>;

    HostGoldfishPipeDevice();
    ~HostGoldfishPipeDevice();

    static HostGoldfishPipeDevice* get();

    // Opens/closes pipe services.
    void* connect(const char* name);
    void close(void* hwPipe);
    void close(uint32_t hwPipeId);

    // Closes all pipes and resets to initial state.
    void clear();

    // Gets the host side pipe object corresponding to hwpipe.
    // Not persistent across snapshot save/load.
    void* getHostPipe(const void* hwpipe) const;

    // Loads a single pipe from a stream, returns null if the pipe can't be loaded.
    void* load(base::Stream* stream);

    // Saves/loads the entire set of pipes including pre/post save/load.
    void saveSnapshot(base::Stream* stream);
    void loadSnapshot(base::Stream* stream);

    // Save/load the device, but only for a particular hwpipe.
    void saveSnapshot(base::Stream* stream, void* hwpipe);
    void* loadSnapshotSinglePipe(base::Stream* stream);

    // Read/write for a particular pipe, along with C++ versions.
    ssize_t read(void* pipe, void* buffer, size_t len);
    ssize_t write(void* pipe, const void* buffer, size_t len);

    ReadResult read(void* pipe, size_t maxLength);
    WriteResult write(void* pipe, const std::vector<uint8_t>& data);

    unsigned poll(void* pipe) const;
    int getErrno() const;

    // Sets a callback that will be invoked when the pipe is signaled by the
    // host, accepts an "int wakes" parameter that matches contains PIPE_WAKE_*
    // flags.
    void setWakeCallback(void* pipe, std::function<void(int)> callback);

    // Callbacks for AndroidPipeHwFuncs.
    static void resetPipeCallback(void* hwpipe, void* internal_pipe);
    static void closeFromHostCallback(void* hwpipe);
    static void signalWakeCallback(void* hwpipe, unsigned wakes);
    static int getPipeIdCallback(void* hwpipe);

private:
    void initialize();

    struct InternalPipe {};

    struct HostHwPipe {
        static constexpr uint32_t kMagic = 0x70697065;

        explicit HostHwPipe(uint32_t i);
        ~HostHwPipe();

        uint32_t getId() const;

        static HostHwPipe* from(void* ptr);
        static const HostHwPipe* from(const void* ptr);

        static std::unique_ptr<HostHwPipe> create();
        static std::unique_ptr<HostHwPipe> create(uint32_t id);

    private:
        uint32_t magic;
        uint32_t id;
    };

    HostHwPipe* associatePipes(std::unique_ptr<HostHwPipe> hwPipe,
                               InternalPipe* hostPipe);
    bool eraseHwPipe(uint32_t id);

    ssize_t writeInternal(void* pipe, const void* buffer, size_t len);

    // Returns the host-side pipe for a connector pipe, if the pipe has been
    // reset.
    InternalPipe* popHostPipe(InternalPipe* connectorPipe);
    void setErrno(ssize_t res);

    // Called to associate a given hwpipe with a host-side pipe.
    //
    // |hwpipe| - hwpipe pointer, the goldfish pipe pointer.
    // |internal_pipe| - Host-side pipe, corresponding to AndroidPipe*.
    void resetPipe(void* hwpipe, void* internal_pipe);

    // Close a hwpipe.
    void closeHwPipe(HostHwPipe* hwpipe);
    // Wake a hwpipe.
    void signalWake(const HostHwPipe* hwpipe, int wakes);

    bool mInitialized = false;
    int mErrno = 0;
    InternalPipe* mCurrentPipeWantingConnection = nullptr;

    struct HostHwPipeInfo {
        std::unique_ptr<HostHwPipe> hwPipe;
        InternalPipe* hostPipe = nullptr;
        std::function<void(int)> wakeCallback = [](int){};
    };

    std::unordered_map<InternalPipe*, InternalPipe*> mResettedPipes;
    std::unordered_map<InternalPipe*, HostHwPipe*> mPipeToHwPipe;
    std::unordered_map<uint32_t, HostHwPipeInfo> mHwPipeInfo;
};

// Initializes a global pipe instance for the current process.
HostGoldfishPipeDevice* getHostPipeInstance();

} // namespace android

