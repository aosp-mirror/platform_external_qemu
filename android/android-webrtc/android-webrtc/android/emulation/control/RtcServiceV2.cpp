// Copyright (C) 2023 The Android Open Source Project
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
#include "android/emulation/control/RtcServiceV2.h"

#include <google/protobuf/util/json_util.h>  // for MessageToJsonString
#include <grpcpp/grpcpp.h>                   // for Status, ServerCo...
#include <stdint.h>                          // for uint16_t
#include <chrono>                            // for milliseconds
#include <memory>                            // for shared_ptr, uniq...
#include <ostream>                           // for operator<<, basi...
#include <ratio>                             // for ratio

// #include "android/android.h"
#include "aemu/base/Log.h"                         // for LogStreamVoidify
#include "aemu/base/Optional.h"                    // for Optional
#include "aemu/base/Stopwatch.h"                   // for Stopwatch
#include "aemu/base/Uuid.h"                        // for Uuid
#include "aemu/base/sockets/ScopedSocket.h"        // for ScopedSocket
#include "aemu/base/sockets/SocketUtils.h"         // for socketGetPort
#include "android/base/system/System.h"            // for System::Pid, System
#include "android/console.h"                       // for AndroidConsoleAg...
#include "android/emulation/control/RtcBridge.h"     // for RtcBridge, System
#include "android/emulation/control/TurnConfig.h"  // for TurnConfig
#include "emulator/webrtc/Switchboard.h"           // for Switchboard
#include "rtc_service_v2.grpc.pb.h"                // for Rtc, Rtc::Stub
#include "rtc_service_v2.pb.h"                     // for JsepMsg, RtcId

#include <rtc_base/logging.h>      // for LogSink, LogStre...
#include <rtc_base/ssl_adapter.h>  // for CleanupSSL, Init...

namespace google {
namespace protobuf {
class Empty;
}  // namespace protobuf
}  // namespace google

using android::emulation::control::RtcBridge;
using android::emulation::control::TurnConfig;
using emulator::webrtc::Switchboard;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using grpc::StatusCode;

using namespace android::base;

namespace android {
namespace emulation {
namespace control {
namespace v2 {

class RtcServiceImpl : public Rtc::Service {
public:
    RtcServiceImpl(RtcBridge* bridge) : mBridge(bridge) {
        rtc::InitializeSSL();
    }

    ~RtcServiceImpl() { rtc::CleanupSSL(); }

    Status RequestRtcStream(ServerContext* context,
                            const RtcStreamRequest* request,
                            RtcStreamResponse* reply) override {
        std::string jsonStr;
        if (request->has_ice_server_config()) {
            // Create a json_string from sr.
            google::protobuf::util::JsonPrintOptions options;
            options.add_whitespace = true;
            options.always_print_primitive_fields = true;
            options.preserve_proto_field_names = true;
            auto status = google::protobuf::util::MessageToJsonString(
                    request->ice_server_config(), &jsonStr, options);
            if (status.ok()) {
                LOG(VERBOSE) << "Accept ice_server_config message: " << jsonStr;
            } else {
                LOG(ERROR) << "Failed to parse the ice_server_config message "
                              "to json: "
                           << status.ToString();
                return Status(
                        ::grpc::StatusCode::INVALID_ARGUMENT,
                        "The ice server config is not valid json: " +
                                request->ice_server_config().ShortDebugString(),
                        "");
            }
        }
        std::string id = base::Uuid::generate().toString();
        if (jsonStr.empty()) {
            getBridge()->connect(id);
        } else {
            getBridge()->connect(id, std::move(jsonStr));
        }
        reply->mutable_id()->set_guid(id);
        return Status::OK;
    }

    Status SendJsepMessage(ServerContext* context,
                           const SendJsepMessageRequest* request,
                           SendJsepMessageResponse* reply) override {
        std::string id = request->jsep_msg().id().guid();
        std::string msg = request->jsep_msg().message();
        auto accepted = getBridge()->acceptJsepMessage(id, msg);
        LOG(INFO) << "acceptJsepMessage: " << id << ", " << msg
                  << ", accepted: " << accepted;
        return Status::OK;
    }

    Status ReceiveJsepMessageStream(
            ServerContext* context,
            const ReceiveJsepMessageRequest* request,
            ServerWriter<ReceiveJsepMessageResponse>* writer) override {
        std::string id = request->id().guid();

        // Wait at most 1/4 of second, so we do not block this thread
        // upon service exit.
        const auto kTimeToWaitForMsg = std::chrono::milliseconds(250);

        bool clientAvailable = !context->IsCancelled();
        ReceiveJsepMessageResponse reply;
        reply.mutable_jsep_msg()->mutable_id()->set_guid(id);
        while (clientAvailable) {
            std::string msg;

            // Since this is a synchronous call we want to wait at
            // most kTimeToWaitForMsg so we can check if the client is still
            // there. (All clients get disconnected on emulator shutdown).
            bool state = getBridge()->nextMessage(id, &msg,
                                                  kTimeToWaitForMsg.count());
            clientAvailable = state || msg.empty();
            if (!msg.empty()) {
                reply.mutable_jsep_msg()->set_message(msg);
                LOG(INFO) << "RtcPacket: " << reply.ShortDebugString();
                clientAvailable = writer->Write(reply) && state;
            }
            clientAvailable = !context->IsCancelled() && clientAvailable;
        }
        return Status::OK;
    }

    Status ReceiveJsepMessage(ServerContext* context,
                              const ReceiveJsepMessageRequest* request,
                              ReceiveJsepMessageResponse* reply) override {
        std::string msg;
        std::string id = request->id().guid();
        // Block and wait for at most 5 seconds.
        const std::chrono::milliseconds kWait5Seconds = std::chrono::seconds(5);
        getBridge()->nextMessage(id, &msg, kWait5Seconds.count());
        reply->mutable_jsep_msg()->mutable_id()->set_guid(request->id().guid());
        reply->mutable_jsep_msg()->set_message(msg);
        return Status::OK;
    }

private:
    RtcBridge* getBridge() { return mBridge; }

    RtcBridge* mBridge;
    int mAdbPort;
    static constexpr uint16_t k5SecondsWait = 5 * 1000;
};

grpc::Service* getRtcService(RtcBridge* bridge) {
    return new RtcServiceImpl(bridge);
}

// A simple logsink that throws everyhting on stderr.
class StdLogSink : public rtc::LogSink {
public:
    StdLogSink() = default;
    ~StdLogSink() override {}

    void OnLogMessage(const std::string& message) override {
        LOG(VERBOSE) << message;
    }
};

static const int kMaxFileLogSizeInBytes = 128 * 1024 * 1024;

grpc::Service* getRtcService(const char* turncfg,
                             const char* audioDump,
                             bool verbose) {
    if (verbose) {
        rtc::LogMessage::AddLogToStream(new StdLogSink(),
                                        rtc::LoggingSeverity::LS_INFO);
    }
    TurnConfig config(turncfg ? turncfg : "");
    if (!config.validCommand()) {
        derror("command `%s` does not produce a valid turn configuration.",
               turncfg);
        exit(1);
    }
    return getRtcService(new Switchboard(config, audioDump ? audioDump : ""));
}

}  // namespace v2
}  // namespace control
}  // namespace emulation
}  // namespace android
