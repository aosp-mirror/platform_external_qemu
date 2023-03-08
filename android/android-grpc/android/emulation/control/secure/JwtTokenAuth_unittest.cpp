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

#include <gtest/gtest.h>
#include <fstream>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "aemu/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestEvent.h"
#include "android/base/testing/TestTempDir.h"
#include "android/emulation/control/secure/BasicTokenAuth.h"
#include "nlohmann/json.hpp"
#include "tink/config/tink_config.h"
#include "tink/jwt/jwk_set_converter.h"
#include "tink/jwt/jwt_key_templates.h"
#include "tink/jwt/jwt_public_key_sign.h"
#include "tink/jwt/jwt_signature_config.h"
#include "tink/jwt/jwt_validator.h"
#include "tink/jwt/raw_jwt.h"
#include "tink/keyset_handle.h"
#include "tink/util/status.h"
#include "tink/util/statusor.h"

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

// Reads a file into a string.
static std::string readFile(Path fname) {
    std::ifstream fstream(fname);
    std::string contents((std::istreambuf_iterator<char>(fstream)),
                         std::istreambuf_iterator<char>());
    fstream.close();
    return contents;
}

TEST_F(JwkTokenAuthTest, writes_a_discovery_file) {
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto token = (*sign)->SignAndEncode(*mSampleJwt);

    auto discover_file = pj(mTempDir->path(), "loaded.jwk");
    JwtTokenAuth jwt(mTempDir->path(), discover_file);

    EXPECT_TRUE(base::System::get()->pathExists(discover_file));
}

TEST_F(JwkTokenAuthTest, discovery_file_contains_our_key) {
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto token = (*sign)->SignAndEncode(*mSampleJwt);

    auto discover_file = pj(mTempDir->path(), "loaded.jwk");
    JwtTokenAuth jwt(mTempDir->path(), discover_file);

    EXPECT_TRUE(base::System::get()->pathExists(discover_file));
    auto discoverd_json = readFile(discover_file);
    auto discovered_handle =
            crypto::tink::JwkSetToPublicKeysetHandle(discoverd_json);
    auto public_handle = private_handle->GetPublicKeysetHandle();

    EXPECT_TRUE(discovered_handle.ok()) << public_handle.status().message();
    auto ours =
            crypto::tink::JwkSetFromPublicKeysetHandle(*public_handle->get());
    auto loaded = crypto::tink::JwkSetFromPublicKeysetHandle(
            *discovered_handle->get());
    EXPECT_EQ(json::parse(ours.ValueOrDie()), json::parse(loaded.ValueOrDie()));
}

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
                           .SetIssuedAt(now + absl::Seconds(30))
                           .Build();

    JwtTokenAuth jwt(mTempDir->path());
    auto token = (*sign)->SignAndEncode(*raw_jwt);
    EXPECT_FALSE(jwt.isTokenValid("a", *token).ok());
}

TEST_F(JwkTokenAuthTest, message) {
    absl::Time now = absl::Now();
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto raw_jwt = tink::RawJwtBuilder()
                           .SetIssuer("JwkTokenAuthTest")
                           .SetAudiences({"a", "b", "c"})
                           .SetExpiration(now + absl::Seconds(300))
                           .SetIssuedAt(now)
                           .Build();

    JwtTokenAuth jwt(mTempDir->path());
    auto token = (*sign)->SignAndEncode(*raw_jwt);
    auto message = std::string(jwt.isTokenValid("d/e/f", *token).message());
    EXPECT_EQ(message, "Access denied, does you aud claim include: d/e/f ?");
}

TEST_F(JwkTokenAuthTest, any_message) {
    absl::Time now = absl::Now();
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto raw_jwt = tink::RawJwtBuilder()
                           .SetIssuer("JwkTokenAuthTest")
                           .SetAudiences({"a", "b", "c"})
                           .SetExpiration(now + absl::Seconds(300))
                           .SetIssuedAt(now)
                           .Build();

    JwtTokenAuth jwt(mTempDir->path());
    auto token = (*sign)->SignAndEncode(*raw_jwt);

    auto anyauth = std::vector<std::unique_ptr<BasicTokenAuth>>();
    anyauth.emplace_back(std::make_unique<StaticTokenAuth>("foo"));
    anyauth.emplace_back(std::make_unique<JwtTokenAuth>(mTempDir->path()));

    AnyTokenAuth any(std::move(anyauth));
    auto message = std::string(any.isTokenValid("d/e/f", *token).message());
    EXPECT_EQ(message,
              "Validation failed: [\"Incorrect token\", \"Access denied, does "
              "you aud claim include: d/e/f ?\", ]");
}

TEST_F(JwkTokenAuthTest, deleted_jwks_is_rejected) {
#ifdef __APPLE__
    // There are some issues  on darwin around file deletion detection
    // that differes by OS version.
    GTEST_SKIP();
#endif
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto token = (*sign)->SignAndEncode(*mSampleJwt);
    auto discover_file = pj(mTempDir->path(), "loaded.jwk");

    JwtTokenAuth jwt(mTempDir->path(), discover_file);
    EXPECT_TRUE(
            base::System::get()->deleteFile(pj(mTempDir->path(), "valid.jwk")));

    // We have to wait until the discovery file becomes empty, indicating that
    // the emulator activated a new keyset.
    auto json = readFile(discover_file);
    for (int i = 0; json != "" && i < 10; i++) {
        base::System::get()->sleepMs(100);
        json = readFile(discover_file);
    }

    EXPECT_FALSE(jwt.isTokenValid("c", *token).ok());
}
}  // namespace control
}  // namespace emulation
}  // namespace android
