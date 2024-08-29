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
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <thread>
#include <chrono>

#include "absl/strings/string_view.h"
#include "aemu/base/files/PathUtils.h"
#include "android/base/testing/TestTempDir.h"
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

class JwkKeyLoaderTest : public ::testing::Test {
public:
    void SetUp() override {
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
};

TEST_F(JwkKeyLoaderTest, refuses_large_files) {
    JwkKeyLoader loader;
    write("foo", std::string(8196 * 2, 'x'));
    auto status = loader.add(pj(mTempDir->path(), "foo"));
    EXPECT_FALSE(status.ok());
    ASSERT_THAT(status.message(), ContainsSubstr("which is over our max of"));
}

TEST_F(JwkKeyLoaderTest, will_bail_on_retries_with_empty) {
    using namespace std::chrono_literals;

    JwkKeyLoader loader;
    write("foo", std::string(0, 'x'));
    auto start = std::chrono::system_clock::now();
    auto status =
            loader.addWithRetryForEmpty(pj(mTempDir->path(), "foo"), 8, 10ms);
    auto end = std::chrono::system_clock::now();
    std::chrono::milliseconds waited =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_FALSE(status.ok());
    ASSERT_EQ(status.code(), absl::StatusCode::kUnavailable);
    EXPECT_GE(waited, 79ms)
            << "We should have waited around 80ms, not: " << waited.count()
            << " ms.";
}

TEST_F(JwkKeyLoaderTest, eventually_detects_written_file) {
    using namespace std::chrono_literals;
    std::string valid = R"##(
        {
            "keys":[
                {
                    "y":"AMPDr6djvlXpu8PsUIAY7VEFj9oB9PLkM0EugPt5PjCxDFzxBKmb7cAaocsUVJ2iOYnkGNHdLzkNogpcMPXgOmNp",
                    "alg":"ES512",
                    "key_ops":[
                        "verify"
                    ],
                    "crv":"P-521",
                    "kty":"EC",
                    "use":"sig",
                    "kid":"test",
                    "x":"AUweorpR5_H-ZbypEJAywniBZaVpht7-k1E75oDGIDhYYGKwHB77M8SUHOYrQahOhzA4KZB5_cJmIX3CshQVDmsT"
                }
            ]
        })##";

    JwkKeyLoader loader;
    write("foo", std::string(0, 'x'));

    // Write the actual token after 15ms delay..
    auto t = std::thread([&]() {
        std::this_thread::sleep_for(15ms);
        write("foo", valid);
    });

    auto start = std::chrono::system_clock::now();
    auto status =
            loader.addWithRetryForEmpty(pj(mTempDir->path(), "foo"), 100, 10ms);
    auto end = std::chrono::system_clock::now();
    std::chrono::milliseconds waited =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start);


    EXPECT_TRUE(status.ok()) << "Failure: " << status.message();
    EXPECT_GT(waited, 10ms) << "We had a write delay of at least 15ms, so we should have hit at least one wait.";

    t.join();
}

TEST_F(JwkKeyLoaderTest, accepts_json) {
    JwkKeyLoader loader;

    std::string b273331311 = R"##(
        {
            "keys":[
                {
                    "y":"AMPDr6djvlXpu8PsUIAY7VEFj9oB9PLkM0EugPt5PjCxDFzxBKmb7cAaocsUVJ2iOYnkGNHdLzkNogpcMPXgOmNp",
                    "alg":"ES512",
                    "key_ops":[
                        "verify"
                    ],
                    "crv":"P-521",
                    "kty":"EC",
                    "use":"sig",
                    "kid":"test",
                    "x":"AUweorpR5_H-ZbypEJAywniBZaVpht7-k1E75oDGIDhYYGKwHB77M8SUHOYrQahOhzA4KZB5_cJmIX3CshQVDmsT"
                }
            ]
        })##";

    auto status = loader.add("test", b273331311);
    EXPECT_TRUE(status.ok()) << "Failed: " << status.message();
    EXPECT_EQ(loader.size(), 1);

    auto keyset = loader.activeKeySet();
    EXPECT_TRUE(keyset.ok()) << "Failed: " << keyset.status().message();
}

TEST_F(JwkKeyLoaderTest, accepts_multiple_json) {
    JwkKeyLoader loader;

    std::string fst = R"##(
        {
            "keys":[
                {
                    "y":"AMPDr6djvlXpu8PsUIAY7VEFj9oB9PLkM0EugPt5PjCxDFzxBKmb7cAaocsUVJ2iOYnkGNHdLzkNogpcMPXgOmNp",
                    "alg":"ES512",
                    "key_ops":[
                        "verify"
                    ],
                    "crv":"P-521",
                    "kty":"EC",
                    "use":"sig",
                    "kid":"fst",
                    "x":"AUweorpR5_H-ZbypEJAywniBZaVpht7-k1E75oDGIDhYYGKwHB77M8SUHOYrQahOhzA4KZB5_cJmIX3CshQVDmsT"
                }
            ]
        })##";

    std::string snd = R"##(
        {
            "keys":[
                {
                    "y":"AMPDr6djvlXpu8PsUIAY7VEFj9oB9PLkM0EugPt5PjCxDFzxBKmb7cAaocsUVJ2iOYnkGNHdLzkNogpcMPXgOmNp",
                    "alg":"ES512",
                    "key_ops":[
                        "verify"
                    ],
                    "crv":"P-521",
                    "kty":"EC",
                    "use":"sig",
                    "kid":"snd",
                    "x":"AUweorpR5_H-ZbypEJAywniBZaVpht7-k1E75oDGIDhYYGKwHB77M8SUHOYrQahOhzA4KZB5_cJmIX3CshQVDmsT"
                }
            ]
        })##";

    EXPECT_EQ(loader.add("fst", fst), absl::OkStatus());
    EXPECT_EQ(loader.add("snd", snd), absl::OkStatus());
    EXPECT_EQ(loader.size(), 2);

    auto keyset = loader.activeKeySet();
    EXPECT_TRUE(keyset.ok()) << "Failed: " << keyset.status().message();
}

TEST_F(JwkKeyLoaderTest, accepts_json_in_file) {
    JwkKeyLoader loader;

    std::string b273331311 = R"##(
        {
            "keys":[
                {
                    "y":"AMPDr6djvlXpu8PsUIAY7VEFj9oB9PLkM0EugPt5PjCxDFzxBKmb7cAaocsUVJ2iOYnkGNHdLzkNogpcMPXgOmNp",
                    "alg":"ES512",
                    "key_ops":[
                        "verify"
                    ],
                    "crv":"P-521",
                    "kty":"EC",
                    "use":"sig",
                    "kid":"VVfyNA",
                    "x":"AUweorpR5_H-ZbypEJAywniBZaVpht7-k1E75oDGIDhYYGKwHB77M8SUHOYrQahOhzA4KZB5_cJmIX3CshQVDmsT"
                }
            ]
        })##";

    write("sample.jwk", b273331311);
    auto status = loader.add(pj(mTempDir->path(), "sample.jwk"));
    EXPECT_TRUE(status.ok()) << "Failed: " << status.message();
    EXPECT_EQ(loader.size(), 1);

    auto keyset = loader.activeKeySet();
    EXPECT_TRUE(keyset.ok()) << "Failed: " << keyset.status().message();
}

TEST_F(JwkKeyLoaderTest, gracefully_rejects_broken_json) {
    JwkKeyLoader loader;

    std::string borked = R"##(
        {
            "keys":[ << BORKED >>
                {
                    "y":"AMPDr6djvlXpu8PsUIAY7VEFj9oB9PLkM0EugPt5PjCxDFzxBKmb7cAaocsUVJ2iOYnkGNHdLzkNogpcMPXgOmNp",
                    "alg":"ES512",
                    "key_ops":[
                        "verify"
                    ],
                    "crv":"P-521",
                    "kty":"EC",
                    "use":"sig",
                    "kid":"VVfyNA",
                    "x":"AUweorpR5_H-ZbypEJAywniBZaVpht7-k1E75oDGIDhYYGKwHB77M8SUHOYrQahOhzA4KZB5_cJmIX3CshQVDmsT"
                }
            ]
        })##";

    auto status = loader.add("test", borked);
    EXPECT_FALSE(status.ok());
    EXPECT_THAT(status.message(),
                ContainsSubstr("test does not contain a valid jwk"))
            << status.message();
    EXPECT_TRUE(loader.empty());
}

TEST_F(JwkKeyLoaderTest, gracefully_rejects_broken_jwk) {
    JwkKeyLoader loader;

    std::string missing_y_field = R"##(
        {
            "keys":[
                {
                    "alg":"ES512",
                    "key_ops":[
                        "verify"
                    ],
                    "crv":"P-521",
                    "kty":"EC",
                    "use":"sig",
                    "kid":"VVfyNA",
                    "x":"AUweorpR5_H-ZbypEJAywniBZaVpht7-k1E75oDGIDhYYGKwHB77M8SUHOYrQahOhzA4KZB5_cJmIX3CshQVDmsT"
                }
            ]
        })##";

    auto status = loader.add("test", missing_y_field);
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.message(), "test does not contain a valid jwk");
    EXPECT_TRUE(loader.empty());
}

TEST_F(JwkKeyLoaderTest, gracefully_rejects_missing_file) {
    JwkKeyLoader loader;
    auto status = loader.add("this_path_does_not_exist");

    EXPECT_FALSE(status.ok());
    EXPECT_THAT(
            status.message(),
            ContainsSubstr("this_path_does_not_exist does not exist."));
    EXPECT_TRUE(loader.empty());
}

TEST_F(JwkKeyLoaderTest, can_remove_key) {
    JwkKeyLoader loader;

    std::string key = R"##(
        {
            "keys":[
                {
                    "y":"AMPDr6djvlXpu8PsUIAY7VEFj9oB9PLkM0EugPt5PjCxDFzxBKmb7cAaocsUVJ2iOYnkGNHdLzkNogpcMPXgOmNp",
                    "alg":"ES512",
                    "key_ops":[
                        "verify"
                    ],
                    "crv":"P-521",
                    "kty":"EC",
                    "use":"sig",
                    "kid":"VVfyNA",
                    "x":"AUweorpR5_H-ZbypEJAywniBZaVpht7-k1E75oDGIDhYYGKwHB77M8SUHOYrQahOhzA4KZB5_cJmIX3CshQVDmsT"
                }
            ]
        })##";

    auto status = loader.add("sample.jwk", key);
    EXPECT_TRUE(status.ok()) << "Failed: " << status.message();
    EXPECT_EQ(loader.size(), 1);

    EXPECT_EQ(loader.remove("sample.jwk"), absl::OkStatus());
    EXPECT_TRUE(loader.empty());

    auto keyset = loader.activeKeySet();
    EXPECT_FALSE(keyset.ok());
    EXPECT_EQ(keyset.status().message(), "keys list is empty");
}

TEST_F(JwkKeyLoaderTest, no_keys) {
    JwkKeyLoader loader;

    std::string key = R"##(
        {
            "keys":[

            ]
        })##";

    auto status = loader.add("sample.jwk", key);
    EXPECT_FALSE(status.ok());
    EXPECT_TRUE(loader.empty());

    auto keyset = loader.activeKeySet();
    EXPECT_FALSE(keyset.ok());
    EXPECT_EQ(keyset.status().message(), "keys list is empty");
}

TEST_F(JwkKeyLoaderTest, missing_key_id) {
    JwkKeyLoader loader;

    std::string key = R"##(
        {
            "keys":[
                {
                    "y":"AMPDr6djvlXpu8PsUIAY7VEFj9oB9PLkM0EugPt5PjCxDFzxBKmb7cAaocsUVJ2iOYnkGNHdLzkNogpcMPXgOmNp",
                    "alg":"ES512",
                    "key_ops":[
                        "verify"
                    ],
                    "crv":"P-521",
                    "kty":"EC",
                    "use":"sig",
                    "x":"AUweorpR5_H-ZbypEJAywniBZaVpht7-k1E75oDGIDhYYGKwHB77M8SUHOYrQahOhzA4KZB5_cJmIX3CshQVDmsT"
                }
            ]
        })##";

    auto status = loader.add("sample.jwk", key);
    EXPECT_TRUE(status.ok()) << "Failed: " << status.message();
    EXPECT_EQ(loader.size(), 1);

    auto keyset = loader.activeKeySet();
    EXPECT_TRUE(keyset.ok());
}

}  // namespace control
}  // namespace emulation
}  // namespace android