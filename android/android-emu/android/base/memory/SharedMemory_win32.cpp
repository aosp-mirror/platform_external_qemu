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

#include <sys/stat.h>
#include <windows.h>

namespace android {
namespace base {

#define UNMAPPED 0

SharedMemory::SharedMemory(std::string name, size_t size)
    : mName(name), mSize(size), mAddr(UNMAPPED), mCreate(false) {
    assert(mName.size() <= NAME_MAX);
}

SharedMemory::SharedMemory(SharedMemory&& other) {
    mName = other.mName;
    mSize = other.mSize;
    mAddr = other.mAddr;
    mFd = other.mFd;
    mCreate = other.mCreate;

    other.mFd = INVALID_HANDLE_VALUE;
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

        other.mFd = INVALID_HANDLE_VALUE;
        other.mAddr = UNMAPPED;
        other.mCreate = false;
    }

    return *this;
}

// Creates a shared region, you will be considered the owner, and will have
// write access. Returns 0 on success, or an error code otheriwse.
int SharedMemory::create(mode_t mode) {
    return open(READ_WRITE);
}

// Opens the shared memory object, returns 0 on success
// or the error code.
int SharedMemory::open(AccessMode access) {
    if (mCreate) {
        return EEXIST;
    }
    DWORD highMem = ((uint64_t) mSize >> 32) & 0xFFFFFFFF;
    DWORD lowMem = mSize & 0xFFFFFFFF;

    // Allows views to be mapped for read-only or copy-on-write access.
    DWORD flProtect = PAGE_READONLY;

    if (access == READ_WRITE) {
        // Allows views to be mapped for read-only, copy-on-write, or read/write
        // access.
        flProtect = PAGE_READWRITE;
    }

    HANDLE hMapFile = CreateFileMapping(
            INVALID_HANDLE_VALUE,  // use paging file
            NULL,                  // default security
            PAGE_READWRITE,        // read/write access
            highMem,               // maximum object size (high-order DWORD)
            lowMem,                // maximum object size (low-order DWORD)
            mName.c_str());        // name of mapping object

    if (hMapFile == NULL) {
        int err = GetLastError();
        close();
        return err;
    }

    flProtect = FILE_MAP_READ;
    if (access == READ_WRITE) {
        flProtect = FILE_MAP_WRITE;
    }

    mAddr = (LPTSTR)MapViewOfFile(hMapFile, flProtect, 0, 0, mSize);

    if (hMapFile == NULL) {
        int err = GetLastError();
        close();
        return err;
    }

    mCreate = true;
    return 0;
}

void SharedMemory::close() {
    if (mAddr != UNMAPPED) {
        UnmapViewOfFile(mAddr);
        mAddr = UNMAPPED;
    }
    if (mFd) {
        CloseHandle(mFd);
        mFd = INVALID_HANDLE_VALUE;
    }

    assert(!isOpen());
}

bool SharedMemory::isOpen() const {
    return mAddr != UNMAPPED;
}
}  // namespace base
}  // namespace android
