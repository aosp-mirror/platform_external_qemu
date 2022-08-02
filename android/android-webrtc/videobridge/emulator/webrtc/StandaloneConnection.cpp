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
#include "StandaloneConnection.h"

#include <api/scoped_refptr.h>            // for scoped_refptr
#include <assert.h>                       // for assert
#include <rtc_base/ref_counted_object.h>  // for RefCountedObject
#include <memory>                         // for unique_ptr, make_shared
#include <ostream>                        // for operator<<, basic_ostream
#include <string>                         // for string, operator==

#include "android/base/Log.h"                 // for LogStreamVoidify, LOG
#include "android/base/async/AsyncSocket.h"   // for AsyncSocket
#include "android/emulation/control/utils/EmulatorGrcpClient.h"  // for EmulatorGrpcClient
#include "emulator/webrtc/Participant.h"      // for Participant, DataChanne...
#include "emulator_controller.pb.h"           // for KeyboardEvent, MouseEvent
#include "google/protobuf/empty.pb.h"         // for Empty
#include "rtc_service.grpc.pb.h"              // for Rtc::Stub

namespace emulator {

namespace webrtc {

using android::emulation::control::JsepMsg;
using google::protobuf::Empty;
using grpc::Status;
using grpc::StatusCode;

StandaloneConnection::StandaloneConnection(EmulatorGrpcClient* client,
                                           int adbPort,
                                           TurnConfig turnconfig)
    : RtcConnection(client), mAdbPort(adbPort), mTurnConfig(turnconfig) {};

StandaloneConnection::~StandaloneConnection() {
    close();
    if (mJsepThread) {
        mJsepThread->join();
    }
}

void StandaloneConnection::registerConnectionClosedListener(
        ClosedCallback callback) {
    mClosedCallback = callback;
}

void StandaloneConnection::close() {
    mAlive = false;
    if (auto context = mCtx.lock()) {
        LOG(INFO) << "Cancelling context.";
        context->TryCancel();
    }
}

void StandaloneConnection::connect() {
    mJsepThread = std::make_unique<std::thread>([this] {
        driveJsep();

        // We are done..
        if (mClosedCallback) {
            mClosedCallback();
        }
    });
}

void StandaloneConnection::send(std::string to, json msg) {
    auto ctx = getEmulatorClient()->newContext();
    auto rtc = getEmulatorClient()->stub<android::emulation::control::Rtc>();
    JsepMsg reply;
    reply.mutable_id()->set_guid(mGuid.guid());
    reply.set_message(msg.dump());
    Empty response;
    Status status = rtc->sendJsepMessage(ctx.get(), reply, &response);
    if (status.error_code() != StatusCode::OK) {
        // Request close..
        if (auto participant = mParticipant.lock()) {
            participant->Close();
        }
    }
}

bool StandaloneConnection::connectAdbSocket(int fd) {
    if (auto participant = mParticipant.lock()) {
        auto channel = participant->GetDataChannel(DataChannelLabel::adb);
        if (channel) {
            LOG(INFO) << "Forwarding socket on fd: " << fd;
            auto asyncSocket = std::make_shared<android::base::AsyncSocket>(
                    getLooper(), android::base::ScopedSocket(fd));
            mAdbForwarders.emplace_back(asyncSocket, channel);
        }
    }
    if (auto socketServer = mAdbSocketServer.lock()) {
        socketServer->startListening();
    }

    return true;
}

std::shared_ptr<AsyncSocketServer> StandaloneConnection::createAdbForwarder() {
    LOG(INFO) << "Registering adb forwarder on " << mAdbPort
              << ", Looper:" << static_cast<void*>(getLooper())
              << ", this: " << static_cast<void*>(this);
    auto server = AsyncSocketServer::createTcpLoopbackServer(
            mAdbPort, [this](int fd) { return connectAdbSocket(fd); },
            AsyncSocketServer::LoopbackMode::kIPv4AndIPv6, getLooper());

    if (server) {
        server->startListening();
    }
    return server;
}

void StandaloneConnection::rtcConnectionClosed(const std::string participant) {
    LOG(INFO) << "RtcConnection closed, time to clean up.";
    signalingThread()->PostDelayedTask(
            RTC_FROM_HERE,
            [this] {
                if (auto participant = mParticipant.lock()) {
                    participant->Close();
                }
            },
            1);
}

void StandaloneConnection::driveJsep() {
    // This drives the jsep protocol, and owns the only active partipant.
    mAlive = true;
    std::shared_ptr<grpc::ClientContext> context;
    std::shared_ptr<Participant> participant;
    std::shared_ptr<AsyncSocketServer> socketServer;

    auto rtc = getEmulatorClient()->stub<android::emulation::control::Rtc>();
    Status status = rtc->requestRtcStream(
            getEmulatorClient()->newContext().get(), Empty(), &mGuid);
    if (status.error_code() != StatusCode::OK) {
        LOG(ERROR) << "Error requesting stream start: " << status.error_code()
                   << ", " << status.error_message();
        return;
    }

    context = getEmulatorClient()->newContext();
    mCtx = context;

    auto messages = rtc->receiveJsepMessages(context.get(), mGuid);
    if (!messages) {
        LOG(ERROR) << "Error requesting jsep protocol: " << status.error_code()
                   << ", " << status.error_message();
        return;
    }

    json turnConfig = mTurnConfig.getConfig();
    participant = std::make_shared<Participant>(this, mGuid.guid(), turnConfig,
                                                &mVideoReceiver);
    mParticipant = participant;

    if (mAdbPort > 0) {
        socketServer = createAdbForwarder();
        if (!socketServer) {
            LOG(ERROR) << "Failed to create listener on port " << mAdbPort;
            return;
        }
        mAdbSocketServer = socketServer;
    }

    JsepMsg msg;
    while (mAlive && messages->Read(&msg)) {
        assert(mGuid.guid() == msg.id().guid());
        if (json::jsonaccept(msg.message())) {
            auto jsonMessage = json::parse(msg.message());
            participant->IncomingMessage(jsonMessage);
        }
    }

    LOG(INFO) << "Finished stream, waiting for participant to disappear.";
    participant->WaitForClose();
    mAdbForwarders.clear();
}

void StandaloneConnection::sendKey(const KeyboardEvent* keyEvent) {
    if (auto participant = mParticipant.lock()) {
        participant->SendToDataChannel(DataChannelLabel::keyboard,
                                       keyEvent->SerializeAsString());
    }
}

void StandaloneConnection::sendMouse(const MouseEvent* mouseEvent) {
    if (auto participant = mParticipant.lock()) {
        participant->SendToDataChannel(DataChannelLabel::mouse,
                                       mouseEvent->SerializeAsString());
    }
}
void StandaloneConnection::sendTouch(const TouchEvent* touchEvent) {
    if (auto participant = mParticipant.lock()) {
        participant->SendToDataChannel(DataChannelLabel::touch,
                                       touchEvent->SerializeAsString());
    }
}

}  // namespace webrtc
}  // namespace emulator
