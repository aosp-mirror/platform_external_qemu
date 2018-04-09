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

#include <cassert>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef _WIN32
  #include <windows.h>
#endif

namespace android {

namespace base {

// SharedMemory - A class to share memory between 2 process.
//
// Usage examples:
// Proc1: The owner
//    std::string message = "Hello world!";
//    SharedMemory writer("foo", 4096);
//    writer.create(0600);
//    memcpy(*writer, message.c_str(), message.size());
//
// Proc2: The observer
//    SharedMemory reader("foo", 4096);
//    reader.open(SharedMemory::AccessMode::READ_ONLY);
//    std::string read((const char*) *reader));
//
// Quirks:
//   It is not possible to find out the size of an in memory shared region on
//   Win32 (boo!). For this reason the size has to be explicitly set in the
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
//   - There is no notion of an owner. The OS will release the region as
//     soon as all references to a region disappear.
//   - The first call to create will determine the size of the region. Regions
//     cannot grow. Multiple calls to create have no effect, and behave like
//     open.
//   - Win32 does not support determining the size of a shared region.
//   - The access control lists (ACL) in the default security descriptor for
//     a file mapping object come from the primary or impersonation token of
//     the creator.
class SharedMemory {
public:
#ifdef _WIN32
    typedef LPTSTR memory_type;
    typedef HANDLE handle_type;
#else
    typedef void* memory_type;
    typedef int   handle_type;
#endif
    enum AccessMode {
        READ_ONLY,
        READ_WRITE
    };

    SharedMemory(std::string name, size_t size);
    SharedMemory(SharedMemory&& other);
    ~SharedMemory();

    // Let's not have any weirdness due to double unlinks due to destructors.
    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;
    SharedMemory& operator=(SharedMemory&& other);

    // Creates a shared region, you will be considered the owner, and will have
    // write access. Returns 0 on success, or an error code otheriwse. The error
    // code (errno) is platform dependent.
    int create(mode_t mode);

    // Opens the shared memory object, returns 0 on success
    // or the error code.
    int open(AccessMode access);
    bool isOpen() const;
    void close();

    size_t size() const { return mSize; }
    std::string name() const { return mName; }
    memory_type get() { return mAddr; }
    memory_type operator*() { return get(); }


private:
    int open(int oflag, int mode);

    std::string mName;
    size_t mSize;
    memory_type mAddr;
    handle_type mFd;
    bool mCreate;
};

// Using directly shared memory, nothing should go to disk.
}  // namespace base
}  // namespace android
