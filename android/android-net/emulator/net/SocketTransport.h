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

#include <memory>
#include <string>

#include "aemu/base/async/AsyncSocketAdapter.h"

namespace emulator {
namespace net {

using ::android::base::AsyncSocketAdapter;
using ::android::base::AsyncSocketEventListener;

enum class State {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
};

// std::ostream& operator<<(std::ostream& os, const State state);

class SocketTransport;

template <class T>
class ProtocolReader {
public:
    virtual void received(SocketTransport* from, T object) = 0;
    virtual void stateConnectionChange(SocketTransport* connection,
                                       State current) = 0;
};

template <class T>
class ProtocolWriter {
public:
    virtual bool write(SocketTransport* to, T object) = 0;
};

using MessageReceiver = ProtocolReader<std::string>;

// A SocketTransport can open a socket to an endpoint.
// The socket is asynchronous and will invoke the callbacks on the receiver when
// needed.
class SocketTransport : public AsyncSocketEventListener {
public:
    explicit SocketTransport(MessageReceiver* receiver,
                             AsyncSocketAdapter* connection);
    ~SocketTransport();
    SocketTransport(const SocketTransport&) = delete;
    SocketTransport(SocketTransport&&) = delete;

    void close();
    bool send(const std::string msg);
    bool connect();
    void registerCallback(const MessageReceiver* receiver);
    State state() { return mState; }

private:
    static const uint16_t RECV_BUFFER_SIZE = 1024;
    void registerCallbacks();
    void setState(State newState);
    void onRead(AsyncSocketAdapter* socket) override;
    void onClose(AsyncSocketAdapter* socket, int err) override;
    void onConnected(AsyncSocketAdapter* socket) override;

    std::unique_ptr<AsyncSocketAdapter> mSocket;
    MessageReceiver* mReceiver = nullptr;
    State mState = State::DISCONNECTED;
};
}  // namespace net
}  // namespace emulator
