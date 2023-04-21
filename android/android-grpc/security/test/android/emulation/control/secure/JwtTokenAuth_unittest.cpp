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

const std::string kGRADLE = "gradle-utp-emulator-control";

class AllYellow : public AllowList {
public:
    bool requiresAuthentication(std::string_view path) override {
        return true;
    };

    bool isAllowed(std::string_view sub, std::string_view path) override {
        return false;
    }

    bool isProtected(std::string_view sub, std::string_view path) override {
        return true;
    }
};

class AllGreen : public AllowList {
public:
    bool requiresAuthentication(std::string_view path) override {
        return true;
    };

    bool isAllowed(std::string_view sub, std::string_view path) override {
        return true;
    }

    bool isProtected(std::string_view sub, std::string_view path) override {
        return false;
    }
};

class AllRed : public AllowList {
public:
    bool requiresAuthentication(std::string_view path) override {
        return true;
    };

    bool isAllowed(std::string_view sub, std::string_view path) override {
        return false;
    }

    bool isProtected(std::string_view sub, std::string_view path) override {
        return false;
    }
};

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
    AllGreen mAllGreen;
    AllYellow mAllYellow;
    AllRed mAllRed;
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
    JwtTokenAuth jwt(mTempDir->path(), discover_file, &mAllYellow);

    EXPECT_TRUE(base::System::get()->pathExists(discover_file));
}

TEST_F(JwkTokenAuthTest, discovery_file_contains_our_key) {
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto token = (*sign)->SignAndEncode(*mSampleJwt);

    auto discover_file = pj(mTempDir->path(), "loaded.jwk");
    JwtTokenAuth jwt(mTempDir->path(), discover_file, &mAllYellow);

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

TEST_F(JwkTokenAuthTest, accept_yellow) {
    // AUD on the yellow list, you're ok
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto token = (*sign)->SignAndEncode(*mSampleJwt);

    JwtTokenAuth jwt(mTempDir->path(), "", &mAllYellow);
    EXPECT_TRUE(jwt.isTokenValid("c", "Bearer " + *token).ok());
}

TEST_F(JwkTokenAuthTest, accept_green) {
    // AUD on the green list, you're ok
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto token = (*sign)->SignAndEncode(*mSampleJwt);

    JwtTokenAuth jwt(mTempDir->path(), "", &mAllGreen);
    EXPECT_TRUE(jwt.isTokenValid("c", "Bearer " + *token).ok());
}

TEST_F(JwkTokenAuthTest, reject_red_list) {
    // AUD on the red list, means you are rejected.
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto token = (*sign)->SignAndEncode(*mSampleJwt);

    JwtTokenAuth jwt(mTempDir->path(), "", &mAllRed);
    EXPECT_FALSE(jwt.isTokenValid("c", "Bearer " + *token).ok());
}

TEST_F(JwkTokenAuthTest, invalid_audience) {
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto token = (*sign)->SignAndEncode(*mSampleJwt);

    JwtTokenAuth jwt(mTempDir->path(), "", &mAllYellow);
    EXPECT_FALSE(jwt.isTokenValid("not_in_aud_set", "Bearer " + *token).ok());
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

    JwtTokenAuth jwt(mTempDir->path(), "", &mAllYellow);
    auto token = (*sign)->SignAndEncode(*raw_jwt);
    EXPECT_FALSE(jwt.isTokenValid("a", "Bearer " + *token).ok());
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

    JwtTokenAuth jwt(mTempDir->path(), "", &mAllYellow);
    auto token = (*sign)->SignAndEncode(*raw_jwt);
    EXPECT_FALSE(jwt.isTokenValid("a", "Bearer " + *token).ok());
}

TEST_F(JwkTokenAuthTest, fail_with_generic_message) {
    absl::Time now = absl::Now();
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto raw_jwt = tink::RawJwtBuilder()
                           .SetIssuer("JwkTokenAuthTest")
                           .SetAudiences({"a", "b", "c"})
                           .SetExpiration(now + absl::Seconds(300))
                           .SetIssuedAt(now)
                           .Build();

    JwtTokenAuth jwt(mTempDir->path(), "", &mAllYellow);
    auto token = (*sign)->SignAndEncode(*raw_jwt);
    auto message = std::string(
            jwt.isTokenValid("d/e/f", "Bearer " + *token).message());
    EXPECT_EQ(message,
              "The JWT does not include d/e/f in the aud claim. Make sure to "
              "add it to the array.");
}

TEST_F(JwkTokenAuthTest, fail_with_gradle_message) {
    absl::Time now = absl::Now();
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto raw_jwt = tink::RawJwtBuilder()
                           .SetIssuer(kGRADLE)
                           .SetAudiences({"a", "b", "c"})
                           .SetExpiration(now + absl::Seconds(300))
                           .SetIssuedAt(now)
                           .Build();

    JwtTokenAuth jwt(mTempDir->path(), "", &mAllYellow);
    auto token = (*sign)->SignAndEncode(*raw_jwt);
    auto message = std::string(
            jwt.isTokenValid("d/e/f", "Bearer " + *token).message());
    EXPECT_EQ(message,
              "Make sure to add allowedEndpoints.add(\"d/e/f\") to the "
              "emulatorControl block in your gradle build file.");
}

TEST_F(JwkTokenAuthTest, fail_with_generic_message_no_aud) {
    absl::Time now = absl::Now();
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto raw_jwt = tink::RawJwtBuilder()
                           .SetIssuer("JwkTokenAuthTest")
                           .SetExpiration(now + absl::Seconds(300))
                           .SetIssuedAt(now)
                           .Build();

    JwtTokenAuth jwt(mTempDir->path(), "", &mAllYellow);
    auto token = (*sign)->SignAndEncode(*raw_jwt);
    auto message = std::string(
            jwt.isTokenValid("d/e/f", "Bearer " + *token).message());
    EXPECT_EQ(message,
              "The JWT does not have an aud claim. Make sure to include: "
              "\"aud\" : [\"d/e/f\"] in your JWT.");
}

TEST_F(JwkTokenAuthTest, fail_with_gradle_message_no_aud) {
    absl::Time now = absl::Now();
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto raw_jwt = tink::RawJwtBuilder()
                           .SetIssuer(kGRADLE)
                           .SetExpiration(now + absl::Seconds(300))
                           .SetIssuedAt(now)
                           .Build();

    JwtTokenAuth jwt(mTempDir->path(), "", &mAllYellow);
    auto token = (*sign)->SignAndEncode(*raw_jwt);
    auto message = std::string(
            jwt.isTokenValid("d/e/f", "Bearer " + *token).message());
    EXPECT_EQ(message,
              "Make sure to add allowedEndpoints.add(\"d/e/f\") to the "
              "emulatorControl block in your gradle build file.");
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

    JwtTokenAuth jwt(mTempDir->path(), "", &mAllYellow);
    auto token = (*sign)->SignAndEncode(*raw_jwt);

    auto anyauth = std::vector<std::unique_ptr<BasicTokenAuth>>();
    anyauth.emplace_back(std::make_unique<StaticTokenAuth>(
            "foo", "android-studio", &mAllYellow));
    anyauth.emplace_back(
            std::make_unique<JwtTokenAuth>(mTempDir->path(), "", &mAllYellow));

    AnyTokenAuth any(std::move(anyauth), &mAllYellow);
    auto message = std::string(
            any.isTokenValid("d/e/f", "Bearer " + *token).message());
    EXPECT_EQ(message,
              "The JWT does not include d/e/f in the aud claim. Make sure to "
              "add it to the array.");
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

    JwtTokenAuth jwt(mTempDir->path(), discover_file, &mAllYellow);
    EXPECT_TRUE(
            base::System::get()->deleteFile(pj(mTempDir->path(), "valid.jwk")));

    // We have to wait until the discovery file becomes empty, indicating that
    // the emulator activated a new keyset.
    auto json = readFile(discover_file);
    for (int i = 0; json != "" && i < 10; i++) {
        base::System::get()->sleepMs(100);
        json = readFile(discover_file);
    }

    EXPECT_FALSE(jwt.isTokenValid("c", "Bearer " + *token).ok());
}
}  // namespace control
}  // namespace emulation
}  // namespace android
