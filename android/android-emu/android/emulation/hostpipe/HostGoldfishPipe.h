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

#include <sys/types.h>

// Fake pipe driver and device for testing goldfish pipe on host without
// running a VM.
namespace android {

class HostGoldfishPipeDevice {
public:
    using ReadResult = android::base::Result<std::vector<uint8_t>, int>;
    using WriteResult = android::base::Result<ssize_t, int>;

    HostGoldfishPipeDevice() = default;
    ~HostGoldfishPipeDevice();

    static HostGoldfishPipeDevice* get();

    // Opens/closes pipe services.
    void* connect(const char* name);
    void close(void* pipe);

    // Gets the host side pipe object corresponding to hwpipe.
    // Not persistent across snapshot save/load.
    void* getHostPipe(void* hwpipe) const;

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

private:
    void initialize();
    // Opens a new pipe connection, returns a pointer to the host connector
    // pipe.
    //
    // hwpipe is an opaque pointer that will be used as the hwpipe id, generate
    // one with createNewHwPipeId().
    void* open(void* hwpipe);
    ssize_t writeInternal(void* pipe, const void* buffer, size_t len);
    // Returns the host-side pipe for a connector pipe, if the pipe has been
    // reset.
    void* popHostPipe(void* connectorPipe);
    void setErrno(ssize_t res);

    void* createNewHwPipeId();

    // Called to associate a given hwpipe with a host-side pipe.
    //
    // |hwpipe| - hwpipe pointer, the goldfish pipe pointer.
    // |internal_pipe| - Host-side pipe, corresponding to AndroidPipe*.
    void resetPipe(void* hwpipe, void* internal_pipe);

    // Close a hwpipe.
    void closeHwPipe(void* hwpipe);
    // Wake a hwpipe.
    void signalWake(void* hwpipe, int wakes);

    uintptr_t mNextHwPipe = 1;

    bool mInitialized = false;
    int mErrno = 0;
    void* mCurrentPipeWantingConnection = nullptr;
    std::unordered_map<void*, void*> mResettedPipes;
    std::unordered_map<void*, void*> mPipeToHwPipe;
    std::unordered_map<void*, void*> mHwPipeToPipe;
    std::unordered_map<void*, std::function<void(int)>> mHwPipeWakeCallbacks;
};

// Initializes a global pipe instance for the current process.
HostGoldfishPipeDevice* getHostPipeInstance();

} // namespace android

