/*
 * Copyright 2014 The Android Open Source Project
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

#ifndef SYSTEM_KEYMASTER_KEYMASTER_TAGS_H_
#define SYSTEM_KEYMASTER_KEYMASTER_TAGS_H_

/**
 * This header contains various definitions that make working with keymaster tags safer and easier.
 * It makes use of a fair amount of template metaprogramming, which is genarally a bad idea for
 * maintainability, but in this case all of the metaprogramming serves the purpose of making it
 * impossible to make certain classes of mistakes when operating on keymaster authorizations.  For
 * example, it's an error to create a keymaster_param_t with tag == KM_TAG_PURPOSE and then to
 * assign KM_ALGORITHM_RSA to the enumerated element of its union, but because "enumerated" is a
 * uint32_t, there's no way for the compiler, ordinarily, to diagnose it.  Also, generic functions
 * to manipulate authorizations of multiple types can't be written, because they need to know which
 * union parameter to modify.
 *
 * The machinery in this header solves these problems.  The core elements are two templated classes,
 * TypedTag and TypedEnumTag.  These classes are templated on a tag type and a tag value, and in the
 * case of TypedEnumTag, an enumeration type as well.  Specializations are created for each
 * keymaster tag, associating the tag type with the tag, and an instance of each specialization is
 * created, and named the same as the keymaster tag, but with the KM_ prefix omitted.  Because the
 * classes include a conversion operator to keymaster_tag_t, they can be used anywhere a
 * keymaster_tag_t is expected.
 *
 * They also define a "value_type" typedef, which specifies the type of values associated with that
 * particular tag.  This enables template functions to be written that check that the correct
 * parameter type is used for a given tag, and that use the correct union entry for the tag type.  A
 * very useful example is the overloaded "Authorization" function defined below, which takes tag and
 * value arguments and correctly constructs a keyamster_param_t struct.
 *
 * Because the classes have no data members and all of their methods are inline, they have ZERO
 * run-time cost in and of themselves.  The one way in which they can create code bloat is when
 * template functions using them are expanded multiple times.  The standard method of creating
 * trivial, inlined template functions which call non-templated functions which are compact but not
 * type-safe, allows the program to have both the type-safety of the templates and the compactness
 * of the non-templated functions, at the same time.
 */

#include <hardware/hw_auth_token.h>
#include <hardware/keymaster_defs.h>

namespace keymaster {

// The following create the numeric values that KM_TAG_PADDING and KM_TAG_DIGEST used to have.  We
// need these old values to be able to support old keys that use them.
static const keymaster_tag_t KM_TAG_DIGEST_OLD = static_cast<keymaster_tag_t>(KM_ENUM | 5);
static const keymaster_tag_t KM_TAG_PADDING_OLD = static_cast<keymaster_tag_t>(KM_ENUM | 7);

// Until we have C++11, fake std::static_assert.
template <bool b> struct StaticAssert {};
template <> struct StaticAssert<true> {
    static void check() {}
};

// An unusable type that we can associate with tag types that don't have a simple value type.
// That will prevent the associated type from being used inadvertently.
class Void {
    Void();
    ~Void();
};

/**
 * A template that defines the association between non-enumerated tag types and their value
 * types.  For each tag type we define a specialized struct that contains a typedef "value_type".
 */
template <keymaster_tag_type_t tag_type> struct TagValueType {};
template <> struct TagValueType<KM_ULONG> { typedef uint64_t value_type; };
template <> struct TagValueType<KM_ULONG_REP> { typedef uint64_t value_type; };
template <> struct TagValueType<KM_DATE> { typedef uint64_t value_type; };
template <> struct TagValueType<KM_UINT> { typedef uint32_t value_type; };
template <> struct TagValueType<KM_UINT_REP> { typedef uint32_t value_type; };
template <> struct TagValueType<KM_INVALID> { typedef Void value_type; };
template <> struct TagValueType<KM_BOOL> { typedef bool value_type; };
template <> struct TagValueType<KM_BYTES> { typedef keymaster_blob_t value_type; };
template <> struct TagValueType<KM_BIGNUM> { typedef keymaster_blob_t value_type; };

/**
 * TypedTag is a templatized version of keymaster_tag_t, which provides compile-time checking of
 * keymaster tag types. Instances are convertible to keymaster_tag_t, so they can be used wherever
 * keymaster_tag_t is expected, and because they encode the tag type it's possible to create
 * function overloadings that only operate on tags with a particular type.
 */
template <keymaster_tag_type_t tag_type, keymaster_tag_t tag> class TypedTag {
  public:
    typedef typename TagValueType<tag_type>::value_type value_type;

    inline TypedTag() {
        // Ensure that it's impossible to create a TypedTag instance whose 'tag' doesn't have type
        // 'tag_type'.  Attempting to instantiate a tag with the wrong type will result in a compile
        // error (no match for template specialization StaticAssert<false>), with no run-time cost.
        StaticAssert<(tag & tag_type) == tag_type>::check();
        StaticAssert<(tag_type != KM_ENUM) && (tag_type != KM_ENUM_REP)>::check();
    }
    inline operator keymaster_tag_t() { return tag; }
    inline long masked_tag() { return static_cast<long>(keymaster_tag_mask_type(tag)); }
};

template <keymaster_tag_type_t tag_type, keymaster_tag_t tag, typename KeymasterEnum>
class TypedEnumTag {
  public:
    typedef KeymasterEnum value_type;

    inline TypedEnumTag() {
        // Ensure that it's impossible to create a TypedTag instance whose 'tag' doesn't have type
        // 'tag_type'.  Attempting to instantiate a tag with the wrong type will result in a compile
        // error (no match for template specialization StaticAssert<false>), with no run-time cost.
        StaticAssert<(tag & tag_type) == tag_type>::check();
        StaticAssert<(tag_type == KM_ENUM) || (tag_type == KM_ENUM_REP)>::check();
    }
    inline operator keymaster_tag_t() { return tag; }
    inline long masked_tag() { return static_cast<long>(keymaster_tag_mask_type(tag)); }
};

#ifdef KEYMASTER_NAME_TAGS
const char* StringifyTag(keymaster_tag_t tag);
#endif

// DECLARE_KEYMASTER_TAG is used to declare TypedTag instances for each non-enum keymaster tag.
#define DECLARE_KEYMASTER_TAG(type, name) extern TypedTag<type, KM_##name> name

DECLARE_KEYMASTER_TAG(KM_INVALID, TAG_INVALID);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_KEY_SIZE);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_MAC_LENGTH);
DECLARE_KEYMASTER_TAG(KM_BOOL, TAG_CALLER_NONCE);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_MIN_MAC_LENGTH);
DECLARE_KEYMASTER_TAG(KM_ULONG, TAG_RSA_PUBLIC_EXPONENT);
DECLARE_KEYMASTER_TAG(KM_BOOL, TAG_ECIES_SINGLE_HASH_MODE);
DECLARE_KEYMASTER_TAG(KM_BOOL, TAG_INCLUDE_UNIQUE_ID);
DECLARE_KEYMASTER_TAG(KM_DATE, TAG_ACTIVE_DATETIME);
DECLARE_KEYMASTER_TAG(KM_DATE, TAG_ORIGINATION_EXPIRE_DATETIME);
DECLARE_KEYMASTER_TAG(KM_DATE, TAG_USAGE_EXPIRE_DATETIME);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_MIN_SECONDS_BETWEEN_OPS);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_MAX_USES_PER_BOOT);
DECLARE_KEYMASTER_TAG(KM_BOOL, TAG_ALL_USERS);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_USER_ID);
DECLARE_KEYMASTER_TAG(KM_ULONG_REP, TAG_USER_SECURE_ID);
DECLARE_KEYMASTER_TAG(KM_BOOL, TAG_NO_AUTH_REQUIRED);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_AUTH_TIMEOUT);
DECLARE_KEYMASTER_TAG(KM_BOOL, TAG_ALLOW_WHILE_ON_BODY);
DECLARE_KEYMASTER_TAG(KM_BOOL, TAG_ALL_APPLICATIONS);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_APPLICATION_ID);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_APPLICATION_DATA);
DECLARE_KEYMASTER_TAG(KM_DATE, TAG_CREATION_DATETIME);
DECLARE_KEYMASTER_TAG(KM_BOOL, TAG_ROLLBACK_RESISTANT);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ROOT_OF_TRUST);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ASSOCIATED_DATA);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_NONCE);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_AUTH_TOKEN);
DECLARE_KEYMASTER_TAG(KM_BOOL, TAG_BOOTLOADER_ONLY);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_OS_VERSION);
DECLARE_KEYMASTER_TAG(KM_UINT, TAG_OS_PATCHLEVEL);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_UNIQUE_ID);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_CHALLENGE);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_APPLICATION_ID);
DECLARE_KEYMASTER_TAG(KM_BOOL, TAG_RESET_SINCE_ID_ROTATION);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_ID_BRAND);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_ID_DEVICE);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_ID_PRODUCT);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_ID_SERIAL);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_ID_IMEI);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_ID_MEID);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_ID_MANUFACTURER);
DECLARE_KEYMASTER_TAG(KM_BYTES, TAG_ATTESTATION_ID_MODEL);

// DECLARE_KEYMASTER_ENUM_TAG is used to declare TypedEnumTag instances for each enum keymaster tag.
#define DECLARE_KEYMASTER_ENUM_TAG(type, name, enumtype)                                           \
    extern TypedEnumTag<type, KM_##name, enumtype> name

DECLARE_KEYMASTER_ENUM_TAG(KM_ENUM_REP, TAG_PURPOSE, keymaster_purpose_t);
DECLARE_KEYMASTER_ENUM_TAG(KM_ENUM, TAG_ALGORITHM, keymaster_algorithm_t);
DECLARE_KEYMASTER_ENUM_TAG(KM_ENUM_REP, TAG_BLOCK_MODE, keymaster_block_mode_t);
DECLARE_KEYMASTER_ENUM_TAG(KM_ENUM_REP, TAG_DIGEST, keymaster_digest_t);
DECLARE_KEYMASTER_ENUM_TAG(KM_ENUM, TAG_DIGEST_OLD, keymaster_digest_t);
DECLARE_KEYMASTER_ENUM_TAG(KM_ENUM_REP, TAG_PADDING, keymaster_padding_t);
DECLARE_KEYMASTER_ENUM_TAG(KM_ENUM, TAG_PADDING_OLD, keymaster_padding_t);
DECLARE_KEYMASTER_ENUM_TAG(KM_ENUM, TAG_BLOB_USAGE_REQUIREMENTS,
                           keymaster_key_blob_usage_requirements_t);
DECLARE_KEYMASTER_ENUM_TAG(KM_ENUM, TAG_ORIGIN, keymaster_key_origin_t);
DECLARE_KEYMASTER_ENUM_TAG(KM_ENUM, TAG_USER_AUTH_TYPE, hw_authenticator_type_t);
DECLARE_KEYMASTER_ENUM_TAG(KM_ENUM_REP, TAG_KDF, keymaster_kdf_t);
DECLARE_KEYMASTER_ENUM_TAG(KM_ENUM, TAG_EC_CURVE, keymaster_ec_curve_t);

//
// Overloaded function "Authorization" to create keymaster_key_param_t objects for all of tags.
//

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_BOOL, Tag> tag) {
    return keymaster_param_bool(tag);
}

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_UINT, Tag> tag, uint32_t value) {
    return keymaster_param_int(tag, value);
}

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_UINT_REP, Tag> tag, uint32_t value) {
    return keymaster_param_int(tag, value);
}

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_ULONG, Tag> tag, uint64_t value) {
    return keymaster_param_long(tag, value);
}

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_ULONG_REP, Tag> tag, uint64_t value) {
    return keymaster_param_long(tag, value);
}

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_DATE, Tag> tag, uint64_t value) {
    return keymaster_param_date(tag, value);
}

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_BYTES, Tag> tag, const void* bytes,
                                           size_t bytes_len) {
    return keymaster_param_blob(tag, reinterpret_cast<const uint8_t*>(bytes), bytes_len);
}

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_BYTES, Tag> tag,
                                           const keymaster_blob_t& blob) {
    return keymaster_param_blob(tag, blob.data, blob.data_length);
}

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_BIGNUM, Tag> tag, const void* bytes,
                                           size_t bytes_len) {
    return keymaster_param_blob(tag, reinterpret_cast<const uint8_t*>(bytes), bytes_len);
}

template <keymaster_tag_t Tag>
inline keymaster_key_param_t Authorization(TypedTag<KM_BIGNUM, Tag> tag,
                                           const keymaster_blob_t& blob) {
    return keymaster_param_blob(tag, blob.data, blob.data_length);
}

template <keymaster_tag_t Tag, typename KeymasterEnum>
inline keymaster_key_param_t Authorization(TypedEnumTag<KM_ENUM, Tag, KeymasterEnum> tag,
                                           KeymasterEnum value) {
    return keymaster_param_enum(tag, value);
}

template <keymaster_tag_t Tag, typename KeymasterEnum>
inline keymaster_key_param_t Authorization(TypedEnumTag<KM_ENUM_REP, Tag, KeymasterEnum> tag,
                                           KeymasterEnum value) {
    return keymaster_param_enum(tag, value);
}

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_KEYMASTER_TAGS_H_
