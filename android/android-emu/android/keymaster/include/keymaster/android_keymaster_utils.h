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

#ifndef SYSTEM_KEYMASTER_ANDROID_KEYMASTER_UTILS_H_
#define SYSTEM_KEYMASTER_ANDROID_KEYMASTER_UTILS_H_

#include <stdint.h>
#include <string.h>
#include <time.h>  // for time_t.

#include <UniquePtr.h>

#include <hardware/keymaster_defs.h>
#include <keymaster/serializable.h>

namespace keymaster {

/**
 * Convert the specified time value into "Java time", which is a signed 64-bit integer representing
 * elapsed milliseconds since Jan 1, 1970.
 */
inline int64_t java_time(time_t time) {
    // The exact meaning of a time_t value is implementation-dependent.  If this code is ported to a
    // platform that doesn't define it as "seconds since Jan 1, 1970 UTC", this function will have
    // to be revised.
    return static_cast<int64_t>(time) * 1000;
}

/*
 * Array Manipulation functions.  This set of templated inline functions provides some nice tools
 * for operating on c-style arrays.  C-style arrays actually do have a defined size associated with
 * them, as long as they are not allowed to decay to a pointer.  These template methods exploit this
 * to allow size-based array operations without explicitly specifying the size.  If passed a pointer
 * rather than an array, they'll fail to compile.
 */

/**
 * Return the size in bytes of the array \p a.
 */
template <typename T, size_t N> inline size_t array_size(const T (&a)[N]) {
    return sizeof(a);
}

/**
 * Return the number of elements in array \p a.
 */
template <typename T, size_t N> inline size_t array_length(const T (&)[N]) {
    return N;
}

/**
 * Duplicate the array \p a.  The memory for the new array is allocated and the caller takes
 * responsibility.
 */
template <typename T> inline T* dup_array(const T* a, size_t n) {
    T* dup = new (std::nothrow) T[n];
    if (dup)
        for (size_t i = 0; i < n; ++i)
            dup[i] = a[i];
    return dup;
}

/**
 * Duplicate the array \p a.  The memory for the new array is allocated and the caller takes
 * responsibility.  Note that the dup is necessarily returned as a pointer, so size is lost.  Call
 * array_length() on the original array to discover the size.
 */
template <typename T, size_t N> inline T* dup_array(const T (&a)[N]) {
    return dup_array(a, N);
}

/**
 * Duplicate the buffer \p buf.  The memory for the new buffer is allocated and the caller takes
 * responsibility.
 */
uint8_t* dup_buffer(const void* buf, size_t size);

/**
 * Copy the contents of array \p arr to \p dest.
 */
template <typename T, size_t N> inline void copy_array(const T (&arr)[N], T* dest) {
    for (size_t i = 0; i < N; ++i)
        dest[i] = arr[i];
}

/**
 * Search array \p a for value \p val, returning true if found.  Note that this function is
 * early-exit, meaning that it should not be used in contexts where timing analysis attacks could be
 * a concern.
 */
template <typename T, size_t N> inline bool array_contains(const T (&a)[N], T val) {
    for (size_t i = 0; i < N; ++i) {
        if (a[i] == val) {
            return true;
        }
    }
    return false;
}

/**
 * Variant of memset() that uses GCC-specific pragmas to disable optimizations, so effect is not
 * optimized away.  This is important because we often need to wipe blocks of sensitive data from
 * memory.  As an additional convenience, this implementation avoids writing to NULL pointers.
 */
#ifdef __clang__
#define OPTNONE __attribute__((optnone))
#else  // not __clang__
#define OPTNONE __attribute__((optimize("O0")))
#endif  // not __clang__
inline OPTNONE void* memset_s(void* s, int c, size_t n) {
    if (!s)
        return s;
    return memset(s, c, n);
}
#undef OPTNONE

/**
 * Variant of memcmp that has the same runtime regardless of whether the data matches (i.e. doesn't
 * short-circuit).  Not an exact equivalent to memcmp because it doesn't return <0 if p1 < p2, just
 * 0 for match and non-zero for non-match.
 */
int memcmp_s(const void* p1, const void* p2, size_t length);

/**
 * Eraser clears buffers.  Construct it with a buffer or object and the destructor will ensure that
 * it is zeroed.
 */
class Eraser {
  public:
    /* Not implemented.  If this gets used, we want a link error. */
    template <typename T> explicit Eraser(T* t);

    template <typename T>
    explicit Eraser(T& t) : buf_(reinterpret_cast<uint8_t*>(&t)), size_(sizeof(t)) {}

    template <size_t N> explicit Eraser(uint8_t (&arr)[N]) : buf_(arr), size_(N) {}

    Eraser(void* buf, size_t size) : buf_(static_cast<uint8_t*>(buf)), size_(size) {}
    ~Eraser() { memset_s(buf_, 0, size_); }

  private:
    Eraser(const Eraser&);
    void operator=(const Eraser&);

    uint8_t* buf_;
    size_t size_;
};

/**
 * ArrayWrapper is a trivial wrapper around a C-style array that provides begin() and end()
 * methods. This is primarily to facilitate range-based iteration on arrays.  It does not copy, nor
 * does it take ownership; it just holds pointers.
 */
template <typename T> class ArrayWrapper {
  public:
    ArrayWrapper(T* array, size_t size) : begin_(array), end_(array + size) {}

    T* begin() { return begin_; }
    T* end() { return end_; }

  private:
    T* begin_;
    T* end_;
};
template <typename T> ArrayWrapper<T> array_range(T* begin, size_t length) {
    return ArrayWrapper<T>(begin, length);
}

/**
 * Convert any unsigned integer from network to host order.  We implement this here rather than
 * using the functions from arpa/inet.h because the TEE doesn't have inet.h.  This isn't the most
 * efficient implementation, but the compiler should unroll the loop and tighten it up.
 */
template <typename T> T ntoh(T t) {
    const uint8_t* byte_ptr = reinterpret_cast<const uint8_t*>(&t);
    T retval = 0;
    for (size_t i = 0; i < sizeof(t); ++i) {
        retval <<= 8;
        retval |= byte_ptr[i];
    }
    return retval;
}

/**
 * Convert any unsigned integer from host to network order.  We implement this here rather than
 * using the functions from arpa/inet.h because the TEE doesn't have inet.h.  This isn't the most
 * efficient implementation, but the compiler should unroll the loop and tighten it up.
 */
template <typename T> T hton(T t) {
    T retval;
    uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(&retval);
    for (size_t i = sizeof(t); i > 0; --i) {
        byte_ptr[i - 1] = t & 0xFF;
        t >>= 8;
    }
    return retval;
}

/**
 * KeymasterKeyBlob is a very simple extension of the C struct keymaster_key_blob_t.  It manages its
 * own memory, which makes avoiding memory leaks much easier.
 */
struct KeymasterKeyBlob : public keymaster_key_blob_t {
    KeymasterKeyBlob() {
        key_material = nullptr;
        key_material_size = 0;
    }

    KeymasterKeyBlob(const uint8_t* data, size_t size) {
        key_material_size = 0;
        key_material = dup_buffer(data, size);
        if (key_material)
            key_material_size = size;
    }

    explicit KeymasterKeyBlob(size_t size) {
        key_material_size = 0;
        key_material = new (std::nothrow) uint8_t[size];
        if (key_material)
            key_material_size = size;
    }

    explicit KeymasterKeyBlob(const keymaster_key_blob_t& blob) {
        key_material_size = 0;
        key_material = dup_buffer(blob.key_material, blob.key_material_size);
        if (key_material)
            key_material_size = blob.key_material_size;
    }

    KeymasterKeyBlob(const KeymasterKeyBlob& blob) {
        key_material_size = 0;
        key_material = dup_buffer(blob.key_material, blob.key_material_size);
        if (key_material)
            key_material_size = blob.key_material_size;
    }

    void operator=(const KeymasterKeyBlob& blob) {
        Clear();
        key_material = dup_buffer(blob.key_material, blob.key_material_size);
        key_material_size = blob.key_material_size;
    }

    ~KeymasterKeyBlob() { Clear(); }

    const uint8_t* begin() const { return key_material; }
    const uint8_t* end() const { return key_material + key_material_size; }

    void Clear() {
        memset_s(const_cast<uint8_t*>(key_material), 0, key_material_size);
        delete[] key_material;
        key_material = nullptr;
        key_material_size = 0;
    }

    const uint8_t* Reset(size_t new_size) {
        Clear();
        key_material = new (std::nothrow) uint8_t[new_size];
        if (key_material)
            key_material_size = new_size;
        return key_material;
    }

    // The key_material in keymaster_key_blob_t is const, which is the right thing in most
    // circumstances, but occasionally we do need to write into it.  This method exposes a non-const
    // version of the pointer.  Use sparingly.
    uint8_t* writable_data() { return const_cast<uint8_t*>(key_material); }

    keymaster_key_blob_t release() {
        keymaster_key_blob_t tmp = {key_material, key_material_size};
        key_material = nullptr;
        key_material_size = 0;
        return tmp;
    }

    size_t SerializedSize() const { return sizeof(uint32_t) + key_material_size; }
    uint8_t* Serialize(uint8_t* buf, const uint8_t* end) const {
        return append_size_and_data_to_buf(buf, end, key_material, key_material_size);
    }

    bool Deserialize(const uint8_t** buf_ptr, const uint8_t* end) {
        Clear();
        UniquePtr<uint8_t[]> tmp;
        if (!copy_size_and_data_from_buf(buf_ptr, end, &key_material_size, &tmp)) {
            key_material = nullptr;
            key_material_size = 0;
            return false;
        }
        key_material = tmp.release();
        return true;
    }
};

struct Characteristics_Delete {
    void operator()(keymaster_key_characteristics_t* p) {
        keymaster_free_characteristics(p);
        free(p);
    }
};

struct Malloc_Delete {
    void operator()(void* p) { free(p); }
};

struct CertificateChainDelete {
    void operator()(keymaster_cert_chain_t* p) {
        if (!p)
            return;
        for (size_t i = 0; i < p->entry_count; ++i)
            delete[] p->entries[i].data;
        delete[] p->entries;
        delete p;
    }
};

keymaster_error_t EcKeySizeToCurve(uint32_t key_size_bits, keymaster_ec_curve_t* curve);
keymaster_error_t EcCurveToKeySize(keymaster_ec_curve_t curve, uint32_t* key_size_bits);

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_ANDROID_KEYMASTER_UTILS_H_
