//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <cassert>


#include "aemu/base/EintrWrapper.h"
#include "aemu/base/memory/SharedMemory.h"
#ifndef _MSC_VER
#include <unistd.h>
#endif

namespace android {
namespace base {

SharedMemory::SharedMemory(const std::string& uriOrHandle, size_t size) : mSize(size) {
    constexpr std::string_view kFileUri = "file://";
    if (uriOrHandle.find(kFileUri, 0) == 0) {
        mShareType = ShareType::FILE_BACKED;
        mName = uriOrHandle.substr(kFileUri.size());
    } else {
        mShareType = ShareType::SHARED_MEMORY;
        mName = uriOrHandle;
    }
}

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

void SharedMemory::close(bool forceDestroy) {
    if (mAddr != unmappedMemory()) {
        munmap(mAddr, mSize);
        mAddr = unmappedMemory();
    }
    if (mFd) {
        ::close(mFd);
        mFd = invalidHandle();
    }

    assert(!isOpen());
    if (forceDestroy || mCreate) {
        if (mShareType == ShareType::FILE_BACKED) {
            remove(mName.c_str());
        } else {
#if !defined(__BIONIC__)
            shm_unlink(mName.c_str());
#endif
        }
    }
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
    if (mShareType == ShareType::SHARED_MEMORY) {
#if !defined(__BIONIC__)
        mFd = shm_open(mName.c_str(), oflag, mode);
#else
        return ENOTTY;
#endif
    } else {
        mFd = ::open(mName.c_str(), oflag, mode);
        // Make sure the file can hold at least mSize bytes..
        struct stat stat;
        if (!fstat(mFd, &stat) && stat.st_size < mSize) {
            ftruncate(mFd, mSize);
        }
    }
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
