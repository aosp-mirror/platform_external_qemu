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
#include "android/base/system/Win32UnicodeString.h"

#include <cassert>
#include <string>

namespace android {
namespace base {

SharedMemory::SharedMemory(StringView name, size_t size)
    : mName(std::string("SHM_") + std::string(name)), mSize(size) { }

// Creates a shared region, you will be considered the owner, and will have
// write access. Returns 0 on success, or an error code otheriwse.
int SharedMemory::create(mode_t mode) {
    return openInternal(AccessMode::READ_WRITE);
}

int SharedMemory::createNoMapping(mode_t mode) {
    return openInternal(AccessMode::READ_WRITE, false /* no mapping */);
}

// Opens the shared memory object, returns 0 on success
// or the error code.
int SharedMemory::open(AccessMode access) {
    return openInternal(access);
}

int SharedMemory::openInternal(AccessMode access, bool doMapping) {
    if (mCreate) {
        return EEXIST;
    }
    LARGE_INTEGER memory;
    memory.QuadPart = (LONGLONG)mSize;

    // Allows views to be mapped for read-only or copy-on-write access.
    DWORD protection = PAGE_READONLY;

    if (access == AccessMode::READ_WRITE) {
        // Allows views to be mapped for read-only, copy-on-write, or read/write
        // access.
        protection = PAGE_READWRITE;
    }
    const Win32UnicodeString name(mName);

    HANDLE hMapFile = CreateFileMappingW(
            invalidHandle(),  // use paging file
            NULL,             // default security
            protection,       // read/write access
            memory.HighPart,  // maximum object size (high-order DWORD)
            memory.LowPart,   // maximum object size (low-order DWORD)
            name.c_str());    // name of mapping object

    if (hMapFile == NULL) {
        int err = -GetLastError();
        close();
        return err;
    }

    if (doMapping) {
        // MapViewOfFile has a slightly different way of naming protection.
        protection = FILE_MAP_READ;
        if (access == AccessMode::READ_WRITE) {
            protection = FILE_MAP_WRITE;
        }

        mAddr = MapViewOfFile(hMapFile, protection, 0, 0, mSize);

        if (mAddr == NULL) {
            int err = -GetLastError();
            close();
            return err;
        }
    }

    mFd = hMapFile;
    mCreate = true;
    return 0;
}

void SharedMemory::close() {
    if (mAddr != unmappedMemory()) {
        UnmapViewOfFile(mAddr);
        mAddr = unmappedMemory();
    }
    if (mFd) {
        CloseHandle(mFd);
        mFd = invalidHandle();
    }

    assert(!isOpen());
}

bool SharedMemory::isOpen() const {
    return mFd != invalidHandle();
}

}  // namespace base
}  // namespace android
