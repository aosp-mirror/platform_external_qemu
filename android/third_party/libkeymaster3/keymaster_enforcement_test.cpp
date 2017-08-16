/*
 * Copyright (C) 2014 The Android Open Source Project
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

#include <errno.h>
#include <stdio.h>
#include <time.h>

#include <keymaster/android_keymaster.h>
#include <keymaster/authorization_set.h>
#include <keymaster/keymaster_enforcement.h>

#include "android_keymaster_test_utils.h"

namespace keymaster {
namespace test {

class TestKeymasterEnforcement : public KeymasterEnforcement {
  public:
    TestKeymasterEnforcement()
        : KeymasterEnforcement(3, 3), current_time_(10000), report_token_valid_(true) {}

    keymaster_error_t AuthorizeOperation(const keymaster_purpose_t purpose, const km_id_t keyid,
                                         const AuthorizationSet& auth_set) {
        AuthorizationSet empty_set;
        return KeymasterEnforcement::AuthorizeOperation(
            purpose, keyid, auth_set, empty_set, 0 /* op_handle */, true /* is_begin_operation */);
    }
    using KeymasterEnforcement::AuthorizeOperation;

    uint32_t get_current_time() const override { return current_time_; }
    bool activation_date_valid(uint64_t activation_date) const override {
        // Convert java date to time_t, non-portably.
        time_t activation_time = activation_date / 1000;
        return difftime(time(NULL), activation_time) >= 0;
    }
    bool expiration_date_passed(uint64_t expiration_date) const override {
        // Convert jave date to time_t, non-portably.
        time_t expiration_time = expiration_date / 1000;
        return difftime(time(NULL), expiration_time) > 0;
    }
    bool auth_token_timed_out(const hw_auth_token_t& token, uint32_t timeout) const {
        return current_time_ > ntoh(token.timestamp) + timeout;
    }
    bool ValidateTokenSignature(const hw_auth_token_t&) const override {
        return report_token_valid_;
    }

    void tick(unsigned seconds = 1) { current_time_ += seconds; }
    uint32_t current_time() { return current_time_; }
    void set_report_token_valid(bool report_token_valid) {
        report_token_valid_ = report_token_valid;
    }

  private:
    uint32_t current_time_;
    bool report_token_valid_;
};

class KeymasterBaseTest : public ::testing::Test {
  protected:
    KeymasterBaseTest() {
        past_time = 0;

        time_t t = time(NULL);
        future_tm = localtime(&t);
        future_tm->tm_year += 1;
        future_time = static_cast<uint64_t>(mktime(future_tm)) * 1000;
        sign_param = Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN);
    }
    virtual ~KeymasterBaseTest() {}

    TestKeymasterEnforcement kmen;

    tm past_tm;
    tm* future_tm;
    uint64_t past_time;
    uint64_t future_time;
    static const km_id_t key_id = 0xa;
    static const uid_t uid = 0xf;
    keymaster_key_param_t sign_param;
};

TEST_F(KeymasterBaseTest, TestValidKeyPeriodNoTags) {
    keymaster_key_param_t params[] = {
        sign_param,
    };
    AuthorizationSet single_auth_set(params, array_length(params));

    keymaster_error_t kmer = kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, single_auth_set);
    ASSERT_EQ(KM_ERROR_OK, kmer);
}

TEST_F(KeymasterBaseTest, TestInvalidActiveTime) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA), Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN),
        Authorization(TAG_NO_AUTH_REQUIRED), Authorization(TAG_ACTIVE_DATETIME, future_time),
    };

    AuthorizationSet auth_set(params, array_length(params));

    ASSERT_EQ(KM_ERROR_KEY_NOT_YET_VALID,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set));

    // Pubkey ops allowed.
    ASSERT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set));
}

TEST_F(KeymasterBaseTest, TestValidActiveTime) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN), Authorization(TAG_ACTIVE_DATETIME, past_time),
    };

    AuthorizationSet auth_set(params, array_length(params));

    keymaster_error_t kmer_valid_time = kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set);
    ASSERT_EQ(KM_ERROR_OK, kmer_valid_time);
}

TEST_F(KeymasterBaseTest, TestInvalidOriginationExpireTime) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA), Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN),
        Authorization(TAG_ORIGINATION_EXPIRE_DATETIME, past_time),
    };

    AuthorizationSet auth_set(params, array_length(params));

    ASSERT_EQ(KM_ERROR_KEY_EXPIRED, kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set));

    // Pubkey ops allowed.
    ASSERT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set));
}

TEST_F(KeymasterBaseTest, TestInvalidOriginationExpireTimeOnUsgae) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY),
        Authorization(TAG_ORIGINATION_EXPIRE_DATETIME, past_time),
    };

    AuthorizationSet auth_set(params, array_length(params));

    keymaster_error_t kmer_invalid_origination =
        kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set);
    ASSERT_EQ(KM_ERROR_OK, kmer_invalid_origination);
}

TEST_F(KeymasterBaseTest, TestValidOriginationExpireTime) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN),
        Authorization(TAG_ORIGINATION_EXPIRE_DATETIME, future_time),
    };

    AuthorizationSet auth_set(params, array_length(params));

    keymaster_error_t kmer_valid_origination =
        kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set);
    ASSERT_EQ(KM_ERROR_OK, kmer_valid_origination);
}

TEST_F(KeymasterBaseTest, TestInvalidUsageExpireTime) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_ALGORITHM, KM_ALGORITHM_AES),
        Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY),
        Authorization(TAG_USAGE_EXPIRE_DATETIME, past_time),
    };

    AuthorizationSet auth_set(params, array_length(params));

    keymaster_error_t kmer_invalid_origination =
        kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set);
    ASSERT_EQ(KM_ERROR_KEY_EXPIRED, kmer_invalid_origination);
}

TEST_F(KeymasterBaseTest, TestInvalidPubkeyUsageExpireTime) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA),
        Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY),
        Authorization(TAG_USAGE_EXPIRE_DATETIME, past_time),
    };

    AuthorizationSet auth_set(params, array_length(params));

    keymaster_error_t kmer_invalid_origination =
        kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set);
    // Pubkey ops allowed.
    ASSERT_EQ(KM_ERROR_OK, kmer_invalid_origination);
}

TEST_F(KeymasterBaseTest, TestInvalidUsageExpireTimeOnOrigination) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN),
        Authorization(TAG_USAGE_EXPIRE_DATETIME, past_time),
    };

    AuthorizationSet auth_set(params, array_length(params));

    keymaster_error_t kmer_invalid_origination =
        kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set);
    ASSERT_EQ(KM_ERROR_OK, kmer_invalid_origination);
}

TEST_F(KeymasterBaseTest, TestValidUsageExpireTime) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY),
        Authorization(TAG_USAGE_EXPIRE_DATETIME, future_time),
    };

    AuthorizationSet auth_set(params, array_length(params));

    keymaster_error_t kmer_valid_usage =
        kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set);
    ASSERT_EQ(KM_ERROR_OK, kmer_valid_usage);
}

TEST_F(KeymasterBaseTest, TestValidSingleUseAccesses) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN),
    };

    AuthorizationSet auth_set(params, array_length(params));

    keymaster_error_t kmer1 = kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set);
    keymaster_error_t kmer2 = kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set);

    ASSERT_EQ(KM_ERROR_OK, kmer1);
    ASSERT_EQ(KM_ERROR_OK, kmer2);
}

TEST_F(KeymasterBaseTest, TestInvalidMaxOps) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN), Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA),
        Authorization(TAG_MAX_USES_PER_BOOT, 4),
    };

    AuthorizationSet auth_set(params, array_length(params));

    ASSERT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set));
    ASSERT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set));
    ASSERT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set));
    ASSERT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set));
    ASSERT_EQ(KM_ERROR_KEY_MAX_OPS_EXCEEDED,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set));
    // Pubkey ops allowed.
    ASSERT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set));
}

TEST_F(KeymasterBaseTest, TestOverFlowMaxOpsTable) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA), Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN),
        Authorization(TAG_MAX_USES_PER_BOOT, 2),
    };

    AuthorizationSet auth_set(params, array_length(params));

    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_SIGN, 1 /* key_id */, auth_set));

    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_SIGN, 2 /* key_id */, auth_set));

    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_SIGN, 3 /* key_id */, auth_set));

    // Key 4 should fail, because table is full.
    EXPECT_EQ(KM_ERROR_TOO_MANY_OPERATIONS,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, 4 /* key_id */, auth_set));

    // Key 1 still works, because it's already in the table and hasn't reached max.
    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_SIGN, 1 /* key_id */, auth_set));

    // Key 1 no longer works, because it's reached max.
    EXPECT_EQ(KM_ERROR_KEY_MAX_OPS_EXCEEDED,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, 1 /* key_id */, auth_set));

    // Key 4 should fail, because table is (still and forever) full.
    EXPECT_EQ(KM_ERROR_TOO_MANY_OPERATIONS,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, 4 /* key_id */, auth_set));

    // Pubkey ops allowed.
    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 1 /* key_id */, auth_set));
    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 4 /* key_id */, auth_set));
}

TEST_F(KeymasterBaseTest, TestInvalidTimeBetweenOps) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA), Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN),
        Authorization(TAG_MIN_SECONDS_BETWEEN_OPS, 10),
    };

    AuthorizationSet auth_set(params, array_length(params));

    keymaster_error_t kmer1 = kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set);
    keymaster_error_t kmer2 = kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set);
    keymaster_error_t kmer3 = kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set);

    ASSERT_EQ(KM_ERROR_OK, kmer1);
    kmen.tick(2);
    ASSERT_EQ(KM_ERROR_KEY_RATE_LIMIT_EXCEEDED, kmer2);

    // Allowed because it's a pubkey op.
    ASSERT_EQ(KM_ERROR_OK, kmer3);
}

TEST_F(KeymasterBaseTest, TestValidTimeBetweenOps) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN), Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY),
        Authorization(TAG_MIN_SECONDS_BETWEEN_OPS, 2),
    };

    AuthorizationSet auth_set(params, array_length(params));

    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set));
    kmen.tick();
    EXPECT_EQ(KM_ERROR_KEY_RATE_LIMIT_EXCEEDED,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set));
    kmen.tick();
    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set));
}

TEST_F(KeymasterBaseTest, TestOptTimeoutTableOverflow) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_ALGORITHM, KM_ALGORITHM_AES),
        Authorization(TAG_MIN_SECONDS_BETWEEN_OPS, 4),
        Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY),
    };

    AuthorizationSet auth_set(params, array_length(params));

    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 1 /* key_id */, auth_set));

    kmen.tick();

    // Key 1 fails because it's too soon
    EXPECT_EQ(KM_ERROR_KEY_RATE_LIMIT_EXCEEDED,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 1 /* key_id */, auth_set));

    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 2 /* key_id */, auth_set));

    kmen.tick();

    // Key 1 fails because it's too soon
    EXPECT_EQ(KM_ERROR_KEY_RATE_LIMIT_EXCEEDED,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 1 /* key_id */, auth_set));
    // Key 2 fails because it's too soon
    EXPECT_EQ(KM_ERROR_KEY_RATE_LIMIT_EXCEEDED,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 2 /* key_id */, auth_set));

    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 3 /* key_id */, auth_set));

    kmen.tick();

    // Key 1 fails because it's too soon
    EXPECT_EQ(KM_ERROR_KEY_RATE_LIMIT_EXCEEDED,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 1 /* key_id */, auth_set));
    // Key 2 fails because it's too soon
    EXPECT_EQ(KM_ERROR_KEY_RATE_LIMIT_EXCEEDED,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 2 /* key_id */, auth_set));
    // Key 3 fails because it's too soon
    EXPECT_EQ(KM_ERROR_KEY_RATE_LIMIT_EXCEEDED,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 3 /* key_id */, auth_set));
    // Key 4 fails because the table is full
    EXPECT_EQ(KM_ERROR_TOO_MANY_OPERATIONS,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 4 /* key_id */, auth_set));

    kmen.tick();

    // Key 4 succeeds because key 1 expired.
    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 4 /* key_id */, auth_set));

    // Key 1 fails because the table is full... and key 1 is no longer in it.
    EXPECT_EQ(KM_ERROR_TOO_MANY_OPERATIONS,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 1 /* key_id */, auth_set));
    // Key 2 fails because it's too soon
    EXPECT_EQ(KM_ERROR_KEY_RATE_LIMIT_EXCEEDED,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 2 /* key_id */, auth_set));
    // Key 3 fails because it's too soon
    EXPECT_EQ(KM_ERROR_KEY_RATE_LIMIT_EXCEEDED,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 3 /* key_id */, auth_set));

    kmen.tick();

    // Key 1 succeeds because key 2 expired
    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 1 /* key_id */, auth_set));
    // Key 2 fails because the table is full... and key 2 is no longer in it.
    EXPECT_EQ(KM_ERROR_TOO_MANY_OPERATIONS,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 2 /* key_id */, auth_set));
    // Key 3 fails because it's too soon
    EXPECT_EQ(KM_ERROR_KEY_RATE_LIMIT_EXCEEDED,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 3 /* key_id */, auth_set));
    // Key 4 fails because it's too soon
    EXPECT_EQ(KM_ERROR_KEY_RATE_LIMIT_EXCEEDED,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 4 /* key_id */, auth_set));

    kmen.tick(4);

    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 1 /* key_id */, auth_set));
    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 2 /* key_id */, auth_set));
    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 3 /* key_id */, auth_set));
}

TEST_F(KeymasterBaseTest, TestPubkeyOptTimeoutTableOverflow) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA),
        Authorization(TAG_MIN_SECONDS_BETWEEN_OPS, 4), Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN),
    };

    AuthorizationSet auth_set(params, array_length(params));

    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_SIGN, 1 /* key_id */, auth_set));

    kmen.tick();

    // Key 1 fails because it's too soon
    EXPECT_EQ(KM_ERROR_KEY_RATE_LIMIT_EXCEEDED,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, 1 /* key_id */, auth_set));
    // Too soo, but pubkey ops allowed.
    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, 1 /* key_id */, auth_set));
}

TEST_F(KeymasterBaseTest, TestInvalidPurpose) {
    keymaster_purpose_t invalidPurpose1 = static_cast<keymaster_purpose_t>(-1);
    keymaster_purpose_t invalidPurpose2 = static_cast<keymaster_purpose_t>(4);

    AuthorizationSet auth_set(
        AuthorizationSetBuilder().Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY));

    EXPECT_EQ(KM_ERROR_UNSUPPORTED_PURPOSE,
              kmen.AuthorizeOperation(invalidPurpose1, key_id, auth_set));
    EXPECT_EQ(KM_ERROR_UNSUPPORTED_PURPOSE,
              kmen.AuthorizeOperation(invalidPurpose2, key_id, auth_set));
}

TEST_F(KeymasterBaseTest, TestIncompatiblePurposeSymmetricKey) {
    AuthorizationSet auth_set(AuthorizationSetBuilder()
                                  .Authorization(TAG_ALGORITHM, KM_ALGORITHM_AES)
                                  .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                                  .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN));

    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set));
    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set));

    EXPECT_EQ(KM_ERROR_INCOMPATIBLE_PURPOSE,
              kmen.AuthorizeOperation(KM_PURPOSE_ENCRYPT, key_id, auth_set));
    EXPECT_EQ(KM_ERROR_INCOMPATIBLE_PURPOSE,
              kmen.AuthorizeOperation(KM_PURPOSE_DECRYPT, key_id, auth_set));
}

TEST_F(KeymasterBaseTest, TestIncompatiblePurposeAssymmetricKey) {
    AuthorizationSet auth_set(AuthorizationSetBuilder()
                                  .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                                  .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                                  .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN));

    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set));
    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set));

    // This one is allowed because it's a pubkey op.
    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_ENCRYPT, key_id, auth_set));
    EXPECT_EQ(KM_ERROR_INCOMPATIBLE_PURPOSE,
              kmen.AuthorizeOperation(KM_PURPOSE_DECRYPT, key_id, auth_set));
}

TEST_F(KeymasterBaseTest, TestInvalidCallerNonce) {
    AuthorizationSet no_caller_nonce(AuthorizationSetBuilder()
                                         .Authorization(TAG_PURPOSE, KM_PURPOSE_ENCRYPT)
                                         .Authorization(TAG_PURPOSE, KM_PURPOSE_DECRYPT)
                                         .Authorization(TAG_ALGORITHM, KM_ALGORITHM_AES));
    AuthorizationSet caller_nonce(AuthorizationSetBuilder()
                                      .Authorization(TAG_PURPOSE, KM_PURPOSE_ENCRYPT)
                                      .Authorization(TAG_PURPOSE, KM_PURPOSE_DECRYPT)
                                      .Authorization(TAG_ALGORITHM, KM_ALGORITHM_HMAC)
                                      .Authorization(TAG_CALLER_NONCE));
    AuthorizationSet begin_params(AuthorizationSetBuilder().Authorization(TAG_NONCE, "foo", 3));

    EXPECT_EQ(KM_ERROR_OK,
              kmen.AuthorizeOperation(KM_PURPOSE_ENCRYPT, key_id, caller_nonce, begin_params,
                                      0 /* challenge */, true /* is_begin_operation */));
    EXPECT_EQ(KM_ERROR_OK,
              kmen.AuthorizeOperation(KM_PURPOSE_DECRYPT, key_id, caller_nonce, begin_params,
                                      0 /* challenge */, true /* is_begin_operation */));
    EXPECT_EQ(KM_ERROR_CALLER_NONCE_PROHIBITED,
              kmen.AuthorizeOperation(KM_PURPOSE_ENCRYPT, key_id, no_caller_nonce, begin_params,
                                      0 /* challenge */, true /* is_begin_operation */));
    EXPECT_EQ(KM_ERROR_OK,
              kmen.AuthorizeOperation(KM_PURPOSE_DECRYPT, key_id, no_caller_nonce, begin_params,
                                      0 /* challenge */, true /* is_begin_operation */));
}

TEST_F(KeymasterBaseTest, TestBootloaderOnly) {
    AuthorizationSet auth_set(AuthorizationSetBuilder()
                                  .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
                                  .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                                  .Authorization(TAG_BOOTLOADER_ONLY));
    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set));

    // Pubkey ops allowed.
    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set));
}

TEST_F(KeymasterBaseTest, TestInvalidTag) {
    AuthorizationSet auth_set(AuthorizationSetBuilder()
                                  .Authorization(TAG_INVALID)
                                  .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN));

    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set));
}

TEST_F(KeymasterBaseTest, TestAuthPerOpSuccess) {
    hw_auth_token_t token;
    memset(&token, 0, sizeof(token));
    token.version = HW_AUTH_TOKEN_VERSION;
    token.challenge = 99;
    token.user_id = 9;
    token.authenticator_id = 0;
    token.authenticator_type = hton(static_cast<uint32_t>(HW_AUTH_PASSWORD));
    token.timestamp = 0;

    AuthorizationSet auth_set(AuthorizationSetBuilder()
                                  .Authorization(TAG_USER_SECURE_ID, token.user_id)
                                  .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_ANY)
                                  .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN));

    AuthorizationSet op_params;
    op_params.push_back(Authorization(TAG_AUTH_TOKEN, &token, sizeof(token)));

    EXPECT_EQ(KM_ERROR_OK,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set, op_params, token.challenge,
                                      false /* is_begin_operation */));
}

TEST_F(KeymasterBaseTest, TestAuthPerOpInvalidTokenSignature) {
    hw_auth_token_t token;
    memset(&token, 0, sizeof(token));
    token.version = HW_AUTH_TOKEN_VERSION;
    token.challenge = 99;
    token.user_id = 9;
    token.authenticator_id = 0;
    token.authenticator_type = hton(static_cast<uint32_t>(HW_AUTH_PASSWORD));
    token.timestamp = 0;

    AuthorizationSet auth_set(AuthorizationSetBuilder()
                                  .Authorization(TAG_ALGORITHM, KM_ALGORITHM_EC)
                                  .Authorization(TAG_USER_SECURE_ID, token.user_id)
                                  .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_ANY)
                                  .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN));

    AuthorizationSet op_params;
    op_params.push_back(Authorization(TAG_AUTH_TOKEN, &token, sizeof(token)));

    kmen.set_report_token_valid(false);
    EXPECT_EQ(KM_ERROR_KEY_USER_NOT_AUTHENTICATED,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set, op_params, token.challenge,
                                      false /* is_begin_operation */));
    // Pubkey ops allowed.
    EXPECT_EQ(KM_ERROR_OK,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set, op_params,
                                      token.challenge, false /* is_begin_operation */));
}

TEST_F(KeymasterBaseTest, TestAuthPerOpWrongChallenge) {
    hw_auth_token_t token;
    memset(&token, 0, sizeof(token));
    token.version = HW_AUTH_TOKEN_VERSION;
    token.challenge = 99;
    token.user_id = 9;
    token.authenticator_id = 0;
    token.authenticator_type = hton(static_cast<uint32_t>(HW_AUTH_PASSWORD));
    token.timestamp = 0;

    AuthorizationSet auth_set(AuthorizationSetBuilder()
                                  .Authorization(TAG_USER_SECURE_ID, token.user_id)
                                  .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_ANY)
                                  .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN));

    AuthorizationSet op_params;
    op_params.push_back(Authorization(TAG_AUTH_TOKEN, &token, sizeof(token)));

    EXPECT_EQ(KM_ERROR_KEY_USER_NOT_AUTHENTICATED,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set, op_params,
                                      token.challenge + 1 /* doesn't match token */,
                                      false /* is_begin_operation */));
}

TEST_F(KeymasterBaseTest, TestAuthPerOpNoAuthType) {
    hw_auth_token_t token;
    memset(&token, 0, sizeof(token));
    token.version = HW_AUTH_TOKEN_VERSION;
    token.challenge = 99;
    token.user_id = 9;
    token.authenticator_id = 0;
    token.authenticator_type = hton(static_cast<uint32_t>(HW_AUTH_PASSWORD));
    token.timestamp = 0;

    AuthorizationSet auth_set(AuthorizationSetBuilder()
                                  .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                                  .Authorization(TAG_USER_SECURE_ID, token.user_id)
                                  .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN));

    AuthorizationSet op_params;
    op_params.push_back(Authorization(TAG_AUTH_TOKEN, &token, sizeof(token)));

    EXPECT_EQ(KM_ERROR_KEY_USER_NOT_AUTHENTICATED,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set, op_params, token.challenge,
                                      false /* is_begin_operation */));
    // Pubkey ops allowed.
    EXPECT_EQ(KM_ERROR_OK,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set, op_params,
                                      token.challenge, false /* is_begin_operation */));
}

TEST_F(KeymasterBaseTest, TestAuthPerOpWrongAuthType) {
    hw_auth_token_t token;
    memset(&token, 0, sizeof(token));
    token.version = HW_AUTH_TOKEN_VERSION;
    token.challenge = 99;
    token.user_id = 9;
    token.authenticator_id = 0;
    token.authenticator_type = hton(static_cast<uint32_t>(HW_AUTH_PASSWORD));
    token.timestamp = 0;

    AuthorizationSet auth_set(
        AuthorizationSetBuilder()
            .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
            .Authorization(TAG_USER_SECURE_ID, token.user_id)
            .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_FINGERPRINT /* doesn't match token */)
            .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN));

    AuthorizationSet op_params;
    op_params.push_back(Authorization(TAG_AUTH_TOKEN, &token, sizeof(token)));

    EXPECT_EQ(KM_ERROR_KEY_USER_NOT_AUTHENTICATED,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set, op_params, token.challenge,
                                      false /* is_begin_operation */));
    // Pubkey ops allowed.
    EXPECT_EQ(KM_ERROR_OK,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set, op_params,
                                      token.challenge, false /* is_begin_operation */));
}

TEST_F(KeymasterBaseTest, TestAuthPerOpWrongSid) {
    hw_auth_token_t token;
    memset(&token, 0, sizeof(token));
    token.version = HW_AUTH_TOKEN_VERSION;
    token.challenge = 99;
    token.user_id = 9;
    token.authenticator_id = 0;
    token.authenticator_type = hton(static_cast<uint32_t>(HW_AUTH_PASSWORD));
    token.timestamp = 0;

    AuthorizationSet auth_set(
        AuthorizationSetBuilder()
            .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
            .Authorization(TAG_USER_SECURE_ID, token.user_id + 1 /* doesn't match token */)
            .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_ANY)
            .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN));

    AuthorizationSet op_params;
    op_params.push_back(Authorization(TAG_AUTH_TOKEN, &token, sizeof(token)));

    EXPECT_EQ(KM_ERROR_KEY_USER_NOT_AUTHENTICATED,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set, op_params, token.challenge,
                                      false /* is_begin_operation */));
    // Pubkey op allowed.
    EXPECT_EQ(KM_ERROR_OK,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set, op_params,
                                      token.challenge, false /* is_begin_operation */));
}

TEST_F(KeymasterBaseTest, TestAuthPerOpSuccessAlternateSid) {
    hw_auth_token_t token;
    memset(&token, 0, sizeof(token));
    token.version = HW_AUTH_TOKEN_VERSION;
    token.challenge = 99;
    token.user_id = 9;
    token.authenticator_id = 10;
    token.authenticator_type = hton(static_cast<uint32_t>(HW_AUTH_PASSWORD));
    token.timestamp = 0;

    AuthorizationSet auth_set(AuthorizationSetBuilder()
                                  .Authorization(TAG_USER_SECURE_ID, token.authenticator_id)
                                  .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_ANY)
                                  .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN));

    AuthorizationSet op_params;
    op_params.push_back(Authorization(TAG_AUTH_TOKEN, &token, sizeof(token)));

    EXPECT_EQ(KM_ERROR_OK,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set, op_params, token.challenge,
                                      false /* is_begin_operation */));
}

TEST_F(KeymasterBaseTest, TestAuthPerOpMissingToken) {
    hw_auth_token_t token;
    memset(&token, 0, sizeof(token));
    token.version = HW_AUTH_TOKEN_VERSION;
    token.challenge = 99;
    token.user_id = 9;
    token.authenticator_id = 0;
    token.authenticator_type = hton(static_cast<uint32_t>(HW_AUTH_PASSWORD));
    token.timestamp = 0;

    AuthorizationSet auth_set(AuthorizationSetBuilder()
                                  .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                                  .Authorization(TAG_USER_SECURE_ID, token.user_id)
                                  .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_ANY)
                                  .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN));

    AuthorizationSet op_params;

    // During begin we can skip the auth token
    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set, op_params,
                                                   token.challenge, true /* is_begin_operation */));
    // Afterwards we must have authentication
    EXPECT_EQ(KM_ERROR_KEY_USER_NOT_AUTHENTICATED,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set, op_params, token.challenge,
                                      false /* is_begin_operation */));
    // Pubkey ops allowed
    EXPECT_EQ(KM_ERROR_OK,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set, op_params,
                                      token.challenge, false /* is_begin_operation */));

    auth_set.Reinitialize(AuthorizationSetBuilder()
                              .Authorization(TAG_ALGORITHM, KM_ALGORITHM_AES)
                              .Authorization(TAG_USER_SECURE_ID, token.user_id)
                              .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_ANY)
                              .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
                              .build());

    EXPECT_EQ(KM_ERROR_KEY_USER_NOT_AUTHENTICATED,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set, op_params,
                                      token.challenge, false /* is_begin_operation */));
}

TEST_F(KeymasterBaseTest, TestAuthAndNoAuth) {
    AuthorizationSet auth_set(AuthorizationSetBuilder()
                                  .Authorization(TAG_USER_SECURE_ID, 1)
                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                  .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN));

    EXPECT_EQ(KM_ERROR_INVALID_KEY_BLOB,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set));
}

TEST_F(KeymasterBaseTest, TestTimedAuthSuccess) {
    hw_auth_token_t token;
    memset(&token, 0, sizeof(token));
    token.version = HW_AUTH_TOKEN_VERSION;
    token.challenge = 99;
    token.user_id = 9;
    token.authenticator_id = 0;
    token.authenticator_type = hton(static_cast<uint32_t>(HW_AUTH_PASSWORD));
    token.timestamp = hton(kmen.current_time());

    AuthorizationSet auth_set(AuthorizationSetBuilder()
                                  .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                                  .Authorization(TAG_USER_SECURE_ID, token.user_id)
                                  .Authorization(TAG_AUTH_TIMEOUT, 1)
                                  .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_ANY)
                                  .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN));

    AuthorizationSet op_params;
    op_params.push_back(Authorization(TAG_AUTH_TOKEN, &token, sizeof(token)));

    EXPECT_EQ(KM_ERROR_OK,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set, op_params,
                                      0 /* irrelevant */, false /* is_begin_operation */));
}

TEST_F(KeymasterBaseTest, TestTimedAuthTimedOut) {
    hw_auth_token_t token;
    memset(&token, 0, sizeof(token));
    token.version = HW_AUTH_TOKEN_VERSION;
    token.challenge = 99;
    token.user_id = 9;
    token.authenticator_id = 0;
    token.authenticator_type = hton(static_cast<uint32_t>(HW_AUTH_PASSWORD));
    token.timestamp = hton(static_cast<uint64_t>(kmen.current_time()));

    AuthorizationSet auth_set(AuthorizationSetBuilder()
                                  .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                                  .Authorization(TAG_USER_SECURE_ID, token.user_id)
                                  .Authorization(TAG_AUTH_TIMEOUT, 1)
                                  .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_ANY)
                                  .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN));

    AuthorizationSet op_params;
    op_params.push_back(Authorization(TAG_AUTH_TOKEN, &token, sizeof(token)));

    EXPECT_EQ(KM_ERROR_OK,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set, op_params,
                                      0 /* irrelevant */, false /* is_begin_operation */));

    kmen.tick(1);

    // token still good
    EXPECT_EQ(KM_ERROR_OK,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set, op_params,
                                      0 /* irrelevant */, false /* is_begin_operation */));

    kmen.tick(1);

    // token expired, not allowed during begin.
    EXPECT_EQ(KM_ERROR_KEY_USER_NOT_AUTHENTICATED,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set, op_params,
                                      0 /* irrelevant */, true /* is_begin_operation */));

    // token expired, afterwards it's okay.
    EXPECT_EQ(KM_ERROR_OK,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set, op_params,
                                      0 /* irrelevant */, false /* is_begin_operation */));

    // Pubkey ops allowed.
    EXPECT_EQ(KM_ERROR_OK,
              kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set, op_params,
                                      0 /* irrelevant */, true /* is_begin_operation */));
}

TEST_F(KeymasterBaseTest, TestTimedAuthMissingToken) {
    hw_auth_token_t token;
    memset(&token, 0, sizeof(token));
    token.version = HW_AUTH_TOKEN_VERSION;
    token.challenge = 99;
    token.user_id = 9;
    token.authenticator_id = 0;
    token.authenticator_type = hton(static_cast<uint32_t>(HW_AUTH_PASSWORD));
    token.timestamp = hton(static_cast<uint64_t>(kmen.current_time()));

    AuthorizationSet auth_set(AuthorizationSetBuilder()
                                  .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                                  .Authorization(TAG_USER_SECURE_ID, token.user_id)
                                  .Authorization(TAG_AUTH_TIMEOUT, 1)
                                  .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_ANY)
                                  .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN));

    AuthorizationSet op_params;

    // Unlike auth-per-op, must have the auth token during begin.
    EXPECT_EQ(KM_ERROR_KEY_USER_NOT_AUTHENTICATED,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set, op_params, token.challenge,
                                      true /* is_begin_operation */));

    // Later we don't check (though begin would fail, so there wouldn't be a later).
    EXPECT_EQ(KM_ERROR_OK,
              kmen.AuthorizeOperation(KM_PURPOSE_SIGN, key_id, auth_set, op_params, token.challenge,
                                      false /* is_begin_operation */));

    // Pubkey ops allowed.
    EXPECT_EQ(KM_ERROR_OK, kmen.AuthorizeOperation(KM_PURPOSE_VERIFY, key_id, auth_set, op_params,
                                                   token.challenge, true /* is_begin_operation */));
}

TEST_F(KeymasterBaseTest, TestCreateKeyId) {
    keymaster_key_blob_t blob = {reinterpret_cast<const uint8_t*>("foobar"), 6};

    km_id_t key_id = 0;
    EXPECT_TRUE(KeymasterEnforcement::CreateKeyId(blob, &key_id));
    EXPECT_NE(0U, key_id);
}

}; /* namespace test */
}; /* namespace keymaster */
