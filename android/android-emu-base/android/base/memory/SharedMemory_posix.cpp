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
#include "android/base/EintrWrapper.h"
#include "android/base/memory/SharedMemory.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

namespace android {
namespace base {

SharedMemory::SharedMemory(StringView name, size_t size)
    : mName(name), mSize(size) { }

int SharedMemory::create(mode_t mode) {
    return openInternal(O_CREAT | O_RDWR, mode);
}

int SharedMemory::createNoMapping(mode_t mode) {
    return openInternal(O_CREAT | O_RDWR, mode, false /* no mapping */);
}

int SharedMemory::open(AccessMode access) {
    int oflag = O_RDONLY;
    int mode = 0400;
    if (access == AccessMode::READ_WRITE) {
        oflag = O_RDWR;
        mode = 0600;
    }
    return openInternal(oflag, mode);
}

void SharedMemory::close() {
    if (mAddr != unmappedMemory()) {
        munmap(mAddr, mSize);
        mAddr = unmappedMemory();
    }
    if (mFd) {
        ::close(mFd);
        mFd = invalidHandle();
    }

    assert(!isOpen());
    if (mCreate)
        shm_unlink(mName.c_str());
}

bool SharedMemory::isOpen() const {
    return mFd != invalidHandle();
}

int SharedMemory::openInternal(int oflag, int mode, bool doMapping) {
    if (isOpen()) {
        return EEXIST;
    }

    int err = 0;
    struct stat sb;
    mFd = shm_open(mName.c_str(), oflag, mode);
    if (mFd == -1) {
        err = -errno;
        close();
        return err;
    }

    if (oflag & O_CREAT) {
        if (HANDLE_EINTR(fstat(mFd, &sb)) == -1) {
            err = -errno;
            close();
            return err;
        }

        // Only increase size, as we don't want to yank away memory
        // from another process.
        if (mSize > sb.st_size && HANDLE_EINTR(ftruncate(mFd, mSize)) == -1) {
            err = -errno;
            close();
            return err;
        }
    }

    if (doMapping) {
        int mapFlags = PROT_READ;
        if (oflag & O_RDWR || oflag & O_CREAT) {
            mapFlags |= PROT_WRITE;
        }

        mAddr = mmap(nullptr, mSize, mapFlags, MAP_SHARED, mFd, 0);
        if (mAddr == unmappedMemory()) {
            err = -errno;
            close();
            return err;
        }
    }

    mCreate |= (oflag & O_CREAT);
    assert(isOpen());
    return 0;
}
}  // namespace base
}  // namespace android