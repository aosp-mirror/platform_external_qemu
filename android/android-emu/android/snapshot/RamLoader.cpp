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

#include "android/avd/info.h"
#include "android/base/ArraySize.h"
#include "android/base/EintrWrapper.h"
#include "android/base/files/MemStream.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/preadwrite.h"
#include "android/snapshot/Compressor.h"
#include "android/snapshot/Decompressor.h"
#include "android/snapshot/Snapshot.h"
#include "android/utils/debug.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <memory>

using android::base::MemStream;

namespace android {
namespace snapshot {

void RamLoader::FileIndex::clear() {
    decltype(pages)().swap(pages);
    decltype(blocks)().swap(blocks);
}

const RamLoader::Page* RamLoader::FileIndex::findPage(int blockIdx,
                                                      int pageIdx) const {
    if (blockIdx < 0 || blockIdx >= blocks.size()) {
        return nullptr;
    }
    const auto& block = blocks[blockIdx];
    if (pageIdx < 0 || pageIdx > block.pagesEnd - block.pagesBegin) {
        return nullptr;
    }
    return &*(block.pagesBegin + pageIdx);
}

RamLoader::RamLoader(const Snapshot& snapshot, OperationFlags flags)
    : mStreams(snapshot.hierarchy().size()),
      mStreamFds(mStreams.size()),
      mReaderThread([this]() { readerWorker(); }),
      mFlags(flags),
      mSnapshot(snapshot) {
    if (MemoryAccessWatch::isSupported()) {
        mAccessWatch.emplace([this](void* ptr) { loadRamPage(ptr); },
                             [this]() { return backgroundPageLoad(); });
        if (mAccessWatch->valid()) {
            mOnDemandEnabled = true;
        } else {
            derror("Failed to initialize memory access watcher, falling back "
                   "to synchronous RAM loading");
            mAccessWatch.clear();
        }
    }
}

RamLoader::~RamLoader() {
    if (mWasStarted) {
        interruptReading();
        mReaderThread.wait();
        if (mAccessWatch) {
            mAccessWatch->join();
            mAccessWatch.clear();
        }
        assert(hasError() || !mAccessWatch);
    }
}

void RamLoader::loadRam(void* ptr, uint64_t size) {
    uint32_t num_pages = (size + mPageSize - 1) / mPageSize;
    char* pagePtr = (char*)((uintptr_t)ptr & ~(mPageSize - 1));
    for (uint32_t i = 0; i < num_pages; i++) {
        loadRamPage(pagePtr + i * mPageSize);
    }
}

void RamLoader::registerBlock(const RamBlock& block) {
    mPageSize = block.pageSize;
    mIndex.blocks.push_back({block, {}, {}});
}

bool RamLoader::start() {
    if (mWasStarted) {
        return !mHasError;
    }

    mStartTime = base::System::get()->getHighResTimeUs();

    mWasStarted = true;
    if (!readIndex()) {
        mHasError = true;
        return false;
    }
    if (!mAccessWatch) {
        bool res = readAllPages();
        mEndTime = base::System::get()->getHighResTimeUs();
#if SNAPSHOT_PROFILE > 1
        printf("Eager RAM load complete in %.03f ms\n",
               (mEndTime - mStartTime) / 1000.0);
#endif
        return res;
    }

    if (!registerPageWatches()) {
        mHasError = true;
        return false;
    }
    mBackgroundPageIt = mIndex.pages.begin();
    mAccessWatch->doneRegistering();
    mReaderThread.start();
    return true;
}

void RamLoader::join() {
    mJoining = true;
    mReaderThread.wait();
    if (mAccessWatch) {
        mAccessWatch->join();
        mAccessWatch.clear();
    }
    for (auto&& stream : mStreams) {
        stream.close();
    }
}

void RamLoader::interrupt() {
    mReadDataQueue.stop();
    mReadingQueue.stop();
    mReaderThread.wait();
    if (mAccessWatch) {
        mAccessWatch->join();
        mAccessWatch.clear();
    }
    for (auto&& stream : mStreams) {
        stream.close();
    }
}

static base::MemStream openIndex(base::StdioStream& stream,
                                 base::System::FileSize size) {
    auto fd = fileno(stream.get());
    auto indexPos = stream.getBe64();

    MemStream::Buffer buffer(size - indexPos);
    if (base::pread(fd, buffer.data(), buffer.size(), indexPos) !=
        buffer.size()) {
        return false;
    }

    return MemStream{std::move(buffer)};
}

std::pair<RamLoader::FileIndex, GapTracker::Ptr> RamLoader::loadIndex(
        const Snapshot& snapshot, base::System::FileSize* outSize) {
    auto file = base::PathUtils::join(snapshot.dataDir(), "ram.bin");
    auto stream = base::StdioStream(fopen(file.c_str(), "rb"),
                                    base::StdioStream::kOwner);
    if (!stream.get()) {
        return {};
    }
    auto fd = fileno(stream.get());
    base::System::FileSize size;
    if (!base::System::get()->fileSize(fd, &size)) {
        return {};
    }
    if (outSize) {
        *outSize = size;
    }

    auto indexStream = openIndex(stream, size);

    FileIndex index;
    GapTracker::Ptr gaps;
    if (!RamLoader::readIndex(snapshot, indexStream, index, gaps, nullptr)) {
        return {};
    }

    index.offset = size - indexStream.buffer().size();

    return {std::move(index), std::move(gaps)};
}

void RamLoader::interruptReading() {
    mLoadingCompleted.store(true, std::memory_order_relaxed);
    mReadDataQueue.stop();
    mReadingQueue.stop();
}

void RamLoader::zeroOutPage(const Page& page) {
    auto ptr = pagePtr(page);
    const RamBlock& block = mIndex.blocks[page.blockIndex].ramBlock;
    if (!isBufferZeroed(ptr, block.pageSize)) {
        memset(ptr, 0, size_t(block.pageSize));
    }
}

bool RamLoader::readIndex() {
#if SNAPSHOT_PROFILE > 1
    auto start = base::System::get()->getHighResTimeUs();
#endif

    auto file = base::PathUtils::join(mSnapshot.dataDir(), "ram.bin");
    mStreams[0] = base::StdioStream(fopen(file.c_str(), "rb"),
                                    base::StdioStream::kOwner);
    mStreamFds[0] = fileno(mStreams[0].get());
    base::System::FileSize size;
    if (!base::System::get()->fileSize(mStreamFds[0], &size)) {
        return false;
    }
    mDiskSize = size;
    auto indexStream = openIndex(mStreams[0], size);
    if (!RamLoader::readIndex(mSnapshot, indexStream, mIndex, mGaps,
                              &mStreams)) {
        return false;
    }

    mIndex.offset = mDiskSize - indexStream.buffer().size();

    for (int i = 1, size = mStreams.size(); i < size; ++i) {
        if (mStreams[i].get()) {
            mStreamFds[i] = fileno(mStreams[i].get());
        }
    }

#if SNAPSHOT_PROFILE > 1
    printf("readIndex() time: %.03f\n",
           (base::System::get()->getHighResTimeUs() - start) / 1000.0);
#endif
    return true;
}

bool RamLoader::readIndex(const Snapshot& snapshot,
                          base::Stream& indexStream,
                          FileIndex& index,
                          GapTracker::Ptr& gaps,
                          std::vector<base::StdioStream>* streams) {
    if (!readIndexPages(indexStream, 0, index)) {
        return false;
    }

    if (index.version > 1) {
        gaps = index.compressed() ? GapTracker::Ptr(new GenericGapTracker())
                                  : GapTracker::Ptr(new OneSizeGapTracker());
        gaps->load(indexStream);
    } else {
        gaps.reset(new NullGapTracker());
    }

    if (index.hasParents()) {
        // now open and read all parents' indexes to fill in the gaps in pages
        for (int parentIdx = 0; parentIdx < index.usedParents.size();
             ++parentIdx) {
            if (!index.usedParents[parentIdx]) {
                continue;
            }

            auto parentFile = base::PathUtils::join(
                    Snapshot::dataDir(snapshot.hierarchy()[parentIdx]
                                              .incremental()
                                              .parent()
                                              .name()),
                    "ram.bin");
            auto stream = base::StdioStream(fopen(parentFile.c_str(), "rb"),
                                            base::StdioStream::kOwner);
            auto fd = fileno(stream.get());

            base::System::FileSize size = 0;
            base::System::get()->fileSize(fd, &size);
            auto indexStream = openIndex(stream, size);
            if (!readIndexPages(indexStream, parentIdx + 1, index)) {
                return false;
            }

            if (streams) {
                assert(streams->size() > parentIdx + 1);
                (*streams)[parentIdx + 1] = std::move(stream);
            }
        }
    }

    return true;
}

bool RamLoader::readIndexPages(base::Stream& indexStream,
                               uint8_t fileNo,
                               RamLoader::FileIndex& index) {
    bool onlyPages = fileNo != 0;
    bool incremental;
    int version;
    if (!onlyPages) {
        // root file, populate all fields
        version = index.version = indexStream.getBe32();
        index.flags = RamIndexFlags(indexStream.getBe32());
        incremental = index.hasParents();
    } else {
        version = indexStream.getBe32();
        auto flags = RamIndexFlags(indexStream.getBe32());
        incremental = nonzero(flags & RamIndexFlags::Incremental);
    }
    if (version < 1 || version > 3) {
        return false;
    }
    if (incremental) {
        auto parentsCount = indexStream.getPackedNum();
        if (!onlyPages) {
            index.usedParents.resize(parentsCount);
        }
        for (int i = 0; i < parentsCount; ++i) {
            bool used = indexStream.getByte() != 0;
            if (!onlyPages) {
                index.usedParents[i] = used;
            }
        }
    }

    auto pageCount = indexStream.getBe32();
    if (onlyPages) {
        if (index.pages.size() != pageCount) {
            return false;
        }
    } else {
        index.pages.reserve(pageCount);
    }
    uint16_t blocksCount = index.blocks.size();
    if (version >= 3) {
        blocksCount = indexStream.getBe16();
    } else if (index.blocks.empty()) {
        return false;
    }

    int64_t runningFilePos = 8;
    int32_t prevPageSizeOnDisk = 0;
    for (size_t loadedBlockIdx = 0; loadedBlockIdx < blocksCount;
         ++loadedBlockIdx) {
        const auto nameLength = indexStream.getByte();
        char name[256];
        indexStream.read(name, nameLength);
        name[nameLength] = 0;
        auto blockIt = std::find_if(index.blocks.begin(), index.blocks.end(),
                                    [&name](const FileIndex::Block& b) {
                                        return strcmp(b.ramBlock.id, name) == 0;
                                    });
        if (blockIt == index.blocks.end()) {
            if (onlyPages) {
                return false;
            } else {
                // No registered block yet: add a placeholder to fill in later.
                // TODO: deal with the leak
                index.blocks.push_back(FileIndex::Block{
                        {strdup(name), 0, nullptr, 0, kDefaultPageSize},
                        {},
                        {}});
                blockIt = std::prev(index.blocks.end());
            }
        }
        readBlockPages(&indexStream, index, blockIt, &runningFilePos,
                       &prevPageSizeOnDisk, fileNo, incremental);
    }
    return true;
}

void RamLoader::readBlockPages(base::Stream* stream,
                               FileIndex& index,
                               FileIndex::Blocks::iterator blockIt,
                               int64_t* runningFilePosPtr,
                               int32_t* prevPageSizeOnDiskPtr,
                               int fileIndex,
                               bool incremental) {
    auto runningFilePos = *runningFilePosPtr;
    auto prevPageSizeOnDisk = *prevPageSizeOnDiskPtr;

    const auto blockIndex = std::distance(index.blocks.begin(), blockIt);

    const auto blockPagesCount = stream->getBe32();
    FileIndex::Block& block = *blockIt;
    if (block.ramBlock.totalSize == 0) {
        block.ramBlock.totalSize = blockPagesCount * block.ramBlock.pageSize;
    }
    auto pageIt = index.pages.end();
    auto endIt = index.pages.end();
    if (fileIndex == 0) {
        block.pagesBegin = pageIt;
        index.pages.resize(index.pages.size() + blockPagesCount);
        endIt = index.pages.end();
        block.pagesEnd = endIt;
    } else {
        pageIt = block.pagesBegin;
        endIt = block.pagesEnd;
    }

    auto readPage = [&]() -> Page {
        Page page;
        page.filled = true;
        page.fileIndex = uint8_t(fileIndex);
        page.blockIndex = uint16_t(blockIndex);
        const auto sizeOnDisk = stream->getPackedNum();
        if (sizeOnDisk == 0) {
            // Empty page
            page.state.store(uint8_t(State::Read), std::memory_order_relaxed);
            page.sizeOnDisk = 0;
            page.filePos = 0;
        } else {
            page.sizeOnDisk = uint16_t(sizeOnDisk);
            auto posDelta = stream->getPackedSignedNum();
            if (index.compressed()) {
                posDelta += prevPageSizeOnDisk;
                prevPageSizeOnDisk = int32_t(page.sizeOnDisk);
            } else {
                page.sizeOnDisk *= uint32_t(block.ramBlock.pageSize);
                posDelta *= block.ramBlock.pageSize;
            }
            if (index.version >= 2) {
                stream->read(page.hash.data(), page.hash.size());
            }
            runningFilePos += posDelta;
            page.filePos = uint64_t(runningFilePos);
        }
        return page;
    };

    if (incremental) {
        const auto count = stream->getBe32();
        for (int i = 0; i < count; ++i) {
            auto deltaPos = stream->getPackedNum();
            pageIt = std::next(pageIt, deltaPos);

            auto page = readPage();
            if (!pageIt->filled) {
                *pageIt = std::move(page);
            }
        }
    } else {
        for (; pageIt != endIt; ++pageIt) {
            auto page = readPage();
            if (!pageIt->filled) {
                *pageIt = std::move(page);
            }
        }
    }
    *runningFilePosPtr = runningFilePos;
    *prevPageSizeOnDiskPtr = prevPageSizeOnDisk;
}

bool RamLoader::registerPageWatches() {
    uint8_t* startPtr = nullptr;
    uint64_t curSize = 0;
    for (const Page& page : mIndex.pages) {
        auto ptr = pagePtr(page);
        auto size = pageSize(page);
        if (ptr == startPtr + curSize) {
            curSize += size;
        } else {
            if (startPtr) {
                if (!mAccessWatch->registerMemoryRange(startPtr, curSize)) {
                    return false;
                }
            }
            startPtr = ptr;
            curSize = size;
        }
    }
    if (startPtr) {
        if (!mAccessWatch->registerMemoryRange(startPtr, curSize)) {
            return false;
        }
    }
    return true;
}

uint8_t* RamLoader::pagePtr(const RamLoader::Page& page) const {
    const FileIndex::Block& block = mIndex.blocks[page.blockIndex];
    return block.ramBlock.hostPtr + uint64_t(&page - &*block.pagesBegin) *
                                            uint64_t(block.ramBlock.pageSize);
}

uint32_t RamLoader::pageSize(const RamLoader::Page& page) const {
    return uint32_t(mIndex.blocks[page.blockIndex].ramBlock.pageSize);
}

template <class Num>
static bool isPowerOf2(Num num) {
    return !(num & (num - 1));
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

    assert(isPowerOf2(blockIt->ramBlock.pageSize));
    auto pageStart = reinterpret_cast<uint8_t*>(
            (uintptr_t(ptr)) & uintptr_t(~(blockIt->ramBlock.pageSize - 1)));
    auto pageIndex = (pageStart - blockIt->ramBlock.hostPtr) /
                     blockIt->ramBlock.pageSize;
    auto pageIt = blockIt->pagesBegin + pageIndex;
    assert(pageIt != blockIt->pagesEnd);
    assert(ptr >= pagePtr(*pageIt));
    assert(ptr < pagePtr(*pageIt) + pageSize(*pageIt));
    return *pageIt;
}

void RamLoader::readerWorker() {
    while (auto pagePtr = mReadingQueue.receive()) {
        Page* page = *pagePtr;
        if (!page) {
            mReadDataQueue.send(nullptr);
            mReadingQueue.stop();
            break;
        }

        if (readDataFromDisk(page)) {
            mReadDataQueue.send(page);
        }
    }

    mEndTime = base::System::get()->getHighResTimeUs();
#if SNAPSHOT_PROFILE > 1
    printf("Background loading complete in %.03f ms\n",
           (mEndTime - mStartTime) / 1000.0);
#endif
}

MemoryAccessWatch::IdleCallbackResult RamLoader::backgroundPageLoad() {
    if (mReadingQueue.isStopped() && mReadDataQueue.isStopped()) {
        return MemoryAccessWatch::IdleCallbackResult::AllDone;
    }

    {
        Page* page = nullptr;
        if (mReadDataQueue.tryReceive(&page)) {
            return fillPageInBackground(page);
        }
    }

    for (int i = 0; i < int(mReadingQueue.capacity()); ++i) {
        // Find next page to queue.
        mBackgroundPageIt = std::find_if(
                mBackgroundPageIt, mIndex.pages.end(), [](const Page& page) {
                    auto state = page.state.load(std::memory_order_acquire);
                    return state == uint8_t(State::Empty) ||
                           (state == uint8_t(State::Read) && !page.data);
                });
#if SNAPSHOT_PROFILE > 2
        const auto count = int(mBackgroundPageIt - mIndex.pages.begin());
        if ((count % 10000) == 0 || count == int(mIndex.pages.size())) {
            printf("Background loading: at page #%d of %d\n", count,
                   int(mIndex.pages.size()));
        }
#endif

        if (mBackgroundPageIt == mIndex.pages.end()) {
            if (!mSentEndOfPagesMarker) {
                mSentEndOfPagesMarker = mReadingQueue.trySend(nullptr);
            }
            return mJoining ? MemoryAccessWatch::IdleCallbackResult::RunAgain
                            : MemoryAccessWatch::IdleCallbackResult::Wait;
        }

        if (mBackgroundPageIt->state.load(std::memory_order_relaxed) ==
            uint8_t(State::Read)) {
            Page* const page = &*mBackgroundPageIt++;
            return fillPageInBackground(page);
        }

        if (mReadingQueue.trySend(&*mBackgroundPageIt)) {
            ++mBackgroundPageIt;
        } else {
            // The queue is full - let's wait for a while to give the reader
            // time to empty it.
            return mJoining ? MemoryAccessWatch::IdleCallbackResult::RunAgain
                            : MemoryAccessWatch::IdleCallbackResult::Wait;
        }
    }

    return MemoryAccessWatch::IdleCallbackResult::RunAgain;
}

MemoryAccessWatch::IdleCallbackResult RamLoader::fillPageInBackground(
        RamLoader::Page* page) {
    if (page) {
        fillPageData(page);
        delete[] page->data;
        page->data = nullptr;
        // If we've loaded a page then this function took quite a while
        // and it's better to check for a pagefault before proceeding to
        // queuing pages into the reader thread.
        return mJoining ? MemoryAccessWatch::IdleCallbackResult::RunAgain
                        : MemoryAccessWatch::IdleCallbackResult::Wait;
    } else {
        // null page == all pages were loaded, stop.
        interruptReading();
        return MemoryAccessWatch::IdleCallbackResult::AllDone;
    }
}

void RamLoader::loadRamPage(void* ptr) {
    // It's possible for us to try to RAM load
    // things that are not registered in the index
    // (like from qemu_iovec_init_external).
    // Make sure that it is in the index.
    const auto blockIt = std::find_if(
            mIndex.blocks.begin(), mIndex.blocks.end(),
            [ptr](const FileIndex::Block& b) {
                return ptr >= b.ramBlock.hostPtr &&
                       ptr < b.ramBlock.hostPtr + b.ramBlock.totalSize;
            });

    if (blockIt == mIndex.blocks.end()) {
        return;
    }

    Page& page = this->page(ptr);
    uint8_t buf[kDefaultPageSize];
    readDataFromDisk(&page, ARRAY_SIZE(buf) >= pageSize(page) ? buf : nullptr);
    fillPageData(&page);
    if (page.data != buf) {
        delete[] page.data;
    }
    page.data = nullptr;
}

bool RamLoader::readDataFromDisk(Page* pagePtr, uint8_t* preallocatedBuffer) {
    Page& page = *pagePtr;
    if (page.sizeOnDisk == 0) {
        assert(page.state.load(std::memory_order_relaxed) >=
               uint8_t(State::Read));
        page.data = nullptr;
        return true;
    }

    auto state = uint8_t(State::Empty);
    if (!page.state.compare_exchange_strong(state, uint8_t(State::Reading),
                                            std::memory_order_acquire)) {
        // Spin until the reading thread finishes.
        while (state < uint8_t(State::Read)) {
            base::System::get()->yield();
            state = uint8_t(page.state.load(std::memory_order_acquire));
        }
        return false;
    }

    uint8_t compressedBuf[compress::maxCompressedSize(kDefaultPageSize)];
    auto size = page.sizeOnDisk;
    const bool compressed =
            nonzero(mIndex.flags & RamIndexFlags::CompressedPages) &&
            (mIndex.version == 1 || page.sizeOnDisk < kDefaultPageSize);

    // We need to allocate a dynamic buffer if:
    // - page is compressed and there's a decompressing thread pool
    // - page is compressed and local buffer is too small
    // - there's no preallocated buffer passed from the caller
    bool allocateBuffer = (compressed && (mDecompressor ||
                                          ARRAY_SIZE(compressedBuf) < size)) ||
                          !preallocatedBuffer;
    auto buf = allocateBuffer ? new uint8_t[size]
                              : compressed ? compressedBuf : preallocatedBuffer;
    auto read = HANDLE_EINTR(base::pread(mStreamFds[pagePtr->fileIndex], buf,
                                         size, int64_t(page.filePos)));
    if (read != int64_t(size)) {
        VERBOSE_PRINT(snapshot,
                      "Error: (%d) Reading page %p from disk returned less "
                      "data: %d of %d at %lld",
                      errno, this->pagePtr(page), int(read), int(size),
                      static_cast<long long>(page.filePos));
        if (allocateBuffer) {
            delete[] buf;
        }
        page.state.store(uint8_t(State::Error));
        mHasError = true;
        return false;
    }

    if (compressed) {
        if (mDecompressor) {
            page.data = buf;
            mDecompressor->enqueue(&page);
        } else {
            auto decompressed = preallocatedBuffer
                                        ? preallocatedBuffer
                                        : new uint8_t[pageSize(page)];
            if (!Decompressor::decompress(buf, int32_t(size), decompressed,
                                          int32_t(pageSize(page)))) {
                VERBOSE_PRINT(snapshot,
                              "Error: Decompressing page %p @%llu (%d -> %d) "
                              "failed",
                              this->pagePtr(page),
                              (unsigned long long)page.filePos, int(size),
                              int(pageSize(page)));
                if (!preallocatedBuffer) {
                    delete[] decompressed;
                }
                page.state.store(uint8_t(State::Error));
                mHasError = true;
                return false;
            }
            buf = decompressed;
        }
    }

    page.data = buf;
    page.state.store(uint8_t(State::Read), std::memory_order_release);
    return true;
}

void RamLoader::fillPageData(Page* pagePtr) {
    Page& page = *pagePtr;
    auto state = uint8_t(State::Read);
    if (!page.state.compare_exchange_strong(state, uint8_t(State::Filling),
                                            std::memory_order_acquire)) {
        while (state < uint8_t(State::Filled)) {
            base::System::get()->yield();
            state = page.state.load(std::memory_order_relaxed);
        }
        return;
    }

#if SNAPSHOT_PROFILE > 2
    printf("%s: loading page %p\n", __func__, this->pagePtr(page));
#endif
    if (mAccessWatch) {
        bool res = mAccessWatch->fillPage(
                this->pagePtr(page), pageSize(page), page.data,
                nonzero(mFlags & OperationFlags::Quickboot));
        if (!res) {
            mHasError = true;
        }
        page.state.store(uint8_t(res ? State::Filled : State::Error),
                         std::memory_order_release);
    }
}

bool RamLoader::readAllPages() {
#if SNAPSHOT_PROFILE > 1
    auto startTime = base::System::get()->getHighResTimeUs();
#endif
    if (nonzero(mIndex.flags & RamIndexFlags::CompressedPages) &&
        !mAccessWatch) {
        startDecompressor();
    }

    // Rearrange the nonzero pages in sequential disk order for faster reading.
    // Zero out all zero pages right here.
    std::vector<Page*> sortedPages;
    sortedPages.reserve(mIndex.pages.size());

#if SNAPSHOT_PROFILE > 1
    auto startTime1 = base::System::get()->getHighResTimeUs();
#endif

    for (Page& page : mIndex.pages) {
        if (page.sizeOnDisk) {
            sortedPages.emplace_back(&page);
        } else if (!nonzero(mFlags & OperationFlags::Quickboot)) {
            zeroOutPage(page);
        }
    }

#if SNAPSHOT_PROFILE > 1
    printf("zeroing took %.03f ms\n",
           (base::System::get()->getHighResTimeUs() - startTime1) / 1000.0);
#endif

    std::sort(sortedPages.begin(), sortedPages.end(),
              [](const Page* l, const Page* r) {
                  return l->filePos < r->filePos;
              });

#if SNAPSHOT_PROFILE > 1
    printf("Starting unpacker + sorting took %.03f ms\n",
           (base::System::get()->getHighResTimeUs() - startTime) / 1000.0);
#endif

    for (Page* page : sortedPages) {
        if (!readDataFromDisk(page, pagePtr(*page))) {
            mHasError = true;
            return false;
        }
    }

    mDecompressor.clear();
    return true;
}

void RamLoader::startDecompressor() {
    mDecompressor.emplace([this](Page* page) {
        const bool res = Decompressor::decompress(
                page->data, int32_t(page->sizeOnDisk), pagePtr(*page),
                int32_t(pageSize(*page)));
        delete[] page->data;
        page->data = nullptr;
        if (!res) {
            derror("Decompressing page %p failed", pagePtr(*page));
            mHasError = true;
            page->state.store(uint8_t(State::Error));
        }
    });
    mDecompressor->start();
}

}  // namespace snapshot
}  // namespace android
