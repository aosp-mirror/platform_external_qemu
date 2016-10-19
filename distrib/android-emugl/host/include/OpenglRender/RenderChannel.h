// Copyright (C) 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include "android/base/EnumFlags.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <assert.h>

namespace emugl {

// Turn the RenderChannel::Event enum into flags.
using namespace ::android::base::EnumFlags;

// WIP: a vector with small buffer optimization
template <class T>
class SmallVector {
public:
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T*;
    using pointer = T*;
    using const_pointer = T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;

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

protected:
    SmallVector() = default;
    SmallVector(iterator begin, iterator end, size_type capacity)
        : mBegin(begin), mEnd(end), mCapacity(capacity) {}
    ~SmallVector() = default;

    iterator mBegin;
    iterator mEnd;
    size_type mCapacity;
};

template <class T, size_t SmallSize>
class SmallFixedVector : public SmallVector<T> {
public:
    using DynamicVector = std::vector<T>;
    using value_type = typename SmallVector<T>::value_type;
    using iterator = typename SmallVector<T>::iterator;
    using const_iterator = typename SmallVector<T>::const_iterator;
    using pointer = typename SmallVector<T>::pointer;
    using const_pointer = typename SmallVector<T>::const_pointer;
    using reference = typename SmallVector<T>::reference;
    using const_reference = typename SmallVector<T>::const_reference;
    using size_type = typename SmallVector<T>::size_type;

    SmallFixedVector() : SmallVector<T>(mData.array, mData.array, SmallSize) {}

    SmallFixedVector(const_iterator b, const_iterator e) : SmallFixedVector() {
        insert_back(b, e);
    }

    SmallFixedVector(DynamicVector&& v) {
        new (&mData.vector) DynamicVector(std::move(v));
        this->mBegin = &*mData.vector.begin();
        this->mEnd = &*mData.vector.end();
        this->mCapacity = mData.vector.capacity();
    }

    SmallFixedVector(SmallFixedVector&& other) : SmallFixedVector() {
        if (other.isAllocated()) {
            new (&mData.vector) DynamicVector(std::move(other.mData.vector));
            this->mBegin = &*mData.vector.begin();
            this->mEnd = &*mData.vector.end();
            this->mCapacity = mData.vector.capacity();
            other.mBegin = &*other.mData.vector.begin();
            other.mEnd = &*other.mData.vector.end();
            other.mCapacity = other.mData.vector.capacity();
        } else {
            std::move(other.begin(), other.end(), this->begin());
            this->mEnd = this->mBegin + other.size();
        }
    }

    SmallFixedVector(SmallVector<T>&& other) = delete;

    SmallFixedVector& operator=(SmallFixedVector&& other) {
        if (other.isAllocated()) {
            if (!isAllocated()) {
                destruct(this->begin(), this->end());
                new (&mData.vector) DynamicVector(std::move(other.mData.vector));
            } else {
                mData.vector = std::move(other.mData.vector);
            }
            this->mBegin = &*mData.vector.begin();
            this->mEnd = &*mData.vector.end();
            this->mCapacity = mData.vector.capacity();
            other.mBegin = &*other.mData.vector.begin();
            other.mEnd = &*other.mData.vector.end();
            other.mCapacity = other.mData.vector.capacity();
        } else {
            if (isAllocated()) {
                if (mData.vector.capacity() >= other.size()) {
                    mData.vector.resize(other.size());
                    std::move(other.begin(), other.end(), this->begin());
                    assert(this->mBegin == &*mData.vector.begin());
                    this->mEnd = this->mBegin + other.size();
                    return *this;
                }
                mData.vector.~DynamicVector();
                this->mBegin = &mData.array[0];
                this->mCapacity = SmallSize;
            } else if (this->size() < other.size()) {
                // tricky: move() the part that fits into the size(),
                // move-construct new elements.
                std::move(other.begin(),
                          other.begin() + this->size(),
                          this->begin());
                for (auto it = this->begin() + this->size(),
                        itOther = other.begin() + this->size();
                     itOther != other.end();
                     ++it, ++itOther) {
                    new (it) T(std::move(*itOther));
                }
                this->mEnd = this->begin() + other.size();
                return *this;
            } else { // size() >= other.size()
                destruct(this->begin() + other.size(), this->end());
            }
            std::move(other.begin(), other.end(), this->begin());
            this->mEnd = this->begin() + other.size();
        }
        return *this;
    }

    SmallFixedVector& operator=(SmallVector<T>&& other) = delete;

    ~SmallFixedVector() {
        if (isAllocated()) {
            mData.vector.~DynamicVector();
        } else {
            destruct(this->begin(), this->end());
        }
    }

    bool isAllocated() const { return this->begin() != mData.array; }

    void reserve(size_type newCap) {
        if (newCap <= this->capacity()) {
            return;
        }

        if (isAllocated()) {
            mData.vector.reserve(newCap);
            this->mBegin = &*mData.vector.begin();
            this->mEnd = &*mData.vector.end();
            this->mCapacity = newCap;
        } else {
            assert(newCap > SmallSize);
            toDynamic(newCap);
        }
    }

    template <class Iter>
    void insert(iterator where, Iter b, Iter e) {
        // This is the only kind of insert we need for now.
        assert(where == this->end());
        insert_back(b, e);
    }

private:
    void toDynamic(size_type newCap) {
        assert(!isAllocated());

        DynamicVector vec;
        vec.reserve(newCap);
        vec.assign(std::move_iterator<iterator>(this->begin()),
                   std::move_iterator<iterator>(this->end()));
        destruct(this->begin(), this->end());
        new (&mData.vector) DynamicVector(std::move(vec));
        this->mBegin = &*mData.vector.begin();
        this->mEnd = &*mData.vector.end();
        this->mCapacity = mData.vector.capacity();
    }

    template <class Iter>
    void insert_back(Iter b, Iter e) {
        const auto newSize = this->size() + (e - b);
        if (newSize > this->capacity()) {
            reserve(std::max(newSize, 2 * this->capacity()));
        }
        if (isAllocated()) {
            mData.vector.insert(mData.vector.end(), b, e);
            // Should not change after the reserve() call.
            assert(this->mBegin == &*mData.vector.begin());
            this->mEnd = &*mData.vector.end();
        } else {
            auto it = this->end();
            for (; b != e; ++it, ++b) {
                new (it) T(*b);
            }
            this->mEnd = it;
        }
    }

    static void destruct(T* b, T* e) {
        for (; b != e; ++b) {
            b->~T();
        }
    }

    union Data {
        T array[SmallSize];
        DynamicVector vector;

        // These two are needed as compiler doesn't know how to generate
        // defaults for a union with std::vector<> in it.
        Data() {}
        ~Data() {}
    } mData;
};

// A type used for data passing.
using ChannelBuffer = SmallFixedVector<char, 512>;

// RenderChannel - an interface for a single guest to host renderer connection.
// It allows the guest to send GPU emulation protocol-serialized messages to an
// asynchronous renderer, read the responses and subscribe for state updates.
class RenderChannel {
public:
    // Flags for the channel state.
    enum class State {
        // Can't use None here, some system header declares it as a macro.
        Empty = 0,
        CanRead = 1 << 0,
        CanWrite = 1 << 1,
        Stopped = 1 << 2,
    };

    // Possible points of origin for an event in EventCallback.
    enum class EventSource {
        RenderChannel,
        Client,
    };

    // Types of read() the channel supports.
    enum class CallType {
        Blocking,    // if the call can't do what it needs, block until it can
        Nonblocking, // immidiately return if the call can't do the job
    };

    // Sets a single (!) callback that is called if some event happends that
    // changes the channel state - e.g. when it's stopped, or it gets some data
    // the client can read after being empty, or it isn't full anymore and the
    // client may write again without blocking.
    // If the state isn't State::Empty, the |callback| is called for the first
    // time during the setEventCallback() to report this initial state.
    using EventCallback = std::function<void(State, EventSource)>;
    virtual void setEventCallback(EventCallback callback) = 0;

    // Writes the data in |buffer| into the channel. |buffer| is moved from.
    // Blocks if there's no room in the channel (shouldn't really happen).
    // Returns false if the channel is stopped.
    virtual bool write(ChannelBuffer&& buffer) = 0;
    // Reads a chunk of data from the channel. Returns false if there was no
    // data for a non-blocking call or if the channel is stopped.
    virtual bool read(ChannelBuffer* buffer, CallType callType) = 0;

    // Get the current state flags.
    virtual State currentState() const = 0;

    // Abort all pending operations. Any following operation is a noop.
    virtual void stop() = 0;
    // Check if the channel is stopped.
    virtual bool isStopped() const = 0;

protected:
    ~RenderChannel() = default;
};

using RenderChannelPtr = std::shared_ptr<RenderChannel>;

}  // namespace emugl
