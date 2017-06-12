// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/snapshot/RamLoader.h"

#include "android/base/ArraySize.h"
#include "android/base/EintrWrapper.h"
#include "android/base/files/pread.h"
#include "android/utils/debug.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <memory>

#include <signal.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/mman.h>

#include <set>

Lock gRamLoaderLock;

namespace android {
namespace snapshot {

hva2gpa_t hva2gpa_call = 0;

void set_hva2gpa_func(hva2gpa_t f) {
    hva2gpa_call = f;
}

struct RamLoader::Page {
    std::atomic<uint8_t> state;
    uint16_t blockIndex;
    uint32_t pageIndex;
    uint64_t filePos;
    uint8_t* data;

    Page() = default;
    Page(Page&& other)
        : state(other.state.load(std::memory_order_relaxed)),
          blockIndex(other.blockIndex),
          pageIndex(other.pageIndex),
          filePos(other.filePos),
          data(other.data) {}
};

std::set<uint64_t> gpasDone;
std::set<void*> hvasDone;

RamLoader::RamLoader(ZeroChecker zeroChecker)
    : mStream(nullptr),
      mZeroChecker(zeroChecker),
      mReaderThread([this]() { ; }) {
    fprintf(stderr, "%s: start\n", __func__);
    if (MemoryAccessWatch::isSupported()) {
        mAccessWatch.emplace(
                [this](void* ptr) {
                AutoLock lock(gRamLoaderLock);
                    // if (hvasDone.find(ptr) != hvasDone.end()) return;
                    // hvasDone.insert(ptr);

                    if (0x21522000 == this->getGuestPhysicalAddress(ptr)) {
                    // fprintf(stderr, "%s; bail\n", __func__);
                    // abort();

                    }
                    // fprintf(stderr, "%s: accesscallback %p\n", __func__, ptr);
                    Page& page = this->page(ptr);
                    uint8_t buf[4096];
                    assert(ARRAY_SIZE(buf) >= pageSize(page));
                    readDataFromDisk(&page, [&buf](int) { return buf; });
                    fillPageData(&page, [&buf](uint8_t* ptr) {
                        if (ptr != &buf[0]) {
                            delete[] ptr;
                        }
                    });

                    // readDataFromDisk(&page, [&page, this](int) { return pagePtr(page); });
                    // fillPageData(&page, [](uint8_t* ptr) { ; });
                    // }
                },
                [this]() { return backgroundPageLoad(); });
    }
}

RamLoader::~RamLoader() {
    interruptReading();
    mAccessWatch.clear();
    mReaderThread.wait();
}

void RamLoader::registerBlock(const RamBlock& block) {
    mIndex.blocks.push_back({block});
}

bool RamLoader::startLoading(base::StdioStream&& stream) {
    mStream = std::move(stream);
    if (!readIndex()) {
        return false;
    }
    mWasStarted = true;
    if (!mAccessWatch) {
        return readAllPages();
    }

    if (!registerPageWatches()) {
        return false;
    }

    mBackgroundPageIt = mIndex.pages.begin();
    mAccessWatch->doneRegistering();
    mReaderThread.start();
    return true;
}

void RamLoader::interruptReading() {
    mReadDataQueue.stop();
    mReadingQueue.stop();
}

void RamLoader::zeroOutPage(const RamBlock& block, uint32_t index) {
    auto ptr = block.hostPtr + (uint64_t)index * block.pageSize;
    if (!mZeroChecker(ptr, block.pageSize)) {
        memset(ptr, 0, block.pageSize);
    }
}

bool RamLoader::readIndex() {
#if SNAPSHOT_PROFILE > 1
    auto start = base::System::get()->getHighResTimeUs();
#endif
    mStreamFd = fileno(mStream.get());
    uint64_t indexPos = mStream.getBe64();
    fseek(mStream.get(), indexPos, SEEK_SET);

    int version = mStream.getBe32();
    if (version != 1) {
        return false;
    }

    int nonzeroPageCount = mStream.getBe32();
    mIndex.pages.reserve(nonzeroPageCount);
    for (size_t loadedBlockCount = 0; loadedBlockCount < mIndex.blocks.size();
         ++loadedBlockCount) {
        const int nameLength = mStream.getByte();
        char name[256];
        mStream.read(name, nameLength);
        name[nameLength] = 0;
        auto blockIt = std::find_if(mIndex.blocks.begin(), mIndex.blocks.end(),
                                    [&name](const FileIndex::Block& b) {
                                        return b.ramBlock.id == name;
                                    });
        if (blockIt == mIndex.blocks.end()) {
            return false;
        }
        FileIndex::Block& block = *blockIt;

        if (uint32_t zeroPagesCount = mStream.getBe32()) {
            uint32_t index = mStream.getBe32();
            zeroOutPage(block.ramBlock, index);
            for (uint32_t j = 1; j != zeroPagesCount; ++j) {
                auto diff = mStream.getPackedNum();
                index += diff;
                zeroOutPage(block.ramBlock, index);
            }
        }

        block.pagesBegin = mIndex.pages.end();
        if (uint32_t blockPagesCount = mStream.getBe32()) {
            const auto blockIndex = &block - mIndex.blocks.data();
            mIndex.pages.emplace_back();
            Page& page = mIndex.pages.back();
            page.blockIndex = blockIndex;
            page.state = uint8_t(State::Empty);
            uint32_t index = page.pageIndex = mStream.getBe32();
            uint64_t pos = page.filePos = mStream.getBe64();
            for (uint32_t i = 1; i != blockPagesCount; ++i) {
                const auto indexDiff = mStream.getPackedNum();
                const auto posDiff = mStream.getPackedNum();
                index += indexDiff;
                pos += posDiff * block.ramBlock.pageSize;

                mIndex.pages.emplace_back();
                Page& page = mIndex.pages.back();
                page.blockIndex = blockIndex;
                page.state = uint8_t(State::Empty);
                page.pageIndex = index;
                page.filePos = pos;
            }
        }
        block.pagesEnd = mIndex.pages.end();
    }

#if SNAPSHOT_PROFILE > 1
    printf("readIndex() time: %.03f\n",
           (base::System::get()->getHighResTimeUs() - start) / 1000.0);
#endif
    return true;
}

bool RamLoader::registerPageWatches() {
    uint8_t* startPtr = nullptr;
    Page* lastPage = nullptr;
    uint32_t curSize = 0;
    uint64_t startgpa = 0;
    bool lastFound = false;

    for (Page& page : mIndex.pages) {
        auto ptr = pagePtr(page);
        auto size = pageSize(page);
        if (false) {
            curSize += size;
        } else {
            if (startPtr) {
                // bool found; uint64_t gpa = hva2gpa_call(startPtr, &found);
                // if (!found || gpa >= 0x60000000) {
                // if (!found || gpa >= 0x10000000) {
                // if (!found || (gpa >= 0x21400000 && gpa <= 0x21800000)) {
                // if (true) {
                // if (!lastFound || !startgpa || startgpa < 0x21400000 || startgpa >= 0x21523000) {
                if (!lastFound || !startgpa || startgpa < 0x10000000 || startgpa >= 0x30000000) {
                //if (true) {
                    readDataFromDisk(lastPage, [lastPage, this](int) { return pagePtr(*lastPage); });
                    // mAccessWatch->fillPage(pagePtr(page), curSize, page.data);
                } else {
                    if (!mAccessWatch->registerMemoryRange(startPtr, curSize, startgpa, lastFound)) {
                        return false;
                    }
                }
            }
            startPtr = ptr;
            curSize = size;
            lastPage = &page;
            startgpa = hva2gpa_call(startPtr, &lastFound);
            uint64_t othergpa = getGuestPhysicalAddress(startPtr);
            // if (othergpa == startgpa) {
            //     fprintf(stderr, "%s: yay, hva %p gpa matches for 0x%llx\n", __func__, startPtr, startgpa);
            // } else {
            //     fprintf(stderr, "%s: oops, hva %p gpas dont match: 0x%llx 0x%llx\n", __func__, startPtr, startgpa, othergpa);
            // }
            startgpa = othergpa;
            lastFound = true;
        }
    }
    if (startPtr) {
        // bool found; uint64_t gpa = hva2gpa_call(startPtr, &found);
        // if (!found || gpa >= 0x600000000) {
        // if (true) {
            readDataFromDisk(lastPage, [lastPage, this](int) { return pagePtr(*lastPage); });
            // mAccessWatch->fillPage(pagePtr(*lastPage), curSize, lastPage->data);
        // } else {
            // if (!mAccessWatch->registerMemoryRange(startPtr, curSize, startgpa, lastFound)) {
            //     return false;
            // }
        // }
    }
    return true;
}

uint8_t* RamLoader::pagePtr(const RamLoader::Page& page) const {
    const RamBlock& block = mIndex.blocks[page.blockIndex].ramBlock;
    return block.hostPtr + (uint64_t)page.pageIndex * block.pageSize;
}

uint32_t RamLoader::pageSize(const RamLoader::Page& page) const {
    return mIndex.blocks[page.blockIndex].ramBlock.pageSize;
}

uint64_t RamLoader::getGuestPhysicalAddress(void* ptr) {
    const auto blockIt = std::find_if(
            mIndex.blocks.begin(), mIndex.blocks.end(),
            [ptr](const FileIndex::Block& b) {
                return ptr >= b.ramBlock.hostPtr &&
                       ptr < b.ramBlock.hostPtr + b.ramBlock.totalSize;
            });
    assert(blockIt != mIndex.blocks.end());
    assert(ptr >= blockIt->ramBlock.hostPtr);
    assert(ptr < blockIt->ramBlock.hostPtr + blockIt->ramBlock.totalSize);
    assert(blockIt->pagesBegin != blockIt->pagesEnd);

    uintptr_t hostBlockOff =
        ((uintptr_t)ptr - (uintptr_t)(blockIt->ramBlock.hostPtr));
    uint64_t res = blockIt->ramBlock.guestPhysicalAddress + hostBlockOff;
    uint64_t off = blockIt->ramBlock.startOffset;
    // fprintf(stderr, "%s: hva %p gpa 0x%llx off 0x%llx\n", __func__, ptr, res, off);
    return res;
}

void* RamLoader::getHostRamAddrFromGuestPhysical(uint64_t ptr) {
    const auto blockIt = std::find_if(
            mIndex.blocks.begin(), mIndex.blocks.end(),
            [ptr](const FileIndex::Block& b) {
                return ptr >= b.ramBlock.guestPhysicalAddress &&
                       ptr < b.ramBlock.guestPhysicalAddress + b.ramBlock.totalSize;
            });
    assert(blockIt != mIndex.blocks.end());
    assert(ptr >= blockIt->ramBlock.guestPhysicalAddress);
    assert(ptr < blockIt->ramBlock.guestPhysicalAddress + blockIt->ramBlock.totalSize);
    assert(blockIt->pagesBegin != blockIt->pagesEnd);

    if (blockIt == mIndex.blocks.end()) {
        fprintf(stderr, "%s: fail\n", __func__);
    }

    uintptr_t hostBlockOff =
        ((uintptr_t)ptr - (uintptr_t)(blockIt->ramBlock.guestPhysicalAddress));
    void* res = (void*)(blockIt->ramBlock.hostPtr + hostBlockOff);
    // fprintf(stderr, "%s: gpa 0x%llx hva %p\n", __func__, ptr, res);
    return res;
}

RamLoader::Page& RamLoader::page(void* ptr) {
    const auto blockIt = std::find_if(
            mIndex.blocks.begin(), mIndex.blocks.end(),
            [ptr](const FileIndex::Block& b) {
                return ptr >= b.ramBlock.hostPtr &&
                       ptr < b.ramBlock.hostPtr + b.ramBlock.totalSize;
            });
    assert(blockIt != mIndex.blocks.end());
    assert(ptr >= blockIt->ramBlock.hostPtr);
    assert(ptr < blockIt->ramBlock.hostPtr + blockIt->ramBlock.totalSize);
    assert(blockIt->pagesBegin != blockIt->pagesEnd);

    const auto it =
            std::upper_bound(blockIt->pagesBegin, blockIt->pagesEnd, ptr,
                             [this](const void* ptr, const Page& page) {
                                 return ptr < pagePtr(page) + pageSize(page);
                             });
    assert(it != blockIt->pagesEnd);
    assert(ptr >= pagePtr(*it));
    assert(ptr < pagePtr(*it) + pageSize(*it));
    return *it;
}

RamLoader::Page& RamLoader::pageFromGpa(uint64_t ptr) {
    return this->page(this->getHostRamAddrFromGuestPhysical(ptr));
}

void RamLoader::readerWorker() {
    while (auto pagePtr = mReadingQueue.receive()) {
        Page* page = *pagePtr;
        if (!page) {
            mReadDataQueue.send(nullptr);
            mReadingQueue.stop();
            break;
        }

        std::unique_ptr<uint8_t[]> bufDeleter;
        if (readDataFromDisk(page, [&bufDeleter](int size) {
                bufDeleter.reset(new uint8_t[size]);
                return bufDeleter.get();
            })) {
            bufDeleter.release();
            mReadDataQueue.send(page);
        }
    }
}

MemoryAccessWatch::IdleCallbackResult RamLoader::backgroundPageLoad() {
    if (mReadingQueue.isStopped() && mReadDataQueue.isStopped()) {
        return MemoryAccessWatch::IdleCallbackResult::AllDone;
    }

    Page* page = nullptr;
    if (mReadDataQueue.tryReceive(&page)) {
        if (page) {
            fillPageData(page, [](uint8_t* ptr) { delete[] ptr; });
            // If we've loaded a page then this function took quite a while
            // and it's better to check for a pagefault before proceeding to
            // queuing pages to the reader thread.
            return MemoryAccessWatch::IdleCallbackResult::RunAgain;
        } else {
            // null page == all pages were loaded, let's stop.
            mReadDataQueue.stop();
            mReadingQueue.stop();
            return MemoryAccessWatch::IdleCallbackResult::AllDone;
        }
    }

    for (size_t i = 0; i < mReadingQueue.capacity(); ++i) {
        // Find next page to queue.
        mBackgroundPageIt = std::find_if(
                mBackgroundPageIt, mIndex.pages.end(), [](const Page& page) {
                    return page.state.load(std::memory_order_relaxed) ==
                           uint8_t(State::Empty);
                });

        if (mBackgroundPageIt == mIndex.pages.end()) {
            if (!mSentEndOfPagesMarker) {
                mSentEndOfPagesMarker = mReadingQueue.trySend(nullptr);
            }
            return MemoryAccessWatch::IdleCallbackResult::Wait;
        }

        if (mReadingQueue.trySend(&*mBackgroundPageIt)) {
            ++mBackgroundPageIt;
        } else {
            // The queue is full - let's wait for a while to give the reader
            // time to empty it.
            return MemoryAccessWatch::IdleCallbackResult::Wait;
        }
    }

    return MemoryAccessWatch::IdleCallbackResult::RunAgain;
}

void RamLoader::readPageAtPtr(void* ptr) {
    Page& p = this->page(ptr);
    if (readDataFromDisk(&p, [&p, this](int) { return pagePtr(p); })) {
        bool found;
        uint64_t gpa = hva2gpa_call(ptr, &found);
        if (found) {
            mAccessWatch->fillPage(pagePtr(p), pageSize(p), p.data, gpa);
        }
    }
}


void RamLoader::readAtGpa(uint64_t len, uint64_t gpa) {
    if (gpasDone.find(gpa) != gpasDone.end()) { return; }
    gpasDone.insert(gpa);
    Page& p = this->pageFromGpa(gpa);
    mprotect(pagePtr(p), 4096, PROT_READ | PROT_WRITE | PROT_EXEC);

                    uint8_t buf[4096];
                    readDataFromDisk(&p, [&buf](int) { return buf; });
                    fillPageData(&p, [&buf](uint8_t* ptr) {
                    });
}

template <class BufferGetter>
bool RamLoader::readDataFromDisk(Page* pagePtr, BufferGetter&& bufGetter) {
    // fprintf(stderr, "%s: call\n", __func__);
    Page& page = *pagePtr;
    auto state = uint8_t(State::Empty);
    if (!page.state.compare_exchange_strong(state, (uint8_t)State::Reading)) {
        // Spin until the reading thread finishes.
        while (state < uint8_t(State::Read)) {
            state = uint8_t(page.state.load(std::memory_order_relaxed));
        }
        return false;
    }

    auto size = pageSize(page);
    uint8_t* buf = bufGetter(size);
    auto read = HANDLE_EINTR(base::pread(mStreamFd, buf, size, page.filePos));
    if (read != (int64_t)size) {
        derror("Reading page %p from disk returned less data: %d of %d\n",
               this->pagePtr(page), (int)read, (int)size);
        page.state.store(uint8_t(State::Error));
        abort();
        return false;
    }

    page.data = buf;
    page.state.store(uint8_t(State::Read), std::memory_order_release);
    return true;
}

template <class DataDeleter>
void RamLoader::fillPageData(Page* pagePtr, DataDeleter&& deleter) {
    Page& page = *pagePtr;
    auto state = uint8_t(State::Read);
    if (!page.state.compare_exchange_strong(state, uint8_t(State::Filling))) {
        assert(state == uint8_t(State::Filled));
        assert(page.data == nullptr);
        return;
    }

#if SNAPSHOT_PROFILE > 2
    printf("%s: loading page %p\n", __func__, this->pagePtr(page));
#endif
    void* hostptr = this->pagePtr(page);
    uint64_t gpa = this->getGuestPhysicalAddress(hostptr);
    if (mAccessWatch) {
        bool res = mAccessWatch->fillPage(this->pagePtr(page), pageSize(page),
                                          page.data, gpa);
        deleter(page.data);
        page.data = nullptr;
        page.state.store(uint8_t(res ? State::Filled : State::Error));
    }
}

bool RamLoader::readAllPages() {
    fprintf(stderr, "%s: start\n", __func__);
    for (Page& page : mIndex.pages) {
        if (!readDataFromDisk(&page,
                              [&page, this](int) { return pagePtr(page); })) {
            return false;
        }
    }
    return true;
}

}  // namespace snapshot
}  // namespace android
