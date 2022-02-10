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

#include <functional>  // for __base
#include <iostream>    // for operator<<

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

void registerAsyncStreamEcho(AsyncGrpcHandler* handler,
                     AsyncTestEchoService* testService) {
    handler->registerConnectionHandler(testService,
                                       &AsyncTestEchoService::RequeststreamEcho)
            .withCallback([testService](auto connection) {
                std::cout << "Received a connection from: "
                          << connection->peer() << std::endl;
                connection->setReadCallback([](auto from, auto received) {
                    std::cout << "Received: " << received.DebugString();
                    from->write(received);
                });
                connection->setCloseCallback([testService](auto connection) {
                    std::cout << "Closed: " << connection->peer();
                    testService->plusOne();
                });
            });
}

void registerAsyncAnotherTestEchoService(AsyncGrpcHandler* handler,
                     AsyncAnotherTestEchoService* testService) {
    handler->registerConnectionHandler(testService,
                                       &AsyncAnotherTestEchoService::RequestanotherStreamEcho)
            .withCallback([testService](auto connection) {
                std::cout << "RequestanotherStreamEcho incoming: "
                          << connection->peer() << std::endl;
                connection->setReadCallback([](auto from, auto received) {
                    std::cout << "RequestanotherStreamEcho received: " << received.DebugString();
                    received.set_counter(-received.counter());
                    from->write(received);
                });
                connection->setCloseCallback([testService](auto connection) {
                    std::cout << "RequestanotherStreamEcho Closed: " << connection->peer();
                    testService->plusOne();
                });
            });
}

}  // namespace control
}  // namespace emulation
}  // namespace android
