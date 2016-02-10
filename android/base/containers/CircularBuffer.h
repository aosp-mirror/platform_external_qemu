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

#include <deque>
#include <utility>

namespace android {
namespace base {

// A CircularBuffer<T> is a wrapper over std::deque<T> of fixed capacity
// that has the following properties:
// 1. Only allows adding new elements via |push_back|
// 2. Items can be removed either from the front or from the back.
// 3. When at maximum capacity, the oldest element is removed to make
//    space for the new one.
template <class T, class A = std::allocator<T>>
class CircularBuffer {
public:
    using iterator = typename std::deque<T, A>::iterator;
    using const_iterator = typename std::deque<T, A>::const_iterator;

    // Create a new circular buffer with the given |capacity|.
    explicit CircularBuffer(int capacity) : mCapacity(capacity) {}

    // Adds a new element at the end of the container. If the container
    // is at maximum capacity, the oldest element will be removed to make
    // space for the new one.
    void push_back(const T& value) {
        if (mBuf.size() == mCapacity) {
            pop_front();
        }
        mBuf.push_back(value);
    }

    // Same as above, but allows moving the element into the container
    // as opposed to copying.
    void push_back(T&& value) {
        if (mBuf.size() == mCapacity) {
            pop_front();
        }
        mBuf.push_back(std::forward<T>(value));
    }
 
    // Returns the first element in the container.
    // Undefined behavior if called on an empty container.
    T& front() {
        return mBuf.front();
    }

    // Same as above, const version.
    const T& front() const {
        return mBuf.front();
    }

    // Returns the last element in the container.
    // Undefined behavior if called on an empty container.
    T& back() {
        return mBuf.back();
    }

    // Same as above, const version.
    const T& back() const {
        return mBuf.back();
    }

    // Removes the first element in the container.
    // Undefined behavior if called on an empty container.
    void pop_front() {
        mBuf.pop_front();
    }

    // Removes the last element in the container.
    // Undefined behavior if called on an empty container.
    void pop_back() {
        mBuf.pop_back();
    }

    // Returns true if the container is empty, false otherwise.
    bool empty() const {
        return mBuf.empty();
    }

    // Returns the number of elements in the buffer
    int size() const {
        return mBuf.size();
    }

    iterator begin() { return mBuf.begin(); }
    iterator end() { return mBuf.end(); }

    const_iterator begin() const { return mBuf.begin(); }
    const_iterator end() const { return mBuf.end(); }

private:
    std::deque<T, A> mBuf;
    int mCapacity;
};

}
}
