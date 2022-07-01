// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/AdbHostServer.h"

#include "android/base/Log.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/StringFormat.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"

#include <string>
#include <stdlib.h>

namespace android {
namespace emulation {

using android::base::ScopedSocket;
using android::base::Optional;
using android::base::StringFormat;
using android::base::System;

static ScopedSocket connectToAdbServer(int adbClientPort) {
    ScopedSocket socket(android::base::socketTcp4LoopbackClient(adbClientPort));
    if (!socket.valid()) {
        socket.reset(android::base::socketTcp6LoopbackClient(adbClientPort));
    }
    return socket;
}

// Sends a message to the ADB server on the socket file descriptor.
// The message will be formatted according to the ADB protocol
static bool sendMessage(int fd, const std::string& message) {
    // A client sends a request using the following format:
    //
    // 1. A 4-byte hexadecimal string giving the length of the payload
    // 2. Followed by the payload itself.
    auto wireformat =
            StringFormat("%04x%s", (uint32_t)message.size(), message.c_str());
    return android::base::socketSendAll(fd, wireformat.c_str(),
                                        wireformat.size());
}

// Reads a response from the ADB server on the socket file descriptor.
// At most bytesToRead will be read, the return string will be resized
// to the number of bytes that have been read, and errno will be set.
static std::string readResponse(int fd, int bytesToRead) {
    std::string buffer(bytesToRead, '\0');
    unsigned int bytesRead = 0;
    while (bytesToRead > 0) {
        auto ret = android::base::socketRecv(fd, (void*)&buffer[bytesRead], bytesToRead);

        if (ret == EAGAIN || ret == EWOULDBLOCK) {
            System::get()->sleepMs(500);
            continue;
        }

        if (ret <= 0) {
            break;
        }

        bytesToRead -= ret;
        bytesRead += ret;
    }
    buffer.resize(bytesRead);
    return buffer;
}

// Reads a protocol string from ADB
static std::string readProtocolString(int fd) {
    uint32_t bytesToRead;
    std::string incoming = readResponse(fd, 4);
    if (incoming.size() != 4 || sscanf(incoming.c_str(), "%04x", &bytesToRead) != 1) {
        LOG(ERROR) << "adb protocol fault (couldn't read status length)";
        return {};
    }

    incoming = readResponse(fd, bytesToRead);
    if (incoming.size() != bytesToRead) {
        LOG(ERROR) << "adb protocol fault (couldn't read status message)";
        return {};
    }

    return incoming;
}

Optional<int> AdbHostServer::getProtocolVersion() {
    int port = AdbHostServer::getClientPort();

    // Okay. let's see if we can connect to this adb server.
    ScopedSocket fd = connectToAdbServer(port);
    if (!fd.valid()) {
        LOG(ERROR) << "Unable to connect to adb daemon on port: " << port;
        return {};
    }

    // And ask it for its protocol version.
    if (!sendMessage(fd.get(), "host:version")) {
        return {};
    }

    android::base::socketSetNonBlocking(fd.get());

    // The server should answer a request with one of the following:
    //
    // 1. For success, the 4-byte "OKAY" string
    // 2. For failure, the 4-byte "FAIL" string, followed by a
    //    4-byte hex length, followed by a string giving the reason for failure.
    if (readResponse(fd.get(), 4).compare("OKAY") == 0) {
        // for 'host:version', a 4-byte hex string corresponding to the server's
        // internal version number
        std::string version_result = readProtocolString(fd.get());
        uint32_t version = 0;
        if (sscanf(version_result.c_str(), "%04x", &version) == 1) {
            android::base::socketSetBlocking(fd.get());
            return version;
        }
    }

    android::base::socketSetBlocking(fd.get());

    return {};
}

bool AdbHostServer::notify(int adbEmulatorPort, int adbClientPort) {
    // First connect to ADB server.
    ScopedSocket socket = connectToAdbServer(adbClientPort);

    if (!socket.valid()) {
        // This can happen frequently when there is no ADB Server running
        // on the host, so don't complain with a warning or error message.
        return false;
    }
    // Send the special message
    auto message = StringFormat("host:emulator:%d", adbEmulatorPort);
    return sendMessage(socket.get(), message);
}

int AdbHostServer::getClientPort() {
    int clientPort = kDefaultAdbClientPort;
    const android::base::StringView kVarName = "ANDROID_ADB_SERVER_PORT";
    std::string env = System::get()->envGet(kVarName);
    if (!env.empty()) {
        long port = strtol(env.c_str(), NULL, 0);
        if (port <= 0 || port >= 65536) {
            LOG(ERROR) << "Env. var " << kVarName << " must be a number "
                       << "in 1..65535 range. Got " << port;
            return -1;
        }
        clientPort = static_cast<int>(port);
    }
    return clientPort;
}

}  // namespace emulation
}  // namespace android
