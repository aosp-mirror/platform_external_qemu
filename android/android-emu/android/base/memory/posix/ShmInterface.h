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
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <tuple>


// Policy class that contains the open/close strategy for Posix using shm_open
class ShmInterface {
 public:
    typedef int handle_type;
    typedef void* memory_type;

    static std::tuple<handle_type, memory_type, int> open(std::string handle,
                                                          int oflag,
                                                          mode_t mode,
                                                          uint64_t size) {
        struct stat sb;
        int fd = shm_open(handle.c_str(), oflag, mode);
        if (fd == -1) {
            return std::make_tuple(fd, unmappedMemory(), errno);
        }

        if (fstat(fd, &sb) == -1) {
            return std::make_tuple(fd, unmappedMemory(), errno);
        }

        // Only increase size, as we don't want to yank away memory
        // from another process.
        if (size > sb.st_size && oflag & O_CREAT) {
            // In order to truncate we need write access.
            if (ftruncate(fd, size) == -1) {
                return std::make_tuple(fd, unmappedMemory(), errno);
            }
        }

        int mapFlags = PROT_READ;
        if (oflag & O_RDWR || oflag & O_CREAT) {
            mapFlags |= PROT_WRITE;
        }

        void* addr = mmap(nullptr, size, mapFlags, MAP_SHARED, fd, 0);
        if (addr == unmappedMemory()) {
            return std::make_tuple(fd, unmappedMemory(), errno);
        }

        return std::make_tuple(fd, addr, 0);
    }

    static std::tuple<memory_type, handle_type> close(std::string handle,
                      memory_type addr,
                      handle_type fd,
                      size_t size) {
        if (addr != unmappedMemory()) {
            munmap(addr, size);
            addr = unmappedMemory();
        }
        if (fd) {
            ::close(fd);
            fd = invalidHandle();
        }

        return std::make_tuple(addr, fd);
    }

    static void remove(std::string handle) {
        shm_unlink(handle.c_str());
    }

    constexpr static handle_type invalidHandle() {
        return -1;
    }
    constexpr static memory_type unmappedMemory() {
        return (void*) -1;
    }
};

using NamedSharedMemoryOsPolicy = ShmInterface;
