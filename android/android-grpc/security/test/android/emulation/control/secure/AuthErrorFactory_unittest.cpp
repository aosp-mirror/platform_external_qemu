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
#include "android/emulation/control/secure/JwkKeyLoader.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "absl/strings/string_view.h"
#include "aemu/base/files/PathUtils.h"
#include "android/base/testing/TestTempDir.h"
#include "android/emulation/control/secure/BasicTokenAuth.h"
#include "android/emulation/control/secure/JwtTokenAuth.h"
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

MATCHER_P(ContainsSubstr, expected, "") {
    return arg.find(expected) != std::string::npos;
}

namespace android {
namespace emulation {
namespace control {

namespace tink = crypto::tink;
using android::base::pj;
using android::base::TestTempDir;
using json = nlohmann::json;
using Path = std::string;

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

class AuthErrorsTest : public ::testing::Test {
public:
    void SetUp() override {
        mAllGreen.setSource("/tmp/fake_source.json");
        mAllYellow.setSource("/tmp/fake_source.json");
        mAllRed.setSource("/tmp/fake_source.json");
        auto status = tink::TinkConfig::Register();
        EXPECT_TRUE(status.ok());
        status = tink::JwtSignatureRegister();
        EXPECT_TRUE(status.ok());

        mTempDir.reset(new TestTempDir("watcher_test"));

        mSampleJwt = tink::RawJwtBuilder()
                             .SetIssuer("JwkDirectoryObserverTest")
                             .WithoutExpiration()
                             .Build();

        mSampleValidator = tink::JwtValidatorBuilder()
                                   .ExpectIssuer("JwkDirectoryObserverTest")
                                   .AllowMissingExpiration()
                                   .Build();
    }

    void TearDown() override { mTempDir.reset(); }

    void write(Path fname, json snippet) { write(fname, snippet.dump(2)); }

    void write(Path fname, std::string snippet) {
        std::ofstream out(pj(mTempDir->path(), fname));
        out << snippet;
        out.close();
    }

    std::unique_ptr<tink::KeysetHandle> writeEs512(Path fname) {
        // Let's generate a json key.
        auto status = tink::JwtSignatureRegister();
        EXPECT_TRUE(status.ok());
        auto private_handle =
                tink::KeysetHandle::GenerateNew(tink::JwtEs512Template());
        EXPECT_TRUE(private_handle.ok());
        auto sign = (*private_handle)->GetPrimitive<tink::JwtPublicKeySign>();
        auto public_handle = (*private_handle)->GetPublicKeysetHandle();
        auto jsonSnippet =
                tink::JwkSetFromPublicKeysetHandle(*public_handle->get());
        write(fname, *jsonSnippet);
        return std::move(private_handle.value());
    }

protected:
    std::unique_ptr<TestTempDir> mTempDir;
    absl::StatusOr<tink::RawJwt> mSampleJwt;
    absl::StatusOr<tink::JwtValidator> mSampleValidator;
    AllGreen mAllGreen;
    AllYellow mAllYellow;
    AllRed mAllRed;
};

TEST_F(AuthErrorsTest, static_token_cannot_handle_bad_token) {
    StaticTokenAuth auth("foo", "android-studio", &mAllGreen);
    EXPECT_FALSE(auth.canHandleToken("bar"));
}

TEST_F(AuthErrorsTest, static_token_can_handle_good_token) {
    StaticTokenAuth auth("foo", "android-studio", &mAllGreen);
    EXPECT_TRUE(auth.canHandleToken("Bearer foo"));
}

TEST_F(AuthErrorsTest, static_token_cannot_handle_good_token_on_red_list) {
    StaticTokenAuth auth("foo", "android-studio", &mAllRed);
    auto status = auth.isTokenValid("foo", "Bearer foo");
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.message(),
              "The endpoint: foo is not on the allowlist loaded from: "
              "`/tmp/fake_source.json`. "
              "Add foo to the protected or allowed section of android-studio.");
}

TEST_F(AuthErrorsTest, static_token_error_on_bad_token) {
    StaticTokenAuth auth("foo", "android-studio", &mAllGreen);
    auto status = auth.isTokenValid("foo", "Bearer bar");
    EXPECT_FALSE(status.ok());
    ASSERT_THAT(status.message(),
                ContainsSubstr("The token `Bearer bar` is invalid"));
}

TEST_F(AuthErrorsTest, any_token_cannot_handle_bad_token) {
    StaticTokenAuth auth("foo", "android-studio", &mAllGreen);
    StaticTokenAuth auth2("bar", "android-studio", &mAllGreen);
    AnyTokenAuth anyauth(std::vector<BasicTokenAuth*>({&auth, &auth2}),
                         &mAllGreen);
    EXPECT_FALSE(anyauth.canHandleToken("bar"));
}

TEST_F(AuthErrorsTest, any_token_can_handle_good_token) {
    StaticTokenAuth auth("foo", "android-studio", &mAllGreen);
    StaticTokenAuth auth2("bar", "android-studio", &mAllGreen);
    AnyTokenAuth anyauth(std::vector<BasicTokenAuth*>({&auth, &auth2}),
                         &mAllGreen);
    EXPECT_TRUE(anyauth.canHandleToken("Bearer bar"));
}

TEST_F(AuthErrorsTest, any_token_can_validate_good_token) {
    StaticTokenAuth auth("foo", "android-studio", &mAllGreen);
    StaticTokenAuth auth2("bar", "android-studio", &mAllGreen);
    AnyTokenAuth anyauth(std::vector<BasicTokenAuth*>({&auth, &auth2}),
                         &mAllGreen);
    EXPECT_TRUE(anyauth.isTokenValid("/a/b/c", "Bearer bar").ok());
}

TEST_F(AuthErrorsTest, any_token_reject_good_token_on_red_list) {
    StaticTokenAuth auth("foo", "android-studio", &mAllRed);
    StaticTokenAuth auth2("bar", "android-studio", &mAllRed);
    AnyTokenAuth anyauth(std::vector<BasicTokenAuth*>({&auth, &auth2}),
                         &mAllGreen);
    auto status = anyauth.isTokenValid("/a/b/c", "Bearer bar");
    EXPECT_FALSE(status.ok());
    ASSERT_THAT(status.message(),
                ContainsSubstr(
                        "The endpoint: /a/b/c is not on the allowlist loaded "
                        "from: `/tmp/fake_source.json`. Add /a/b/c to the "
                        "protected or allowed section of android-studio."));
}

TEST_F(AuthErrorsTest, any_token_fatal_on_bad_token) {
    StaticTokenAuth auth("foo", "android-studio", &mAllGreen);
    AnyTokenAuth anyauth(std::vector<BasicTokenAuth*>({&auth}), &mAllGreen);

    auto status = anyauth.isTokenValid("foo", "Bearer bar");
    EXPECT_FALSE(status.ok());
    ASSERT_THAT(status.message(),
                ContainsSubstr(
                        "FATAL: No validator that can handle token. This "
                        "should not happen, please file a bug including this "
                        "information: Validation failure `validators: , "
                        "StaticTokenAuth`, `path: foo`, `token: Bearer bar`"));
}

}  // namespace control
}  // namespace emulation
}  // namespace android