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
#include "android/emulation/control/RtcService.h"

#include <grpcpp/grpcpp.h>                           // for Status, ServerCo...
#include <stdint.h>                                  // for uint16_t
#include <chrono>                                    // for milliseconds
#include <memory>                                    // for shared_ptr, uniq...
#include <ostream>                                   // for operator<<, basi...
#include <ratio>                                     // for ratio
#include <string>                                    // for string, operator+

//#include "android/android.h"
#include "android/base/Log.h"                        // for LogStreamVoidify
#include "android/base/Optional.h"                   // for Optional
#include "android/base/Stopwatch.h"                  // for Stopwatch
#include "android/base/Uuid.h"                       // for Uuid
#include "android/base/sockets/ScopedSocket.h"       // for ScopedSocket
#include "android/base/sockets/SocketUtils.h"        // for socketGetPort
#include "android/base/system/System.h"              // for System::Pid, System
#include "android/console.h"                         // for AndroidConsoleAg...
#include "android/emulation/control/RtcBridge.h"     // for RtcBridge, System
#include "android/emulation/control/WebRtcBridge.h"  // for WebRtcBridge
#include "rtc_service.grpc.pb.h"                     // for Rtc, Rtc::Stub
#include "rtc_service.pb.h"                          // for JsepMsg, RtcId

namespace google {
namespace protobuf {
class Empty;
}  // namespace protobuf
}  // namespace google

using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using grpc::StatusCode;
using namespace android::base;

namespace android {
namespace emulation {
namespace control {

// This forwards the RTC service calls to a separate goldfish-webrtc process.
class RtcServiceForwarder : public Rtc::Service {
public:
    RtcServiceForwarder(int adbPort, std::string turncfg) : mAdbPort(adbPort), mTurnCfg(turncfg) {}

    ~RtcServiceForwarder() {
        if (mBridgePid) {
            LOG(INFO) << "Terminating video bridge.";
            System::get()->killProcess(mBridgePid);
        }
    }

    Status requestRtcStream(ServerContext* context,
                            const ::google::protobuf::Empty* request,
                            RtcId* reply) override {
        if (!connected()) {
            return Status(StatusCode::UNAVAILABLE,
                          "No remote channel to webrtc available.");
        }
        grpc::ClientContext clientContext;
        auto stub = android::emulation::control::Rtc::NewStub(mChannel);
        return stub->requestRtcStream(&clientContext, *request, reply);
    }

    Status sendJsepMessage(ServerContext* context,
                           const JsepMsg* request,
                           ::google::protobuf::Empty* reply) override {
        if (!connected()) {
            return Status(StatusCode::UNAVAILABLE,
                          "No remote channel to webrtc available.");
        }
        grpc::ClientContext clientContext;
        auto stub = android::emulation::control::Rtc::NewStub(mChannel);
        return stub->sendJsepMessage(&clientContext, *request, reply);
    }

    Status receiveJsepMessages(ServerContext* context,
                               const RtcId* request,
                               ServerWriter<JsepMsg>* writer) override {
        if (!connected()) {
            return Status(StatusCode::UNAVAILABLE,
                          "No remote channel to webrtc available.");
        }
        grpc::ClientContext clientContext;
        auto stub = android::emulation::control::Rtc::NewStub(mChannel);
        auto reader = stub->receiveJsepMessages(&clientContext, *request);
        if (!reader) {
            return Status::CANCELLED;
        }
        JsepMsg msg;

        while (reader->Read(&msg) && writer->Write(msg)) {
            // Do nothing..
        }
        return Status::OK;
    }

    Status receiveJsepMessage(ServerContext* context,
                              const RtcId* request,
                              JsepMsg* reply) override {
        if (!connected()) {
            return Status(StatusCode::UNAVAILABLE,
                          "No remote channel to webrtc available.");
        }
        grpc::ClientContext clientContext;
        auto stub = android::emulation::control::Rtc::NewStub(mChannel);
        return stub->receiveJsepMessage(&clientContext, *request, reply);
    }

private:
    // Launches the goldfish-webrtc-bridge and sets up the forwarder if needed.
    bool connected() {
        if (mChannel) {
            auto state = mChannel->GetState(true);
            return state == GRPC_CHANNEL_READY || state == GRPC_CHANNEL_IDLE;
        }

        // Launch the forwarding bridge..
        Stopwatch sw;
        // Find a free port.
        {
            android::base::ScopedSocket s0(socketTcp4LoopbackServer(0));
            mPort = android::base::socketGetPort(s0.get());
        }
        mBridgePid = WebRtcBridge::launch(mPort, mAdbPort, mTurnCfg);
        std::string grpc_address = "localhost:" + std::to_string(mPort);
        mChannel = grpc::CreateChannel(
                grpc_address,
                ::grpc::experimental::LocalCredentials(LOCAL_TCP));
        auto waitUntil = gpr_time_add(gpr_now(GPR_CLOCK_REALTIME),
                                      gpr_time_from_seconds(5, GPR_TIMESPAN));
        bool connect = mChannel->WaitForConnected(waitUntil);
        auto state = mChannel->GetState(true);
        LOG(INFO) << (connect ? "Connected" : "Not connected")
                  << " state: " << state
                  << ", after: " << Stopwatch::sec(sw.elapsedUs()) << " s";
        return state == GRPC_CHANNEL_READY || state == GRPC_CHANNEL_IDLE;
    }

    std::string mTurnCfg;
    int mPort = 0;
    int mAdbPort = 0;
    Optional<System::Pid> mBridgePid;
    std::shared_ptr<::grpc::Channel> mChannel;
};

class RtcServiceImpl : public Rtc::Service {
public:
    RtcServiceImpl(RtcBridge* bridge) : mBridge(bridge) {}

    Status requestRtcStream(ServerContext* context,
                            const ::google::protobuf::Empty* request,
                            RtcId* reply) override {
        std::string id = base::Uuid::generate().toString();
        getBridge()->connect(id);
        reply->set_guid(id);
        return Status::OK;
    }

    Status sendJsepMessage(ServerContext* context,
                           const JsepMsg* request,
                           ::google::protobuf::Empty* reply) override {
        std::string id = request->id().guid();
        std::string msg = request->message();
        auto accepted = getBridge()->acceptJsepMessage(id, msg);
        LOG(INFO) << "acceptJsepMessage: " << id << ", " << msg
                  << ", accepted: " << accepted;
        return Status::OK;
    }

    Status receiveJsepMessages(ServerContext* context,
                               const RtcId* request,
                               ServerWriter<JsepMsg>* writer) override {
        std::string id = request->guid();

        // Wait at most 1/4 of second, so we do not block this thread
        // upon service exit.
        const auto kTimeToWaitForMsg = std::chrono::milliseconds(250);

        bool clientAvailable = !context->IsCancelled();
        JsepMsg reply;
        reply.mutable_id()->set_guid(id);
        while (clientAvailable) {
            std::string msg;

            // Since this is a synchronous call we want to wait at
            // most kTimeToWaitForMsg so we can check if the client is still
            // there. (All clients get disconnected on emulator shutdown).
            bool state = getBridge()->nextMessage(id, &msg,
                                                  kTimeToWaitForMsg.count());
            clientAvailable = state || msg.empty();
            if (!msg.empty()) {
                reply.set_message(msg);
                LOG(INFO) << "RtcPacket: " << reply.ShortDebugString();
                clientAvailable = writer->Write(reply) && state;
            }
            clientAvailable = !context->IsCancelled() && clientAvailable;
        }
        return Status::OK;
    }

    Status receiveJsepMessage(ServerContext* context,
                              const RtcId* request,
                              JsepMsg* reply) override {
        std::string msg;
        std::string id = request->guid();
        // Block and wait for at most 5 seconds.
        const std::chrono::milliseconds kWait5Seconds = std::chrono::seconds(5);
        getBridge()->nextMessage(id, &msg, kWait5Seconds.count());
        reply->mutable_id()->set_guid(request->guid());
        reply->set_message(msg);
        return Status::OK;
    }

private:
    RtcBridge* getBridge() { return mBridge; }

    RtcBridge* mBridge;
    int mAdbPort;
    static constexpr uint16_t k5SecondsWait = 5 * 1000;
};

grpc::Service* getRtcService(const AndroidConsoleAgents* agents,
                            int adbPort,
                             const char* turncfg) {
    return new RtcServiceForwarder(adbPort, turncfg ? turncfg : "");
}

grpc::Service* getRtcService(RtcBridge* bridge) {
    return new RtcServiceImpl(bridge);
}
}  // namespace control
}  // namespace emulation
}  // namespace android
