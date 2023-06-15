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
#include "android/emulation/control/utils/SimpleModemClient.h"

#include <assert.h>
#include <grpcpp/grpcpp.h>
#include <string>

#include "absl/status/status.h"
#include "aemu/base/logging/CLog.h"
#include "android/emulation/control/utils/GenericCallbackFunctions.h"
#include "android/grpc/utils/SimpleAsyncGrpc.h"

namespace android {
namespace emulation {
namespace control {

// #define DEBUG 1
#if DEBUG >= 1
#define DD(fmt, ...)                                                 \
    dinfo("SimpleModemClient: %s:%d| " fmt "\n", __func__, __LINE__, \
          ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

using ::google::protobuf::Empty;

void SimpleModemClient::receiveSmsAsync(SmsMessage message,
                                        OnCompleted<Empty> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<SmsMessage, Empty>(mClient);
    request->CopyFrom(message);
    mService->async()->receiveSms(
            context, request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SimpleModemClient::createCall(Call call, OnCompleted<Call> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<Call, Call>(mClient);
    request->CopyFrom(call);
    mService->async()->createCall(
            context, request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SimpleModemClient::updateCall(Call call, OnCompleted<Call> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<Call, Call>(mClient);
    request->CopyFrom(call);
    mService->async()->updateCall(
            context, request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SimpleModemClient::deleteCall(Call call, OnCompleted<Empty> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<Call, Empty>(mClient);
    request->CopyFrom(call);
    mService->async()->deleteCall(
            context, request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SimpleModemClient::updateTime(
        OnCompleted<::google::protobuf::Empty> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<Empty, Empty>(mClient);
    mService->async()->updateClock(
            context, request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SimpleModemClient::setCellInfo(CellInfo info,
                                    OnCompleted<CellInfo> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<CellInfo, CellInfo>(mClient);
    request->CopyFrom(info);
    mService->async()->setCellInfo(
            context, request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SimpleModemClient::getCellInfo(OnCompleted<CellInfo> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<Empty, CellInfo>(mClient);
    mService->async()->getCellInfo(
            context, request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SimpleModemClient::receivePhoneEvents(OnEvent<PhoneEvent> incoming,
                                           OnFinished onDone) {
    grpc::ClientContext* context = mClient->newContext().release();
    static google::protobuf::Empty empty;
    auto read = new SimpleClientLambdaReader<PhoneEvent>(
            incoming, [onDone, context](auto status) {
                onDone(ConvertGrpcStatusToAbseilStatus(status));
                delete context;
            });
    mService->async()->receivePhoneEvents(context, &empty, read);
    read->StartRead();
    read->StartCall();
}

}  // namespace control
}  // namespace emulation
}  // namespace android