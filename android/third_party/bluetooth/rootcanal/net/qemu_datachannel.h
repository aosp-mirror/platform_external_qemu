// Copyright (C) 2021 The Android Open Source Project
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

#include <stdint.h>     // for uint64_t
#include <sys/types.h>  // for ssize_t

#include "android/base/async/Looper.h"                      // for Looper
#include "android/emulation/control/rootcanal_hci_agent.h"  // for QAndroidR...
#include "net/async_data_channel.h"                         // for ReadCallback

namespace android {
namespace net {

using android::base::Looper;

// An implementation of the AsyncDataChannel interface that talks
// to QAndroidHciAgent to send and receive data.
//
// This channel can deliver spurious receive events.
class QemuDataChannel : public AsyncDataChannel {
public:
    QemuDataChannel(QAndroidHciAgent const* agent, Looper* looper);
    QemuDataChannel(const QemuDataChannel& other) = delete;

    // Make sure to close the socket before hand.
    virtual ~QemuDataChannel() = default;

    // Receive data in the given buffer. Returns the number of bytes read,
    // or a negative number in case of failure. Check the errno variable to
    // learn why the call failed.
    ssize_t Recv(uint8_t* buffer, uint64_t bufferSize) override;

    // Send data in the given buffer. Returns the number of bytes read,
    // or a negative number in case of failure. Check the errno variable to
    // learn why the call failed. Note: This can be EAGAIN if we cannot
    // write to the socket at this time as it would block.
    ssize_t Send(const uint8_t* buffer, uint64_t bufferSize) override;

    // This is always connected.
    bool Connected() override { return true; };

    // This cannot be closed.
    void Close() override{};

    // Registers the given callback to be invoked when a recv call can be made
    // to read data from this socket.
    // Only one callback can be registered per socket.
    bool WatchForNonBlockingRead(
            const ReadCallback& on_read_ready_callback) override;

    void StopWatching() override;

private:
    void ReadReady();
    ReadCallback mCallback;
    QAndroidHciAgent const* mAgent;
    Looper* mLooper;
};
}  // namespace net
}  // namespace android
