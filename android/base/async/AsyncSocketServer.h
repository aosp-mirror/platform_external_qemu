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

#pragma once

#include <functional>

namespace android {
namespace base {

class Looper;

class AsyncSocketServer {
public:
    // Type of a function that is called when a new client connection
    // occurs. The parameter is a socket descriptor. On success, return true
    // and pass ownership of the socket to the callback. On failure, return
    // false (and the caller will close the socket if needed).
    using ConnectCallback = std::function<bool(int)>;

    // Destructor.
    virtual ~AsyncSocketServer() = default;

    // Return the port that this server is bound to. This is useful if
    // the |port| parameter of functions like createTcpLoopbackServer()
    // was 0, which meant the system found a random free port which can
    // be reported by this function.
    virtual int port() const = 0;

    // Start listening for new connections.
    // Note that by default, a new instance doesn't listen for connections.
    // Moreover, the server will stop listening before invoking the callback
    // on a new connection. User code must call startListening() explicitly
    // to allow more connections to proceed.
    virtual void startListening() = 0;

    // Stop listening for new connections.
    virtual void stopListening() = 0;

    // Values used to indicate which loopback interfaces to listen too.
    enum LoopbackMode {
        kIPv4 = (1 << 0),
        kIPv6 = (1 << 1),
        kIPv4AndIPv6 = kIPv4 | kIPv6,
    };

    // Create a new instance that binds to a TCP loopback |port| and will
    // call |connectCallback| when a new host connection occurs. |mode|
    // determines on which interfaces to listen to, and |looper| can be
    // used to specify a Looper instance (otherwise the thread's current
    // looper will be used).
    static AsyncSocketServer* createTcpLoopbackServer(
            int port,
            ConnectCallback connectCallback,
            LoopbackMode mode = kIPv4AndIPv6,
            Looper* looper = nullptr);

protected:
    // No default constructor, use one of the static createXXX() method to
    // create a new instance.
    AsyncSocketServer() = default;
};

}  // namespace base
}  // namespace android
