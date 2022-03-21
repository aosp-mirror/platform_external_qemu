
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

class TestEchoServiceImpl : public TestEcho::Service {
public:
    Status echo(ServerContext* context,
                const Msg* request,
                Msg* response) override;

    Status data(ServerContext* context,
                const ::google::protobuf::Empty* empty,
                Msg* response) override;

    int invocations() { return mCounter; }

    void moveData(std::vector<uint8_t> data) { mData = std::move(data); }

    void plusOne() { mCounter++; }

private:
    int mCounter{0};
    std::vector<uint8_t> mData;
};

class HeartbeatService : public TestEcho::Service {
public:
    ::grpc::Status streamEcho(
            ::grpc::ServerContext* /*context*/,
            ::grpc::ServerReaderWriter<Msg, Msg>* /*stream*/) override;
};

class AsyncHeartbeatService
    : public TestEcho::WithCallbackMethod_streamEcho<HeartbeatService> {
    grpc::ServerBidiReactor<Msg, Msg>* streamEcho(
            ::grpc::CallbackServerContext* context) override;
};

class AsyncTestEchoService
    : public TestEcho::WithCallbackMethod_serverStreamData<
              TestEcho::WithCallbackMethod_streamEcho<TestEchoServiceImpl>> {
public:
    grpc::ServerBidiReactor<Msg, Msg>* streamEcho(
            ::grpc::CallbackServerContext* context) override;
    ::grpc::ServerWriteReactor<Msg>* serverStreamData(
            ::grpc::CallbackServerContext* context,
            const Msg* request) override;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
