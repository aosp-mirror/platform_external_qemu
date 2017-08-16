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

#include <keymaster/authorization_set.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#include <keymaster/android_keymaster_utils.h>
#include <keymaster/logger.h>

namespace keymaster {

static inline bool is_blob_tag(keymaster_tag_t tag) {
    return (keymaster_tag_get_type(tag) == KM_BYTES || keymaster_tag_get_type(tag) == KM_BIGNUM);
}

const size_t STARTING_ELEMS_CAPACITY = 8;

AuthorizationSet::AuthorizationSet(AuthorizationSetBuilder& builder) {
    elems_ = builder.set.elems_;
    builder.set.elems_ = NULL;

    elems_size_ = builder.set.elems_size_;
    builder.set.elems_size_ = 0;

    elems_capacity_ = builder.set.elems_capacity_;
    builder.set.elems_capacity_ = 0;

    indirect_data_ = builder.set.indirect_data_;
    builder.set.indirect_data_ = NULL;

    indirect_data_capacity_ = builder.set.indirect_data_capacity_;
    builder.set.indirect_data_capacity_ = 0;

    indirect_data_size_ = builder.set.indirect_data_size_;
    builder.set.indirect_data_size_ = 0;

    error_ = builder.set.error_;
    builder.set.error_ = OK;
}

AuthorizationSet::~AuthorizationSet() {
    FreeData();
}

bool AuthorizationSet::reserve_elems(size_t count) {
    if (is_valid() != OK)
        return false;

    if (count > elems_capacity_) {
        keymaster_key_param_t* new_elems = new (std::nothrow) keymaster_key_param_t[count];
        if (new_elems == NULL) {
            set_invalid(ALLOCATION_FAILURE);
            return false;
        }
        memcpy(new_elems, elems_, sizeof(*elems_) * elems_size_);
        delete[] elems_;
        elems_ = new_elems;
        elems_capacity_ = count;
    }
    return true;
}

bool AuthorizationSet::reserve_indirect(size_t length) {
    if (is_valid() != OK)
        return false;

    if (length > indirect_data_capacity_) {
        uint8_t* new_data = new (std::nothrow) uint8_t[length];
        if (new_data == NULL) {
            set_invalid(ALLOCATION_FAILURE);
            return false;
        }
        memcpy(new_data, indirect_data_, indirect_data_size_);

        // Fix up the data pointers to point into the new region.
        for (size_t i = 0; i < elems_size_; ++i) {
            if (is_blob_tag(elems_[i].tag))
                elems_[i].blob.data = new_data + (elems_[i].blob.data - indirect_data_);
        }
        delete[] indirect_data_;
        indirect_data_ = new_data;
        indirect_data_capacity_ = length;
    }
    return true;
}

void AuthorizationSet::MoveFrom(AuthorizationSet& set) {
    elems_ = set.elems_;
    elems_size_ = set.elems_size_;
    elems_capacity_ = set.elems_capacity_;
    indirect_data_ = set.indirect_data_;
    indirect_data_size_ = set.indirect_data_size_;
    indirect_data_capacity_ = set.indirect_data_capacity_;
    error_ = set.error_;
    set.elems_ = nullptr;
    set.elems_size_ = 0;
    set.elems_capacity_ = 0;
    set.indirect_data_ = nullptr;
    set.indirect_data_size_ = 0;
    set.indirect_data_capacity_ = 0;
    set.error_ = OK;
}

bool AuthorizationSet::Reinitialize(const keymaster_key_param_t* elems, const size_t count) {
    FreeData();

    if (elems == NULL || count == 0) {
        error_ = OK;
        return true;
    }

    if (!reserve_elems(count))
        return false;

    if (!reserve_indirect(ComputeIndirectDataSize(elems, count)))
        return false;

    memcpy(elems_, elems, sizeof(keymaster_key_param_t) * count);
    elems_size_ = count;
    CopyIndirectData();
    error_ = OK;
    return true;
}

void AuthorizationSet::set_invalid(Error error) {
    FreeData();
    error_ = error;
}

void AuthorizationSet::Sort() {
    qsort(elems_, elems_size_, sizeof(*elems_),
          reinterpret_cast<int (*)(const void*, const void*)>(keymaster_param_compare));
}

void AuthorizationSet::Deduplicate() {
    Sort();

    size_t invalid_count = 0;
    for (size_t i = 1; i < size(); ++i) {
        if (elems_[i - 1].tag == KM_TAG_INVALID)
            ++invalid_count;
        else if (keymaster_param_compare(elems_ + i - 1, elems_ + i) == 0) {
            // Mark dups as invalid.  Note that this "leaks" the data referenced by KM_BYTES and
            // KM_BIGNUM entries, but those are just pointers into indirect_data_, so it will all
            // get cleaned up.
            elems_[i - 1].tag = KM_TAG_INVALID;
            ++invalid_count;
        }
    }
    if (size() > 0 && elems_[size() - 1].tag == KM_TAG_INVALID)
        ++invalid_count;

    if (invalid_count == 0)
        return;

    Sort();

    // Since KM_TAG_INVALID == 0, all of the invalid entries are first.
    elems_size_ -= invalid_count;
    memmove(elems_, elems_ + invalid_count, size() * sizeof(*elems_));
}

void AuthorizationSet::Union(const keymaster_key_param_set_t& set) {
    if (set.length == 0)
        return;

    push_back(set);
    Deduplicate();
}

void AuthorizationSet::Difference(const keymaster_key_param_set_t& set) {
    if (set.length == 0)
        return;

    Deduplicate();

    for (size_t i = 0; i < set.length; i++) {
        int index = -1;
        do {
            index = find(set.params[i].tag, index);
            if (index != -1 && keymaster_param_compare(&elems_[index], &set.params[i]) == 0) {
                erase(index);
                break;
            }
        } while (index != -1);
    }
}

void AuthorizationSet::CopyToParamSet(keymaster_key_param_set_t* set) const {
    assert(set);

    set->length = size();
    set->params =
        reinterpret_cast<keymaster_key_param_t*>(malloc(sizeof(keymaster_key_param_t) * size()));

    for (size_t i = 0; i < size(); ++i) {
        const keymaster_key_param_t src = (*this)[i];
        keymaster_key_param_t& dst(set->params[i]);

        dst = src;
        keymaster_tag_type_t type = keymaster_tag_get_type(src.tag);
        if (type == KM_BIGNUM || type == KM_BYTES) {
            void* tmp = malloc(src.blob.data_length);
            memcpy(tmp, src.blob.data, src.blob.data_length);
            dst.blob.data = reinterpret_cast<uint8_t*>(tmp);
        }
    }
}

int AuthorizationSet::find(keymaster_tag_t tag, int begin) const {
    if (is_valid() != OK)
        return -1;

    int i = ++begin;
    while (i < (int)elems_size_ && elems_[i].tag != tag)
        ++i;
    if (i == (int)elems_size_)
        return -1;
    else
        return i;
}

bool AuthorizationSet::erase(int index) {
    if (index < 0 || index >= static_cast<int>(size()))
        return false;

    --elems_size_;
    for (size_t i = index; i < elems_size_; ++i)
        elems_[i] = elems_[i + 1];
    return true;
}

keymaster_key_param_t empty_param = {KM_TAG_INVALID, {}};
keymaster_key_param_t& AuthorizationSet::operator[](int at) {
    if (is_valid() == OK && at < (int)elems_size_) {
        return elems_[at];
    }
    empty_param = {KM_TAG_INVALID, {}};
    return empty_param;
}

keymaster_key_param_t AuthorizationSet::operator[](int at) const {
    if (is_valid() == OK && at < (int)elems_size_) {
        return elems_[at];
    }
    empty_param = {KM_TAG_INVALID, {}};
    return empty_param;
}

bool AuthorizationSet::push_back(const keymaster_key_param_set_t& set) {
    if (is_valid() != OK)
        return false;

    if (!reserve_elems(elems_size_ + set.length))
        return false;

    if (!reserve_indirect(indirect_data_size_ + ComputeIndirectDataSize(set.params, set.length)))
        return false;

    for (size_t i = 0; i < set.length; ++i)
        if (!push_back(set.params[i]))
            return false;

    return true;
}

bool AuthorizationSet::push_back(keymaster_key_param_t elem) {
    if (is_valid() != OK)
        return false;

    if (elems_size_ >= elems_capacity_)
        if (!reserve_elems(elems_capacity_ ? elems_capacity_ * 2 : STARTING_ELEMS_CAPACITY))
            return false;

    if (is_blob_tag(elem.tag)) {
        if (indirect_data_capacity_ - indirect_data_size_ < elem.blob.data_length)
            if (!reserve_indirect(2 * (indirect_data_capacity_ + elem.blob.data_length)))
                return false;

        memcpy(indirect_data_ + indirect_data_size_, elem.blob.data, elem.blob.data_length);
        elem.blob.data = indirect_data_ + indirect_data_size_;
        indirect_data_size_ += elem.blob.data_length;
    }

    elems_[elems_size_++] = elem;
    return true;
}

static size_t serialized_size(const keymaster_key_param_t& param) {
    switch (keymaster_tag_get_type(param.tag)) {
    case KM_INVALID:
        return sizeof(uint32_t);
    case KM_ENUM:
    case KM_ENUM_REP:
    case KM_UINT:
    case KM_UINT_REP:
        return sizeof(uint32_t) * 2;
    case KM_ULONG:
    case KM_ULONG_REP:
    case KM_DATE:
        return sizeof(uint32_t) + sizeof(uint64_t);
    case KM_BOOL:
        return sizeof(uint32_t) + 1;
    case KM_BIGNUM:
    case KM_BYTES:
        return sizeof(uint32_t) * 3;
    }

    return sizeof(uint32_t);
}

static uint8_t* serialize(const keymaster_key_param_t& param, uint8_t* buf, const uint8_t* end,
                          const uint8_t* indirect_base) {
    buf = append_uint32_to_buf(buf, end, param.tag);
    switch (keymaster_tag_get_type(param.tag)) {
    case KM_INVALID:
        break;
    case KM_ENUM:
    case KM_ENUM_REP:
        buf = append_uint32_to_buf(buf, end, param.enumerated);
        break;
    case KM_UINT:
    case KM_UINT_REP:
        buf = append_uint32_to_buf(buf, end, param.integer);
        break;
    case KM_ULONG:
    case KM_ULONG_REP:
        buf = append_uint64_to_buf(buf, end, param.long_integer);
        break;
    case KM_DATE:
        buf = append_uint64_to_buf(buf, end, param.date_time);
        break;
    case KM_BOOL:
        if (buf < end)
            *buf = static_cast<uint8_t>(param.boolean);
        buf++;
        break;
    case KM_BIGNUM:
    case KM_BYTES:
        buf = append_uint32_to_buf(buf, end, param.blob.data_length);
        buf = append_uint32_to_buf(buf, end, param.blob.data - indirect_base);
        break;
    }
    return buf;
}

static bool deserialize(keymaster_key_param_t* param, const uint8_t** buf_ptr, const uint8_t* end,
                        const uint8_t* indirect_base, const uint8_t* indirect_end) {
    if (!copy_uint32_from_buf(buf_ptr, end, &param->tag))
        return false;

    switch (keymaster_tag_get_type(param->tag)) {
    case KM_INVALID:
        return false;
    case KM_ENUM:
    case KM_ENUM_REP:
        return copy_uint32_from_buf(buf_ptr, end, &param->enumerated);
    case KM_UINT:
    case KM_UINT_REP:
        return copy_uint32_from_buf(buf_ptr, end, &param->integer);
    case KM_ULONG:
    case KM_ULONG_REP:
        return copy_uint64_from_buf(buf_ptr, end, &param->long_integer);
    case KM_DATE:
        return copy_uint64_from_buf(buf_ptr, end, &param->date_time);
        break;
    case KM_BOOL:
        if (*buf_ptr < end) {
            param->boolean = static_cast<bool>(**buf_ptr);
            (*buf_ptr)++;
            return true;
        }
        return false;

    case KM_BIGNUM:
    case KM_BYTES: {
        uint32_t offset;
        if (!copy_uint32_from_buf(buf_ptr, end, &param->blob.data_length) ||
            !copy_uint32_from_buf(buf_ptr, end, &offset))
            return false;
        if (param->blob.data_length + offset < param->blob.data_length ||  // Overflow check
            static_cast<ptrdiff_t>(offset) > indirect_end - indirect_base ||
            static_cast<ptrdiff_t>(offset + param->blob.data_length) > indirect_end - indirect_base)
            return false;
        param->blob.data = indirect_base + offset;
        return true;
    }
    }

    return false;
}

size_t AuthorizationSet::SerializedSizeOfElements() const {
    size_t size = 0;
    for (size_t i = 0; i < elems_size_; ++i) {
        size += serialized_size(elems_[i]);
    }
    return size;
}

size_t AuthorizationSet::SerializedSize() const {
    return sizeof(uint32_t) +           // Size of indirect_data_
           indirect_data_size_ +        // indirect_data_
           sizeof(uint32_t) +           // Number of elems_
           sizeof(uint32_t) +           // Size of elems_
           SerializedSizeOfElements();  // elems_
}

uint8_t* AuthorizationSet::Serialize(uint8_t* buf, const uint8_t* end) const {
    buf = append_size_and_data_to_buf(buf, end, indirect_data_, indirect_data_size_);
    buf = append_uint32_to_buf(buf, end, elems_size_);
    buf = append_uint32_to_buf(buf, end, SerializedSizeOfElements());
    for (size_t i = 0; i < elems_size_; ++i) {
        buf = serialize(elems_[i], buf, end, indirect_data_);
    }
    return buf;
}

bool AuthorizationSet::DeserializeIndirectData(const uint8_t** buf_ptr, const uint8_t* end) {
    UniquePtr<uint8_t[]> indirect_buf;
    if (!copy_size_and_data_from_buf(buf_ptr, end, &indirect_data_size_, &indirect_buf)) {
        LOG_E("Malformed data found in AuthorizationSet deserialization", 0);
        set_invalid(MALFORMED_DATA);
        return false;
    }
    indirect_data_ = indirect_buf.release();
    return true;
}

bool AuthorizationSet::DeserializeElementsData(const uint8_t** buf_ptr, const uint8_t* end) {
    uint32_t elements_count;
    uint32_t elements_size;
    if (!copy_uint32_from_buf(buf_ptr, end, &elements_count) ||
        !copy_uint32_from_buf(buf_ptr, end, &elements_size)) {
        LOG_E("Malformed data found in AuthorizationSet deserialization", 0);
        set_invalid(MALFORMED_DATA);
        return false;
    }

    // Note that the following validation of elements_count is weak, but it prevents allocation of
    // elems_ arrays which are clearly too large to be reasonable.
    if (static_cast<ptrdiff_t>(elements_size) > end - *buf_ptr ||
        elements_count * sizeof(uint32_t) > elements_size ||
        *buf_ptr + (elements_count * sizeof(*elems_)) < *buf_ptr) {
        LOG_E("Malformed data found in AuthorizationSet deserialization", 0);
        set_invalid(MALFORMED_DATA);
        return false;
    }

    if (!reserve_elems(elements_count))
        return false;

    uint8_t* indirect_end = indirect_data_ + indirect_data_size_;
    const uint8_t* elements_end = *buf_ptr + elements_size;
    for (size_t i = 0; i < elements_count; ++i) {
        if (!deserialize(elems_ + i, buf_ptr, elements_end, indirect_data_, indirect_end)) {
            LOG_E("Malformed data found in AuthorizationSet deserialization", 0);
            set_invalid(MALFORMED_DATA);
            return false;
        }
    }
    elems_size_ = elements_count;
    return true;
}

bool AuthorizationSet::Deserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    FreeData();

    if (!DeserializeIndirectData(buf_ptr, end) || !DeserializeElementsData(buf_ptr, end))
        return false;

    if (indirect_data_size_ != ComputeIndirectDataSize(elems_, elems_size_)) {
        LOG_E("Malformed data found in AuthorizationSet deserialization", 0);
        set_invalid(MALFORMED_DATA);
        return false;
    }
    return true;
}

void AuthorizationSet::Clear() {
    memset_s(elems_, 0, elems_size_ * sizeof(keymaster_key_param_t));
    memset_s(indirect_data_, 0, indirect_data_size_);
    elems_size_ = 0;
    indirect_data_size_ = 0;
}

void AuthorizationSet::FreeData() {
    Clear();

    delete[] elems_;
    delete[] indirect_data_;

    elems_ = NULL;
    indirect_data_ = NULL;
    elems_capacity_ = 0;
    indirect_data_capacity_ = 0;
    error_ = OK;
}

/* static */
size_t AuthorizationSet::ComputeIndirectDataSize(const keymaster_key_param_t* elems, size_t count) {
    size_t size = 0;
    for (size_t i = 0; i < count; ++i) {
        if (is_blob_tag(elems[i].tag)) {
            size += elems[i].blob.data_length;
        }
    }
    return size;
}

void AuthorizationSet::CopyIndirectData() {
    memset_s(indirect_data_, 0, indirect_data_capacity_);

    uint8_t* indirect_data_pos = indirect_data_;
    for (size_t i = 0; i < elems_size_; ++i) {
        assert(indirect_data_pos <= indirect_data_ + indirect_data_capacity_);
        if (is_blob_tag(elems_[i].tag)) {
            memcpy(indirect_data_pos, elems_[i].blob.data, elems_[i].blob.data_length);
            elems_[i].blob.data = indirect_data_pos;
            indirect_data_pos += elems_[i].blob.data_length;
        }
    }
    assert(indirect_data_pos == indirect_data_ + indirect_data_capacity_);
    indirect_data_size_ = indirect_data_pos - indirect_data_;
}

size_t AuthorizationSet::GetTagCount(keymaster_tag_t tag) const {
    size_t count = 0;
    for (int pos = -1; (pos = find(tag, pos)) != -1;)
        ++count;
    return count;
}

bool AuthorizationSet::GetTagValueEnum(keymaster_tag_t tag, uint32_t* val) const {
    int pos = find(tag);
    if (pos == -1) {
        return false;
    }
    *val = elems_[pos].enumerated;
    return true;
}

bool AuthorizationSet::GetTagValueEnumRep(keymaster_tag_t tag, size_t instance,
                                          uint32_t* val) const {
    size_t count = 0;
    int pos = -1;
    while (count <= instance) {
        pos = find(tag, pos);
        if (pos == -1) {
            return false;
        }
        ++count;
    }
    *val = elems_[pos].enumerated;
    return true;
}

bool AuthorizationSet::GetTagValueInt(keymaster_tag_t tag, uint32_t* val) const {
    int pos = find(tag);
    if (pos == -1) {
        return false;
    }
    *val = elems_[pos].integer;
    return true;
}

bool AuthorizationSet::GetTagValueIntRep(keymaster_tag_t tag, size_t instance,
                                         uint32_t* val) const {
    size_t count = 0;
    int pos = -1;
    while (count <= instance) {
        pos = find(tag, pos);
        if (pos == -1) {
            return false;
        }
        ++count;
    }
    *val = elems_[pos].integer;
    return true;
}

bool AuthorizationSet::GetTagValueLong(keymaster_tag_t tag, uint64_t* val) const {
    int pos = find(tag);
    if (pos == -1) {
        return false;
    }
    *val = elems_[pos].long_integer;
    return true;
}

bool AuthorizationSet::GetTagValueLongRep(keymaster_tag_t tag, size_t instance,
                                          uint64_t* val) const {
    size_t count = 0;
    int pos = -1;
    while (count <= instance) {
        pos = find(tag, pos);
        if (pos == -1) {
            return false;
        }
        ++count;
    }
    *val = elems_[pos].long_integer;
    return true;
}

bool AuthorizationSet::GetTagValueDate(keymaster_tag_t tag, uint64_t* val) const {
    int pos = find(tag);
    if (pos == -1) {
        return false;
    }
    *val = elems_[pos].date_time;
    return true;
}

bool AuthorizationSet::GetTagValueBlob(keymaster_tag_t tag, keymaster_blob_t* val) const {
    int pos = find(tag);
    if (pos == -1) {
        return false;
    }
    *val = elems_[pos].blob;
    return true;
}

bool AuthorizationSet::GetTagValueBool(keymaster_tag_t tag) const {
    int pos = find(tag);
    if (pos == -1) {
        return false;
    }
    assert(elems_[pos].boolean);
    return elems_[pos].boolean;
}

bool AuthorizationSet::ContainsEnumValue(keymaster_tag_t tag, uint32_t value) const {
    for (auto& entry : *this)
        if (entry.tag == tag && entry.enumerated == value)
            return true;
    return false;
}

bool AuthorizationSet::ContainsIntValue(keymaster_tag_t tag, uint32_t value) const {
    for (auto& entry : *this)
        if (entry.tag == tag && entry.integer == value)
            return true;
    return false;
}

}  // namespace keymaster
