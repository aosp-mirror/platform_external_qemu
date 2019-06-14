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
#pragma once
#include <string>

#include "emulator/net/SocketTransport.h"

namespace emulator {
namespace net {

using WaterfallReceiver = ProtocolReader<std::string>;
using WaterfallWriter = ProtocolWriter<std::string>;

// A simple wire protocol that sends and receives string messages
// that are prefixed by number of bytes as uint32_t little endian.
//
// When a new message has been parsed it will be passed on
// to the receiver. Socket state changes will be forwarded.
class WaterfallProtocol : public MessageReceiver, public WaterfallWriter {
public:
    WaterfallProtocol(WaterfallReceiver* receiver) : mReceiver(receiver) {}
    ~WaterfallProtocol() = default;

    // Wites a string blob to the socket.
    bool write(SocketTransport* to, const std::string msg) override;

    // Called when the socket has bytes, on this callback we reassemble the
    // the received string
    void received(SocketTransport* from, const std::string object) override;

    // Called when the socket changes state.
    void stateConnectionChange(SocketTransport* connection,
                               State current) override;

private:
    WaterfallReceiver* mReceiver;
    std::string mBuffer;
};
}  // namespace net
}  // namespace emulator
