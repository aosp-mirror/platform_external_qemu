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
#include "android/emulation/control/utils/SimpleEmulatorControlClient.h"

#include <grpcpp/grpcpp.h>
#include <tuple>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "aemu/base/logging/CLog.h"
#include "android/emulation/control/utils/GenericCallbackFunctions.h"
#include "android/grpc/utils/SimpleAsyncGrpc.h"

// #define DEBUG 0
/* set  >1 for very verbose debugging */
#if DEBUG <= 1
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif

namespace android {
namespace emulation {
namespace control {

void SimpleEmulatorControlClient::setBattery(BatteryState state,
                                             OnCompleted<Empty> onDone) {
    DD("setBattery %s", state.ShortDebugString().c_str());
    auto [request, response, context] =
            createGrpcRequestContext<BatteryState, Empty>(mClient);
    request->CopyFrom(state);
    mService->async()->setBattery(
            context.get(), request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SimpleEmulatorControlClient::getScreenshot(ImageFormat format,
                                                OnCompleted<Image> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<ImageFormat, Image>(mClient);
    request->CopyFrom(format);
    mService->async()->getScreenshot(
            context.get(), request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SimpleEmulatorControlClient::getStatus(
        OnCompleted<EmulatorStatus> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<Empty, EmulatorStatus>(mClient);
    mService->async()->getStatus(
            context.get(), request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SimpleEmulatorControlClient::getDisplayConfigurations(
        OnCompleted<DisplayConfigurations> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<Empty, DisplayConfigurations>(mClient);
    mService->async()->getDisplayConfigurations(
            context.get(), request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SimpleEmulatorControlClient::setDisplayConfigurations(
        DisplayConfigurations state,
        OnCompleted<DisplayConfigurations> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<DisplayConfigurations,
                                     DisplayConfigurations>(mClient);
    request->CopyFrom(state);
    mService->async()->setDisplayConfigurations(
            context.get(), request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SimpleEmulatorControlClient::receiveEmulatorNotificationEvents(OnEvent<Notification> incoming,
                                           OnFinished onDone) {
    auto context = mClient->newContext();
    static google::protobuf::Empty empty;
    auto read = new SimpleClientLambdaReader<Notification>(
            incoming, context, [onDone](auto status) {
                onDone(ConvertGrpcStatusToAbseilStatus(status));
            });
    mService->async()->streamNotification(context.get(), &empty, read);
    read->StartRead();
    read->StartCall();
}

void SimpleEmulatorControlClient::sendFingerprint(Fingerprint finger,
                                                  OnCompleted<Empty> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<Fingerprint, Empty>(mClient);
    request->CopyFrom(finger);
    mService->async()->sendFingerprint(
            context.get(), request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SimpleEmulatorControlClient::sendKey(KeyboardEvent key,
                                          OnCompleted<Empty> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<KeyboardEvent, Empty>(mClient);
    request->CopyFrom(key);
    mService->async()->sendKey(
            context.get(), request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

std::unique_ptr<SimpleClientWriter<InputEvent>>
SimpleEmulatorControlClient::streamInputEvent() {
    static Empty empty;
    auto context = mClient->newContext();
    auto writer = std::make_unique<SimpleClientWriter<InputEvent>>(std::move(context));
    mService->async()->streamInputEvent(writer->context(), &empty,
                                        writer.get());
    return writer;
}

}  // namespace control
}  // namespace emulation
}  // namespace android