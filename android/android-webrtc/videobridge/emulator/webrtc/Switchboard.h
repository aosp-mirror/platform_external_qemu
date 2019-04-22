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

#include <emulator/net/EmulatorConnection.h>
#include <emulator/net/JsonProtocol.h>
#include <string>
#include "Participant.h"
namespace emulator {

namespace net {
class EmulatorConnection;
}

namespace webrtc {

class Participant;

using net::SocketTransport;
using net::JsonReceiver;
using net::JsonProtocol;
using net::State;
using rtc::AsyncSocket;
using net::EmulatorConnection;

// A switchboard manages a set of web rtc participants:
//
// 1. It creates new participants when a user starts a session
// 2. It removes participants when a user disconnects
// 3. It routes the webrtc json signals to the proper participant.
// 4. Participants that are no longer streaming need to be finalized.
class Switchboard : public JsonReceiver {
public:
    Switchboard(std::string handle);
    Switchboard(std::string handle,
                AsyncSocket* connection,
                EmulatorConnection* parent);
    ~Switchboard();
    void received(SocketTransport* from, const json object) override;
    void stateConnectionChange(SocketTransport* connection,
                               State current) override;
    void send(std::string to, json msg);
    void connect(std::string server, int port);

    // Called when a participant is unable to send the rtc stream.
    // The participant will no longer be in use and can be finalized.
    void rtcConnectionDropped(std::string participant);

    // Cleans up connections that have marked them self as deleted
    // due to a connection dropped.
    void finalizeConnections();

private:

    std::map<std::string, rtc::scoped_refptr<Participant> > mConnections;
    std::map<std::string, std::string> mIdentityMap;
    std::vector<std::string> mDroppedConnections;
    std::string handle_;
    JsonProtocol mProtocol;
    SocketTransport mTransport;
    net::EmulatorConnection* mEmulator;
    rtc::CriticalSection mCleanupCS;

};
}  // namespace webrtc
}  // namespace emulator
