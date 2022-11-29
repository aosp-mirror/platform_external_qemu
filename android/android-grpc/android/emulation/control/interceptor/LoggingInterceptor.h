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
#include <grpcpp/grpcpp.h>  // for InterceptionHookPoints, InterceptionHookP...
#include <stdint.h>         // for uint64_t
#include <array>            // for array
#include <functional>       // for function
#include <string>           // for string

namespace google {
namespace protobuf {
class Message;
}  // namespace protobuf
}  // namespace google

namespace android {
namespace control {
namespace interceptor {

using namespace grpc::experimental;

typedef struct InvocationRecord {
    std::string method = "unknown";          // Invoked method.
    std::string incoming = "...";            // Shortened receive parameters.
    std::string response = "...";            // Shortened response string.
    grpc::Status status = grpc::Status::OK;  // Status
    uint64_t rcvBytes = 0;  // Size of all received protobuf messages.
    uint64_t rcvTime = 0;   // Time spend receiving bytes out over the wire.
    uint64_t sndBytes = 0;  // Size of all send protobuf messages.
    uint64_t sndTime = 0;   // Time spend sending bytes out over the wire.

    uint64_t duration = 0;  // Total lifetime of the request.
    ServerRpcInfo::Type type = ServerRpcInfo::Type::UNARY;

    // Timestamps of the various stages. We will use NUM_INTERCEPTION_HOOKS to
    // store the creation time
    uint64_t mTimestamps
            [static_cast<int>(InterceptionHookPoints::NUM_INTERCEPTION_HOOKS) +
             1] = {};

    static const std::array<std::string, 4> kTypes;
    static const int kStartTimeIdx =
            static_cast<int>(InterceptionHookPoints::NUM_INTERCEPTION_HOOKS);

} InvocationRecord;

using ReportingFunction = std::function<void(const InvocationRecord&)>;

// A simple logging interceptor that collects InvocationRecords.
// The reporting function will be invoked when the gRPC method is completed
// and the interceptor object is deleted.
//
// You can use it as follows:
// - Create a ReportingFunction
// - Register the reporting function with gRPC
//
//    ServerBuilder builder;
//    std::vector<std::unique_ptr<ServerInterceptorFactoryInterface>> creators;
//    creators.emplace_back(std::make_unique<LoggingInterceptorFactory>([](auto
//    log}{ .... }));
//    builder.experimental().SetInterceptorCreators(std::move(creators));
//
class LoggingInterceptor : public grpc::experimental::Interceptor {
public:
    LoggingInterceptor(ServerRpcInfo* info, ReportingFunction reporter);
    ~LoggingInterceptor();

    virtual void Intercept(InterceptorBatchMethods* methods) override;

private:
    std::string chopStr(std::string);
    std::string formatProtobufMessage(const ::google::protobuf::Message* msg);

    // We will cut of all repsone/incoming strings at this length.
    const unsigned int kMaxStringLen = 80;

    // Max field length of protobuf message we are willing to log.
    const unsigned int kMaxProtbufStrlen = 20;

    // Maximum size of a protobuf message we are willing to log.
    const unsigned int kMaxProtobufMsgLogSize = 2048;

    InvocationRecord mLoginfo;
    ReportingFunction mReporter;
};

// The factory class that needs to be registered with the gRPC server/client.
class LoggingInterceptorFactory
    : public grpc::experimental::ServerInterceptorFactoryInterface {
public:
    LoggingInterceptorFactory(ReportingFunction reporter);
    virtual ~LoggingInterceptorFactory() = default;
    virtual Interceptor* CreateServerInterceptor(ServerRpcInfo* info) override;

private:
    ReportingFunction mReporter;
};

// A logging interceptor that logs all the requests to stdout using LOG(INFO)
class StdOutLoggingInterceptorFactory : public LoggingInterceptorFactory {
public:
    StdOutLoggingInterceptorFactory();
};

}  // namespace interceptor
}  // namespace control
}  // namespace android
