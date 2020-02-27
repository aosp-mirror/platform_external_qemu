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

#include <grpcpp/grpcpp.h>  // for Status, ServerCo...

#include <chrono>  // for milliseconds
#include <memory>  // for unique_ptr
#include <mutex>   // for mutex, lock_guard
#include <ratio>   // for ratio
#include <string>  // for string

#include "android/base/Log.h"                        // for base
#include "android/base/Uuid.h"                       // for Uuid
#include "android/base/async/ThreadLooper.h"         // for ThreadLooper
#include "android/console.h"                         // for AndroidConsoleAg...
#include "android/emulation/control/RtcBridge.h"     // for RtcBridge
#include "android/emulation/control/WebRtcBridge.h"  // for WebRtcBridge
#include "rtc_service.grpc.pb.h"                     // for Rtc, Rtc::Service
#include "rtc_service.pb.h"                          // for JsepMsg, RtcId

namespace google {
namespace protobuf {
class Empty;
}  // namespace protobuf
}  // namespace google

using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using namespace android::base;

namespace android {
namespace emulation {
namespace control {

class RtcServiceImpl : public Rtc::Service {
public:
    RtcServiceImpl(const AndroidConsoleAgents* agents, std::string turncfg)
        : mAgents(agents),
          mTurnCfg(turncfg),
          mLooper(android::base::ThreadLooper::get()) {}

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
    RtcBridge* getBridge() {
        const std::lock_guard<std::mutex> lock(mLock);
        if (mBridge)
            return mBridge.get();

        mBridge.reset(android::emulation::control::WebRtcBridge::create(
                mLooper, 0, mAgents, mTurnCfg));
        auto started = mBridge->start();
        return mBridge.get();
    }

    std::string mTurnCfg;
    const AndroidConsoleAgents* const mAgents;
    std::mutex mLock;
    android::base::Looper* mLooper;
    std::unique_ptr<RtcBridge> mBridge;
    static constexpr uint16_t k5SecondsWait = 5 * 1000;
};

grpc::Service* getRtcService(const AndroidConsoleAgents* agents,
                             const char* turncfg) {
    return new RtcServiceImpl(agents, turncfg ? turncfg : "");
}
}  // namespace control
}  // namespace emulation
}  // namespace android
