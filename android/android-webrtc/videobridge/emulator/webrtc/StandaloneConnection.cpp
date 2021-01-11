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
#include "emulator/net/EmulatorGrcpClient.h"  // for EmulatorGrpcClient
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
                                           int adbPort)
    : RtcConnection(client), mAdbPort(adbPort){};

void StandaloneConnection::rtcConnectionDropped(std::string participant) {
    if (mCtx)
        mCtx->TryCancel();
}

StandaloneConnection::~StandaloneConnection() {
    close();
    if (mJsepThread)
        mJsepThread->join();
}

void StandaloneConnection::rtcConnectionClosed(std::string participant) {
    if (mCtx)
        mCtx->TryCancel();
}

void StandaloneConnection::close() {
    const std::lock_guard<std::mutex> lock(mCloseMutex);
    if (!mAlive) {
        return;
    }

    mAlive = false;
    mGuid.set_guid("");
    if (mParticipant)
        mParticipant->Close();

    if (mAdbSocketServer)
        mAdbSocketServer->stopListening();
    LOG(INFO) << "Connection close.";
}

void StandaloneConnection::connect() {
    mJsepThread = std::make_unique<std::thread>([=]() { driveJsep(); });
}

void StandaloneConnection::send(std::string to, json msg) {
    auto ctx = getEmulatorClient()->newContext();
    auto rtc = getEmulatorClient()->rtcStub();
    JsepMsg reply;
    reply.mutable_id()->set_guid(mGuid.guid());
    reply.set_message(msg.dump());
    Empty response;
    Status status = rtc->sendJsepMessage(ctx.get(), reply, &response);
    if (status.error_code() != StatusCode::OK) {
        mParticipant->Close();
    }
}

bool StandaloneConnection::connectAdbSocket(int fd) {
    LOG(INFO) << "Socket on: " << fd
              << ", Looper:" << static_cast<void*>(getLooper())
              << ", this: " << static_cast<void*>(this);
    if (mParticipant) {
        auto channel = mParticipant->GetDataChannel(DataChannelLabel::adb);
        if (channel) {
            auto asyncSocket = std::make_shared<android::base::AsyncSocket>(
                    getLooper(), android::base::ScopedSocket(fd));
            mAdbForwarders.emplace_back(asyncSocket, channel);
        }
    }
    mAdbSocketServer->startListening();
    return true;
}

bool StandaloneConnection::createAdbForwarder() {
    LOG(INFO) << "Registering adb forwarder on " << mAdbPort
              << ", Looper:" << static_cast<void*>(getLooper())
              << ", this: " << static_cast<void*>(this);
    mAdbSocketServer = AsyncSocketServer::createTcpLoopbackServer(
            mAdbPort, [this](int fd) { return connectAdbSocket(fd); },
            AsyncSocketServer::LoopbackMode::kIPv4AndIPv6, getLooper());

    if (!mAdbSocketServer) {
        return false;
    }
    mAdbSocketServer->startListening();
    return true;
}

void StandaloneConnection::driveJsep() {
    mAlive = true;
    auto rtc = getEmulatorClient()->rtcStub();

    Status status = rtc->requestRtcStream(
            getEmulatorClient()->newContext().get(), Empty(), &mGuid);
    if (status.error_code() != StatusCode::OK) {
        LOG(ERROR) << "Error requesting stream start: " << status.error_code()
                   << ", " << status.error_message();
        return;
    }

    mCtx = getEmulatorClient()->newContext();
    auto messages = rtc->receiveJsepMessages(mCtx.get(), mGuid);
    if (!messages) {
        LOG(ERROR) << "Error requesting jsep protocol: " << status.error_code()
                   << ", " << status.error_message();
        return;
    }

    mParticipant = new rtc::RefCountedObject<Participant>(this, mGuid.guid(), &mVideoReceiver);
    if (mAdbPort > 0 && !createAdbForwarder()) {
        LOG(ERROR) << "Failed to create listener on port " << mAdbPort;
        close();
        mParticipant = nullptr;
        return;
    }

    JsepMsg msg;
    while (mAlive && messages->Read(&msg)) {
        assert(mGuid.guid() == msg.id().guid());
        if (json::jsonaccept(msg.message())) {
            auto jsonMessage = json::parse(msg.message());
            mParticipant->IncomingMessage(jsonMessage);
        }
    }
    LOG(INFO) << "Finished stream.";
    mParticipant = nullptr;
}

void StandaloneConnection::sendKey(const KeyboardEvent* keyEvent) {
    if (mParticipant) {
        mParticipant->SendToDataChannel(DataChannelLabel::keyboard,
                                        keyEvent->SerializeAsString());
    }
}

void StandaloneConnection::sendMouse(const MouseEvent* mouseEvent) {
    if (mParticipant) {
        mParticipant->SendToDataChannel(DataChannelLabel::mouse,
                                        mouseEvent->SerializeAsString());
    }
}
void StandaloneConnection::sendTouch(const TouchEvent* touchEvent) {
    if (mParticipant) {
        mParticipant->SendToDataChannel(DataChannelLabel::touch,
                                        touchEvent->SerializeAsString());
    }
}

}  // namespace webrtc
}  // namespace emulator
