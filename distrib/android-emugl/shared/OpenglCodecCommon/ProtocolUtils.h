#ifndef EMUGL_PROTOCOL_UTILS_H
#define EMUGL_PROTOCOL_UTILS_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

namespace emugl {

// Helper macro
#define COMPILE_ASSERT(cond)  static char kAssert##__LINE__[1 - 2 * !(cond)] __attribute__((unused)) = { 0 }

// Helper template: is_pointer.
// is_pointer<T>::value is true iff |T| is a pointer type.
template <typename T> struct is_pointer {
    static const bool value = false;
};

template <typename T> struct is_pointer<T*> {
    static const bool value = true;
};

// A helper template to extract values form the wire protocol stream
// and convert them to appropriate host values.
//
// The wire protocol uses 32-bit exclusively when transferring
// GLintptr or GLsizei values, as well as opaque handles like GLeglImage,
// from the guest (even when the guest is 64-bit).
//
// The corresponding host definitions depend on the host bitness. For
// example, GLintptr is 64-bit on linux-x86_64. The following is a set
// of templates that can simplify the conversion of protocol values
// into host ones.
//
// The most important one is:
//
//     unpack<HOST_TYPE,SIZE_TYPE>(const void* ptr)
//
// Which reads bytes from |ptr|, using |SIZE_TYPE| as the underlying
// sized-integer specifier (e.g. 'uint32_t'), and converting the result
// into a |HOST_TYPE| value. For example:
//
//     unpack<EGLImage,uint32_t>(ptr + 12);
//
// will read a 4-byte value from |ptr + 12| and convert it into
// an EGLImage, which is a host void*. The template detects host
// pointer types to perform proper type casting.
//
// TODO(digit): Add custom unpackers to handle generic opaque void* values.
//              and map them to unique 32-bit values.

template <typename T, typename S, bool IS_POINTER>
struct UnpackerT {};

template <typename T, typename S>
struct UnpackerT<T,S,false> {
    static inline T unpack(const void* ptr) {
        COMPILE_ASSERT(sizeof(T) == sizeof(S));
        return (T)(*(S*)(ptr));
    }
};

template <typename T, typename S>
struct UnpackerT<T,S,true> {
    static inline T unpack(const void* ptr) {
        return (T)(uintptr_t)(*(S*)(ptr));
    }
};

template <>
struct UnpackerT<float,uint32_t,false> {
    static inline float unpack(const void* ptr) {
        union {
            float f;
            uint32_t u;
        } v;
        v.u = *(uint32_t*)(ptr);
        return v.f;
    }
};

template <>
struct UnpackerT<double,uint64_t,false> {
    static inline double unpack(const void* ptr) {
        union {
            double d;
            uint32_t u;
        } v;
        v.u = *(uint64_t*)(ptr);
        return v.d;
    }
};

template <>
struct UnpackerT<ssize_t,uint32_t,false> {
    static inline ssize_t unpack(const void* ptr) {
        return (ssize_t)*(int32_t*)(ptr);
    }
};

template <typename T, typename S>
inline T Unpack(const void* ptr) {
    return UnpackerT<T, S, is_pointer<T>::value>::unpack(ptr);
}

// Helper class used to ensure input buffers passed to EGL/GL functions
// are properly aligned (preventing crashes with some backends).
// Usage example:
//
//    GenericInputBuffer<> inputBuffer(ptr, size);
//    glDoStuff(inputBuffer.get());
//
// inputBuffer.get() will return the original value of |ptr| if it was
// aligned on an 8-byte boundary. Otherwise, it will return the address
// of an aligned heap-allocated copy of the original |size| bytes starting
// from |ptr|. The heap block is released at scope exit.

template <size_t StackSize = 1024, size_t Align = 8>
class GenericInputBuffer {
public:
    GenericInputBuffer(const void* input, size_t size) : mBuff(input) {
        if (((uintptr_t)input & (Align - 1U)) != 0) {
            if (size <= StackSize) {
                mStorage = StorageType::InPlace;
                memcpy(&mArray[0], input, size);
            } else {
                mStorage = StorageType::Heap;
                auto buf = malloc(size);
                memcpy(buf, input, size);
                mBuff = buf;
            }
        }
    }

    ~GenericInputBuffer() {
        if (mStorage == StorageType::Heap) {
            free((void*)mBuff);
        }
    }

    const void* get() const {
        return mStorage == StorageType::InPlace ? &mArray[0] : mBuff;
    }

private:
    enum class StorageType : char {
        Original,
        InPlace,
        Heap
    };

    union {
        char __attribute__((__aligned__(Align))) mArray[StackSize];
        const void* mBuff;
    };
    StorageType mStorage = StorageType::Original;
};

// Helper class used to ensure that output buffers passed to EGL/GL functions
// are aligned on 8-byte addresses.
// Usage example:
//
//    ptr = stream->alloc(size);
//    GenericOutputBuffer<> outputBuffer(ptr, size);
//    glGetStuff(outputBuffer.get());
//    outputBuffer.flush();
//
// outputBuffer.get() returns the original value of |ptr| if it was already
// aligned on an 8=byte boundary. Otherwise, it returns the size of an heap
// allocated zeroed buffer of |size| bytes.
//
// outputBuffer.flush() copies the content of the heap allocated buffer back
// to |ptr| explictly, if needed. If a no-op if |ptr| was aligned.
template <size_t StackSize = 1024, size_t Align = 8>
class GenericOutputBuffer {
public:
    GenericOutputBuffer(unsigned char* ptr, size_t size) :
            mOrgBuff(ptr), mSize(size) {
        if (((uintptr_t)ptr & (Align - 1U)) != 0) {
            if (StackSize <= size) {
                mStorage = StorageType::InPlace;
            } else {
                mStorage = StorageType::Heap;
                mBuff = calloc(1, size);
            }
        }
    }

    ~GenericOutputBuffer() {
        if (mStorage == StorageType::Heap) {
            free(mBuff);
        }
    }

    void* get() const {
        switch (mStorage) {
        case StorageType::Original:
            return mOrgBuff;
        case StorageType::InPlace:
            return (void*)&mArray[0];
        case StorageType::Heap:
            return (void*)mBuff;
        }
        assert(0);
        return nullptr;
    }

    void flush() {
        if (mStorage != StorageType::Original) {
            memcpy(mOrgBuff, get(), mSize);
        }
    }

private:
    enum class StorageType : char {
        Original,
        InPlace,
        Heap
    };

    unsigned char* mOrgBuff;
    union {
        unsigned char __attribute__((__aligned__(Align))) mArray[StackSize];
        void* mBuff;
    };
    size_t mSize;
    StorageType mStorage = StorageType::Original;
};

// Pin the defaults for the generally used type names
using InputBuffer = GenericInputBuffer<>;
using OutputBuffer = GenericOutputBuffer<>;

}  // namespace emugl

#endif  // EMUGL_PROTOCOL_UTILS_H
