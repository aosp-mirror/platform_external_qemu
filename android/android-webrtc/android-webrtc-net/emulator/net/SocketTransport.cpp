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
#include "SocketTransport.h"

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

SocketTransport::SocketTransport(MessageReceiver* receiver)
    : mReceiver(receiver) {}
SocketTransport::SocketTransport(MessageReceiver* receiver,
                                 AsyncSocketAdapter* connection)
    : mReceiver(receiver), mSocket(connection) {
    registerCallbacks();
}

SocketTransport::~SocketTransport() {}

void SocketTransport::OnClose(AsyncSocketAdapter* socket, int err) {
    socket->Close();
    setState(State::NOT_CONNECTED);
}

void SocketTransport::setState(State newState) {
    mState = newState;
    if (mReceiver) {
        mReceiver->stateConnectionChange(this, mState);
    }
}
void SocketTransport::registerCallbacks() {
    mSocket->AddSocketEventListener(this);
}

void SocketTransport::close() {
    mSocket->Close();
    mSocket->RemoveSocketEventListener(this);
    setState(State::NOT_CONNECTED);
}

bool SocketTransport::send(const std::string msg) {
    return mSocket->Send(msg.c_str(), msg.size()) > 0;
}

void SocketTransport::OnRead(AsyncSocketAdapter* socket) {
    char buffer[0xffff];
    do {
        int bytes = socket->Recv(buffer, sizeof(buffer));
        if (bytes <= 0) {
            break;
        }
        mReceiver->received(this, std::string(buffer, bytes));
    } while (true);
}
}  // namespace net
}  // namespace emulator
