
// Copyright (C) 2020 The Android Open Source Project
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
#include <grpcpp/grpcpp.h>  // for Status
#include <stdint.h>         // for uint8_t
#include <cstdint>          // for uint8_t
#include <memory>           // for unique_ptr
#include <type_traits>      // for move
#include <utility>          // for move
#include <vector>           // for vector

#include "grpcpp/impl/codegen/completion_queue.h"  // for ServerC...
#include "grpcpp/impl/codegen/server_context.h"    // for ServerC...
#include "grpcpp/impl/codegen/status.h"            // for Status
#include "grpcpp/impl/codegen/sync_stream.h"       // for ServerR...
#include "test_echo_service.grpc.pb.h"             // for TestEcho

namespace google {
namespace protobuf {
class Empty;
}  // namespace protobuf
}  // namespace google

namespace android {
namespace emulation {
namespace control {
class Msg;

using grpc::ServerContext;
using grpc::Status;

class TestEchoServiceBase : public TestEcho::Service {
public:
    virtual ~TestEchoServiceBase() = default;
    Status echo(ServerContext* context,
                const Msg* request,
                Msg* response) override;

    Status data(ServerContext* context,
                const ::google::protobuf::Empty* empty,
                Msg* response) override;

    int invocations() { return mCounter; }

    void moveData(std::vector<uint8_t> data) { mData = std::move(data); }

    void plusOne() { mCounter++; }

protected:
    int mCounter{0};
    std::vector<uint8_t> mData;
};

class HeartbeatService : public TestEcho::Service {
public:
    ::grpc::Status streamEcho(
            ::grpc::ServerContext* /*context*/,
            ::grpc::ServerReaderWriter<Msg, Msg>* /*stream*/) override;
};

#ifndef DISABLE_ASYNC_GRPC
class AsyncHeartbeatService
    : public TestEcho::WithCallbackMethod_streamEcho<HeartbeatService> {
    grpc::ServerBidiReactor<Msg, Msg>* streamEcho(
            ::grpc::CallbackServerContext* context) override;
};
#else
class AsyncHeartbeatService : public TestEcho::Service {
public:
    ~AsyncHeartbeatService() = default;
    ::grpc::Status streamEcho(
            ::grpc::ServerContext* /*context*/,
            ::grpc::ServerReaderWriter<Msg, Msg>* /*stream*/) override;
};
#endif

#ifndef DISABLE_ASYNC_GRPC
class AsyncTestEchoService
    : public TestEcho::WithCallbackMethod_streamEcho<TestEchoServiceBase> {
public:
    grpc::ServerBidiReactor<Msg, Msg>* streamEcho(
            ::grpc::CallbackServerContext* context) override;
};
#else
class AsyncTestEchoService : public TestEchoServiceBase {
public:
    ~AsyncTestEchoService() = default;
    virtual ::grpc::Status streamEcho(
            ::grpc::ServerContext* context,
            ::grpc::ServerReaderWriter<Msg, Msg>* stream) override;
};
#endif

}  // namespace control
}  // namespace emulation
}  // namespace android
