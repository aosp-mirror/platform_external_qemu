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
#include "android/emulation/control/utils/SimpleAsyncGrpc.h"
#include "test_echo_service.pb.h"

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

grpc::ServerBidiReactor<Msg, Msg>* AsyncTestEchoService::streamEcho(
        ::grpc::CallbackServerContext* context) {
    class StreamEchoHandler : public SimpleBidiStream<Msg, Msg> {
    public:
        StreamEchoHandler(AsyncTestEchoService* parent) : mParent(parent) {}
        void Read(const Msg* msg) override { Write(*msg); }

        void OnDone() override {
            mParent->plusOne();
            delete this;
        }

        void OnCancel() override { Finish(grpc::Status::CANCELLED); }

    private:
        AsyncTestEchoService* mParent;
    };

    return new StreamEchoHandler(this);
}

::grpc::ServerWriteReactor<Msg>* AsyncTestEchoService::serverStreamData(
        ::grpc::CallbackServerContext* context,
        const Msg* request) {
    class StreamDataHandler : public SimpleWriter<Msg> {
    public:
        StreamDataHandler(AsyncTestEchoService* parent, const Msg* received)
            : mParent(parent) {
            Msg reply;
            reply.set_msg("Hello!");
            for (int i = 0; i < received->counter(); i++) {
                reply.set_counter(i + 1);
                Write(reply);
            }
        }

        void OnDone() override {
            mParent->plusOne();
            delete this;
        }

        void OnCancel() override { Finish(grpc::Status::CANCELLED); }

    private:
        AsyncTestEchoService* mParent;
    };

    return new StreamDataHandler(this, request);
}

}  // namespace control
}  // namespace emulation
}  // namespace android
