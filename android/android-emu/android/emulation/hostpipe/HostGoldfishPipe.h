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

#include <cstdint>
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

    void resetPipe(void* pipe, void* internal_pipe);

    // Read/write for a particular pipe, along with C++ versions.
    ssize_t read(void* pipe, void* buffer, size_t len);
    ssize_t write(void* pipe, const void* buffer, size_t len);

    ReadResult read(void* pipe, size_t maxLength);
    WriteResult write(void* pipe, const std::vector<uint8_t>& data);

    unsigned poll(void* pipe) const;
    void signalWake(int wakes);
    int getErrno() const { return mErrno; }

private:
    void initialize();
    void* open();
    void* popHwSidePipe(void* pipe);
    void setErrno(ssize_t res);

    bool mInitialized = false;
    int mErrno = 0;
    void* mCurrentPipeWantingConnection = nullptr;
    std::unordered_set<void*> mPipes;
    std::unordered_map<void*, void*> mResettedPipes;
};

// Initializes a global pipe instance for the current process.
HostGoldfishPipeDevice* getHostPipeInstance();

} // namespace android

