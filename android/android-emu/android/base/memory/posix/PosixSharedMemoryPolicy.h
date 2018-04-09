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
#include <sys/stat.h> /* For mode constants */
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <tuple>


class PosixMemoryPolicy {
 public:
    typedef int handle_type;
    typedef void* memory_type;

    static std::tuple<handle_type, memory_type, int> open(std::string handle,
                                                          int oflag,
                                                          mode_t mode,
                                                          uint64_t size) {
        int fd = shm_open(handle.c_str(), oflag, mode);
        if (fd == -1) {
            return std::make_tuple(fd, unmappedMemory(), errno);
        }

        if (ftruncate(fd, size) == -1) {
            return std::make_tuple(fd,unmappedMemory(), errno);
        }

        void* addr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
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
            shm_unlink(handle.c_str());
            fd = invalidHandle();
        }

        return std::make_tuple(addr, fd);
    }

    constexpr static handle_type invalidHandle() {
        return -1;
    }
    constexpr static memory_type unmappedMemory() {
        return (void*) -1;
    }
};

using NamedSharedMemoryOsPolicy = PosixMemoryPolicy;
