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
#include <grpcpp/grpcpp.h>  // for string_ref
#include <memory>           // for unique_ptr
#include <string>           // for string
#include <vector>           // for vector

#include "absl/status/status.h"  // for Status
#include "android/emulation/control/secure/AllowList.h"

namespace android {
namespace emulation {
namespace control {

/**
 * @brief An implementation of AuthMetadataProcessor that validates a token in a
 * specified header.
 *
 * The BasicTokenAuth class is an implementation of AuthMetadataProcessor that
 * is designed to search for a specific headerand validate the token contained
 * in it. Subclasses must implement the isTokenValid method to determine whether
 * the token is valid or not. The first matching header will be used for
 * validation, and headers will not be removed.
 *
 * Note that headers must be in lowercase, as the gRPC engine rejects headers
 * that do not conform to the HTTP/2 standard.
 *
 * @see grpc::AuthMetadataProcessor
 */
class BasicTokenAuth : public grpc::AuthMetadataProcessor {
public:
    /**
     * Creates a AuthMetadataProcesser that looks for the given header and
     * removes the given prefix before invoking the isTokenValid call.
     *
     * @param header The header to look for.
     * @param list The allow list to use.
     */
    BasicTokenAuth(std::string header, AllowList* list);
    virtual ~BasicTokenAuth();

    /// context is read/write: it contains the properties of the channel peer
    /// and
    /// it is the job of the Process method to augment it with properties
    /// derived from the passed-in auth_metadata. consumed_auth_metadata needs
    /// to be filled with metadata that has been consumed by the processor and
    /// will be removed from the call. response_metadata is the metadata that
    /// will be sent as part of the response. If the return value is not
    /// Status::OK, the rpc call will be aborted with the error code and error
    /// message sent back to the client.
    grpc::Status Process(const InputMetadata& auth_metadata,
                         grpc::AuthContext* context,
                         OutputMetadata* consumed_auth_metadata,
                         OutputMetadata* response_metadata) override;

    // This method should return `absl::OkStatus()` in case of success, or
    // provide a more detailed explanation of the validation faluire.
    virtual absl::Status isTokenValid(std::string_view path,
                                      std::string_view token) = 0;

    virtual bool canHandleToken(std::string_view token) { return false; }

    // The name of this validator.
    virtual std::string name() = 0;

    // Note that the header should be in lower case (gRPC uses HTTP/2)
    //
    // RFC 2616 - "Hypertext Transfer Protocol -- HTTP/1.1", Section 4.2:
    // "Message Headers": Field names are case-insensitive.
    //
    // RFC 7540 - "Hypertext Transfer Protocol Version 2 (HTTP/2)",
    // Section 8.1.2:
    // However, header field names MUST be converted to lowercase
    // prior to their encoding in HTTP/2.
    const static inline std::string DEFAULT_HEADER{"authorization"};

    // The metadata that contains the path of the method that is being invoked.
    const static inline std::string PATH{":path"};

    AllowList* allowList() { return mAllowList; }

private:
    AllowList* mAllowList{&noAccess};
    std::string mHeader;

    static DisableAccess noAccess;
};

// A class that validates that the header:
//
// authorization: Bearer <token>
//
// is present and matches the token used when creating this class.
//
class StaticTokenAuth : public BasicTokenAuth {
public:
    StaticTokenAuth(std::string token, std::string iss, AllowList* list);
    ~StaticTokenAuth() = default;

    bool canHandleToken(std::string_view token) override;

    absl::Status isTokenValid(std::string_view path,
                              std::string_view token) override;

    const static inline std::string DEFAULT_BEARER{"Bearer "};

    std::string name() override { return "StaticTokenAuth"; }

private:
    std::string mStaticToken;
    std::string mIssuer;
};

// A class that will validate that any of the provided validators
// can validate the header:
//
// authorization: Bearer <token>
//
// Only one of the validators has to succeed.
class AnyTokenAuth : public BasicTokenAuth {
public:
    AnyTokenAuth(std::vector<std::unique_ptr<BasicTokenAuth>> validators,
                 AllowList* list);
    AnyTokenAuth(std::vector<BasicTokenAuth*> validators, AllowList* list);
    ~AnyTokenAuth() = default;

    bool canHandleToken(std::string_view token) override;
    absl::Status isTokenValid(std::string_view path,
                              std::string_view token) override;

    std::string name() override { return "AnyTokenAuth"; }

private:
    std::vector<BasicTokenAuth*> mValidators;
    std::vector<std::unique_ptr<BasicTokenAuth>> mUniqueValidators;
};
}  // namespace control
}  // namespace emulation
}  // namespace android
