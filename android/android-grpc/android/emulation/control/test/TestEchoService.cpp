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

#include "android/emulation/control/test/TestEchoService.h"  // for Service

#include <functional>                                         // for __base
#include <iostream>                                           // for operator<<

#include "android/emulation/control/async/AsyncGrcpStream.h"  // for AsyncGr...
#include "grpcpp/impl/codegen/async_stream.h"                 // for ServerA...
#include "test_echo_service.pb.h"                             // for Msg

namespace google {
namespace protobuf {
class Empty;
}  // namespace protobuf
}  // namespace google

using grpc::ServerContext;
using grpc::Status;

namespace android {
namespace emulation {
namespace control {

Status TestEchoServiceImpl::echo(ServerContext* context,
                                 const Msg* request,
                                 Msg* response) {
    response->set_counter(mCounter++);
    response->set_msg(request->msg());
    return Status::OK;
}

Status TestEchoServiceImpl::data(ServerContext* context,
                                 const ::google::protobuf::Empty* empty,
                                 Msg* response) {
    response->set_counter(mCounter++);
    response->set_data(mData.data(), mData.size());
    return Status::OK;
}

std::unique_ptr<AsyncGrpcHandler<Msg, Msg>> asyncStreamEcho(
        AsyncTestEchoService* testService,
        const std::vector<::grpc::ServerCompletionQueue*>& cqs) {
    return std::make_unique<AsyncGrpcHandler<Msg, Msg>>(
            cqs,
            [testService](ServerContext* context,
                          ServerAsyncReaderWriter<Msg, Msg>* stream,
                          ::grpc::CompletionQueue* new_call_cq,
                          ::grpc::ServerCompletionQueue* notification_cq,
                          void* tag) {
                testService->RequeststreamEcho(context, stream, new_call_cq,
                                               notification_cq, tag);
            },
            [testService](auto connection) {
                std::cout << "Received a connection from: "
                          << connection->peer() << std::endl;
                connection->setReadCallback(
                        [](auto connection, const Msg& received) {
                            std::cout << "Received: " << received.DebugString();
                            connection->write(received);
                        });
                connection->setCloseCallback([testService](auto connection) {
                    std::cout << "Closed: " << connection->peer();
                    testService->plusOne();
                });
            });
}

}  // namespace control
}  // namespace emulation
}  // namespace android
