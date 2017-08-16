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

#include "nist_curve_key_exchange.h"

#include <gtest/gtest.h>
#include <openssl/evp.h>

#include <hardware/keymaster_defs.h>
#include <keymaster/android_keymaster_utils.h>

#include "android_keymaster_test_utils.h"

using std::string;

namespace keymaster {
namespace test {

StdoutLogger logger;

static const keymaster_ec_curve_t kEcCurves[] = {KM_EC_CURVE_P_224, KM_EC_CURVE_P_256,
                                                 KM_EC_CURVE_P_384, KM_EC_CURVE_P_521};

/**
 * SharedKey just tests that the basic key exchange identity holds: that both
 * parties end up with the same key.
 */
TEST(NistCurveKeyExchange, SharedKey) {
    for (auto& curve : kEcCurves) {
        AuthorizationSet kex_description(
            AuthorizationSetBuilder().Authorization(TAG_EC_CURVE, curve));
        for (size_t j = 0; j < 5; j++) {
            NistCurveKeyExchange* alice_keyex = NistCurveKeyExchange::GenerateKeyExchange(curve);
            NistCurveKeyExchange* bob_keyex = NistCurveKeyExchange::GenerateKeyExchange(curve);

            ASSERT_TRUE(alice_keyex != nullptr);
            ASSERT_TRUE(bob_keyex != nullptr);

            Buffer alice_public_value;
            ASSERT_TRUE(alice_keyex->public_value(&alice_public_value));
            Buffer bob_public_value;
            ASSERT_TRUE(bob_keyex->public_value(&bob_public_value));

            Buffer alice_shared, bob_shared;
            ASSERT_TRUE(alice_keyex->CalculateSharedKey(bob_public_value, &alice_shared));
            ASSERT_TRUE(bob_keyex->CalculateSharedKey(alice_public_value, &bob_shared));
            EXPECT_EQ(alice_shared.available_read(), bob_shared.available_read());
            EXPECT_EQ(0, memcmp(alice_shared.peek_read(), bob_shared.peek_read(),
                                alice_shared.available_read()));
        }
    }
}

/*
 * This test tries a key agreement with a false public key (i.e. with
 * a point not on the curve.)
 * The expected result of such a protocol should be that the
 * key agreement fails and returns an error.
*/
static const char* kInvalidPublicKeys[] = {
    "04"  // uncompressed public key
    "deadbeef7f56584c5cc632ca65640db91b6bacce3a4df6b42ce7cc838833d287"
    "db71e509e3fd9b060ddb20ba5c51dcc5948d46fbf640dfe0441782cab85fa4ac",
};

TEST(NistCurveKeyExchange, InvalidPublicKey) {
    for (auto& curve : kEcCurves) {
        AuthorizationSet kex_description(
            AuthorizationSetBuilder().Authorization(TAG_EC_CURVE, curve));
        KeyExchange* key_exchange = NistCurveKeyExchange::GenerateKeyExchange(curve);
        ASSERT_TRUE(key_exchange != nullptr);

        string peer_public_key = hex2str(kInvalidPublicKeys[0]);
        Buffer computed_shared_secret;
        ASSERT_FALSE(key_exchange->CalculateSharedKey(
            reinterpret_cast<const uint8_t*>(peer_public_key.data()), peer_public_key.size(),
            &computed_shared_secret));
    }
}

/**
 * Test that key exchange fails when peer public key is the point at infinity.
 */
TEST(NistCurveKeyExchange, TestInfinity) {
    for (auto& curve : kEcCurves) {
        /* Obtain the point at infinity */
        EC_GROUP* group = ec_get_group(curve);
        EC_POINT* point_at_infinity = EC_POINT_new(group);
        EC_POINT_set_to_infinity(group, point_at_infinity);
        EXPECT_EQ(1, EC_POINT_is_on_curve(group, point_at_infinity, nullptr));
        size_t field_len_in_bits;
        ec_get_group_size(group, &field_len_in_bits);
        size_t field_len = (field_len_in_bits + 7) / 8;
        size_t public_key_len = (field_len * 2) + 1;
        uint8_t* public_key = new uint8_t[public_key_len];
        public_key_len = EC_POINT_point2oct(group, point_at_infinity, POINT_CONVERSION_UNCOMPRESSED,
                                            public_key, public_key_len, nullptr /* ctx */);

        /* Perform the key exchange */
        AuthorizationSet kex_description(
            AuthorizationSetBuilder().Authorization(TAG_EC_CURVE, curve));
        NistCurveKeyExchange* key_exchange = NistCurveKeyExchange::GenerateKeyExchange(curve);
        ASSERT_TRUE(key_exchange != nullptr);
        Buffer computed_shared_secret;
        /* It should fail */
        ASSERT_FALSE(key_exchange->CalculateSharedKey(reinterpret_cast<const uint8_t*>(public_key),
                                                      public_key_len, &computed_shared_secret));

        /* Explicitly test that ECDH_compute_key fails when the public key is the point at infinity
         */
        UniquePtr<uint8_t[]> result(new uint8_t[field_len]);
        EXPECT_EQ(-1 /* error */, ECDH_compute_key(result.get(), field_len, point_at_infinity,
                                                   key_exchange->private_key(), nullptr /* kdf */));
    }
}

/* Test vectors for P-256, downloaded from NIST. */
struct NistCurveTest {
    const keymaster_ec_curve_t curve;
    const char* peer_public_key;
    const char* my_private_key;
    const char* shared_secret;
};

static const NistCurveTest kNistCurveTests[] = {
    {
        KM_EC_CURVE_P_256,
        "04"  // uncompressed public key
        "700c48f77f56584c5cc632ca65640db91b6bacce3a4df6b42ce7cc838833d287"
        "db71e509e3fd9b060ddb20ba5c51dcc5948d46fbf640dfe0441782cab85fa4ac",
        // https://tools.ietf.org/html/rfc5915
        "30770201010420"  // DER-encodeded EC private key header
        "7d7dc5f71eb29ddaf80d6214632eeae03d9058af1fb6d22ed80badb62bc1a534"  // private key
        "a00a06082a8648ce3d030107a144034200"  // DER-encoded curve OID,
        "04"
        "ead218590119e8876b29146ff89ca61770c4edbbf97d38ce385ed281d8a6b230"
        "28af61281fd35e2fa7002523acc85a429cb06ee6648325389f59edfce1405141",
        "46fc62106420ff012e54a434fbdd2d25ccc5852060561e68040dd7778997bd7b",
    },
    {
        KM_EC_CURVE_P_256, "04"
                           "809f04289c64348c01515eb03d5ce7ac1a8cb9498f5caa50197e58d43a86a7ae"
                           "b29d84e811197f25eba8f5194092cb6ff440e26d4421011372461f579271cda3",
        // https://tools.ietf.org/html/rfc5915
        "30770201010420"  // DER-encodeded EC private key header
        "38f65d6dce47676044d58ce5139582d568f64bb16098d179dbab07741dd5caf5"  // private key
        "a00a06082a8648ce3d030107a144034200"  // DER-encoded curve OID,
        "04"
        "119f2f047902782ab0c9e27a54aff5eb9b964829ca99c06b02ddba95b0a3f6d0"
        "8f52b726664cac366fc98ac7a012b2682cbd962e5acb544671d41b9445704d1d",
        "057d636096cb80b67a8c038c890e887d1adfa4195e9b3ce241c8a778c59cda67",
    },
    {
        KM_EC_CURVE_P_256, "04"
                           "df3989b9fa55495719b3cf46dccd28b5153f7808191dd518eff0c3cff2b705ed"
                           "422294ff46003429d739a33206c8752552c8ba54a270defc06e221e0feaf6ac4",
        // https://tools.ietf.org/html/rfc5915
        "30770201010420"  // DER-encodeded EC private key header
        "207c43a79bfee03db6f4b944f53d2fb76cc49ef1c9c4d34d51b6c65c4db6932d"  // private key
        "a00a06082a8648ce3d030107a144034200"  // DER-encoded curve OID,
        "04"
        "24277c33f450462dcb3d4801d57b9ced05188f16c28eda873258048cd1607e0d"
        "c4789753e2b1f63b32ff014ec42cd6a69fac81dfe6d0d6fd4af372ae27c46f88",
        "96441259534b80f6aee3d287a6bb17b5094dd4277d9e294f8fe73e48bf2a0024",
    },
};

/**
 * Test that key exchange works with NIST test vectors.
 */
TEST(NistCurveKeyExchange, NistTestVectors) {
    for (auto& test : kNistCurveTests) {
        string private_key = hex2str(test.my_private_key);
        string shared_secret = hex2str(test.shared_secret);

        const uint8_t* private_key_data = reinterpret_cast<const uint8_t*>(private_key.data());
        UniquePtr<EC_KEY, EC_KEY_Delete> ec_key(
            d2i_ECPrivateKey(nullptr, &private_key_data, private_key.size()));
        ASSERT_TRUE(ec_key.get() && EC_KEY_check_key(ec_key.get()));

        keymaster_error_t error;
        NistCurveKeyExchange* key_exchange = new NistCurveKeyExchange(ec_key.release(), &error);
        EXPECT_EQ(KM_ERROR_OK, error);
        ASSERT_TRUE(key_exchange != nullptr);

        Buffer computed_shared_secret;
        string peer_public_key = hex2str(test.peer_public_key);
        ASSERT_TRUE(key_exchange->CalculateSharedKey(
            reinterpret_cast<const uint8_t*>(peer_public_key.data()), peer_public_key.size(),
            &computed_shared_secret));
        EXPECT_EQ(shared_secret.size(), computed_shared_secret.available_read());
        EXPECT_EQ(0, memcmp(shared_secret.data(), computed_shared_secret.peek_read(),
                            shared_secret.size()));

        for (size_t i = 0; i < peer_public_key.size(); i++) {
            // randomly flip some bits in the peer public key to make it invalid
            peer_public_key[i] ^= 0xff;
            ASSERT_FALSE(key_exchange->CalculateSharedKey(
                reinterpret_cast<const uint8_t*>(peer_public_key.data()), peer_public_key.size(),
                &computed_shared_secret));
        }
    }
}

}  // namespace test
}  // namespace keymaster
