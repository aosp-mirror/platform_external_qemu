// Copyright 2016 The Android Open Source Project
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
#include "android/base/Pool.h"

namespace android {
namespace base {

// A Pool consists of:
// - Heaps one for each allocation size
// A Heap consists of:
// - Block(s) for servicing allocation requests with actual memory backing
// A Block consists of:
// - A buffer that is the memory backing
// - Chunks that correspond to usable units of the allocation size
//
// Block implementation:
//
// We want to make it fast to alloc new chunks and to free existing chunks from
// this block, while not having to invalidate all existing pointers when
// allocating new objects.
// I'm p sure by now there's no way to do that withtout
// - significant pre allocation
// - linked list of blocks
// So that means some block chain (hehehehe, pun v much intended) implementation
// Wherein, each block has fast allocation of chunks correponding tot he desired allocation size.
// Any low overhead scheme of that kind,
// like slab alloc or buddy alloc, is fine.
// This one is based on:
//
// Ben Kenwright (b.kenwright@ncl.ac.uk): "Fast Efficient Fixed-Size Memory
// Pool: No Loops and No Overhead" In COMPUTATION TOOLS 2012: The Third
// International Conference on Computational Logics, Algebras, Programming,
// Tools, and Benchmarking

// Make it easy to track all ranges involved so we know which free()
// in which heap to use.
// Assuming this doesn't grow too much past around 100 intervals
// so we dont need to use fancy algorithms to key on it.
struct Interval {
    uintptr_t start;
    uintptr_t end;
};

struct Block {
    Block(size_t _chunkSize, size_t _numChunks) {
        chunkSize = _chunkSize;
        numChunks = _numChunks;

        sizeBytes = chunkSize * numChunks;

        numFree = numChunks;

        storage.resize(sizeBytes);
        data = storage.data();
        nextFree = (uint32_t*)data;
        memset(data, 0x0, sizeBytes);

        firstFreeChunk = 0;
        numAlloced = 0;
    }

    Interval getInterval() const {
        return { (uintptr_t)data, (uintptr_t)(data + sizeBytes) };
    }
    
    bool isFull() const { return numFree == 0; }

    void* alloc() {
        // Lazily constructs the index to the
        // next unallocated chunk.
        if (numAlloced < numChunks) {
            size_t* p =
                (size_t*)
                (data + numAlloced * chunkSize);
            *(size_t*)p = numAlloced + 1;
            ++numAlloced;
        }

        // Returns the next free object,
        // if there is space remaining.
        void* res = nullptr;
        if (numFree) {
            res = (void*)nextFree;
            --numFree;
            if (numFree) {
                // Update nextFree to _point_ at the index
                // of the next free chunk.
                nextFree =
                    data + chunkSize * (*(uint32_t*)nextFree);
            } else {
                // Signal that there are no more
                // chunks available.
                nextFree = nullptr;
            }
        }
        return res;
    }

    void free(void* toFree) {
        if (nextFree) {
            // If there is a chunk available,
            // point the just-freed chunk to that.
            *(uint32_t*)toFree =
                ((uintptr_t)nextFree - (uintptr_t)data) / chunkSize;
            nextFree = toFree;
        } else {
            // If there are no chunks available,
            // point the just-freed chunk to past the end.
            *(uint32_t*)toFree = numChunks;
            nextFree = toFree;
        }
        ++numFree;
    }

    Block* next = nullptr;
    size_t chunkSize = 0;
    size_t numChunks = 0;
    size_t sizeBytes = 0;
    size_t numFree = 0;

    AlignedBuf<uint8_t, 64> storage { 0 };
    uint8_t* data = nullptr;

    size_t firstFreeChunk = 0;
    size_t numAlloced = 0;
    uint32_t* nextFree = 0;
};

class Heap {
public:
    static constexpr size_t kMaxAllocs = 128;
    Heap(size_t allocSize) :
        mAllocSize(allocSize) :
        mBlock(mAllocSize, kMaxAllocs) {
    }

    Interval getInterval() const {
        return mBlock.getInterval();
    }

    bool isFull() const {
        return mBlock.isFull();
    }

    void* alloc() {
        return mBlock.alloc();
    }

    void free(void* ptr) {
        mBlock.free(ptr);
    }

private:
    size_t mAllocSize;
    // Just one block for now
    Block mBlock;
};

class Pool::Impl {
public:
    Impl(size_t minAllocSize, size_t maxFastSize) :
        mMinAllocSize(minAllocSize),
        mMaxFastSize(maxFastSize) {

        mMinAllocLog2 = 0;
        size_t test = 1;
        while (test < minAllocSize) {
            test <<= 1;
            ++mMinAllocLog2;
        }

        // mMinAllocLog2 now known

        size_t n = 1;

        while (n <= maxFastSize / minAllocSize) {
            n <<= 1;
        }

        for (int i = 0; i < n; i++) {
            size_t allocSize = i << minAllocSize;
            Heap* newHeap = new Heap(allocSize);
            HeapInfo newIntervalWithHeap = {
                newHeap->getInterval(),
                allocSize,
                newHeap,
            };
            mHeapInfos.push_back(newIntervalWithHeap);
        }
    }

    void* tryAlloc(size_t wantedSize) {
        if (wantedSize > maxFastSize) return nullptr;

        size_t minAllocSizeNeeded = 
            getMinAllocSizeFor(wantedSize);

        size_t index = minAllocSizeNeeded >> mMinAllocLog2;

        if (mHeapInfos[index].isFull()) return nullptr;

        return mHeapInfos[index].alloc();
    }

    bool free(void* ptr) {
        uintptr_t ptrVal = (uintptr_t)ptr;

        for (auto& info : mHeapInfos) {
            uintptr_t start = info.interval.start;
            uintptr_t end = info.interval.end;

            if (start >= ptrVal && ptrVal < end) {
                info.free(ptr);
                return true;
            }
        }
        return false;
    }

private:
    size_t mMinAllocSize;
    size_t mMinAllocLog2;
    size_t mMaxFastSize;

    size_t getMinAllocSizeFor(size_t wantedSize) {
        size_t sz = 1;
        while (sz < wantedSize) {
            sz <<= 1;
        }
        return sz;
    }

    // No need to get really complex if there are
    // not that many heaps.
    struct HeapInfo {
        Heap* heap;
        size_t allocSize;
        Interval interval;
    };

    std::vector<HeapInfo> mHeapInfos;
};

Pool::Pool(size_t minAllocSize, size_t maxFastSize) :
    mImpl(new Pool::Impl(minAllocSize, maxFastSize)) {
}

Pool::~Pool() {
    delete mImpl;
}

void* Pool::alloc(size_t wantedSize) {
    void* ptr = mImpl->alloc(wantedSize);

    if (ptr) return ptr;

    return malloc(wantedSize);
}

void Pool::free(void* ptr) {
    if (mImpl->free(ptr)) return;

    free(ptr);
}

} // namespace base
} // namespace android
