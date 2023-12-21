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
#include "android/emulation/control/utils/SnapshotClient.h"

#include <grpcpp/grpcpp.h>
#include <tuple>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "aemu/base/logging/Log.h"
#include "android/emulation/control/utils/GenericCallbackFunctions.h"
#include "android/grpc/utils/SimpleAsyncGrpc.h"

namespace android {
namespace emulation {
namespace control {

void SnapshotServiceClient::ListSnapshotsAsync(
        SnapshotFilter filter,
        OnCompleted<SnapshotList> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<SnapshotFilter, SnapshotList>(mClient);
    request->CopyFrom(filter);
    mService->async()->ListSnapshots(
            context.get(), request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SnapshotServiceClient::ListAllSnapshotsAsync(
        OnCompleted<SnapshotList> onDone) {
    SnapshotFilter filter;
    filter.set_statusfilter(SnapshotFilter::All);
    ListSnapshotsAsync(filter, onDone);
}

void SnapshotServiceClient::DeleteSnapshotAsync(
        std::string id,
        OnCompleted<SnapshotPackage> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<SnapshotPackage, SnapshotPackage>(mClient);
    request->set_snapshot_id(id);
    mService->async()->DeleteSnapshot(
            context.get(), request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SnapshotServiceClient::SaveSnapshotAsync(
        std::string id,
        OnCompleted<SnapshotPackage> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<SnapshotPackage, SnapshotPackage>(mClient);
    request->set_snapshot_id(id);

    mService->async()->SaveSnapshot(
            context.get(), request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SnapshotServiceClient::LoadSnapshotAsync(
        std::string id,
        OnCompleted<SnapshotPackage> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<SnapshotPackage, SnapshotPackage>(mClient);
    request->set_snapshot_id(id);

    mService->async()->LoadSnapshot(
            context.get(), request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SnapshotServiceClient::UpdateSnapshotAsync(
        SnapshotUpdateDescription details,
        OnCompleted<SnapshotDetails> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<SnapshotUpdateDescription,
                                     SnapshotDetails>(mClient);
    request->CopyFrom(details);
    mService->async()->UpdateSnapshot(
            context.get(), request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SnapshotServiceClient::GetScreenshotAsync(
        std::string id,
        OnCompleted<SnapshotScreenshotFile> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<SnapshotId, SnapshotScreenshotFile>(
                    mClient);
    request->set_snapshot_id(id);
    mService->async()->GetScreenshot(
            context.get(), request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

}  // namespace control
}  // namespace emulation
}  // namespace android