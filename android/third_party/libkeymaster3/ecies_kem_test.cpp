/*
 * Copyright 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecies_kem.h"

#include <gtest/gtest.h>
#include <openssl/evp.h>

#include <hardware/keymaster_defs.h>
#include <keymaster/android_keymaster_utils.h>

#include "android_keymaster_test_utils.h"
#include "nist_curve_key_exchange.h"

using std::string;

namespace keymaster {
namespace test {

StdoutLogger logger;

static const keymaster_ec_curve_t kEcCurves[] = {KM_EC_CURVE_P_224, KM_EC_CURVE_P_256,
                                                 KM_EC_CURVE_P_384, KM_EC_CURVE_P_521};

/**
 * TestConsistency just tests that the basic key encapsulation hold.
 */
TEST(EciesKem, TestConsistency) {
    static const uint32_t kKeyLen = 32;
    for (auto& curve : kEcCurves) {
        AuthorizationSet kem_description(AuthorizationSetBuilder()
                                             .Authorization(TAG_EC_CURVE, curve)
                                             .Authorization(TAG_KDF, KM_KDF_RFC5869_SHA256)
                                             .Authorization(TAG_ECIES_SINGLE_HASH_MODE)
                                             .Authorization(TAG_KEY_SIZE, kKeyLen));
        keymaster_error_t error;
        EciesKem* kem = new EciesKem(kem_description, &error);
        ASSERT_EQ(KM_ERROR_OK, error);

        NistCurveKeyExchange* key_exchange = NistCurveKeyExchange::GenerateKeyExchange(curve);
        Buffer peer_public_value;
        ASSERT_TRUE(key_exchange->public_value(&peer_public_value));

        Buffer output_clear_key;
        Buffer output_encrypted_key;
        ASSERT_TRUE(kem->Encrypt(peer_public_value, &output_clear_key, &output_encrypted_key));
        ASSERT_EQ(kKeyLen, output_clear_key.available_read());
        ASSERT_EQ(peer_public_value.available_read(), output_encrypted_key.available_read());

        Buffer decrypted_clear_key;
        ASSERT_TRUE(
            kem->Decrypt(key_exchange->private_key(), output_encrypted_key, &decrypted_clear_key));
        ASSERT_EQ(kKeyLen, decrypted_clear_key.available_read());
        EXPECT_EQ(0, memcmp(output_clear_key.peek_read(), decrypted_clear_key.peek_read(),
                            output_clear_key.available_read()));
    }
}

}  // namespace test
}  // namespace keymaster
