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

#include <grpcpp/grpcpp.h>  // for Status

#include "test_echo_service.grpc.pb.h"  // for TestEcho
#include "test_echo_service.pb.h"       // for Msg

using grpc::ServerContext;
using grpc::Status;

namespace android {
namespace emulation {
namespace control {

Status TestEchoServiceImpl::echo(ServerContext* context,
                                 const Msg* request,
                                 Msg* response)  {
    response->set_counter(mCounter++);
    response->set_msg(request->msg());
    return Status::OK;
}

Status TestEchoServiceImpl::data(ServerContext* context,
                                 const ::google::protobuf::Empty* empty,
                                 Msg* response)  {
    response->set_counter(mCounter++);
    response->set_data(mData.data(), mData.size());
    return Status::OK;
}



}  // namespace control
}  // namespace emulation
}  // namespace android
