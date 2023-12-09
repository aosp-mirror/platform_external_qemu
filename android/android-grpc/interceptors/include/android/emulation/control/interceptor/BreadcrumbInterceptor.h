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
#pragma once
#include <grpcpp/grpcpp.h>

namespace android {
namespace control {
namespace interceptor {

using grpc::experimental::ClientRpcInfo;
using grpc::experimental::Interceptor;
using grpc::experimental::InterceptorBatchMethods;
using grpc::experimental::ServerRpcInfo;

// A BreadcrumbInterceptor will create breadcrumbs crash annotations for the
// gRPC calls that are being made. They basically provide you some insight on
// what is going on in the gRPC stack. If crashes happen during a gRPC call
// it might give you an idea of which call was active.
//
// We basically track 2 things:
//
// the grpc tag will track active calls.
// on the individual thread we will track the status of the call.
//
// The crumbs will have the following format:
//
// [phase][crc32][timestamp]
//
// Which will be base64 encoded. They can be decoded by the
// `android/scripts/gen-grpc-sql.py` script. For example:
//
// python android/scripts/gen-grpc-sql.py android/android-grpc/services
// -decode  'annotation_objects["2410469"] = PLxafuJ0o3NlAAAAAA=='
// ('Incoming server call',
// '/android.emulation.control.EmulatorController/sendMouse',
// datetime.datetime(2023, 12, 8, 15, 15))
class BreadcrumbInterceptor : public grpc::experimental::Interceptor {
public:
    explicit BreadcrumbInterceptor(ClientRpcInfo* info);
    explicit BreadcrumbInterceptor(ServerRpcInfo* info);
    ~BreadcrumbInterceptor() override;

    void Intercept(InterceptorBatchMethods* methods) override;

private:
    std::uint32_t mCrc;
};

// The factory class that needs to be registered with the gRPC server/client.
class BreadcrumbInterceptorFactory
    : public grpc::experimental::ServerInterceptorFactoryInterface,
      public grpc::experimental::ClientInterceptorFactoryInterface {
public:
    BreadcrumbInterceptorFactory();
    ~BreadcrumbInterceptorFactory() override = default;
    Interceptor* CreateServerInterceptor(ServerRpcInfo* info) override;
    Interceptor* CreateClientInterceptor(ClientRpcInfo* info) override;
};
}  // namespace interceptor
}  // namespace control
}  // namespace android
