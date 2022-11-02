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
#include <api/data_channel_interface.h>       // for DataChannelInterface
#include <api/scoped_refptr.h>                // for scoped_refptr
#include <rtc_base/copy_on_write_buffer.h>    // for CopyOnWriteBuffer
#include <stddef.h>                           // for size_t
#include <memory>                             // for shared_ptr

#include "aemu/base/async/AsyncSocket.h"  // for AsyncSocketAdapter (ptr...


using android::base::AsyncSocket;
using ::webrtc::DataChannelObserver;
using ::webrtc::DataChannelInterface;


namespace emulator {
namespace net {

// A socket forwarder connects a datachannel to a socket and forwards all in and outgoing messages.
// Usually it is used to establish pipes like outlined below:
//
// socket <---> secure webrtc datachannel <--> socket
class SocketForwarder : public android::base::AsyncSocketEventListener,
                         public DataChannelObserver {
public:
    SocketForwarder(
            std::shared_ptr<AsyncSocket> socket,
            rtc::scoped_refptr<DataChannelInterface> channel);

    // Called when bytes can be read from the socket.
    void onRead(android::base::AsyncSocketAdapter* socket) override;

    // Called when this socket is closed.
    void onClose(android::base::AsyncSocketAdapter* socket, int err) override;

    // Called when this socket (re) established a connection.
    void onConnected(android::base::AsyncSocketAdapter* socket) override;

    void OnStateChange() override;

    // Called when a message was received on the datachannel.
    void OnMessage(const ::webrtc::DataBuffer& buffer) override;

private:
    void sendMessage(const rtc::CopyOnWriteBuffer dataBuffer);
    std::shared_ptr<AsyncSocket> mSocket;
    rtc::scoped_refptr<DataChannelInterface> mDataChannel;

    // Datachannel messages will be < 2 * kMessageSize bytes.
    const size_t kMaxMessageSize = 4096;
};
}  // namespace net
}  // namespace emulator
