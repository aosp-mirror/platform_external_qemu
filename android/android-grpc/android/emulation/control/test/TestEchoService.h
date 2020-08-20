
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
#include <grpcpp/grpcpp.h>

#include <vector>

#include "test_echo_service.grpc.pb.h"  // for TestEcho
#include "test_echo_service.pb.h"       // for Msg

namespace android {
namespace emulation {
namespace control {

using grpc::ServerContext;
using grpc::Status;

class TestEchoServiceImpl final : public TestEcho::Service {
public:
    Status echo(ServerContext* context,
                const Msg* request,
                Msg* response) override;

    Status data(ServerContext* context,
                const ::google::protobuf::Empty* empty,
                Msg* response) override;

    int invocations() { return mCounter; }

    void moveData(std::vector<uint8_t> data) { mData = std::move(data); }

private:
    int mCounter{0};
    std::vector<uint8_t> mData;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
