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
#include <cstdint>  // for uint32_t, uint64_t
#include <istream>  // for iostream, streambuf
#include <limits>   // for numeric_limits
#include <memory>   // for shared_ptr
#include <string>   // for string

namespace emulator {
namespace net {
class AsyncSocketAdapter;
}  // namespace net
}  // namespace emulator

namespace android {
namespace emulation {

using emulator::net::AsyncSocketAdapter;
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

    // Set the write timeout. This is the maximum
    // time we are willing to wait for ADBD to respond with
    // a reply to a WRITE request. You should only need this
    // for unit tests, as we expect ADBD to behave properly.
    virtual void setWriteTimeout(
            uint64_t timeoutMs = std::numeric_limits<uint64_t>::max()) = 0;

protected:
    AdbStream(std::streambuf* buf) : std::iostream(buf) {}
};

// State of the connection to the adb deamon inside the emulator.
enum class AdbState {
    disconnected,  // Socket has not yet been opened.
    socket,      // Socket is open, but we have not yet send any messages to the
                 // emulator
    connecting,  // Haven't received a response from the device yet.
    authorizing,  // Authorizing with keys from ADB_VENDOR_KEYS.
    offer_key,    // A public key has been offered to the consumer.
    connected,    // We are in a valid connected state, you can call open
    failed,       // We've given up and adb working..
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
    // The stream will be in a bad state (i.e. ->bad() == true) if
    // If a connection did not take place within the given timeout.
    // This will re-establish a connection to adbd if it was broken for
    // any reason, which can happen if you connect to ADBD very early on
    // in the boot process.
    //
    // Note that setting aggressive timeouts can result in returning
    // before the handshake has completed. Which can vary greatly depending
    // where you are in the image boot process.
    virtual std::shared_ptr<AdbStream> open(const std::string& service,
                                            uint32_t timeoutMs = 2000) = 0;

    // Returns the state..
    virtual AdbState state() const = 0;

    // Close down the connection, closing all open streams.
    virtual void close() = 0;

    // True if the feature is supported, this will return false
    // if the state() != connected.
    virtual bool hasFeature(const std::string& feature) const = 0;

    // Get the connection to the Adb service, executing the handshake
    // if needed. A timeout of 0 will not try to (re) establish a connection.
    //
    // Note: establishing the connection will continue if the timeout is
    // non-zero. A larger timeout usually reduces the chance of state()
    // not returning ::connected, which is needed for querying features.
    static std::shared_ptr<AdbConnection> connection(int timeoutMs = 1000);

    // Inject the adb socket into the connection. This should be called
    // before anyone obtains a reference to an AdbConnection.
    // The AdbConnection will take ownership of the socket.
    // Note that this is not thread safe!
    static void setAdbSocket(AsyncSocketAdapter* socket);

    // This will create and set the adb socket to the given port
    // initializing it with its own socket. The thread looper associated
    // with this thread will be used.
    static void setAdbPort(int port);

    // We cannot offer auth keys from different connections. b/150160590
    static bool failed();
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
