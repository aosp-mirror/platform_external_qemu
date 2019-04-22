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
#include "Switchboard.h"
#include "rtc_base/logging.h"

namespace emulator {
namespace webrtc {

void Switchboard::stateConnectionChange(SocketTransport* connection,
                                        State current) {
    if (current == State::CONNECTED) {
        // Connect to pubsub
        mProtocol.write(connection, {{"type", "publish"},
                                     {"topic", "connected"},
                                     {"msg", "switchboard"}});
        // Subscribe to disconnect events.
        mProtocol.write(connection, {
                                            {"type", "subscribe"},
                                            {"topic", "disconnected"},
                                    });
    }

    // We got disconnected, clear out all participants.
    if (current == State::NOT_CONNECTED) {
        RTC_LOG(INFO) << "Disconnected";
        mConnections.clear();
        mIdentityMap.clear();
        if (mEmulator)
            mEmulator->disconnect(this);
    }
}

void Switchboard::connect(std::string server, int port) {
    mTransport.connect(server, port);
}

Switchboard::~Switchboard() {}
Switchboard::Switchboard(std::string handle)
    : handle_(handle),
      mProtocol(this),
      mTransport(&mProtocol),
      mEmulator(nullptr) {}

Switchboard::Switchboard(std::string handle,
                         AsyncSocket* connection,
                         net::EmulatorConnection* parent)
    : handle_(handle),
      mProtocol(this),
      mTransport(&mProtocol, connection),
      mEmulator(parent) {}

void Switchboard::rtcConnectionDropped(std::string participant) {
    rtc::CritScope cs(&mCleanupCS);
    mDroppedConnections.push_back(participant);
}

void Switchboard::finalizeConnections() {
    rtc::CritScope cs(&mCleanupCS);
    for (auto participant : mDroppedConnections) {
        send(participant, "{ \"bye\" : \"stream disconnected\" }");
        mIdentityMap.erase(participant);
        mConnections.erase(participant);
    }
    mDroppedConnections.clear();
}

void Switchboard::received(SocketTransport* transport, const json object) {
    RTC_LOG(INFO) << "Received: " << object;
    finalizeConnections();

    // We expect:
    // { 'msg' : some_json_object,
    //   'from': some_sender_string,
    // }
    if (!object.count("msg") || !object.count("from") ||
        !json::accept(object["msg"].get<std::string>())) {
        RTC_LOG(LERROR) << "Message not according spec, ignoring!";
        return;
    }

    std::string from = object["from"];
    json msg = json::parse(object["msg"].get<std::string>(), nullptr, false);

    // Start a new participant.
    if (msg.count("start")) {
        mIdentityMap[from] = msg["start"];

        // Inform the client we are going to start, this is the place
        // where we should send the turn config to the client.
        // Turn is really only needed when your server is
        // locked down pretty well. (Google gce environment for example)

        // The format is json: {"start" : RTCPeerConnection config}
        // (i.e. result from
        // https://networktraversal.googleapis.com/v1alpha/iceconfig?key=....)
        json start = "{ \"start\" : {} }"_json;
        // start["start"] = {};
        send(from, start);

        rtc::scoped_refptr<Participant> stream(
                new rtc::RefCountedObject<Participant>(this, from, handle_));
        if (stream->Initialize()) {
            mConnections[from] = stream;
        }
        return;
    }

    // Client disconnected
    if (msg.is_string() && msg.get<std::string>() == "disconnected" &&
        mConnections.count(from)) {
        mIdentityMap.erase(from);
        mConnections.erase(from);
    }

    // Route webrtc signaling packet
    if (mConnections.count(from)) {
        mConnections[from]->IncomingMessage(msg);
    }
}

void Switchboard::send(std::string to, json message) {
    if (!mIdentityMap.count(to)) {
        return;
    }

    json msg;
    msg["type"] = "publish";
    msg["msg"] = message.dump();
    msg["topic"] = mIdentityMap[to];

    RTC_LOG(INFO) << "Sending " << msg;
    mProtocol.write(&mTransport, msg);
}
}  // namespace webrtc
}  // namespace emulator
