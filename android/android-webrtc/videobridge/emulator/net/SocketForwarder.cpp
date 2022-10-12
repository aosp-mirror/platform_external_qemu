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
#include "emulator/net/SocketForwarder.h"

#include "aemu/base/Log.h"

#include <rtc_base/copy_on_write_buffer.h>  // for CopyOnWriteBuffer
#include <rtc_base/logging.h>               // for RTC_LOG
#include <type_traits>                      // for remove_extent_t

namespace emulator {
namespace net {
SocketForwarder::SocketForwarder(
        std::shared_ptr<AsyncSocket> socket,
        rtc::scoped_refptr<DataChannelInterface> channel)
    : mDataChannel(channel), mSocket(socket) {
    mSocket->setSocketEventListener(this);
    mSocket->wantRead();
    channel->RegisterObserver(this);
}

void SocketForwarder::sendMessage(const rtc::CopyOnWriteBuffer dataBuffer) {
    if (dataBuffer.size() == 0) {
        return;
    }

    if (!mDataChannel->Send(::webrtc::DataBuffer(dataBuffer, true))) {
        LOG(ERROR)
                << "Failed to forward datapacket.. Closing down connection!";
        mSocket->close();
    }
}

// Called when bytes can be read from the socket.
void SocketForwarder::onRead(android::base::AsyncSocketAdapter* socket) {
    int rd = 0;
    char buffer[kMaxMessageSize];
    rtc::CopyOnWriteBuffer dataBuffer;

    // Read incoming data while available.
    do {
        rd = socket->recv(buffer, kMaxMessageSize);
        if (rd > 0) {
            dataBuffer.AppendData(buffer, rd);
        }

        // Chunk it up if needed..
        if (dataBuffer.size() >= kMaxMessageSize) {
            sendMessage(dataBuffer);
            dataBuffer.SetSize(0);
        }
    } while (rd > 0);

    // And forward the leftovers...
    sendMessage(dataBuffer);
};

// Called when this socket is closed.
void SocketForwarder::onClose(android::base::AsyncSocketAdapter* socket, int err) {
    mDataChannel->Close();
};

// Called when this socket (re) established a connection.
void SocketForwarder::onConnected(android::base::AsyncSocketAdapter* socket) {
    // Nop
    LOG(INFO) << "Connection to socket established.";
};

void SocketForwarder::OnStateChange() {
    switch (mDataChannel->state()) {
        case DataChannelInterface::DataState::kClosed:
        case DataChannelInterface::DataState::kClosing:
            // Data channel disappeared, close down the socket side.
            mSocket->close();
            break;
        case DataChannelInterface::DataState::kOpen:
        case DataChannelInterface::DataState::kConnecting:
            // Usually a nop..
            mSocket->connect();
            break;
    }
}
void SocketForwarder::OnMessage(const ::webrtc::DataBuffer& buffer) {
    const char* data = reinterpret_cast<const char*>(buffer.data.data());
    mSocket->send(data, buffer.data.size());
}
}  // namespace net
}  // namespace emulator
