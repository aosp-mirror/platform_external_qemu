// Copyright 2018 The Android Open Source Project
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

#include <memory>
#include <utility>
#include <cassert>

#include <fcntl.h>
#include <stdlib.h>

#ifdef _WIN32
#include "android/base/memory/win32/Win32SharedMemoryPolicy.h"
#else
#include "android/base/memory/posix/PosixSharedMemoryPolicy.h"
#endif

namespace android {

namespace base {

template<class MemoryPolicy>
class SharedMemory {
public:
    typedef typename MemoryPolicy::memory_type memory_type;
    typedef typename MemoryPolicy::handle_type handle_type;

    SharedMemory(std::string name, size_t size) : mName(name), mSize(size), mAddr(MemoryPolicy::unmappedMemory()) {
        assert(mName.size() <= NAME_MAX);
        assert(mName.size() > 1 && mName[0] == '/');
        assert(mName.find('/', 1) == std::string::npos);
    }
    SharedMemory(SharedMemory&& other) {
        mName = other.mName;
        mSize = other.mSize;
        mAddr = other.mAddr;
        mFd = other.mFd;

        other.mFd = MemoryPolicy::invalidHandle();
        other.mAddr = MemoryPolicy::unmappedMemory();
    }

    ~SharedMemory() { close(); }

    // Let's not have any weirdness due to double unlinks due to destructors.
    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;

    SharedMemory& operator=(SharedMemory&& other) {
        if (this != &other) {
            mName = other.mName;
            mSize = other.mSize;
            mAddr = other.mAddr;
            mFd = other.mFd;

            other.mFd = MemoryPolicy::invalid();
            other.mAddr = MemoryPolicy::unmappedMemory();
        }

        return *this;
    }

    // Opens the shared memory object, returns 0 on success
    // or the error code.
    int open(int oflag, mode_t mode) {
        if (isOpen()) {
            close();
        }
        assert(!isOpen());

        int err = 0;
        std::tie(mFd, mAddr, err) = MemoryPolicy::open(mName, oflag, mode, mSize);
        if (err != 0) {
            close();
            return err;
        }

        assert(isOpen());
        return 0;
   }

    void close() {
        std::tie(mAddr, mFd) = MemoryPolicy::close(mName, mAddr, mFd, mSize);
        assert(!isOpen());
    }

    size_t size() { return mSize; }

    bool isOpen() const {
        return mAddr != MemoryPolicy::unmappedMemory();
    }

    memory_type operator*() {
        return get();
    }

    memory_type get() {
        assert(isOpen());
        return mAddr;
    }

private:
    std::string mName;
    size_t mSize;
    memory_type mAddr;
    handle_type mFd;
};

// Using directly shared memory, nothing should go to disk. 
using NamedSharedMemory = SharedMemory<NamedSharedMemoryOsPolicy>;
}  // namespace base
}  // namespace android
