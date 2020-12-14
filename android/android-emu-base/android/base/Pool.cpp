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
#include "android/base/Pool.h"

#include "android/base/AlignedBuf.h"

#include <vector>

#define DEBUG_POOL 0

#if DEBUG_POOL

#define D(fmt,...) fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__);

#else

#define D(fmt,...)

#endif

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
//
// So that means some block chain (hehehehe, pun v much intended)
// implementation Wherein, each block has fast allocation of chunks
// correponding tot he desired allocation size.
//
// Any low overhead scheme of that kind, like slab alloc or buddy alloc, is
// fine.  This one is based on:
//
// Ben Kenwright (b.kenwright@ncl.ac.uk): "Fast Efficient Fixed-Size Memory
// Pool: No Loops and No Overhead" In COMPUTATION TOOLS 2012: The Third
// International Conference on Computational Logics, Algebras, Programming,
// Tools, and Benchmarking

// Interval:
// Make it easy to track all ranges involved so we know which free()
// in which heap to use.
// Assuming this doesn't grow too much past around 100 intervals
// so we dont need to use fancy algorithms to key on it.
struct Interval {
    uintptr_t start;
    uintptr_t end;
};

static size_t ilog2Floor(size_t n) {
    size_t res = 0;
    size_t test = 1;

    while (test < n) {
        test <<= 1;
        ++res;
    }

    return res;
}

struct Block {
    Block(size_t _chunkSize, size_t _numChunks) {
        if (_chunkSize < sizeof(void*)) {
            fprintf(
                stderr,
                "FATAL: Cannot allocate block with chunk size "
                "less then %zu (wanted: %zu)!\n",
                sizeof(void*),
                _chunkSize);
            abort();
        }

        chunkSize = _chunkSize;
        chunkSizeLog2 = ilog2Floor(chunkSize);
        numChunks = _numChunks;

        D("chunk size %zu log2 %zu numChunks %zu",
          chunkSize,
          chunkSizeLog2,
          numChunks);

        sizeBytes = chunkSize * numChunks;

        storage.resize(sizeBytes);
        data = storage.data();

        numFree = numChunks;
        numAlloced = 0;
        nextFree = (size_t*)data;
    }

    Interval getInterval() const {
        uintptr_t start = (uintptr_t)data;
        uintptr_t end = (uintptr_t)(data + sizeBytes);
        return { start, end };
    }

    bool isFull() const { return numFree == 0; }

    uint8_t* getPtr(size_t element) {
        uint8_t* res =
            data + (uintptr_t)chunkSize *
                      (uintptr_t)element;
        D("got %p element %zu chunkSize %zu",
          res, element, chunkSize);
        return res;
    }

    size_t getElement(void* ptr) {
        uintptr_t ptrVal = (uintptr_t)ptr;
        ptrVal -= (uintptr_t)data;
        return (size_t)(ptrVal >> chunkSizeLog2);
    }

    void* alloc() {
        // Lazily constructs the index to the
        // next unallocated chunk.
        if (numAlloced < numChunks) {
            size_t* nextUnallocPtr =
                (size_t*)getPtr(numAlloced);

            ++numAlloced;
            *nextUnallocPtr = numAlloced;
        }

        // Returns the next free object,
        // if there is space remaining.
        void* res = nullptr;
        if (numFree) {

            D("alloc new ptr @ %p\n", nextFree);

            res = (void*)nextFree;
            --numFree;
            if (numFree) {
                // Update nextFree to _point_ at the index
                // of the next free chunk.
                D("store %zu in %p as next free chunk",
                   *nextFree,
                   getPtr(*nextFree));
                nextFree = (size_t*)getPtr(*nextFree);
            } else {
                // Signal that there are no more
                // chunks available.
                nextFree = nullptr;
            }
        }

        return res;
    }

    void free(void* toFree) {
        size_t* toFreeIndexPtr = (size_t*)toFree;
        if (nextFree) {
            D("freeing %p: still have other chunks available.", toFree);
            D("nextFree: %p (end: %p)", nextFree, getPtr(numChunks));
            D("nextFreeElt: %zu\n", getElement(nextFree));
            // If there is a chunk available,
            // point the just-freed chunk to that.
            *toFreeIndexPtr = getElement(nextFree);
        } else {
            D("freeing free %p: no other chunks available.", toFree);
            // If there are no chunks available,
            // point the just-freed chunk to past the end.
            *(size_t*)toFree = numChunks;
        }
        nextFree = (size_t*)toFree;
        D("set next free to %p", nextFree);
        ++numFree;
    }

    // To free everything, just reset back to the initial state :p
    void freeAll() {
        numFree = numChunks;
        numAlloced = 0;
        nextFree = (size_t*)data;
    }

    Block* next = nullptr; // Unused for now

    size_t chunkSize = 0;
    size_t chunkSizeLog2 = 0;
    size_t numChunks = 0;
    size_t sizeBytes = 0;

    AlignedBuf<uint8_t, 64> storage { 0 };
    uint8_t* data = nullptr;

    size_t numFree = 0;
    size_t numAlloced = 0;

    size_t* nextFree = 0;
};

// Straight passthrough to Block for now unless
// we find it necessary to track more than |kMaxAllocs|
// allocs per heap.
class Heap {
public:
    Heap(size_t allocSize, size_t chunksPerSize) :
        mBlock(allocSize, chunksPerSize) {
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

    void freeAll() {
        mBlock.freeAll();
    }

private:
    // Just one block for now
    Block mBlock;
};

static size_t leastPowerof2(size_t n) {
    size_t res = 1;
    while (res < n) {
        res <<= 1;
    }
    return res;
}

class Pool::Impl {
public:
    Impl(size_t minSize,
         size_t maxSize,
         size_t chunksPerSize) :
        // Need to bump up min alloc size
        // because Blocks use free space for bookkeeping
        // purposes.
        mMinAllocSize(std::max(sizeof(void*), minSize)),
        // Compute mMinAllocLog2.
        // mMinAllocLog2 stores
        // the number of bits to shift
        // in order to obtain the heap index
        // corresponding to a desired allocation size.
        mMinAllocLog2(ilog2Floor(mMinAllocSize)),
        mMaxFastSize(maxSize),
        mChunksPerSize(chunksPerSize) {

        size_t numHeaps =
            1 + ilog2Floor(mMaxFastSize >> mMinAllocLog2);

        for (size_t i = 0; i < numHeaps; i++) {

            size_t allocSize = mMinAllocSize << i;

            D("create heap for size %zu", allocSize);

            Heap* newHeap = new Heap(allocSize, mChunksPerSize);

            HeapInfo info = {
                newHeap,
                allocSize,
                newHeap->getInterval(),
            };

            mHeapInfos.push_back(info);
        }
    }

    ~Impl() {
        for (auto& info : mHeapInfos) {
            delete info.heap;
        }
    }

    void* alloc(size_t wantedSize) {
        if (wantedSize > mMaxFastSize) {
            D("requested size %zu too large", wantedSize);
            return nullptr;
        }

        size_t minAllocSizeNeeded =
            std::max(mMinAllocSize, leastPowerof2(wantedSize));

        size_t index =
            ilog2Floor(minAllocSizeNeeded >> mMinAllocLog2);

        D("wanted: %zu min serviceable: %zu heap index: %zu",
          wantedSize, minAllocSizeNeeded, index);

        auto heap = mHeapInfos[index].heap;

        if (heap->isFull()) {
            D("heap %zu is full", index);
            return nullptr;
        }

        return heap->alloc();
    }

    bool free(void* ptr) {

        D("for %p:", ptr);

        uintptr_t ptrVal = (uintptr_t)ptr;

        // Scan through linearly to find any matching
        // interval. Interval information has been
        // brought up to be stored directly in HeapInfo
        // so this should be quite easy on the cache
        // at least until a match is found.
        for (auto& info : mHeapInfos) {
            uintptr_t start = info.interval.start;
            uintptr_t end = info.interval.end;

            if (ptrVal >= start && ptrVal < end) {
                D("found heap to free %p.", ptr)
                info.heap->free(ptr);
                return true;
            }
        }

        D("%p not found in any heap.", ptr);
        return false;
    }

    void freeAll() {
        for (auto& info : mHeapInfos) {
            info.heap->freeAll();
        }
    }

private:
    size_t mMinAllocSize;
    size_t mMinAllocLog2;
    size_t mMaxFastSize;
    size_t mChunksPerSize;

    // No need to get really complex if there are
    // not that many heaps.
    struct HeapInfo {
        Heap* heap;
        size_t allocSize;
        Interval interval;
    };

    std::vector<HeapInfo> mHeapInfos;
};

Pool::Pool(size_t minSize,
           size_t maxSize,
           size_t mChunksPerSize) :
    mImpl(new Pool::Impl(minSize,
                         maxSize,
                         mChunksPerSize)) {
}

Pool::~Pool() {
    delete mImpl;

    for (auto ptr : mFallbackPtrs) {
        D("delete fallback ptr %p\n", ptr);
        ::free(ptr);
    }
}

// Fall back to normal alloc if it cannot be
// serviced by the implementation.
void* Pool::alloc(size_t wantedSize) {
    void* ptr = mImpl->alloc(wantedSize);

    if (ptr) return ptr;

    D("Fallback to malloc");

    ptr = ::malloc(wantedSize);

    if (!ptr) {
        D("Failed to service allocation for %zu bytes", wantedSize);
        abort();
    }

    mFallbackPtrs.insert(ptr);

    D("Fallback to malloc: got ptr %p", ptr);

    return ptr;
}

// Fall back to normal free if it cannot be
// serviced by the implementation.
void Pool::free(void* ptr) {
    if (mImpl->free(ptr)) return;

    D("fallback to free for %p", ptr);
    mFallbackPtrs.erase(ptr);
    ::free(ptr);
}

void Pool::freeAll() {
    mImpl->freeAll();
    for (auto ptr : mFallbackPtrs) {
        ::free(ptr);
    }
    mFallbackPtrs.clear();
}

} // namespace base
} // namespace android
