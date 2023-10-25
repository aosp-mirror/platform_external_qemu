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
#include "android/emulation/control/utils/CarClient.h"

#include "android/emulation/control/utils/GenericCallbackFunctions.h"
#include "android/grpc/utils/SimpleAsyncGrpc.h"
#include "google/protobuf/empty.pb.h"

namespace android {
namespace emulation {
namespace control {

// #define DEBUG 1
#if DEBUG >= 1
#define DD(fmt, ...)                                               \
    dinfo("SimpleCarClient: %s:%d| " fmt "\n", __func__, __LINE__, \
          ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

using ::google::protobuf::Empty;

void CarClient::sendCarEventAsync(incubating::CarEvent event) {
    // TODO(jansene): Should we use a single "permanent" stream?
    auto [request, response, context] =
            createGrpcRequestContext<incubating::CarEvent, Empty>(mClient);
    request->CopyFrom(event);

    static OnCompleted<Empty> nop = [](auto ignored) {};
    mService->async()->sendCarEvent(
            context.get(), request, response,
            grpcCallCompletionHandler(context, request, response, nop));
}

void CarClient::receiveCarEvents(OnEvent<CarEvent> incoming,
                                       OnFinished onDone) {
    auto context = mClient->newContext();
    static google::protobuf::Empty empty;
    auto read = new SimpleClientLambdaReader<CarEvent>(
            context, incoming, [onDone](auto status) {
                onDone(ConvertGrpcStatusToAbseilStatus(status));
            });
    mService->async()->receiveCarEvents(context.get(), &empty, read);
    read->StartRead();
    read->StartCall();
}

}  // namespace control
}  // namespace emulation
}  // namespace android