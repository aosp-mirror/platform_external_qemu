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

#include <memory>

namespace android {

namespace emulation {

// AdbGuestAgent is an abstract interface to an object that models
// a guest ADB pipe connection. These are owned by an AdbHostAgent.
struct AdbGuestAgent {
    // Virtual destructor.
    virtual ~AdbGuestAgent() = default;

    // Called when the host ADB server connects. This passes ownership of
    // |socket| to the AdbGuestAgent instance. After this call, the caller will
    // never do anything with it. Return true on success, false on failure.
    // NOTE: On failure, the method should close the socket.
    virtual bool onHostConnection(int socket) = 0;
};

// AdbHostAgent is an abstract class to the code that actually
// manages the host ADB server port and multiple ADB pipes.
struct AdbHostAgent {
    // Call this method to initialize the agent to listen on TCP localhost
    // port |adbPort| (both IPv4 and IPv6). Return true on success, false/errno
    // on error.
    virtual bool init(int adbPort) = 0;

    // Call this to de-initialize the agent. This force-closes any guest
    // pipe connection, and unbinds the previous ADB TCP port, if bound.
    virtual void deinit() = 0;

    // Called when the guest creates a new AdbGuestAgent instance.
    // Should only be called after init() and before deinit().
    // The agent should call onHostConnection() on the instance
    // once the ADB Host server connects to the server socket.
    // NOTE: This transfers ownership of |pipe| to the agent.
    virtual void addGuest(AdbGuestAgent* pipe) = 0;

    // Called when the guest has closed an existing AdbGuestAgent
    // instance. The implementation should delete |pipe| before
    // returning.
    virtual void deleteGuest(AdbGuestAgent* pipe) = 0;
};

}  // namespace emulation
}  // namespace android
