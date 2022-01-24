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

#include "android/emulation/control/secure/JwkDirectoryObserver.h"

#include <gtest/gtest.h>  // for SuiteApiResolver, Message
#include <stdio.h>        // for printf
#include <fstream>        // for ofstream, operator<<
#include <memory>         // for unique_ptr, operator==
#include <string>         // for char_traits, string
#include <utility>        // for move

#include "absl/status/statusor.h"              // for StatusOr
#include "android/base/files/PathUtils.h"      // for pj
#include "android/base/system/System.h"        // for System
#include "android/base/testing/TestEvent.h"    // for TestEvent
#include "android/base/testing/TestTempDir.h"  // for TestTempDir
#include "gtest/gtest_pred_impl.h"             // for AssertionResult, TEST_F
#include "tink/config/tink_config.h"           // for TinkConfig
#include "tink/jwt/jwk_set_converter.h"        // for JwkSetFromPublicKeyset...
#include "tink/jwt/jwt_key_templates.h"        // for JwtEs512Template
#include "tink/jwt/jwt_public_key_sign.h"      // for JwtPublicKeySign
#include "tink/jwt/jwt_public_key_verify.h"
#include "tink/jwt/jwt_signature_config.h"  // for JwtSignatureRegister
#include "tink/jwt/jwt_validator.h"         // for JwtValidator, JwtValid...
#include "tink/jwt/raw_jwt.h"               // for RawJwt, RawJwtBuilder
#include "tink/keyset_handle.h"             // for KeysetHandle
#include "tink/util/status.h"               // for Status
#include "tink/util/statusor.h"             // for StatusOr
namespace android {
namespace emulation {
namespace control {

using android::base::pj;
using android::base::TestTempDir;
using json = nlohmann::json;
namespace tink = crypto::tink;
using crypto::tink::TinkConfig;

class JwkDirectoryObserverTest : public ::testing::Test {
public:
    void SetUp() override {
        auto status = TinkConfig::Register();
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
        mTestEv.reset();
    }

    void TearDown() override {
        mTempDir.reset();
        mTestEv.reset();
    }

    void write(Path fname, json snippet) { write(fname, snippet.dump(2)); }

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
        auto sign = (*private_handle)->GetPrimitive<tink::JwtPublicKeySign>();
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

std::string RS256_snippet = R"(
{
   "keys":[
      {
         "kty":"RSA",
         "n":"AQAB",
         "e":"AQAB",
         "use":"sig",
         "alg":"RS256",
         "key_ops":[
            "verify"
         ],
         "kid":"ABC"
      }
   ]
})";

// ES256 snippet with
//
std::string ES256_snippet = R"(
{
   "keys":[
      {
         "crv":"P-256",
         "alg":"ES256",
         "kty":"EC",
         "x":"_EHh-Pq84GXDlrIbkcYUGAraDE7SLWvRZJZOgARK4II",
         "y":"vz3xxWFNDPyC-ePlmPF7cVqBsGqkQ2grL5K9ZzzrU2E",
         "kid":"DEF"
      }
   ]
})";

std::string ES256_PRIV = "9PPR4aq2V71P5QD_TJvPsM_edjcSOSkPEK1X3aasJHw";

TEST_F(JwkDirectoryObserverTest, no_jwks_results_in_event) {
    JwkDirectoryObserver observer(mTempDir->path(), [=](auto keyset) {
        // No keys found
        EXPECT_TRUE(keyset == nullptr);
        mTestEv.signal();
    });
    mTestEv.wait();
}

TEST_F(JwkDirectoryObserverTest, finds_jwks) {
    write("sample.jwk", RS256_snippet);
    JwkDirectoryObserver observer(mTempDir->path(),
                                  [=](auto keyset) { mTestEv.signal(); });
    mTestEv.wait();
}

TEST_F(JwkDirectoryObserverTest, duplicates_do_not_fail) {
    write("sample.jwk", RS256_snippet);
    write("sample2.jwk", RS256_snippet);
    JwkDirectoryObserver observer(mTempDir->path(),
                                  [=](auto keyset) { mTestEv.signal(); });
    mTestEv.wait();
}

TEST_F(JwkDirectoryObserverTest, merging_multiple) {
    write("sample.jwk", RS256_snippet);
    write("sample2.jwk", ES256_snippet);
    JwkDirectoryObserver observer(mTempDir->path(),
                                  [=](auto keyset) { mTestEv.signal(); });
    mTestEv.wait();
}

TEST_F(JwkDirectoryObserverTest, create_and_validate) {
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto token = (*sign)->SignAndEncode(*mSampleJwt);

    // Our observer found the public key, and hence can validate the token.
    JwkDirectoryObserver observer(mTempDir->path(), [=](auto keyset) {
        auto validator = tink::JwtValidatorBuilder()
                                 .ExpectIssuer("JwkDirectoryObserverTest")
                                 .AllowMissingExpiration()
                                 .Build();
        auto verify = keyset->template GetPrimitive<tink::JwtPublicKeyVerify>();
        auto verified_jwt =
                (*verify)->VerifyAndDecode(*token, *mSampleValidator);
        EXPECT_EQ(*verified_jwt->GetIssuer(), "JwkDirectoryObserverTest");
        mTestEv.signal();
    });
    mTestEv.wait();
}

TEST_F(JwkDirectoryObserverTest, create_multi_and_validate) {
    // Let's generate a series of json keys
    writeEs512("valid1.jwk");
    writeEs512("valid2.jwk");
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto token = (*sign)->SignAndEncode(*mSampleJwt);

    // Our observer found the public key, and hence can validate the token.
    JwkDirectoryObserver observer(mTempDir->path(), [=](auto keyset) {
        auto verify = keyset->template GetPrimitive<tink::JwtPublicKeyVerify>();
        auto verified_jwt =
                (*verify)->VerifyAndDecode(*token, *mSampleValidator);
        EXPECT_EQ(*verified_jwt->GetIssuer(), "JwkDirectoryObserverTest");
        mTestEv.signal();
    });
    mTestEv.wait();
}

TEST_F(JwkDirectoryObserverTest, create_validate_and_delete) {
    // Let's generate a series of json keys
    writeEs512("valid1.jwk");
    writeEs512("valid2.jwk");
    auto private_handle = writeEs512("valid.jwk");
    auto sign = private_handle->GetPrimitive<tink::JwtPublicKeySign>();
    auto token = (*sign)->SignAndEncode(*mSampleJwt);

    enum TokenState { VALID_JWK_EXISTS, VALID_JWK_DELETED };
    TokenState state = VALID_JWK_EXISTS;
    // Our observer found the public key, and hence can validate the token.
    JwkDirectoryObserver observer(mTempDir->path(), [&](auto keyset) {
        auto verify = keyset->template GetPrimitive<tink::JwtPublicKeyVerify>();
        auto verified_jwt =
                (*verify)->VerifyAndDecode(*token, *mSampleValidator);
        switch (state) {
            case VALID_JWK_EXISTS:
                EXPECT_TRUE(verified_jwt.ok());
                EXPECT_EQ(*verified_jwt->GetIssuer(),
                          "JwkDirectoryObserverTest");
                mTestEv.signal();
                break;
            case VALID_JWK_DELETED:
                EXPECT_FALSE(verified_jwt.ok());
                mTestEv.signal();
        }
    });

    mTestEv.wait();
    mTestEv.reset();

    // We now are going to delete the valid jwk that was used to sign the
    // jwt. This means we no longer have the KID to validate the token in our
    // keyhandle. This in turns means that we will no longer be able to validate
    // the token.
    // Note, we might get multiple events.
    state = VALID_JWK_DELETED;
    auto todelete = pj(mTempDir->path(), "valid.jwk");
    base::System::get()->deleteFile(todelete);
    mTestEv.wait();
}

}  // namespace control
}  // namespace emulation
}  // namespace android
