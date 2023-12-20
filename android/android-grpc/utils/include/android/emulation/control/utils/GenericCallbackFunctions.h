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
#include <functional>
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "aemu/base/logging/Log.h"
#include "android/emulation/control/utils/EmulatorGrcpClient.h"
#include "google/protobuf/empty.pb.h"
namespace android {
namespace emulation {
namespace control {

template <typename T>
using OnCompleted = std::function<void(absl::StatusOr<T*>)>;
using OnFinished = std::function<void(absl::Status)>;
template <typename T>
using OnEvent = std::function<void(const T*)>;

using ::google::protobuf::Empty;
static OnCompleted<Empty> nothing = [](absl::StatusOr<Empty*> _ignored) {};
/**
 * Convert a gRPC status object to an Abseil status object.
 *
 * @param grpc_status The gRPC status object to convert.
 * @return The equivalent Abseil status object.
 */
static absl::Status ConvertGrpcStatusToAbseilStatus(
        const ::grpc::Status& grpc_status) {
    return absl::Status(static_cast<absl::StatusCode>(grpc_status.error_code()),
                        grpc_status.error_message());
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
std::tuple<Request*, Response*, std::shared_ptr<grpc::ClientContext>>
createGrpcRequestContext(const std::shared_ptr<EmulatorGrpcClient>& client) {
    auto request = new Request();
    auto response = new Response();
    auto context = client->newContext();
    return std::make_tuple(request, response, std::move(context));
}

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
        std::shared_ptr<grpc::ClientContext> context,
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
        delete request;
        delete response;
    };
}
}  // namespace control
}  // namespace emulation
}  // namespace android