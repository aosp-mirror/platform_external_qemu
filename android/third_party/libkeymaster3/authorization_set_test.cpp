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

#include <gtest/gtest.h>

#include <keymaster/authorization_set.h>
#include <keymaster/android_keymaster_utils.h>

#include "android_keymaster_test_utils.h"

namespace keymaster {

namespace test {

TEST(Construction, ListProvided) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN), Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY),
        Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA), Authorization(TAG_USER_ID, 7),
        Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD),
        Authorization(TAG_APPLICATION_ID, "my_app", 6), Authorization(TAG_KEY_SIZE, 256),
        Authorization(TAG_AUTH_TIMEOUT, 300),
    };
    AuthorizationSet set(params, array_length(params));
    EXPECT_EQ(8U, set.size());
}

TEST(Construction, Copy) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN), Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY),
        Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA), Authorization(TAG_USER_ID, 7),
        Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD),
        Authorization(TAG_APPLICATION_ID, "my_app", 6), Authorization(TAG_KEY_SIZE, 256),
        Authorization(TAG_AUTH_TIMEOUT, 300),
    };
    AuthorizationSet set(params, array_length(params));
    AuthorizationSet set2(set);
    EXPECT_EQ(set, set2);
}

TEST(Construction, NullProvided) {
    keymaster_key_param_t params[] = {
        Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN), Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY),
    };

    AuthorizationSet set1(params, 0);
    EXPECT_EQ(0U, set1.size());
    EXPECT_EQ(AuthorizationSet::OK, set1.is_valid());

    AuthorizationSet set2(reinterpret_cast<keymaster_key_param_t*>(NULL), array_length(params));
    EXPECT_EQ(0U, set2.size());
    EXPECT_EQ(AuthorizationSet::OK, set2.is_valid());
}

TEST(Lookup, NonRepeated) {
    AuthorizationSet set(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD)
                             .Authorization(TAG_APPLICATION_ID, "my_app", 6)
                             .Authorization(TAG_KEY_SIZE, 256)
                             .Authorization(TAG_AUTH_TIMEOUT, 300));

    EXPECT_EQ(8U, set.size());

    int pos = set.find(TAG_ALGORITHM);
    ASSERT_NE(-1, pos);
    EXPECT_EQ(KM_TAG_ALGORITHM, set[pos].tag);
    EXPECT_EQ(KM_ALGORITHM_RSA, set[pos].enumerated);

    pos = set.find(TAG_MAC_LENGTH);
    EXPECT_EQ(-1, pos);

    uint32_t int_val = 0;
    EXPECT_TRUE(set.GetTagValue(TAG_USER_ID, &int_val));
    EXPECT_EQ(7U, int_val);

    keymaster_blob_t blob_val;
    EXPECT_TRUE(set.GetTagValue(TAG_APPLICATION_ID, &blob_val));
    EXPECT_EQ(6U, blob_val.data_length);
    EXPECT_EQ(0, memcmp(blob_val.data, "my_app", 6));
}

TEST(Lookup, Repeated) {
    AuthorizationSet set(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_USER_SECURE_ID, 47727)
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD)
                             .Authorization(TAG_APPLICATION_ID, "my_app", 6)
                             .Authorization(TAG_KEY_SIZE, 256)
                             .Authorization(TAG_AUTH_TIMEOUT, 300));
    EXPECT_EQ(9U, set.size());

    int pos = set.find(TAG_PURPOSE);
    ASSERT_FALSE(pos == -1);
    EXPECT_EQ(KM_TAG_PURPOSE, set[pos].tag);
    EXPECT_EQ(KM_PURPOSE_SIGN, set[pos].enumerated);

    pos = set.find(TAG_PURPOSE, pos);
    EXPECT_EQ(KM_TAG_PURPOSE, set[pos].tag);
    EXPECT_EQ(KM_PURPOSE_VERIFY, set[pos].enumerated);

    EXPECT_EQ(-1, set.find(TAG_PURPOSE, pos));

    pos = set.find(TAG_USER_SECURE_ID, pos);
    EXPECT_EQ(KM_TAG_USER_SECURE_ID, set[pos].tag);
    EXPECT_EQ(47727U, set[pos].long_integer);
}

TEST(Lookup, Indexed) {
    AuthorizationSet set(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD)
                             .Authorization(TAG_APPLICATION_ID, "my_app", 6)
                             .Authorization(TAG_KEY_SIZE, 256)
                             .Authorization(TAG_AUTH_TIMEOUT, 300));
    EXPECT_EQ(8U, set.size());

    EXPECT_EQ(KM_TAG_PURPOSE, set[0].tag);
    EXPECT_EQ(KM_PURPOSE_SIGN, set[0].enumerated);

    // Lookup beyond end doesn't work, just returns zeros, but doens't blow up either (verify by
    // running under valgrind).
    EXPECT_EQ(KM_TAG_INVALID, set[10].tag);
}

TEST(Serialization, RoundTrip) {
    AuthorizationSet set(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD)
                             .Authorization(TAG_APPLICATION_ID, "my_app", 6)
                             .Authorization(TAG_KEY_SIZE, 256)
                             .Authorization(TAG_USER_SECURE_ID, 47727)
                             .Authorization(TAG_AUTH_TIMEOUT, 300)
                             .Authorization(TAG_ALL_USERS)
                             .Authorization(TAG_RSA_PUBLIC_EXPONENT, 3)
                             .Authorization(TAG_ACTIVE_DATETIME, 10));

    size_t size = set.SerializedSize();
    EXPECT_TRUE(size > 0);

    UniquePtr<uint8_t[]> buf(new uint8_t[size]);
    EXPECT_EQ(buf.get() + size, set.Serialize(buf.get(), buf.get() + size));
    AuthorizationSet deserialized(buf.get(), size);

    EXPECT_EQ(AuthorizationSet::OK, deserialized.is_valid());
    EXPECT_EQ(set, deserialized);

    int pos = deserialized.find(TAG_APPLICATION_ID);
    ASSERT_NE(-1, pos);
    EXPECT_EQ(KM_TAG_APPLICATION_ID, deserialized[pos].tag);
    EXPECT_EQ(6U, deserialized[pos].blob.data_length);
    EXPECT_EQ(0, memcmp(deserialized[pos].blob.data, "my_app", 6));
}

TEST(Deserialization, Deserialize) {
    AuthorizationSet set(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD)
                             .Authorization(TAG_APPLICATION_ID, "my_app", 6)
                             .Authorization(TAG_KEY_SIZE, 256)
                             .Authorization(TAG_AUTH_TIMEOUT, 300));

    size_t size = set.SerializedSize();
    EXPECT_TRUE(size > 0);

    UniquePtr<uint8_t[]> buf(new uint8_t[size]);
    EXPECT_EQ(buf.get() + size, set.Serialize(buf.get(), buf.get() + size));
    AuthorizationSet deserialized;
    const uint8_t* p = buf.get();
    EXPECT_TRUE(deserialized.Deserialize(&p, p + size));
    EXPECT_EQ(p, buf.get() + size);

    EXPECT_EQ(AuthorizationSet::OK, deserialized.is_valid());

    EXPECT_EQ(set.size(), deserialized.size());
    for (size_t i = 0; i < set.size(); ++i) {
        EXPECT_EQ(set[i].tag, deserialized[i].tag);
    }

    int pos = deserialized.find(TAG_APPLICATION_ID);
    ASSERT_NE(-1, pos);
    EXPECT_EQ(KM_TAG_APPLICATION_ID, deserialized[pos].tag);
    EXPECT_EQ(6U, deserialized[pos].blob.data_length);
    EXPECT_EQ(0, memcmp(deserialized[pos].blob.data, "my_app", 6));
}

TEST(Deserialization, TooShortBuffer) {
    uint8_t buf[] = {0, 0, 0};
    AuthorizationSet deserialized(buf, array_length(buf));
    EXPECT_EQ(AuthorizationSet::MALFORMED_DATA, deserialized.is_valid());

    const uint8_t* p = buf;
    EXPECT_FALSE(deserialized.Deserialize(&p, p + array_length(buf)));
    EXPECT_EQ(AuthorizationSet::MALFORMED_DATA, deserialized.is_valid());
}

TEST(Deserialization, InvalidLengthField) {
    AuthorizationSet set(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD)
                             .Authorization(TAG_APPLICATION_ID, "my_app", 6)
                             .Authorization(TAG_KEY_SIZE, 256)
                             .Authorization(TAG_AUTH_TIMEOUT, 300));

    size_t size = set.SerializedSize();
    EXPECT_TRUE(size > 0);

    UniquePtr<uint8_t[]> buf(new uint8_t[size]);
    EXPECT_EQ(buf.get() + size, set.Serialize(buf.get(), buf.get() + size));
    *reinterpret_cast<uint32_t*>(buf.get()) = 9;

    AuthorizationSet deserialized(buf.get(), size);
    EXPECT_EQ(AuthorizationSet::MALFORMED_DATA, deserialized.is_valid());

    const uint8_t* p = buf.get();
    EXPECT_FALSE(deserialized.Deserialize(&p, p + size));
    EXPECT_EQ(AuthorizationSet::MALFORMED_DATA, deserialized.is_valid());
}

static uint32_t read_uint32(const uint8_t* buf) {
    uint32_t val;
    memcpy(&val, buf, sizeof(val));
    return val;
}

static void add_to_uint32(uint8_t* buf, int delta) {
    uint32_t val;
    memcpy(&val, buf, sizeof(val));
    val += delta;
    memcpy(buf, &val, sizeof(val));
}

TEST(Deserialization, MalformedIndirectData) {
    AuthorizationSet set(AuthorizationSetBuilder()
                             .Authorization(TAG_APPLICATION_ID, "my_app", 6)
                             .Authorization(TAG_APPLICATION_DATA, "foo", 3));
    size_t size = set.SerializedSize();

    UniquePtr<uint8_t[]> buf(new uint8_t[size]);
    EXPECT_EQ(buf.get() + size, set.Serialize(buf.get(), buf.get() + size));

    // This sucks.  This test, as written, requires intimate knowledge of the serialized layout of
    // this particular set, which means it's brittle.  But it's important to test that we handle
    // broken serialized data and I can't think of a better way to write this.
    //
    // The contents of buf are:
    //
    // Bytes:   Content:
    // 0-3      Length of string data, which is 9.
    // 4-9      "my_app"
    // 10-12    "foo"
    // 13-16    Number of elements, which is 2.
    // 17-20    Length of elements, which is 24.
    // 21-24    First tag, TAG_APPLICATION_ID
    // 25-28    Length of string "my_app", 6
    // 29-32    Offset of string "my_app", 0
    // 33-36    Second tag, TAG_APPLICATION_DATA
    // 37-40    Length of string "foo", 3
    // 41-44    Offset of string "foo", 6

    // Check that stuff is where we think.
    EXPECT_EQ('m', buf[4]);
    EXPECT_EQ('f', buf[10]);
    // Length of "my_app"
    EXPECT_EQ(6U, read_uint32(buf.get() + 25));
    // Offset of "my_app"
    EXPECT_EQ(0U, read_uint32(buf.get() + 29));
    // Length of "foo"
    EXPECT_EQ(3U, read_uint32(buf.get() + 37));
    // Offset of "foo"
    EXPECT_EQ(6U, read_uint32(buf.get() + 41));

    // Check that deserialization works.
    AuthorizationSet deserialized1(buf.get(), size);
    EXPECT_EQ(AuthorizationSet::OK, deserialized1.is_valid());

    const uint8_t* p = buf.get();
    EXPECT_TRUE(deserialized1.Deserialize(&p, p + size));
    EXPECT_EQ(AuthorizationSet::OK, deserialized1.is_valid());

    //
    // Now mess them up in various ways:
    //

    // Move "foo" offset so offset + length goes off the end
    add_to_uint32(buf.get() + 41, 1);
    AuthorizationSet deserialized2(buf.get(), size);
    EXPECT_EQ(AuthorizationSet::MALFORMED_DATA, deserialized2.is_valid());
    add_to_uint32(buf.get() + 41, -1);

    // Shorten the "my_app" length to make a gap between the blobs.
    add_to_uint32(buf.get() + 25, -1);
    AuthorizationSet deserialized3(buf.get(), size);
    EXPECT_EQ(AuthorizationSet::MALFORMED_DATA, deserialized3.is_valid());
    add_to_uint32(buf.get() + 25, 1);

    // Extend the "my_app" length to make them overlap, and decrease the "foo" length to keep the
    // total length the same.  We don't detect this but should.
    // TODO(swillden): Detect overlaps and holes that leave total size correct.
    add_to_uint32(buf.get() + 25, 1);
    add_to_uint32(buf.get() + 37, -1);
    AuthorizationSet deserialized4(buf.get(), size);
    EXPECT_EQ(AuthorizationSet::OK, deserialized4.is_valid());
}

TEST(Growable, SuccessfulRoundTrip) {
    AuthorizationSet growable;
    EXPECT_TRUE(growable.push_back(Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)));
    EXPECT_EQ(1U, growable.size());

    EXPECT_TRUE(growable.push_back(Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)));
    EXPECT_EQ(2U, growable.size());

    EXPECT_TRUE(growable.push_back(Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)));
    EXPECT_EQ(3U, growable.size());

    EXPECT_TRUE(growable.push_back(Authorization(TAG_APPLICATION_ID, "data", 4)));
    EXPECT_EQ(4U, growable.size());

    EXPECT_TRUE(growable.push_back(Authorization(TAG_APPLICATION_DATA, "some more data", 14)));
    EXPECT_EQ(5U, growable.size());

    size_t serialize_size = growable.SerializedSize();
    UniquePtr<uint8_t[]> serialized(new uint8_t[serialize_size]);
    EXPECT_EQ(serialized.get() + serialize_size,
              growable.Serialize(serialized.get(), serialized.get() + serialize_size));

    AuthorizationSet deserialized(serialized.get(), serialize_size);
    EXPECT_EQ(growable, deserialized);
}

TEST(Growable, InsufficientElemBuf) {
    AuthorizationSet growable;
    EXPECT_EQ(AuthorizationSet::OK, growable.is_valid());

    // First insertion fits.
    EXPECT_TRUE(growable.push_back(Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)));
    EXPECT_EQ(1U, growable.size());
    EXPECT_EQ(AuthorizationSet::OK, growable.is_valid());

    // Second does too.
    EXPECT_TRUE(growable.push_back(Authorization(TAG_RSA_PUBLIC_EXPONENT, 3)));
    EXPECT_EQ(2U, growable.size());
}

TEST(Growable, InsufficientIndirectBuf) {
    AuthorizationSet growable;
    EXPECT_EQ(AuthorizationSet::OK, growable.is_valid());

    EXPECT_TRUE(growable.push_back(Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)));
    EXPECT_EQ(1U, growable.size());
    EXPECT_EQ(AuthorizationSet::OK, growable.is_valid());

    EXPECT_TRUE(growable.push_back(Authorization(TAG_APPLICATION_ID, "1234567890", 10)));
    EXPECT_EQ(2U, growable.size());
    EXPECT_EQ(AuthorizationSet::OK, growable.is_valid());

    EXPECT_TRUE(growable.push_back(Authorization(TAG_APPLICATION_DATA, "1", 1)));
    EXPECT_EQ(3U, growable.size());
    EXPECT_EQ(AuthorizationSet::OK, growable.is_valid());

    // Can still add another entry without indirect data.  Now it's full.
    EXPECT_TRUE(growable.push_back(Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)));
    EXPECT_EQ(4U, growable.size());
    EXPECT_EQ(AuthorizationSet::OK, growable.is_valid());
}

TEST(Growable, PushBackSets) {
    AuthorizationSetBuilder builder;
    builder.Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
        .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
        .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
        .Authorization(TAG_USER_ID, 7)
        .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD)
        .Authorization(TAG_APPLICATION_ID, "my_app", 6)
        .Authorization(TAG_KEY_SIZE, 256)
        .Authorization(TAG_AUTH_TIMEOUT, 300);

    AuthorizationSet set1(builder.build());
    AuthorizationSet set2(builder.build());

    AuthorizationSet combined;
    EXPECT_TRUE(combined.push_back(set1));
    EXPECT_TRUE(combined.push_back(set2));
    EXPECT_EQ(set1.size() + set2.size(), combined.size());
    EXPECT_EQ(12U, combined.indirect_size());
}

TEST(GetValue, GetInt) {
    AuthorizationSet set(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD)
                             .Authorization(TAG_APPLICATION_ID, "my_app", 6)
                             .Authorization(TAG_AUTH_TIMEOUT, 300));

    uint32_t val;
    EXPECT_TRUE(set.GetTagValue(TAG_USER_ID, &val));
    EXPECT_EQ(7U, val);

    // Find one that isn't there
    EXPECT_FALSE(set.GetTagValue(TAG_KEY_SIZE, &val));
}

TEST(GetValue, GetLong) {
    AuthorizationSet set1(AuthorizationSetBuilder()
                              .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
                              .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                              .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                              .Authorization(TAG_RSA_PUBLIC_EXPONENT, 3));

    AuthorizationSet set2(AuthorizationSetBuilder()
                              .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
                              .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                              .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA));

    uint64_t val;
    EXPECT_TRUE(set1.GetTagValue(TAG_RSA_PUBLIC_EXPONENT, &val));
    EXPECT_EQ(3U, val);

    // Find one that isn't there
    EXPECT_FALSE(set2.GetTagValue(TAG_RSA_PUBLIC_EXPONENT, &val));
}

TEST(GetValue, GetLongRep) {
    AuthorizationSet set1(AuthorizationSetBuilder()
                              .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
                              .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                              .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                              .Authorization(TAG_USER_SECURE_ID, 8338)
                              .Authorization(TAG_USER_SECURE_ID, 4334)
                              .Authorization(TAG_RSA_PUBLIC_EXPONENT, 3));

    AuthorizationSet set2(AuthorizationSetBuilder()
                              .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
                              .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                              .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA));

    uint64_t val;
    EXPECT_TRUE(set1.GetTagValue(TAG_USER_SECURE_ID, 0, &val));
    EXPECT_EQ(8338U, val);
    EXPECT_TRUE(set1.GetTagValue(TAG_USER_SECURE_ID, 1, &val));
    EXPECT_EQ(4334U, val);

    // Find one that isn't there
    EXPECT_FALSE(set2.GetTagValue(TAG_USER_SECURE_ID, &val));
}

TEST(GetValue, GetEnum) {
    AuthorizationSet set(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD)
                             .Authorization(TAG_APPLICATION_ID, "my_app", 6)
                             .Authorization(TAG_AUTH_TIMEOUT, 300));

    keymaster_algorithm_t val;
    EXPECT_TRUE(set.GetTagValue(TAG_ALGORITHM, &val));
    EXPECT_EQ(KM_ALGORITHM_RSA, val);

    // Find one that isn't there
    keymaster_padding_t val2;
    EXPECT_FALSE(set.GetTagValue(TAG_PADDING, &val2));
}

TEST(GetValue, GetEnumRep) {
    AuthorizationSet set(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD)
                             .Authorization(TAG_APPLICATION_ID, "my_app", 6)
                             .Authorization(TAG_AUTH_TIMEOUT, 300));

    keymaster_purpose_t val;
    EXPECT_TRUE(set.GetTagValue(TAG_PURPOSE, 0, &val));
    EXPECT_EQ(KM_PURPOSE_SIGN, val);
    EXPECT_TRUE(set.GetTagValue(TAG_PURPOSE, 1, &val));
    EXPECT_EQ(KM_PURPOSE_VERIFY, val);

    // Find one that isn't there
    EXPECT_FALSE(set.GetTagValue(TAG_PURPOSE, 2, &val));
}

TEST(GetValue, GetDate) {
    AuthorizationSet set(AuthorizationSetBuilder()
                             .Authorization(TAG_ACTIVE_DATETIME, 10)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD)
                             .Authorization(TAG_APPLICATION_ID, "my_app", 6)
                             .Authorization(TAG_AUTH_TIMEOUT, 300));

    uint64_t val;
    EXPECT_TRUE(set.GetTagValue(TAG_ACTIVE_DATETIME, &val));
    EXPECT_EQ(10U, val);

    // Find one that isn't there
    EXPECT_FALSE(set.GetTagValue(TAG_USAGE_EXPIRE_DATETIME, &val));
}

TEST(GetValue, GetBlob) {
    AuthorizationSet set(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA)
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD)
                             .Authorization(TAG_APPLICATION_ID, "my_app", 6)
                             .Authorization(TAG_AUTH_TIMEOUT, 300));

    keymaster_blob_t val;
    EXPECT_TRUE(set.GetTagValue(TAG_APPLICATION_ID, &val));
    EXPECT_EQ(6U, val.data_length);
    EXPECT_EQ(0, memcmp(val.data, "my_app", 6));

    // Find one that isn't there
    EXPECT_FALSE(set.GetTagValue(TAG_APPLICATION_DATA, &val));
}

TEST(Deduplication, NoDuplicates) {
    AuthorizationSet set(AuthorizationSetBuilder()
                             .Authorization(TAG_ACTIVE_DATETIME, 10)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD));
    AuthorizationSet copy(set);

    EXPECT_EQ(copy, set);
    set.Deduplicate();
    EXPECT_EQ(copy.size(), set.size());

    // Sets no longer compare equal, because of ordering (ugh, maybe it should be
    // AuthorizationList, not AuthorizationSet).
    EXPECT_NE(copy, set);
}

TEST(Deduplication, NoDuplicatesHasInvalid) {
    AuthorizationSet set(AuthorizationSetBuilder()
                             .Authorization(TAG_ACTIVE_DATETIME, 10)
                             .Authorization(TAG_INVALID)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD));
    AuthorizationSet copy(set);

    EXPECT_EQ(copy, set);
    set.Deduplicate();

    // Deduplicate should have removed the invalid.
    EXPECT_EQ(copy.size() - 1, set.size());
    EXPECT_NE(copy, set);
}

TEST(Deduplication, DuplicateEnum) {
    AuthorizationSet set(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ACTIVE_DATETIME, 10)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD));
    AuthorizationSet copy(set);

    EXPECT_EQ(copy, set);
    set.Deduplicate();
    EXPECT_EQ(copy.size() - 2, set.size());
    EXPECT_NE(copy, set);
}

TEST(Deduplication, DuplicateBlob) {
    AuthorizationSet set(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ACTIVE_DATETIME, 10)
                             .Authorization(TAG_APPLICATION_DATA, "data", 4)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_APPLICATION_DATA, "data", 4)
                             .Authorization(TAG_APPLICATION_DATA, "foo", 3)
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD));
    AuthorizationSet copy(set);

    EXPECT_EQ(copy, set);
    set.Deduplicate();
    EXPECT_EQ(copy.size() - 3, set.size());
    EXPECT_NE(copy, set);

    // The real test here is that valgrind reports no leak.
}

TEST(Union, Disjoint) {
    AuthorizationSet set1(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ACTIVE_DATETIME, 10)
                             .Authorization(TAG_APPLICATION_DATA, "data", 4));

    AuthorizationSet set2(AuthorizationSetBuilder()
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_APPLICATION_DATA, "foo", 3)
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD));

    AuthorizationSet expected(AuthorizationSetBuilder()
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_ACTIVE_DATETIME, 10)
                             .Authorization(TAG_APPLICATION_DATA, "data", 4)
                             .Authorization(TAG_APPLICATION_DATA, "foo", 3));

    set1.Union(set2);
    EXPECT_EQ(expected, set1);
}

TEST(Union, Overlap) {
    AuthorizationSet set1(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ACTIVE_DATETIME, 10)
                             .Authorization(TAG_APPLICATION_DATA, "data", 4));

    AuthorizationSet set2(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ACTIVE_DATETIME, 10)
                             .Authorization(TAG_APPLICATION_DATA, "data", 4));

    AuthorizationSet expected(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ACTIVE_DATETIME, 10)
                             .Authorization(TAG_APPLICATION_DATA, "data", 4));

    set1.Union(set2);
    EXPECT_EQ(expected, set1);
}

TEST(Union, Empty) {
    AuthorizationSet set1(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ACTIVE_DATETIME, 10)
                             .Authorization(TAG_APPLICATION_DATA, "data", 4));

    AuthorizationSet set2;

    AuthorizationSet expected(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ACTIVE_DATETIME, 10)
                             .Authorization(TAG_APPLICATION_DATA, "data", 4));

    set1.Union(set2);
    EXPECT_EQ(expected, set1);
}

TEST(Difference, Disjoint) {
    AuthorizationSet set1(AuthorizationSetBuilder()
                             .Authorization(TAG_APPLICATION_DATA, "data", 4)
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ACTIVE_DATETIME, 10));

    AuthorizationSet set2(AuthorizationSetBuilder()
                             .Authorization(TAG_USER_ID, 7)
                             .Authorization(TAG_APPLICATION_DATA, "foo", 3)
                             .Authorization(TAG_USER_AUTH_TYPE, HW_AUTH_PASSWORD));

    // Elements are the same as set1, but happen to be in a different order
    AuthorizationSet expected(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ACTIVE_DATETIME, 10)
                             .Authorization(TAG_APPLICATION_DATA, "data", 4));

    set1.Difference(set2);
    EXPECT_EQ(expected, set1);
}

TEST(Difference, Overlap) {
    AuthorizationSet set1(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ACTIVE_DATETIME, 10)
                             .Authorization(TAG_APPLICATION_DATA, "data", 4));

    AuthorizationSet set2(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ACTIVE_DATETIME, 10)
                             .Authorization(TAG_APPLICATION_DATA, "data", 4));

    AuthorizationSet empty;
    set1.Difference(set2);
    EXPECT_EQ(empty, set1);
    EXPECT_EQ(0U, set1.size());
}

TEST(Difference, NullSet) {
    AuthorizationSet set1(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ACTIVE_DATETIME, 10)
                             .Authorization(TAG_APPLICATION_DATA, "data", 4));

    AuthorizationSet set2;

    AuthorizationSet expected(AuthorizationSetBuilder()
                             .Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY)
                             .Authorization(TAG_ACTIVE_DATETIME, 10)
                             .Authorization(TAG_APPLICATION_DATA, "data", 4));

    set1.Difference(set2);
    EXPECT_EQ(expected, set1);
}

}  // namespace test
}  // namespace keymaster
