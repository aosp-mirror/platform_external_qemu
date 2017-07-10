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

#ifndef SYSTEM_KEYMASTER_AUTHORIZATION_SET_H_
#define SYSTEM_KEYMASTER_AUTHORIZATION_SET_H_

#include <UniquePtr.h>

#include <hardware/keymaster_defs.h>
#include <keymaster/keymaster_tags.h>
#include <keymaster/serializable.h>

namespace keymaster {

class AuthorizationSetBuilder;

/**
 * An extension of the keymaster_key_param_set_t struct, which provides serialization memory
 * management and methods for easy manipulation and construction.
 */
class AuthorizationSet : public Serializable, public keymaster_key_param_set_t {
  public:
    /**
     * Construct an empty, dynamically-allocated, growable AuthorizationSet.  Does not actually
     * allocate any storage until elements are added, so there is no cost to creating an
     * AuthorizationSet with this constructor and then reinitializing it to point at pre-allocated
     * buffers, with \p Reinitialize.
     */
    AuthorizationSet()
        : elems_capacity_(0), indirect_data_(NULL), indirect_data_size_(0),
          indirect_data_capacity_(0), error_(OK) {
        elems_ = nullptr;
        elems_size_ = 0;
    }

    /**
     * Construct an AuthorizationSet from the provided array.  The AuthorizationSet copies the data
     * from the provided array (and the data referenced by its embedded pointers, if any) into
     * dynamically-allocated storage.  If allocation of the needed storage fails, \p is_valid() will
     * return ALLOCATION_FAILURE. It is the responsibility of the caller to check before using the
     * set, if allocations might fail.
     */
    AuthorizationSet(const keymaster_key_param_t* elems, size_t count) : indirect_data_(nullptr) {
        elems_ = nullptr;
        Reinitialize(elems, count);
    }

    explicit AuthorizationSet(const keymaster_key_param_set_t& set) : indirect_data_(nullptr) {
        elems_ = nullptr;
        Reinitialize(set.params, set.length);
    }

    explicit AuthorizationSet(const uint8_t* serialized_set, size_t serialized_size)
        : indirect_data_(nullptr) {
        elems_ = nullptr;
        Deserialize(&serialized_set, serialized_set + serialized_size);
    }

    /**
     * Construct an AuthorizationSet from the provided builder.  This extracts the data from the
     * builder, rather than copying it, so after this call the builder is empty.
     */
    explicit AuthorizationSet(/* NOT const */ AuthorizationSetBuilder& builder);

    // Copy constructor.
    AuthorizationSet(const AuthorizationSet& set) : Serializable(), indirect_data_(nullptr) {
        elems_ = nullptr;
        error_ = set.error_;
        if (error_ != OK) return;
        Reinitialize(set.elems_, set.elems_size_);
    }

    // Move constructor.
    AuthorizationSet(AuthorizationSet&& set) : Serializable() {
        MoveFrom(set);
    }

    // Copy assignment.
    AuthorizationSet& operator=(const AuthorizationSet& set) {
        Reinitialize(set.elems_, set.elems_size_);
        error_ = set.error_;
        return *this;
    }

    // Move assignment.
    AuthorizationSet& operator=(AuthorizationSet&& set) {
        FreeData();
        MoveFrom(set);
        return *this;
    }

    /**
     * Clear existing authorization set data
     */
    void Clear();

    /**
     * Reinitialize an AuthorizationSet as a dynamically-allocated, growable copy of the data in the
     * provided array (and the data referenced by its embedded pointers, if any).  If the allocation
     * of the needed storage fails this method will return false and \p is_valid() will return
     * ALLOCATION_FAILURE.
     */
    bool Reinitialize(const keymaster_key_param_t* elems, size_t count);

    bool Reinitialize(const AuthorizationSet& set) {
        return Reinitialize(set.elems_, set.elems_size_);
    }

    bool Reinitialize(const keymaster_key_param_set_t& set) {
        return Reinitialize(set.params, set.length);
    }

    ~AuthorizationSet();

    enum Error {
        OK,
        ALLOCATION_FAILURE,
        MALFORMED_DATA,
    };

    Error is_valid() const { return error_; }

    /**
     * Returns the size of the set.
     */
    size_t size() const { return elems_size_; }

    /**
     * Returns true if the set is empty.
     */
    bool empty() const { return size() == 0; }

    /**
     * Returns the total size of all indirect data referenced by set elements.
     */
    size_t indirect_size() const { return indirect_data_size_; }

    /**
     * Returns the data in the set, directly. Be careful with this.
     */
    const keymaster_key_param_t* data() const { return elems_; }

    /**
     * Sorts the set
     */
    void Sort();

    /**
     * Sorts the set and removes duplicates (inadvertently duplicating tags is easy to do with the
     * AuthorizationSetBuilder).
     */
    void Deduplicate();

    /**
     * Adds all elements from \p set that are not already present in this AuthorizationSet.  As a
     * side-effect, if \p set is not null this AuthorizationSet will end up sorted.
     */
    void Union(const keymaster_key_param_set_t& set);

    /**
     * Removes all elements in \p set from this AuthorizationSet.
     */
    void Difference(const keymaster_key_param_set_t& set);

    /**
     * Returns the data in a keymaster_key_param_set_t, suitable for returning to C code.  For C
     * compatibility, the contents are malloced, not new'ed, and so must be freed with free(), or
     * better yet with keymaster_free_param_set, not delete.  The caller takes ownership.
     */
    void CopyToParamSet(keymaster_key_param_set_t* set) const;

    /**
     * Returns the offset of the next entry that matches \p tag, starting from the element after \p
     * begin.  If not found, returns -1.
     */
    int find(keymaster_tag_t tag, int begin = -1) const;

    /**
     * Removes the entry at the specified index. Returns true if successful, false if the index was
     * out of bounds.
     */
    bool erase(int index);

    /**
     * Returns iterator (pointer) to beginning of elems array, to enable STL-style iteration
     */
    const keymaster_key_param_t* begin() const { return elems_; }

    /**
     * Returns iterator (pointer) one past end of elems array, to enable STL-style iteration
     */
    const keymaster_key_param_t* end() const { return elems_ + elems_size_; }

    /**
     * Returns the nth element of the set.
     */
    keymaster_key_param_t& operator[](int n);

    /**
     * Returns the nth element of the set.
     */
    keymaster_key_param_t operator[](int n) const;

    /**
     * Returns true if the set contains at least one instance of \p tag
     */
    bool Contains(keymaster_tag_t tag) const {
        return find(tag) != -1;
    }

    /**
     * Returns the number of \p tag entries.
     */
    size_t GetTagCount(keymaster_tag_t tag) const;

    /**
     * Returns true if the set contains the specified tag and value.
     */
    template <keymaster_tag_t Tag, typename T>
    bool Contains(TypedEnumTag<KM_ENUM_REP, Tag, T> tag, T val) const {
        return ContainsEnumValue(tag, val);
    }

    /**
     * Returns true if the set contains the specified tag and value.
     */
    template <keymaster_tag_t Tag, typename T>
    bool Contains(TypedEnumTag<KM_ENUM, Tag, T> tag, T val) const {
        return ContainsEnumValue(tag, val);
    }

    /**
     * Returns true if the set contains the specified tag and value.
     */
    template <keymaster_tag_t Tag>
    bool Contains(TypedTag<KM_UINT, Tag> tag, uint32_t val) const {
        return ContainsIntValue(tag, val);
    }

    /**
     * If the specified integer-typed \p tag exists, places its value in \p val and returns true.
     * If \p tag is not present, leaves \p val unmodified and returns false.
     */
    template <keymaster_tag_t T>
    inline bool GetTagValue(TypedTag<KM_UINT, T> tag, uint32_t* val) const {
        return GetTagValueInt(tag, val);
    }

    /**
     * If the specified instance of the specified integer-typed \p tag exists, places its value
     * in \p val and returns true.  If \p tag is not present, leaves \p val unmodified and returns
     * false.
     */
    template <keymaster_tag_t Tag>
    bool GetTagValue(TypedTag<KM_UINT_REP, Tag> tag, size_t instance, uint32_t* val) const {
        return GetTagValueIntRep(tag, instance, val);
    }

    /**
     * If the specified long-typed \p tag exists, places its value in \p val and returns true.
     * If \p tag is not present, leaves \p val unmodified and returns false.
     */
    template <keymaster_tag_t T>
    inline bool GetTagValue(TypedTag<KM_ULONG, T> tag, uint64_t* val) const {
        return GetTagValueLong(tag, val);
    }

    /**
     * If the specified instance of the specified integer-typed \p tag exists, places its value
     * in \p val and returns true.  If \p tag is not present, leaves \p val unmodified and returns
     * false.
     */
    template <keymaster_tag_t Tag>
    bool GetTagValue(TypedTag<KM_ULONG_REP, Tag> tag, size_t instance, uint64_t* val) const {
        return GetTagValueLongRep(tag, instance, val);
    }

    /**
     * If the specified enumeration-typed \p tag exists, places its value in \p val and returns
     * true.  If \p tag is not present, leaves \p val unmodified and returns false.
     */
    template <keymaster_tag_t Tag, typename T>
    bool GetTagValue(TypedEnumTag<KM_ENUM, Tag, T> tag, T* val) const {
        return GetTagValueEnum(tag, reinterpret_cast<uint32_t*>(val));
    }

    /**
     * If the specified instance of the specified enumeration-typed \p tag exists, places its value
     * in \p val and returns true.  If \p tag is not present, leaves \p val unmodified and returns
     * false.
     */
    template <keymaster_tag_t Tag, typename T>
    bool GetTagValue(TypedEnumTag<KM_ENUM_REP, Tag, T> tag, size_t instance, T* val) const {
        return GetTagValueEnumRep(tag, instance, reinterpret_cast<uint32_t*>(val));
    }

    /**
     * If exactly one instance of the specified enumeration-typed \p tag exists, places its value in
     * \p val and returns true.  If \p tag is not present or if multiple copies are present, leaves
     * \p val unmodified and returns false.
     */
    template <keymaster_tag_t Tag, typename T>
    bool GetTagValue(TypedEnumTag<KM_ENUM_REP, Tag, T> tag, T* val) const {
        if (GetTagCount(tag) != 1)
            return false;
        return GetTagValueEnumRep(tag, 0, reinterpret_cast<uint32_t*>(val));
    }

    /**
     * If the specified date-typed \p tag exists, places its value in \p val and returns
     * true.  If \p tag is not present, leaves \p val unmodified and returns false.
     */
    template <keymaster_tag_t Tag>
    bool GetTagValue(TypedTag<KM_UINT_REP, Tag> tag, size_t instance,
                     typename TypedTag<KM_UINT_REP, Tag>::value_type* val) const {
        return GetTagValueIntRep(tag, instance, val);
    }

    /**
     * If the specified bytes-typed \p tag exists, places its value in \p val and returns
     * true.  If \p tag is not present, leaves \p val unmodified and returns false.
     */
    template <keymaster_tag_t Tag>
    bool GetTagValue(TypedTag<KM_BYTES, Tag> tag, keymaster_blob_t* val) const {
        return GetTagValueBlob(tag, val);
    }

    /**
     * If the specified bignum-typed \p tag exists, places its value in \p val and returns
     * true.  If \p tag is not present, leaves \p val unmodified and returns false.
     */
    template <keymaster_tag_t Tag>
    bool GetTagValue(TypedTag<KM_BIGNUM, Tag> tag, keymaster_blob_t* val) const {
        return GetTagValueBlob(tag, val);
    }

    /**
     * Returns true if the specified tag is present, and therefore has the value 'true'.
     */
    template <keymaster_tag_t Tag> bool GetTagValue(TypedTag<KM_BOOL, Tag> tag) const {
        return GetTagValueBool(tag);
    }

    /**
     * If the specified \p tag exists, places its value in \p val and returns true.  If \p tag is
     * not present, leaves \p val unmodified and returns false.
     */
    template <keymaster_tag_t Tag, keymaster_tag_type_t Type>
    bool GetTagValue(TypedTag<Type, Tag> tag, typename TagValueType<Type>::value_type* val) const {
        return GetTagValueLong(tag, val);
    }

    bool push_back(keymaster_key_param_t elem);

    /**
     * Grow the elements array to ensure it can contain \p count entries.  Preserves any existing
     * entries.
     */
    bool reserve_elems(size_t count);

    /**
     * Grow the indirect data array to ensure it can contain \p length bytes.  Preserves any
     * existing indirect data.
     */
    bool reserve_indirect(size_t length);

    bool push_back(const keymaster_key_param_set_t& set);

    /**
     * Append the tag and enumerated value to the set.
     */
    template <keymaster_tag_t Tag, keymaster_tag_type_t Type, typename KeymasterEnum>
    bool push_back(TypedEnumTag<Type, Tag, KeymasterEnum> tag, KeymasterEnum val) {
        return push_back(Authorization(tag, val));
    }

    /**
     * Append the boolean tag (value "true") to the set.
     */
    template <keymaster_tag_t Tag> bool push_back(TypedTag<KM_BOOL, Tag> tag) {
        return push_back(Authorization(tag));
    }

    /**
     * Append the tag and byte array to the set.  Copies the array into internal storage; does not
     * take ownership of the passed-in array.
     */
    template <keymaster_tag_t Tag>
    bool push_back(TypedTag<KM_BYTES, Tag> tag, const void* bytes, size_t bytes_len) {
        return push_back(keymaster_param_blob(tag, static_cast<const uint8_t*>(bytes), bytes_len));
    }

    /**
     * Append the tag and blob to the set.  Copies the blob contents into internal storage; does not
     * take ownership of the blob's data.
     */
    template <keymaster_tag_t Tag>
    bool push_back(TypedTag<KM_BYTES, Tag> tag, const keymaster_blob_t& blob) {
        return push_back(tag, blob.data, blob.data_length);
    }

    /**
     * Append the tag and bignum array to the set.  Copies the array into internal storage; does not
     * take ownership of the passed-in array.
     */
    template <keymaster_tag_t Tag>
    bool push_back(TypedTag<KM_BIGNUM, Tag> tag, const void* bytes, size_t bytes_len) {
        return push_back(keymaster_param_blob(tag, static_cast<const uint8_t*>(bytes), bytes_len));
    }

    template <keymaster_tag_t Tag, keymaster_tag_type_t Type>
    bool push_back(TypedTag<Type, Tag> tag, typename TypedTag<Type, Tag>::value_type val) {
        return push_back(Authorization(tag, val));
    }

    template <keymaster_tag_t Tag, keymaster_tag_type_t Type>
    bool push_back(TypedTag<Type, Tag> tag, const void* bytes, size_t bytes_len) {
        return push_back(Authorization(tag, bytes, bytes_len));
    }

    /* Virtual methods from Serializable */
    size_t SerializedSize() const;
    uint8_t* Serialize(uint8_t* serialized_set, const uint8_t* end) const;
    bool Deserialize(const uint8_t** buf_ptr, const uint8_t* end);

    size_t SerializedSizeOfElements() const;

  private:
    void FreeData();
    void MoveFrom(AuthorizationSet& set);

    void set_invalid(Error err);

    static size_t ComputeIndirectDataSize(const keymaster_key_param_t* elems, size_t count);
    void CopyIndirectData();
    bool CheckIndirectData();

    bool DeserializeIndirectData(const uint8_t** buf_ptr, const uint8_t* end);
    bool DeserializeElementsData(const uint8_t** buf_ptr, const uint8_t* end);

    bool GetTagValueEnum(keymaster_tag_t tag, uint32_t* val) const;
    bool GetTagValueEnumRep(keymaster_tag_t tag, size_t instance, uint32_t* val) const;
    bool GetTagValueInt(keymaster_tag_t tag, uint32_t* val) const;
    bool GetTagValueIntRep(keymaster_tag_t tag, size_t instance, uint32_t* val) const;
    bool GetTagValueLong(keymaster_tag_t tag, uint64_t* val) const;
    bool GetTagValueLongRep(keymaster_tag_t tag, size_t instance, uint64_t* val) const;
    bool GetTagValueDate(keymaster_tag_t tag, uint64_t* val) const;
    bool GetTagValueBlob(keymaster_tag_t tag, keymaster_blob_t* val) const;
    bool GetTagValueBool(keymaster_tag_t tag) const;

    bool ContainsEnumValue(keymaster_tag_t tag, uint32_t val) const;
    bool ContainsIntValue(keymaster_tag_t tag, uint32_t val) const;

    // Define elems_ and elems_size_ as aliases to params and length, respectively.  This is to
    // avoid using the variables without the trailing underscore in the implementation.
    keymaster_key_param_t*& elems_ = keymaster_key_param_set_t::params;
    size_t& elems_size_ = keymaster_key_param_set_t::length;

    size_t elems_capacity_;
    uint8_t* indirect_data_;
    size_t indirect_data_size_;
    size_t indirect_data_capacity_;
    Error error_;
};

class AuthorizationSetBuilder {
  public:
    template <typename TagType, typename ValueType>
    AuthorizationSetBuilder& Authorization(TagType tag, ValueType value) {
        set.push_back(tag, value);
        return *this;
    }

    template <keymaster_tag_t Tag>
    AuthorizationSetBuilder& Authorization(TypedTag<KM_BOOL, Tag> tag) {
        set.push_back(tag);
        return *this;
    }

    template <keymaster_tag_t Tag>
    AuthorizationSetBuilder& Authorization(TypedTag<KM_INVALID, Tag> tag) {
        keymaster_key_param_t param;
        param.tag = tag;
        set.push_back(param);
        return *this;
    }

    template <keymaster_tag_t Tag>
    AuthorizationSetBuilder& Authorization(TypedTag<KM_BYTES, Tag> tag, const uint8_t* data,
                                           size_t data_length) {
        set.push_back(tag, data, data_length);
        return *this;
    }

    template <keymaster_tag_t Tag>
    AuthorizationSetBuilder& Authorization(TypedTag<KM_BYTES, Tag> tag, const char* data,
                                           size_t data_length) {
        return Authorization(tag, reinterpret_cast<const uint8_t*>(data), data_length);
    }

    AuthorizationSetBuilder& RsaKey(uint32_t key_size, uint64_t public_exponent);
    AuthorizationSetBuilder& EcdsaKey(uint32_t key_size);
    AuthorizationSetBuilder& AesKey(uint32_t key_size);
    AuthorizationSetBuilder& HmacKey(uint32_t key_size);

    AuthorizationSetBuilder& RsaSigningKey(uint32_t key_size, uint64_t public_exponent);
    AuthorizationSetBuilder& RsaEncryptionKey(uint32_t key_size, uint64_t public_exponent);
    AuthorizationSetBuilder& EcdsaSigningKey(uint32_t key_size);
    AuthorizationSetBuilder& AesEncryptionKey(uint32_t key_size);

    AuthorizationSetBuilder& SigningKey();
    AuthorizationSetBuilder& EncryptionKey();
    AuthorizationSetBuilder& NoDigestOrPadding();
    AuthorizationSetBuilder& EcbMode();

    AuthorizationSetBuilder& Digest(keymaster_digest_t digest) {
        return Authorization(TAG_DIGEST, digest);
    }

    AuthorizationSetBuilder& Padding(keymaster_padding_t padding) {
        return Authorization(TAG_PADDING, padding);
    }

    AuthorizationSetBuilder& Deduplicate() {
        set.Deduplicate();
        return *this;
    }

    AuthorizationSet build() const { return set; }

  private:
    friend AuthorizationSet;
    AuthorizationSet set;
};

inline AuthorizationSetBuilder& AuthorizationSetBuilder::RsaKey(uint32_t key_size,
                                                                uint64_t public_exponent) {
    Authorization(TAG_ALGORITHM, KM_ALGORITHM_RSA);
    Authorization(TAG_KEY_SIZE, key_size);
    Authorization(TAG_RSA_PUBLIC_EXPONENT, public_exponent);
    return *this;
}

inline AuthorizationSetBuilder& AuthorizationSetBuilder::EcdsaKey(uint32_t key_size) {
    Authorization(TAG_ALGORITHM, KM_ALGORITHM_EC);
    Authorization(TAG_KEY_SIZE, key_size);
    return *this;
}

inline AuthorizationSetBuilder& AuthorizationSetBuilder::AesKey(uint32_t key_size) {
    Authorization(TAG_ALGORITHM, KM_ALGORITHM_AES);
    return Authorization(TAG_KEY_SIZE, key_size);
}

inline AuthorizationSetBuilder& AuthorizationSetBuilder::HmacKey(uint32_t key_size) {
    Authorization(TAG_ALGORITHM, KM_ALGORITHM_HMAC);
    Authorization(TAG_KEY_SIZE, key_size);
    return SigningKey();
}

inline AuthorizationSetBuilder& AuthorizationSetBuilder::RsaSigningKey(uint32_t key_size,
                                                                       uint64_t public_exponent) {
    RsaKey(key_size, public_exponent);
    return SigningKey();
}

inline AuthorizationSetBuilder&
AuthorizationSetBuilder::RsaEncryptionKey(uint32_t key_size, uint64_t public_exponent) {
    RsaKey(key_size, public_exponent);
    return EncryptionKey();
}

inline AuthorizationSetBuilder& AuthorizationSetBuilder::EcdsaSigningKey(uint32_t key_size) {
    EcdsaKey(key_size);
    return SigningKey();
}

inline AuthorizationSetBuilder& AuthorizationSetBuilder::AesEncryptionKey(uint32_t key_size) {
    AesKey(key_size);
    return EncryptionKey();
}

inline AuthorizationSetBuilder& AuthorizationSetBuilder::SigningKey() {
    Authorization(TAG_PURPOSE, KM_PURPOSE_SIGN);
    return Authorization(TAG_PURPOSE, KM_PURPOSE_VERIFY);
}

inline AuthorizationSetBuilder& AuthorizationSetBuilder::EncryptionKey() {
    Authorization(TAG_PURPOSE, KM_PURPOSE_ENCRYPT);
    return Authorization(TAG_PURPOSE, KM_PURPOSE_DECRYPT);
}

inline AuthorizationSetBuilder& AuthorizationSetBuilder::NoDigestOrPadding() {
    Authorization(TAG_DIGEST, KM_DIGEST_NONE);
    return Authorization(TAG_PADDING, KM_PAD_NONE);
}

inline AuthorizationSetBuilder& AuthorizationSetBuilder::EcbMode() {
    return Authorization(TAG_BLOCK_MODE, KM_MODE_ECB);
}

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_KEY_AUTHORIZATION_SET_H_
