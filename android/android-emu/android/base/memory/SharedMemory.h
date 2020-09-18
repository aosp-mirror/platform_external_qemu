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

#include "android/base/StringView.h"

#ifdef _WIN32
#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <windows.h>
#endif  // _MSC_VER
#endif  // _WIN32

using android::base::StringView;

namespace android {

namespace base {

// SharedMemory - A class to share memory between 2 process. The region can
// be shared in 2 modes:
//
// - As shared memory (/dev/shm, memory page file in windows)
// - Using memory mapped files backed by a file on the file system.
//
// To use memory mapped files prefix the handle with "file://" followed
// by an absolute path. For example: file:///c:/WINDOWS/shared.mem or
// file:///tmp/shared.mem
//
// Usage examples:
// Proc1: The owner
//    StringView message = "Hello world!";
//    SharedMemory writer("foo", 4096);
//    writer.create(0600);
//    memcpy(*writer, message.c_str(), message.size());
//
// Proc2: The observer
//    SharedMemory reader("foo", 4096);
//    reader.open(SharedMemory::AccessMode::READ_ONLY);
//    StringView read((const char*) *reader));
//
// Using file backed ram:
//
// Proc1: The owner
//    StringView message = "Hello world!";
//    SharedMemory writer("file:///abs/path/to/a/file", 4096);
//    writer.create(0600);
//    memcpy(*writer, message.c_str(), message.size());
//
// Proc2: The observer
//    SharedMemory reader("file::///abs/path/to/a/file", 4096);
//    reader.open(SharedMemory::AccessMode::READ_ONLY);
//    StringView read((const char*) *reader));
//
//   It is not possible to find out the size of an in memory shared region on
//   Win32 (boo!), there are undocumented workaround (See:
//   https://stackoverflow.com/questions/34860452/extracting-shared-memorys-size/47951175#47951175)
//   that we are not using.
//
//   For this reason the size has to be explicitly set in the
//   constructor. As a workaround you could write the region size in the first
//   few bytes of the region, or use a different channel to exchange region
//   size.
//
//   Shared memory behaves differently on Win32 vs Posix. You as a user must be
//   very well aware of these differences, or run into unexpected results on
//   different platforms:
//
// Posix (linux/macos):
//  - There is a notion of an OWNER of a SharedMemory region. The one to call
//    create will own the region. If this object goes out of scope the region
//    will be unlinked, meaning that mappings (calls to open) will fail. As
//    soon as all other references to the shared memory go away the handle will
//    disappear from /dev/shm as well.
//  - Multiple calls to create for the same region can cause undefined behavior
//    due to closing and potential resizing of the shared memory.
//  - Shared memory can outlive processes that are using it. So don't crash
//    while a shared object is still alive.
//  - Access control is observed by mode permissions
// Mac:
//  - The name cannot exceed 30 chars.
// Win32:
//   - Names are prefixed with SHM_ to prevent collision with other objects
//     in windows;
//   - There is no notion of an owner. The OS will release the region as
//     soon as all references to a region disappear.
//   - The first call to create will determine the size of the region.
//     According to msdn regions cannot grow. Multiple calls to create
//     have no effect, and behave like open. (Note, you can grow size according
//     to https://blogs.msdn.microsoft.com/oldnewthing/20150130-00/?p=44793)
//   - Win32 does not officially support determining the size of a shared
//     region.
//   - The access control lists (ACL) in the default security descriptor for
//     a file mapping object come from the primary or impersonation token of
//     the creator.
//
// If the shared memory region is backed by a file you must keep the following
// things in mind:
//
// Win32:
//  - If an application specifies a size for the file mapping object that
//    is larger than the size of the actual named file on disk and if the page
//    protection allows write access (that is, the flProtect parameter specifies
//    PAGE_READWRITE or PAGE_EXECUTE_READWRITE), then the file on disk is
//    increased to match the specified size of the file mapping object. If the
//    file is extended, the contents of the file between the old end of the file
//    and the new end of the file are not guaranteed to be zero; the behavior is
//    defined by the file system. If the file on disk cannot be increased,
//    CreateFileMapping fails and GetLastError returns ERROR_DISK_FULL.
//    In practice this means that the shared memory region will not contain
//    useful data upon creation.
//  - A sharing object will be created for each view on the file.
//  - The memory mapped file will be deleted by the owner upond destruction.
//    this is however not guaranteed!
// Posix:
//  - The notion of ownership is handled at the filesystem level. Usually this
//    means that a memory mapped file will be deleted once all references have
//    been removed.
//  - The memory mapped file will be deleted by the owner upond destruction.
//    this is however not guaranteed!
class SharedMemory {
public:
#ifdef _WIN32
    using memory_type = void*;
    using handle_type = HANDLE;
    constexpr static handle_type invalidHandle() {
        // This is the invalid return value for
        // CreateFileMappingW; INVALID_HANDLE_VALUE
        // could mean the paging file on Windows.
        return NULL;
    }
    constexpr static memory_type unmappedMemory() { return nullptr; }
#else
    using memory_type = void*;
    using handle_type = int;
    constexpr static handle_type invalidHandle() { return -1; }
    static memory_type unmappedMemory() {
        return reinterpret_cast<memory_type>(-1);
    }
#endif
    enum class AccessMode { READ_ONLY, READ_WRITE };
    enum class ShareType { SHARED_MEMORY, FILE_BACKED };

    // Creates a SharedMemory region either backed by a shared memory handle
    // or by a file. If the string uriOrHandle starts with `file://` it will be
    // file backed otherwise it will be a named shared memory region.
    // |uriHandle| A file:// uri or handle
    // |size| Size of the desired shared memory region. Cannot change after
    // creation.
    SharedMemory(const StringView uriOrHandle, size_t size);
    ~SharedMemory() { close(); }

    SharedMemory(SharedMemory&& other) {
        mName = std::move(other.mName);
        mSize = other.mSize;
        mAddr = other.mAddr;
        mFd = other.mFd;
        mCreate = other.mCreate;
        mShareType = other.mShareType;
        other.clear();
    }

    SharedMemory& operator=(SharedMemory&& other) {
        mName = std::move(other.mName);
        mSize = other.mSize;
        mAddr = other.mAddr;
        mShareType = other.mShareType;
        mFd = other.mFd;
        mCreate = other.mCreate;

        other.clear();
        return *this;
    }

    // Let's not have any weirdness due to double unlinks due to destructors.
    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;

    // Creates a shared region, you will be considered the owner, and will have
    // write access. Returns 0 on success, or an negative error code otheriwse.
    // The error code (errno) is platform dependent.
    int create(mode_t mode);
    // Creates a shared object in the same manner as create(), except for
    // performing actual mapping.
    int createNoMapping(mode_t mode);

    // Opens the shared memory object, returns 0 on success
    // or the negative error code.
    // The shared memory object will be mapped.
    int open(AccessMode access);

    bool isOpen() const;
    void close(bool forceDestroy = false);

    size_t size() const { return mSize; }
    StringView name() const { return mName; }
    memory_type get() const { return mAddr; }
    memory_type operator*() const { return get(); }

    handle_type getFd() { return mFd; }
    bool isMapped() const { return mAddr != unmappedMemory(); }

private:
#ifdef _WIN32
    int openInternal(AccessMode access, bool doMapping = true);
#else
    int openInternal(int oflag, int mode, bool doMapping = true);
#endif

    void clear() {
        mSize = 0;
        mName = "";
        mCreate = false;
        mFd = invalidHandle();
        mAddr = unmappedMemory();
    }

    memory_type mAddr = unmappedMemory();
    handle_type mFd = invalidHandle();
    handle_type mFile = invalidHandle();
    bool mCreate = false;

    std::string mName;
    size_t mSize;
    ShareType mShareType;
};
}  // namespace base
}  // namespace android
