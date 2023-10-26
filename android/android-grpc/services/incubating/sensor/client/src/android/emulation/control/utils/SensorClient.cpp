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
#include "android/emulation/control/utils/SensorClient.h"

#include <grpcpp/grpcpp.h>

#include "android/emulation/control/utils/GenericCallbackFunctions.h"
#include "android/grpc/utils/SimpleAsyncGrpc.h"
#include "google/protobuf/empty.pb.h"
#include "sensor_service.pb.h"

namespace android {
namespace emulation {
namespace control {

// #define DEBUG 1
#if DEBUG >= 1
#define DD(fmt, ...)                                                  \
    dinfo("SensorClient: %s:%d| " fmt "\n", __func__, __LINE__, \
          ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

using ::google::protobuf::Empty;
using incubating::PhysicalModelValue;
using incubating::SensorValue;

void SensorClient::setPhysicalModelAsync(const PhysicalModelValue value,
                                               OnCompleted<Empty> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<PhysicalModelValue, Empty>(mClient);
    request->CopyFrom(value);
    mService->async()->setPhysicalModel(
            context.get(), request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SensorClient::setSensorValueAsync(const SensorValue value,
                                             OnCompleted<Empty> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<SensorValue, Empty>(mClient);
    request->CopyFrom(value);
    mService->async()->setSensor(
            context.get(), request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

absl::StatusOr<PhysicalModelValue> SensorClient::getPhysicalModel(
        int parameterId,
        int type) {
    PhysicalModelValue request;
    request.set_target((PhysicalModelValue::PhysicalType)(parameterId + 1));
    request.set_value_type((PhysicalModelValue::ParameterValueType)(type + 1));
    PhysicalModelValue response;

    auto context = mClient->newContext();
    auto status = mService->getPhysicalModel(context.get(), request, &response);
    if (!status.ok()) {
        return ConvertGrpcStatusToAbseilStatus(status);
    }
    return response;
}

absl::StatusOr<SensorValue> SensorClient::getSensorValue(int sensorId) {
    SensorValue request;
    request.set_target((SensorValue::SensorType)(sensorId + 1));
    SensorValue response;

    auto context = mClient->newContext();
    auto status = mService->getSensor(context.get(), request, &response);
    if (!status.ok()) {
        return ConvertGrpcStatusToAbseilStatus(status);
    }
    return response;
}

void SensorClient::receivePhysicalModelEvents(
        PhysicalModelValue value,
        OnEvent<PhysicalModelValue> incoming,
        OnFinished onDone) {
    PhysicalModelValue* request = new PhysicalModelValue();
    request->CopyFrom(value);
    auto context = mClient->newContext();

    auto read = new SimpleClientLambdaReader<PhysicalModelValue>(
            context, incoming,  [onDone, request](auto status) {
                delete request;
                onDone(ConvertGrpcStatusToAbseilStatus(status));
            });
    mService->async()->receivePhysicalModelEvents(context.get(), request, read);
    read->StartRead();
    read->StartCall();
}

void SensorClient::receivePhysicalStateEvents(
        OnEvent<PhysicalStateEvent> incoming,
        OnFinished onDone) {
    auto context = mClient->newContext();
    static google::protobuf::Empty empty;
    auto read = new SimpleClientLambdaReader<PhysicalStateEvent>(
            context, incoming,  [onDone](auto status) {
                onDone(ConvertGrpcStatusToAbseilStatus(status));
            });
    mService->async()->receivePhysicalStateEvents(context.get(), &empty, read);
    read->StartRead();
    read->StartCall();
}

}  // namespace control
}  // namespace emulation
}  // namespace android