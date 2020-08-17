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
#include <cassert>
#include <string>

#include "android/base/memory/SharedMemory.h"
#include "android/base/system/Win32UnicodeString.h"

namespace android {
namespace base {

SharedMemory::SharedMemory(StringView name, size_t size, ShareType shareType)
    : mName(std::string(name)), mSize(size), mShareType(shareType) {
    if (mShareType == ShareType::SHARED_MEMORY) {
        mName.insert(0, "SHM_");
    }
}

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
    auto object_name = name.c_str();

    HANDLE sharedFileHandle = invalidHandle();  // use paging file
    if (mShareType == ShareType::FILE_BACKED) {
        sharedFileHandle = CreateFileW(
                object_name, 0,
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        object_name = nullptr;
    }

    HANDLE hMapFile = CreateFileMappingW(
            sharedFileHandle,
            NULL,             // default security
            protection,       // read/write access
            memory.HighPart,  // maximum object size (high-order DWORD)
            memory.LowPart,   // maximum object size (low-order DWORD)
            object_name);     // name of mapping object

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

void SharedMemory::close(bool forceDestroy) {
    if (mAddr != unmappedMemory()) {
        UnmapViewOfFile(mAddr);
        mAddr = unmappedMemory();
    }
    if (mFd) {
        CloseHandle(mFd);
        mFd = invalidHandle();
    }
    if (mShareType == ShareType::FILE_BACKED) {
        const Win32UnicodeString name(mName);
        DeleteFileW(name.c_str());
    }

    assert(!isOpen());
}

bool SharedMemory::isOpen() const {
    return mFd != invalidHandle();
}

}  // namespace base
}  // namespace android
