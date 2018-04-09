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
#include "android/base/memory/SharedMemory.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <tuple>

namespace android {
namespace base {

#define UNMAPPED ((void*)-1)
#define INVALID_HANDLE -1

SharedMemory::SharedMemory(std::string name, size_t size)
    : mName(name), mSize(size), mAddr(UNMAPPED), mCreate(false) {
    assert(mName.size() <= NAME_MAX);  // On mac SHM_NAME_MAX == 30, which is
                                       // not defined in NAME_MAX
}

SharedMemory::SharedMemory(SharedMemory&& other) {
    mName = other.mName;
    mSize = other.mSize;
    mAddr = other.mAddr;
    mFd = other.mFd;
    mCreate = other.mCreate;

    other.mFd = INVALID_HANDLE;
    other.mAddr = UNMAPPED;
}

SharedMemory::~SharedMemory() {
    close();
}

SharedMemory& SharedMemory::operator=(SharedMemory&& other) {
    if (this != &other) {
        mName = other.mName;
        mSize = other.mSize;
        mAddr = other.mAddr;
        mFd = other.mFd;
        mCreate = other.mCreate;

        other.mFd = INVALID_HANDLE;
        other.mAddr = UNMAPPED;
        other.mCreate = false;
    }

    return *this;
}

// Creates a shared region, you will be considered the owner, and will have
// write access. Returns 0 on success, or an error code otheriwse. The error
// code (errno) is platform dependent.
int SharedMemory::create(mode_t mode) {
    return open(O_CREAT | O_RDWR, mode);
}

// Opens the shared memory object, returns 0 on success
// or the error code.
int SharedMemory::open(AccessMode access) {
    int oflag = O_RDONLY;
    int mode = 0400;
    if (access == READ_WRITE) {
        oflag = O_RDWR;
        mode = 0600;
    }
    return open(oflag, mode);
}

void SharedMemory::close() {
    if (mAddr != UNMAPPED) {
        munmap(mAddr, mSize);
        mAddr = UNMAPPED;
    }
    if (mFd) {
        ::close(mFd);
        mFd = INVALID_HANDLE;
    }

    assert(!isOpen());
    if (mCreate)
        shm_unlink(mName.c_str());
}

bool SharedMemory::isOpen() const {
    return mAddr != UNMAPPED;
}

int SharedMemory::open(int oflag, int mode) {
    if (isOpen()) {
        return EEXIST;
    }

    int err = 0;
    struct stat sb;
    mFd = shm_open(mName.c_str(), oflag, mode);
    if (mFd == -1) {
        err = errno;
        close();
        return err;
    }

    if (fstat(mFd, &sb) == -1) {
        err = errno;
        close();
        return err;
    }

    // Only increase size, as we don't want to yank away memory
    // from another process.
    if (mSize > sb.st_size && oflag & O_CREAT) {
        // In order to truncate we need write access.
        if (ftruncate(mFd, mSize) == -1) {
            err = errno;
            close();
            return err;
        }
    }

    int mapFlags = PROT_READ;
    if (oflag & O_RDWR || oflag & O_CREAT) {
        mapFlags |= PROT_WRITE;
    }

    mAddr = mmap(nullptr, mSize, mapFlags, MAP_SHARED, mFd, 0);
    if (mAddr == UNMAPPED) {
        err = errno;
        close();
        return err;
    }

    mCreate |= (oflag & O_CREAT);
    assert(isOpen());
    return 0;
}
}
}  // namespace android
