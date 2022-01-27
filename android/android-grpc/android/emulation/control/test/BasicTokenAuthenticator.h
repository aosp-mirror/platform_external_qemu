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
#include <grpcpp/grpcpp.h>  // for Status

#include <string>  // for string

#include "absl/status/statusor.h"  // for StatusOr
#include "android/emulation/control/secure/BasicTokenAuth.h"
#include "tink/config/tink_config.h"        // for TinkConfig
#include "tink/jwt/jwk_set_converter.h"     // for JwkSetFromPublicKeyset...
#include "tink/jwt/jwt_key_templates.h"     // for JwtEs512Template
#include "tink/jwt/jwt_public_key_sign.h"   // for JwtPublicKeySign
#include "tink/jwt/jwt_signature_config.h"  // for JwtSignatureRegister
#include "tink/jwt/jwt_validator.h"         // for JwtValidator, JwtValid...
#include "tink/jwt/raw_jwt.h"               // for RawJwt, RawJwtBuilder
#include "tink/keyset_handle.h"             // for KeysetHandle
#include "tink/util/status.h"               // for Status
#include "tink/util/statusor.h"             // for StatusOr

using android::base::pj;
namespace tink = crypto::tink;
using crypto::tink::TinkConfig;

namespace android {
namespace emulation {
namespace control {

using ::android::emulation::control::BasicTokenAuth;

// A credentials plugin that injects the given token when making a gRPC call.
class BasicTokenAuthenticator : public grpc::MetadataCredentialsPlugin {
public:
    BasicTokenAuthenticator(const grpc::string& token)
        : mToken("Bearer " + token) {}
    ~BasicTokenAuthenticator() = default;

    grpc::Status GetMetadata(
            grpc::string_ref service_url,
            grpc::string_ref method_name,
            const grpc::AuthContext& channel_auth_context,
            std::multimap<grpc::string, grpc::string>* metadata) override {
        metadata->insert(
                std::make_pair(BasicTokenAuth::DEFAULT_HEADER, mToken));
        return grpc::Status::OK;
    }

private:
    grpc::string mToken;
};

class JwtTokenAuthenticator : public grpc::MetadataCredentialsPlugin {
public:
    JwtTokenAuthenticator(const grpc::string& path) : mPath(path) {
        auto status = TinkConfig::Register();
        assert(status.ok());
        mPrivateKey = std::move(writeEs512(pj(path, "unittest.jwk")));
    }
    ~JwtTokenAuthenticator() = default;

    grpc::Status GetMetadata(
            grpc::string_ref service_url,
            grpc::string_ref method_name,
            const grpc::AuthContext& channel_auth_context,
            std::multimap<grpc::string, grpc::string>* metadata) override {
        // Reconstruct the path, this i.e. /a/b/c/${method_name}
        std::string url(service_url.begin(), service_url.end());
        std::string method(method_name.begin(), method_name.end());
        std::size_t found = url.rfind("/");
        auto aud = absl::StrFormat(
                "%s/%s", std::string(url.begin() + found, url.end()), method);

        absl::Time now = absl::Now();

        auto raw_jwt = tink::RawJwtBuilder()
                               .SetIssuer("JwkTokenAuthTest")
                               .SetAudience(aud)
                               .SetExpiration(now + absl::Seconds(300))
                               .SetIssuedAt(now)
                               .Build();
        auto sign = mPrivateKey->GetPrimitive<tink::JwtPublicKeySign>();
        auto token = (*sign)->SignAndEncode(*raw_jwt);
        metadata->insert(std::make_pair(BasicTokenAuth::DEFAULT_HEADER,
                                        absl::StrFormat("Bearer %s", *token)));
        return grpc::Status::OK;
    }

private:
    void write(std::string fname, std::string snippet) {
        std::ofstream out(fname);
        out << snippet;
        out.close();
    }

    std::unique_ptr<tink::KeysetHandle> writeEs512(std::string fname) {
        // Let's generate a json key.
        auto status = tink::JwtSignatureRegister();
        auto private_handle =
                tink::KeysetHandle::GenerateNew(tink::JwtEs512Template());
        auto public_handle = (*private_handle)->GetPublicKeysetHandle();
        auto jsonSnippet =
                tink::JwkSetFromPublicKeysetHandle(*public_handle->get());
        write(fname, *jsonSnippet);
        return std::move(private_handle.ValueOrDie());
    }

    grpc::string mPath;
    std::unique_ptr<tink::KeysetHandle> mPrivateKey;
};
}  // namespace control
}  // namespace emulation
}  // namespace android
