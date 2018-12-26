// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "android/base/SubAllocator.h"

#include "android/base/address_space.h"
#include "android/base/Log.h"

#include <iomanip>
#include <sstream>
#include <string>

namespace android {
namespace base {

class SubAllocator::Impl {
public:
    Impl(
        void* _buffer,
        uint64_t _totalSize,
        uint64_t _pageSize) :
        buffer(_buffer),
        totalSize(_totalSize),
        pageSize(_pageSize),
        startAddr((uintptr_t)buffer),
        endAddr(startAddr + totalSize) {

        address_space_allocator_init(
            &addr_alloc,
            totalSize,
            32);
    }

    ~Impl() {
        address_space_allocator_destroy(&addr_alloc);
    }

    void rangeCheck(const char* task, void* ptr) {
        uint64_t addr = (uintptr_t)ptr;
        if (addr < startAddr ||
            addr > endAddr) {
            std::stringstream ss;
            ss << "SubAllocator " << task << ": ";
            ss << "Out of range: " << std::hex << addr << " ";
            ss << "Range: " <<
                std::hex << startAddr << " " <<
                std::hex << endAddr;
            std::string msg = ss.str();
            LOG(FATAL) << msg.c_str();
        }
    }

    uint64_t getOffset(void* checkedPtr) {
        uint64_t addr = (uintptr_t)checkedPtr;
        return addr - startAddr;
    }

    void free(void* ptr) {
        if (!ptr) return;

        rangeCheck("free", ptr);
        address_space_allocator_deallocate(
            &addr_alloc, getOffset(ptr));
    }

    void freeAll() {
        address_space_allocator_reset(&addr_alloc);
    }

    void* alloc(size_t wantedSize) {
        if (wantedSize == 0) return nullptr;

        uint64_t wantedSize64 =
            (uint64_t)wantedSize;

        size_t toPageSize =
            pageSize *
            ((wantedSize + pageSize - 1) / pageSize);

        uint64_t offset =
            address_space_allocator_allocate(
                &addr_alloc, toPageSize);

        if (offset == ANDROID_EMU_ADDRESS_SPACE_BAD_OFFSET) {
            return nullptr;
        }

        return (void*)(uintptr_t)(startAddr + offset);
    }

    void* buffer;
    uint64_t totalSize;
    uint64_t pageSize;
    uint64_t startAddr;
    uint64_t endAddr;
    struct address_space_allocator addr_alloc;
};

SubAllocator::SubAllocator(
    void* buffer,
    uint64_t totalSize,
    uint64_t pageSize) :
    mImpl(
        new SubAllocator::Impl(buffer, totalSize, pageSize)) { }

SubAllocator::~SubAllocator() {
    delete mImpl;
}

void* SubAllocator::alloc(size_t wantedSize) {
    return mImpl->alloc(wantedSize);
}

void SubAllocator::free(void* ptr) {
    mImpl->free(ptr);
}

void SubAllocator::freeAll() {
    mImpl->freeAll();
}

uint64_t SubAllocator::getOffset(void* ptr) {
    return mImpl->getOffset(ptr);
}

} // namespace base
} // namespace android
