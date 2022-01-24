// Copyright 2022 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/emulation/control/secure/JwtTokenAuth.h"

#include <gtest/gtest.h>  // for SuiteApiResolver, Message
#include <fstream>        // for ofstream, operator<<
#include <memory>         // for unique_ptr, operator==
#include <string>         // for char_traits, string
#include <utility>        // for move

#include "absl/status/statusor.h"              // for StatusOr
#include "android/base/files/PathUtils.h"      // for pj
#include "android/base/system/System.h"        // for System
#include "android/base/testing/TestEvent.h"    // for TestEvent
#include "android/base/testing/TestTempDir.h"  // for TestTempDir
#include "tink/config/tink_config.h"           // for TinkConfig
#include "tink/jwt/jwk_set_converter.h"        // for JwkSetFromPublicKeyset...
#include "tink/jwt/jwt_key_templates.h"        // for JwtEs512Template
#include "tink/jwt/jwt_public_key_sign.h"      // for JwtPublicKeySign
#include "tink/jwt/jwt_signature_config.h"     // for JwtSignatureRegister
#include "tink/jwt/jwt_validator.h"            // for JwtValidator, JwtValid...
#include "tink/jwt/raw_jwt.h"                  // for RawJwt, RawJwtBuilder
#include "tink/keyset_handle.h"                // for KeysetHandle
#include "tink/util/status.h"                  // for Status
#include "tink/util/statusor.h"                // for StatusOr

namespace crypto {
namespace tink {
class JwtPublicKeyVerify;
}  // namespace tink
}  // namespace crypto

namespace android {
namespace emulation {
namespace control {

using android::base::pj;
using android::base::TestTempDir;
using json = nlohmann::json;
namespace tink = crypto::tink;
using crypto::tink::TinkConfig;

class JwkTokenAuthTest : public ::testing::Test {
public:
    void SetUp() override {
        auto status = TinkConfig::Register();
        EXPECT_TRUE(status.ok());
        status = tink::JwtSignatureRegister();
        EXPECT_TRUE(status.ok());
        mTempDir.reset(new TestTempDir("watcher_test"));

        absl::Time now = absl::Now();
        mSampleJwt = tink::RawJwtBuilder()
                             .SetIssuer("JwkTokenAuthTest")
                             .SetAudiences({"a", "b", "c"})
                             .SetExpiration(now + absl::Seconds(300))
                             .SetIssuedAt(now)
                             .Build();
    }

    void TearDown() override { mTempDir.reset(); }

    void write(Path fname, std::string snippet) {
        std::ofstream out(pj(mTempDir->path(), fname));
        out << snippet;
        out.close();
    }

    std::unique_ptr<KeysetHandle> writeEs512(Path fname) {
        // Let's generate a json key.
        auto status = tink::JwtSignatureRegister();
        EXPECT_TRUE(status.ok());
        auto private_handle =
                KeysetHandle::GenerateNew(tink::JwtEs512Template());
        EXPECT_TRUE(private_handle.ok());
        auto public_handle = (*private_handle)->GetPublicKeysetHandle();
        auto jsonSnippet =
                tink::JwkSetFromPublicKeysetHandle(*public_handle->get());
        write(fname, *jsonSnippet);
        return std::move(private_handle.ValueOrDie());
    }

protected:
    std::unique_ptr<TestTempDir> mTempDir;
    TestEvent mTestEv;
    absl::StatusOr<tink::RawJwt> mSampleJwt;
    absl::StatusOr<tink::JwtValidator> mSampleValidator;
};

TEST_F(JwkTokenAuthTest, create_and_validate) {
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto token = (*sign)->SignAndEncode(*mSampleJwt);

    JwtTokenAuth jwt(mTempDir->path());
    EXPECT_TRUE(jwt.isTokenValid("c", *token).ok());
}

TEST_F(JwkTokenAuthTest, invalid_audience) {
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto token = (*sign)->SignAndEncode(*mSampleJwt);

    JwtTokenAuth jwt(mTempDir->path());
    EXPECT_FALSE(jwt.isTokenValid("not_in_aud_set", *token).ok());
}

TEST_F(JwkTokenAuthTest, reject_expired) {
    absl::Time now = absl::Now();
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto raw_jwt = tink::RawJwtBuilder()
                           .SetIssuer("JwkTokenAuthTest")
                           .SetAudiences({"a", "b", "c"})
                           .SetExpiration(now - absl::Seconds(300))
                           .SetIssuedAt(now)
                           .Build();

    JwtTokenAuth jwt(mTempDir->path());
    auto token = (*sign)->SignAndEncode(*raw_jwt);
    EXPECT_FALSE(jwt.isTokenValid("a", *token).ok());
}

TEST_F(JwkTokenAuthTest, reject_not_ready_yet) {
    absl::Time now = absl::Now();
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto raw_jwt = tink::RawJwtBuilder()
                           .SetIssuer("JwkTokenAuthTest")
                           .SetAudiences({"a", "b", "c"})
                           .SetExpiration(now + absl::Seconds(300))
                           .SetIssuedAt(now  + absl::Seconds(30))
                           .Build();

    JwtTokenAuth jwt(mTempDir->path());
    auto token = (*sign)->SignAndEncode(*raw_jwt);
    EXPECT_FALSE(jwt.isTokenValid("a", *token).ok());
}

TEST_F(JwkTokenAuthTest, deleted_jwks_is_rejected) {
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto token = (*sign)->SignAndEncode(*mSampleJwt);

    JwtTokenAuth jwt(mTempDir->path());
    base::System::get()->deleteFile(pj(mTempDir->path(), "valid.jwk"));

    // Sadly we need to allow some time for the detector to register the deleted key.
    base::System::get()->sleepMs(50);

    EXPECT_FALSE(jwt.isTokenValid("c", *token).ok());
}
}  // namespace control
}  // namespace emulation
}  // namespace android
