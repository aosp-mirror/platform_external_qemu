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
#include "aemu/base/SubAllocator.h"

#include "aemu/base/address_space.h"
#include "aemu/base/files/Stream.h"
#include "aemu/base/Log.h"

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
        address_space_allocator_destroy_nocleanup(&addr_alloc);
    }

    void clear() {
        address_space_allocator_destroy_nocleanup(&addr_alloc);
        address_space_allocator_init(
            &addr_alloc,
            totalSize,
            32);
    }

    bool save(Stream* stream) {
        address_space_allocator_iter_func_t allocatorSaver =
            [](void* context, struct address_space_allocator* allocator) {
                Stream* stream = reinterpret_cast<Stream*>(context);
                stream->putBe32(allocator->size);
                stream->putBe32(allocator->capacity);
                stream->putBe64(allocator->total_bytes);
            };
        address_block_iter_func_t allocatorBlockSaver =
            [](void* context, struct address_block* block) {
                Stream* stream = reinterpret_cast<Stream*>(context);
                stream->putBe64(block->offset);
                stream->putBe64(block->size_available);
            };
        address_space_allocator_run(
            &addr_alloc,
            (void*)stream,
            allocatorSaver,
            allocatorBlockSaver);

        stream->putBe64(pageSize);
        stream->putBe64(totalSize);
        stream->putBe32(allocCount);

        return true;
    }

    bool load(Stream* stream) {
        clear();
        address_space_allocator_iter_func_t allocatorLoader =
            [](void* context, struct address_space_allocator* allocator) {
                Stream* stream = reinterpret_cast<Stream*>(context);
                allocator->size = stream->getBe32();
                allocator->capacity = stream->getBe32();
                allocator->total_bytes = stream->getBe64();
            };
        address_block_iter_func_t allocatorBlockLoader =
            [](void* context, struct address_block* block) {
                Stream* stream = reinterpret_cast<Stream*>(context);
                block->offset = stream->getBe64();
                block->size_available = stream->getBe64();
            };
        address_space_allocator_run(
            &addr_alloc,
            (void*)stream,
            allocatorLoader,
            allocatorBlockLoader);

        pageSize = stream->getBe64();
        totalSize = stream->getBe64();
        allocCount = stream->getBe32();

        return true;
    }

    bool postLoad(void* postLoadBuffer) {
        buffer = postLoadBuffer;
        startAddr =
            (uint64_t)(uintptr_t)postLoadBuffer;
        return true;
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

    bool free(void* ptr) {
        if (!ptr) return false;

        rangeCheck("free", ptr);
        if (EINVAL == address_space_allocator_deallocate(
            &addr_alloc, getOffset(ptr))) {
            return false;
        }

        --allocCount;
        return true;
    }

    void freeAll() {
        address_space_allocator_reset(&addr_alloc);
        allocCount = 0;
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

        ++allocCount;
        return (void*)(uintptr_t)(startAddr + offset);
    }

    void* allocFixed(size_t wantedSize, uint64_t offset) {

        if (wantedSize == 0) return nullptr;

        uint64_t wantedSize64 =
            (uint64_t)wantedSize;

        size_t toPageSize =
            pageSize *
            ((wantedSize + pageSize - 1) / pageSize);

        if (address_space_allocator_allocate_fixed(
                &addr_alloc, toPageSize, offset)) {
            return nullptr;
        }

        ++allocCount;
        return (void*)(uintptr_t)(startAddr + offset);
    }

    bool empty() const {
        return allocCount == 0;
    }

    void* buffer;
    uint64_t totalSize;
    uint64_t pageSize;
    uint64_t startAddr;
    uint64_t endAddr;
    struct address_space_allocator addr_alloc;
    uint32_t allocCount = 0;
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

// Snapshotting
bool SubAllocator::save(Stream* stream) {
    return mImpl->save(stream);
}

bool SubAllocator::load(Stream* stream) {
    return mImpl->load(stream);
}

bool SubAllocator::postLoad(void* postLoadBuffer) {
    return mImpl->postLoad(postLoadBuffer);
}

void* SubAllocator::alloc(size_t wantedSize) {
    return mImpl->alloc(wantedSize);
}

void* SubAllocator::allocFixed(size_t wantedSize, uint64_t offset) {
    return mImpl->allocFixed(wantedSize, offset);
}

bool SubAllocator::free(void* ptr) {
    return mImpl->free(ptr);
}

void SubAllocator::freeAll() {
    mImpl->freeAll();
}

uint64_t SubAllocator::getOffset(void* ptr) {
    return mImpl->getOffset(ptr);
}

bool SubAllocator::empty() const {
    return mImpl->empty();
}

} // namespace base
} // namespace android
