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

#include <cassert>
#include <string>

#include "android/base/files/PathUtils.h"
#include "android/base/system/Win32UnicodeString.h"

namespace android {
namespace base {

SharedMemory::SharedMemory(StringView name, size_t size) : mSize(size) {
    constexpr StringView kFileUri = "file://";
    if (name.find(kFileUri, 0) == 0) {
        mShareType = ShareType::FILE_BACKED;
        auto path = name.substr(kFileUri.size());
        mName = PathUtils::recompose(PathUtils::decompose(path));
    } else {
        mShareType = ShareType::SHARED_MEMORY;
        mName = "SHM_" + name.str();
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
    auto objectName = name.c_str();

    if (mShareType == ShareType::FILE_BACKED) {
        auto fileOptions = GENERIC_READ;
        // File sharing permissions must be the same for readers/writers.
        // otherwise readers will not be able to open the file.
        auto fileShare = FILE_SHARE_READ | FILE_SHARE_WRITE;
        auto openMode = OPEN_EXISTING;
        if (access == AccessMode::READ_WRITE) {
            fileOptions = GENERIC_READ | GENERIC_WRITE;
            openMode = OPEN_ALWAYS;
        }
        mFile = CreateFileW(objectName, fileOptions, fileShare, NULL, openMode,
                            FILE_FLAG_RANDOM_ACCESS, NULL);
        if (mFile == INVALID_HANDLE_VALUE) {
            int err = -GetLastError();
            close();
            return err;
        }
        objectName = nullptr;
    }

    HANDLE hMapFile = CreateFileMappingW(
            mFile,
            NULL,             // default security
            protection,       // read/write access
            memory.HighPart,  // maximum object size (high-order DWORD)
            memory.LowPart,   // maximum object size (low-order DWORD)
            objectName);      // name of mapping object

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
    if (mFile) {
        CloseHandle(mFile);
        mFile = invalidHandle();
        if ((forceDestroy || mCreate) && mShareType == ShareType::FILE_BACKED) {
            const Win32UnicodeString name(mName);
            DeleteFileW(name.c_str());
        }
    }

    assert(!isOpen());
}

bool SharedMemory::isOpen() const {
    return mFd != invalidHandle();
}

}  // namespace base
}  // namespace android
