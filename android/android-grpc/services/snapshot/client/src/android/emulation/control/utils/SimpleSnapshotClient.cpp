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
#include "android/emulation/control/utils/SimpleSnapshotClient.h"

#include <grpcpp/grpcpp.h>
#include <tuple>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "aemu/base/logging/CLog.h"
#include "android/grpc/utils/SimpleAsyncGrpc.h"

namespace android {
namespace emulation {
namespace control {

/**
 * Convert a gRPC status object to an Abseil status object.
 *
 * @param grpc_status The gRPC status object to convert.
 * @return The equivalent Abseil status object.
 */
static absl::Status ConvertGrpcStatusToAbseilStatus(
        const grpc::Status& grpc_status) {
    return absl::Status(static_cast<absl::StatusCode>(grpc_status.error_code()),
                        grpc_status.error_message());
}

template <typename T>
using OnCompleted = std::function<void(absl::StatusOr<T*>)>;

/**
 * Creates a callback function that is responsible for invoking the
 * onDone callback with the given response object when the gRPC call completes.
 * The function also performs cleanup of unused resources, such as the client
 * context, request, and response objects. If the gRPC call fails, the function
 * converts the grpc::Status object to an absl::StatusOr and passes it to the
 * onDone callback.
 *
 * @tparam Request The type of the gRPC request message.
 * @tparam Response The type of the gRPC response message.
 * @param context A pointer to the client context used to make the gRPC call.
 * @param request A pointer to the request message object used in the gRPC call.
 * @param response A pointer to the response message object used in the gRPC
 * call.
 * @param onDone The callback function to invoke when the gRPC call completes or
 * fails.
 *
 * @return A std::function object that can be used as the completion callback
 * for the gRPC call.
 */
template <class Request, class Response>
std::function<void(::grpc::Status)> grpcCallCompletionHandler(
        grpc::ClientContext* context,
        Request* request,
        Response* response,
        OnCompleted<Response> onDone) {
    return [context, request, response, onDone](::grpc::Status status) {
        if (status.ok()) {
            // Call completed successfully, invoke the onDone callback with the
            // response object.
            onDone(response);
        } else {
            // Call failed somehow, convert the grpc::Status object to an
            // absl::StatusOr and pass it to the onDone callback.
            dwarning("Failed to complete call to %s due to: %s",
                     context->peer().c_str(), status.error_message().c_str());
            onDone(ConvertGrpcStatusToAbseilStatus(status));
        }

        // Cleanup the resources used by the gRPC call.
        delete context;
        delete request;
        delete response;
    };
}

/**
 * Creates a new gRPC request and client context for the given
 * EmulatorGrpcClient. The returned objects should be used to make an
 * asynchronous call to the gRPC service. The caller is responsible for deleting
 * the returned objects after the call has completed.
 *
 * @tparam Request The request message type.
 * @tparam Response The response message type.
 * @param client The EmulatorGrpcClient instance to use for the call.
 * @return A tuple containing pointers to the request and response objects, and
 * a unique_ptr to the client context object.
 */
template <class Request, class Response>
std::tuple<Request*, Response*, grpc::ClientContext*> createGrpcRequestContext(
        const std::shared_ptr<EmulatorGrpcClient>& client) {
    auto request = new Request();
    auto response = new Response();
    auto context = client->newContext();
    return std::make_tuple(request, response, context.release());
}

void SimpleSnapshotServiceClient::ListSnapshotsAsync(
        SnapshotFilter filter,
        OnCompleted<SnapshotList> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<SnapshotFilter, SnapshotList>(mClient);
    request->CopyFrom(filter);
    mService->async()->ListSnapshots(
            context, request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SimpleSnapshotServiceClient::ListAllSnapshotsAsync(
        OnCompleted<SnapshotList> onDone) {
    SnapshotFilter filter;
    filter.set_statusfilter(SnapshotFilter::All);
    ListSnapshotsAsync(filter, onDone);
}

void SimpleSnapshotServiceClient::DeleteSnapshotAsync(
        std::string id,
        OnCompleted<SnapshotPackage> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<SnapshotPackage, SnapshotPackage>(mClient);
    request->set_snapshot_id(id);
    mService->async()->DeleteSnapshot(
            context, request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SimpleSnapshotServiceClient::SaveSnapshotAsync(
        std::string id,
        OnCompleted<SnapshotPackage> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<SnapshotPackage, SnapshotPackage>(mClient);
    request->set_snapshot_id(id);

    mService->async()->SaveSnapshot(
            context, request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SimpleSnapshotServiceClient::LoadSnapshotAsync(
        std::string id,
        OnCompleted<SnapshotPackage> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<SnapshotPackage, SnapshotPackage>(mClient);
    request->set_snapshot_id(id);

    mService->async()->LoadSnapshot(
            context, request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}


void SimpleSnapshotServiceClient::UpdateSnapshotAsync(
        SnapshotUpdateDescription details,
        OnCompleted<SnapshotDetails> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<SnapshotUpdateDescription, SnapshotDetails>(mClient);
    request->CopyFrom(details);
    mService->async()->UpdateSnapshot(
            context, request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

void SimpleSnapshotServiceClient::GetScreenshotAsync(
        std::string id,
        OnCompleted<SnapshotScreenshotFile> onDone) {
    auto [request, response, context] =
            createGrpcRequestContext<SnapshotId, SnapshotScreenshotFile>(mClient);
    request->set_snapshot_id(id);
    mService->async()->GetScreenshot(
            context, request, response,
            grpcCallCompletionHandler(context, request, response, onDone));
}

}  // namespace control
}  // namespace emulation
}  // namespace android