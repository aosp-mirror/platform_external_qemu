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

#include "android/base/Optional.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/system/System.h"
#include "rtc_base/logging.h"

using namespace android::base;

namespace emulator {
namespace webrtc {

std::string Switchboard::BRIDGE_RECEIVER ="WebrtcVideoBridge";

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
    if (current == State::DISCONNECTED) {
        RTC_LOG(INFO) << "Disconnected";
        mConnections.clear();
        mIdentityMap.clear();
        if (mEmulator)
            mEmulator->disconnect(this);
    }
}

Switchboard::~Switchboard() {}

Switchboard::Switchboard(const std::string handle,
                         const std::string& turnConfig,
                         AsyncSocketAdapter* connection,
                         net::EmulatorConnection* parent)
    : mHandle(handle),
      mProtocol(this),
      mTransport(&mProtocol, connection),
      mEmulator(parent) {
    split(turnConfig, " ", [this](StringView str) {
        if (!str.empty()) {
            mTurnConfig.push_back(str);
        }
    });
}

void Switchboard::rtcConnectionDropped(std::string participant) {
    rtc::CritScope cs(&mCleanupCS);
    mDroppedConnections.push_back(participant);
}

void Switchboard::finalizeConnections() {
    rtc::CritScope cs(&mCleanupCS);
    for (auto participant : mDroppedConnections) {
        json msg;
        msg["bye"] = "stream disconnected";
        msg["topic"] = participant;
        RTC_LOG(INFO) << "Sending " << msg;
        mProtocol.write(&mTransport, msg);
        mIdentityMap.erase(participant);
        mConnections.erase(participant);
    }
    mDroppedConnections.clear();
}

void Switchboard::received(SocketTransport* transport, const json object) {
    RTC_LOG(INFO) << "Received: " << object;
    finalizeConnections();

    // { 'handle' : xx, 'fps' : int} messages to configure the video sharing.

    if (object.count("handle") && object.count("fps")) {
        // ignore for now as we have 1-1 correspondance between brdige -
        // emulator. mHandle = object["handle"];
        std::string fps = object["fps"];
        sscanf(fps.c_str(), "%d", &mFps);
        RTC_LOG(INFO) << "Not updating handle.. Updated fps: " << fps;
        return;
    }

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
        json turnConfig = "{}"_json;
        if (mTurnConfig.size() > 0) {
            System::ProcessExitCode exitCode;
            Optional<std::string> turn = System::get()->runCommandWithResult(
                    mTurnConfig, kMaxTurnConfigTime, &exitCode);
            if (exitCode == 0 && turn && json::accept(*turn)) {
                json config = json::parse(*turn, nullptr, false);
                if (config.count("iceServers")) {
                    turnConfig = config;
                } else {
                    RTC_LOG(LS_ERROR) << "Unusable turn config: " << turn;
                }
            }
        }

        // The format is json: {"start" : RTCPeerConnection config}
        // (i.e. result from
        // https://networktraversal.googleapis.com/v1alpha/iceconfig?key=....)
        json start;
        start["start"] = turnConfig;
        send(from, start);

        rtc::scoped_refptr<Participant> stream(
                new rtc::RefCountedObject<Participant>(this, from, mHandle,
                                                       mFps));
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
    if (to.compare(BRIDGE_RECEIVER) == 0) {
        // Directly forward messages intended for the bridge
        message["topic"] = BRIDGE_RECEIVER;
        mProtocol.write(&mTransport, message);
        return;
    }

    if (!mIdentityMap.count(to)) {
        return;
    }
    json msg;
    msg["msg"] = message.dump();
    msg["topic"] = mIdentityMap[to];

    RTC_LOG(INFO) << "Sending " << msg;
    mProtocol.write(&mTransport, msg);
}
}  // namespace webrtc
}  // namespace emulator
