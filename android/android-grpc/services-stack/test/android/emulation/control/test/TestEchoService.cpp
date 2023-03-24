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

#include "android/emulation/control/test/TestEchoService.h"

#include <grpcpp/grpcpp.h>
#include "test_echo_service.pb.h"

#ifndef DISABLE_ASYNC_GRPC
#include "android/grpc/utils/SimpleAsyncGrpc.h"
#else
#include "android/grpc/utils/SyncToAsyncAdapter.h"
#endif

using grpc::ServerContext;
using grpc::Status;

namespace android {
namespace emulation {
namespace control {

Status TestEchoServiceBase::echo(ServerContext* context,
                                 const Msg* request,
                                 Msg* response) {
    response->set_counter(mCounter++);
    response->set_msg(request->msg());
    return Status::OK;
}

Status TestEchoServiceBase::data(ServerContext* context,
                                 const ::google::protobuf::Empty* empty,
                                 Msg* response) {
    response->set_counter(mCounter++);
    response->set_data(mData.data(), mData.size());
    return Status::OK;
}

class StreamEchoHandler : public SimpleServerBidiStream<Msg, Msg> {
public:
    StreamEchoHandler(TestEchoServiceBase* parent) : mParent(parent) {}
    void Read(const Msg* msg) override {
        Msg reply;
        reply.set_msg("Hello!");
        for (int i = 0; i < msg->counter(); i++) {
            reply.set_counter(i + 1);
            Write(reply);
        }
    }

    void OnDone() override {
        mParent->plusOne();
        delete this;
    }

private:
    TestEchoServiceBase* mParent;
};

#ifndef DISABLE_ASYNC_GRPC
grpc::ServerBidiReactor<Msg, Msg>* AsyncTestEchoService::streamEcho(
        ::grpc::CallbackServerContext* context) {
    return new StreamEchoHandler(this);
}
#else
::grpc::Status AsyncTestEchoService::streamEcho(
        ::grpc::ServerContext* context,
        ::grpc::ServerReaderWriter<Msg, Msg>* stream) {
    return BidiRunner<StreamEchoHandler>(stream, this).status();
};
#endif

}  // namespace control
}  // namespace emulation
}  // namespace android
