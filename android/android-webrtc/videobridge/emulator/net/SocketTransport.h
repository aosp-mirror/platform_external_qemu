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

#include <rtc_base/nethelpers.h>
#include <rtc_base/socketaddress.h>
#include <string>
using rtc::AsyncSocket;

namespace emulator {
namespace net {

enum class State {
    NOT_CONNECTED,
    SIGNING_IN,
    RESOLVING,
    CONNECTED,
};

std::ostream& operator<<(std::ostream& os, const State state);
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
class SocketTransport : public sigslot::has_slots<> {
public:
    explicit SocketTransport(MessageReceiver* receiver);
    explicit SocketTransport(MessageReceiver* receiver,
                             AsyncSocket* connection);
    ~SocketTransport() override;
    SocketTransport(const SocketTransport&) = delete;
    SocketTransport(SocketTransport&&) = delete;

    void connect(const std::string server, int port);
    void close();
    bool send(const std::string msg);
    void registerCallback(const MessageReceiver* receiver);
    State state() { return mState; }

private:
    void registerCallbacks();
    void doConnect();
    void OnRead(rtc::AsyncSocket* socket);
    void OnClose(rtc::AsyncSocket* socket, int err);
    void OnResolveResult(rtc::AsyncResolverInterface* resolver);
    void OnConnect(rtc::AsyncSocket* socket);
    void setState(State newState);

    rtc::SocketAddress mAddress;
    std::unique_ptr<rtc::AsyncResolver> mResolver;
    std::unique_ptr<rtc::AsyncSocket> mSocket;

    MessageReceiver* mReceiver;
    State mState = State::NOT_CONNECTED;
};
}  // namespace net
}  // namespace emulator
