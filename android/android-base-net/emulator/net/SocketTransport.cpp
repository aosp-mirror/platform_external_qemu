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
#include "emulator/net/SocketTransport.h"

#if DEBUG >= 2
#define DD(fmt, ...)                                                   \
        printf("SocketTransport: %s:%d " fmt "\n", __func__, __LINE__, \
               ##__VA_ARGS__);
#else
#define DD(fmt, ...) (void)0
#endif

namespace emulator {
namespace net {
/*
std::ostream& operator<<(std::ostream& os, const State state) {
switch (state) {
    case State::NOT_CONNECTED:
        os << "NOT_CONNECTED";
        break;
    case State::CONNECTED:
        os << "CONNECTED";
        break;
}
return os;
}
*/

SocketTransport::SocketTransport(MessageReceiver* receiver,
                                 AsyncSocketAdapter* connection,
                                 TransportMode mode)
    : mReceiver(receiver), mSocket(connection), mMode(mode) {
    if (mSocket->connected()) {
        mState = State::CONNECTED;
    }
    registerCallbacks();
}

SocketTransport::~SocketTransport() {
    DD("~SocketTransport()");
    mSocket->removeSocketEventListener(this);
}

void SocketTransport::onClose(AsyncSocketAdapter* socket, int err) {
    mSocket->removeSocketEventListener(this);
    socket->close();
    setState(State::DISCONNECTED);
}

void SocketTransport::onConnected(AsyncSocketAdapter* socket) {
    setState(State::CONNECTED);
}

void SocketTransport::setState(State newState) {
    mState = newState;
    if (mReceiver) {
        mReceiver->stateConnectionChange(this, mState);
    }
}
void SocketTransport::registerCallbacks() {
    mSocket->addSocketEventListener(this);
}

bool SocketTransport::connect() {
    if (mState == State::CONNECTED) {
        return true;
    }
    setState(State::CONNECTING);
    return mSocket->connect();
}

void SocketTransport::close() {
    mSocket->close();
    setState(State::DISCONNECTED);
}

bool SocketTransport::send(const std::string msg) {
    return mState == State::CONNECTED && (mMode & TransportMode::Write) &&
           (mSocket->send(msg.c_str(), msg.size()) > 0);
}

void SocketTransport::onRead(AsyncSocketAdapter* socket) {
    if ((mMode & TransportMode::Read) == 0) {
        DD("Not configured for reading.");
        return;
    }
    char buffer[RECV_BUFFER_SIZE];
    do {
        int bytes = socket->recv(buffer, sizeof(buffer));

        if (bytes <= 0) {
            break;
        }

        if (mReceiver) {
            mReceiver->received(this, std::string(buffer, bytes));
        } else {
            DD("%s: Dropping bytes: %d, [%s]\n", __func__, bytes,
               std::string(buffer, bytes).c_str());
        }
    } while (true);
}
}  // namespace net
}  // namespace emulator
