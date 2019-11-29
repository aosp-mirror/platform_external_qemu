// Copyright (C) 2019 The Android Open Source Project
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
#include <cstdint>
#include <memory>
#include <string>

#include "android/emulation/control/logcat/RingStreambuf.h"

namespace android {
namespace emulation {
namespace control {

// A connection to the adbd inside the emulator.
class AdbStream {
public:
    virtual ~AdbStream() {}

    // Send the given set of bytes to adbd.
    virtual void send(std::string msg) = 0;

    // Stream with buffered incoming messages.
    virtual RingStreambuf& stream() = 0;

    // True if this connection is alive.
    virtual bool isOpen() = 0;

    // Call this if you wish to close the stream, this
    // will unblock waiters on the streambuf as well.
    virtual void close() = 0;
};

class AdbConnection {
public:
    virtual ~AdbConnection() {}

    // Opens up an adb stream to the given service.
    // For example shell:<cmd>
    virtual std::shared_ptr<AdbStream> open(std::string service) = 0;

    // Attempts to connect, up authAttempts will be made, and
    // wait for at most timeoutMs to establish a connection.
    virtual bool connect(uint32_t authAttempts, uint32_t timeoutMs) = 0;

    // Create a new connection to adbd, this will open up a socket to
    // the emulator adb port.
    static AdbConnection* create();
};

}  // namespace control
}  // namespace emulation
}  // namespace android
