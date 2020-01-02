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
#include <iostream>
#include <memory>
#include <string>

#include "android/emulation/control/AdbInterface.h"

namespace android {
namespace emulation {

// A connection to a service endpoint, this is a general
// std::iostream, you can read bytes that came from the service
// and write bytes to the service.
//
// The streams are blocking and will execute the proper
// adb protocol to interact with the service.
//
// For example:
//
// auto stream = AdbConnection::connection()->open("shell:getprop")
// while (getline (stream, line)) {
//     cout << line << '\n';
// }
//
// Note! These streams use a specialized rdbuf, do not replace it
// expecting success.
class AdbStream : public std::iostream {
public:
    virtual ~AdbStream() {}

    // Call this if you wish to close the stream, this
    // will unblock waiters on the streambuf as well.
    virtual void close() = 0;

protected:
    AdbStream(std::streambuf* buf) : std::iostream(buf) {}
};

// State of the connection to the adb deamon inside the emulator.
enum class AdbState {
    disconnected,  // Socket has not yet been opened.
    connecting,    // Haven't received a response from the device yet.
    authorizing,   // Authorizing with keys from ADB_VENDOR_KEYS.
    connected,     // We are in a valid connected state, you can call open
    failed,        // We've given up and adb working..
};

// A connection to the adb deamon running inside the emulator.
// The adb connection will connect to the local adb port that the
// emulator advertises.
class AdbConnection {
public:
    virtual ~AdbConnection() {}

    // Opens up an adb stream to the given service.
    // For example shell:<cmd>, or reboot:
    // note that push/pull is not yet supported.
    virtual std::shared_ptr<AdbStream> open(const std::string& service) = 0;

    // Returns the state..
    virtual AdbState state() const = 0;

    // Close down the connection, closing all open streams.
    virtual void close() = 0;

    // True if the feature is supported
    virtual bool hasFeature(const std::string& feature) const = 0;

    // connect a new connection to adbd, this will open up a socket to
    // the emulator adb port. You do not own this..
    static AdbConnection* connection();

    // Inject the adbport into the connection. This should be called
    // before anyone obtains a reference to an AdbConnection
    static void setAdbPort(int port);
};

// Adb uses the shell protocol when making use of the shell service v2.
// For example:
// adbconnection->open("shell,v2:ls -l")
//
// The ShellProtocol is very simple, it consists of a ShellHeader (as defined
// below) followed by a sequence of bytes defined in the header.
struct ShellHeader {
    // This is an unscoped enum to make it easier to compare against raw bytes.
    enum Id : uint8_t {
        kIdStdin = 0,
        kIdStdout = 1,
        kIdStderr = 2,
        kIdExit = 3,  // data[0] will have the exit code.

        // Close subprocess stdin if possible.
        kIdCloseStdin = 4,

        // Window size change (an ASCII version of struct winsize).
        kIdWindowSizeChange = 5,

        // Indicates an invalid or unknown packet.
        kIdInvalid = 255,
    };

    Id id;
    uint32_t length;
} __attribute__((packed));

}  // namespace emulation
}  // namespace android
