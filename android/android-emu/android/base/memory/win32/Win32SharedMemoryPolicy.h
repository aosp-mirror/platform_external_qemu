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
#include <windows.h>
#include <sys/stat.h>

class Win32MemoryPolicy {
 public:
    typedef HANDLE handle_type;
    typedef LPTSTR memory_type;

    static std::tuple<handle_type, memory_type, int> open(std::string handle,
                                                          int oflag,
                                                          mode_t mode,
                                                          uint64_t size) {

        DWORD highMem = (size >> 32) & 0xFFFFFFFF;
        DWORD lowMem = size & 0xFFFFFFFF;
        HANDLE hMapFile = CreateFileMapping(
                INVALID_HANDLE_VALUE,    // use paging file
                NULL,                    // default security
                PAGE_READWRITE,          // read/write access
                highMem,                 // maximum object size (high-order DWORD)
                lowMem,                  // maximum object size (low-order DWORD)
                handle.c_str());         // name of mapping object

        if (hMapFile == NULL) {
            return std::make_tuple(invalidHandle(), unmappedMemory(), GetLastError());
        }


        // Translate mode..
        DWORD flProtect = 0;
        if (mode & _S_IREAD) {
            flProtect |= FILE_MAP_READ;
        }
        if (mode & _S_IWRITE) {
            flProtect = FILE_MAP_WRITE;
        }

        LPTSTR pBuf = (LPTSTR) MapViewOfFile(hMapFile,
                                      flProtect,
                                      0,
                                      0,
                                      size);

        if (hMapFile == NULL) {
            return std::make_tuple(hMapFile, unmappedMemory(), GetLastError());
        }

        return std::make_tuple(hMapFile, pBuf, 0);
    }

    static std::tuple<memory_type, handle_type> close(std::string handle,
                      memory_type addr,
                      handle_type fd,
                      size_t size) {
        if (addr != unmappedMemory()) {
            UnmapViewOfFile(addr);
            addr = NULL;
        }
        if (fd != INVALID_HANDLE_VALUE) {
            CloseHandle(fd);
            fd = invalidHandle();
        }

        return std::make_tuple(addr, fd);
    }

    // NOP in win32.
    static void remove(std::string handle) { }

    constexpr static handle_type invalidHandle() {
        return INVALID_HANDLE_VALUE;
    }
    constexpr static memory_type unmappedMemory() {
        return (memory_type) 0;
    }
};

using NamedSharedMemoryOsPolicy = Win32MemoryPolicy;

