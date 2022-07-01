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
#include <nlohmann/json.hpp>
#include <string>

#include "SocketTransport.h"

using json = nlohmann::json;
namespace emulator {
namespace net {

using JsonReceiver = ProtocolReader<json>;
using JsonWriter = ProtocolWriter<json>;

// A simple wire protocol that sends and receive json messages
// that are seperated by '\0000' (null).
//
// When a new json message has been parsed it will be passed on
// to the receiver. Socket state changes will be forwarded.
class JsonProtocol : public MessageReceiver, public JsonWriter {
public:
    JsonProtocol(JsonReceiver* receiver) : mReceiver(receiver) {}

    // Wites a json blob to the socket.
    bool write(SocketTransport* to, const json object) override;

    // Called when the socket has bytes, on this callback we reassemble the
    // json packet.
    void received(SocketTransport* from, const std::string object) override;

    // Called when the socket changes state.
    void stateConnectionChange(SocketTransport* connection,
                               State current) override;

private:
    ProtocolReader<json>* mReceiver;
    std::string mBuffer;
};
}  // namespace net
}  // namespace emulator
