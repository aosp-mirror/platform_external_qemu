// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <algorithm>
#include <type_traits>
#include <utility>

#include <assert.h>
#include <stddef.h>

namespace android {
namespace base {

// Forward-declare a 'real' small vector class.
template <class T, size_t S>
class SmallFixedVector;

//
// SmallVector<T> - an interface for a small-buffer-optimized vector.
// It hides the fixed size from its type, so one can use it to pass small
// vectors around and not leak the buffer size to all callers:
//
//  void process(SmallVector<Foo>& data);
//  ...
//  ...
//  SmallFixedVector<Foo, 100> aLittleBitOfFoos = ...;
//  process(aLittleBitOfFoos);
//  ...
//  SmallFixedVector<Foo, 1000> moreFoos = ...;
//  process(moreFoos);
//
template <class T>
class SmallVector {
    template <class U, size_t S>
    friend class SmallFixedVector;

public:
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T*;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;

    // It's ok to delete SmallVector<> through the base class - dtor() actually
    // takes care of all living elements and the allocated memory
    ~SmallVector() { dtor(); }

    iterator begin() { return mBegin; }
    const_iterator begin() const { return mBegin; }
    const_iterator cbegin() const { return mBegin; }

    iterator end() { return mEnd; }
    const_iterator end() const { return mEnd; }
    const_iterator cend() const { return mEnd; }

    size_type size() const { return end() - begin(); }
    size_type capacity() const { return mCapacity; }

    reference operator[](size_t i) { return *(begin() + i); }
    const_reference operator[](size_t i) const { return *(cbegin() + i); }

    pointer data() { return mBegin; }
    const_pointer data() const { return mBegin; }

    void clear() {
        destruct(begin(), end());
        mEnd = mBegin;
    }

    void reserve(size_type newCap) {
        if (newCap <= this->capacity()) {
            return;
        }
        grow_capacity(newCap);
    }

    void resize(size_type newSize, bool initialize = true) {
        if (newSize < this->size()) {
            const auto newEnd = this->begin() + newSize;
            this->destruct(newEnd, this->end());
            this->mEnd = newEnd;
        } else if (newSize > this->size()) {
            grow_for_size(newSize);
            const auto newEnd = this->begin() + newSize;
            if (initialize) {
                std::uninitialized_fill(this->end(), newEnd, T());
            }
            this->mEnd = newEnd;
        }
    }

    bool isAllocated() const { return this->cbegin() != smallBufferStart(); }

protected:
    SmallVector() = default;

    void dtor() {
        this->destruct(this->begin(), this->end());
        if (isAllocated()) {
            free(this->mBegin);
        }
    }

    void init(iterator begin, iterator end, size_type capacity) {
        this->mBegin = begin;
        this->mEnd = end;
        this->mCapacity = capacity;
    }

    template <class Iter>
    void insert_back(Iter b, Iter e) {
        if (b == e) {
            return;
        }
        const auto newSize = this->size() + (e - b);
        grow_for_size(newSize);
        this->mEnd = std::uninitialized_copy(b, e, this->mEnd);
    }

    void grow_for_size(size_type newSize) {
        // Grow by 1.5x by default.
        if (newSize > capacity()) {
            grow_capacity(std::max(newSize, capacity() + capacity() / 2));
        }
    }

    void grow_capacity(size_type newCap) {
        // Here we can only be switching to the dynamic vector, as static one
        // always has its capacity on the maximum.
        const auto newBegin = (T*)malloc(sizeof(T) * newCap);
        if (!newBegin) {
            abort();  // what else can we do here?
        }
        auto newEnd = std::uninitialized_copy(
                std::make_move_iterator(this->begin()),
                std::make_move_iterator(this->end()), newBegin);
        dtor();
        this->mBegin = newBegin;
        this->mEnd = newEnd;
        this->mCapacity = newCap;
    }

    constexpr const void* smallBufferStart() const {
        return (const void*)(&mCapacity + 1);
    }

    static void destruct(T* b, T* e) {
        if (!std::is_trivially_destructible<T>::value) {
            for (; b != e; ++b) {
                b->~T();
            }
        }
    }

    iterator mBegin;
    iterator mEnd;
    size_type mCapacity;
};

template <class T, size_t SmallSize>
class SmallFixedVector : public SmallVector<T> {
    using base = SmallVector<T>;

public:
    using value_type = typename base::value_type;
    using iterator = typename base::iterator;
    using const_iterator = typename base::const_iterator;
    using pointer = typename base::pointer;
    using const_pointer = typename base::const_pointer;
    using reference = typename base::reference;
    using const_reference = typename base::const_reference;
    using size_type = typename base::size_type;

    static constexpr size_type kSmallSize = SmallSize;

    // Default constructor - set up an emtpt vector with capacity at full
    // internal array size.
    SmallFixedVector() {
        // Make sure that the small array starts exactly where base class
        // expects it: right after the |mCapacity|.
        static_assert(offsetof(base, mCapacity) + sizeof(base::mCapacity) ==
                                      offsetof(SmallFixedVector, mData) &&
                              offsetof(Data, array) == 0,
                      "SmallFixedVector<> class layout is wrong, "
                      "|mData| needs to follow |mCapacity|");

        init_static();
    }

    // Ctor from a range of iterators
    template <class Iter>
    SmallFixedVector(Iter b, Iter e) : SmallFixedVector() {
        this->insert_back(b, e);
    }

    template <class Range>
    explicit SmallFixedVector(const Range& r)
        : SmallFixedVector(std::begin(r), std::end(r)) {}

    SmallFixedVector(SmallFixedVector&& other) {
        if (other.isAllocated()) {
            this->mBegin = other.mBegin;
            this->mEnd = other.mEnd;
            this->mCapacity = other.mCapacity;
            other.init_static();
        } else {
            this->mBegin = mData.array;
            this->mEnd = std::uninitialized_copy(
                    std::make_move_iterator(other.begin()),
                    std::make_move_iterator(other.end()), this->begin());
            this->mCapacity = kSmallSize;
        }
    }

    SmallFixedVector(base&& other) = delete;

    SmallFixedVector& operator=(SmallFixedVector&& other) {
        if (other.isAllocated()) {
            this->dtor();
            this->mBegin = other.mBegin;
            this->mEnd = other.mEnd;
            this->mCapacity = other.mCapacity;
            other.init_static();
        } else {
            if (this->isAllocated()) {
                if (this->mCapacity >= other.size()) {
                    this->destruct(this->begin(), this->end());
                    auto newEnd = std::uninitialized_copy(
                            std::make_move_iterator(other.begin()),
                            std::make_move_iterator(other.end()), this->mBegin);
                    this->mEnd = newEnd;
                    // |other| is valid as-is now
                    return *this;
                }
                // We've got not enough capacity, fall back to the static vector
                this->dtor();
                init_static();
            } else if (this->size() < other.size()) {
                // tricky: move() the part that fits into the size(),
                // move-construct new elements.
                std::move(other.begin(), other.begin() + this->size(),
                          this->mBegin);
                auto newEnd = std::uninitialized_copy(
                        std::make_move_iterator(other.begin() + this->size()),
                        std::make_move_iterator(other.end()), this->mEnd);
                this->mEnd = newEnd;
                return *this;
            } else {  // size() >= other.size()
                this->destruct(this->begin() + other.size(), this->end());
            }
            this->mEnd = std::move(other.begin(), other.end(), this->begin());
        }
        return *this;
    }

    SmallFixedVector& operator=(base&& other) = delete;

private:
    void init_static() { this->init(mData.array, mData.array, kSmallSize); }

    union Data {
        alignas(size_type) T array[kSmallSize];

        Data() {}
        ~Data() {}
    } mData;
};

}  // namespace base
}  // namespace android
