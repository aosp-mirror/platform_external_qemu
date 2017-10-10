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

#include "android/snapshot/RamSaver.h"

#include "android/base/files/MemStream.h"
#include "android/base/files/preadwrite.h"
#include "android/base/system/System.h"
#include "android/snapshot/RamLoader.h"

#include "MurmurHash3.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility>

namespace android {
namespace snapshot {

static int total = 0;
static int same = 0;
static int notyet = 0;
static int still0 = 0;
static int samehash = 0;

using android::base::MemStream;
using android::base::System;

void RamSaver::FileIndex::clear() {
    decltype(blocks)().swap(blocks);
}

RamSaver::RamSaver(base::StdioStream&& stream, Flags flags)
    : mStream(std::move(stream)),
      mStreamFd(fileno(mStream.get())),
      mFlags(flags) {
    // Put a placeholder for the index offset right now.
    mStream.putBe64(0);
    if (nonzero(mFlags & Flags::Compress)) {
        mIndex.flags |= int32_t(FileIndex::Flags::CompressedPages);
        mCompressor.emplace(
                [this](QueuedPageInfo&& pi, const uint8_t* data, int32_t size) {
                    writePage(pi, data, size);
                });
    }
    if (nonzero(mFlags & Flags::Async)) {
        mSavingWorker.emplace([this](QueuedPageInfo&& pi) {
            return savePageInWorker(std::move(pi));
        });
        mSavingWorker->start();
    }
}

static RamLoader* prepareLoader(RamLoader& loader) {
    loader.interrupt();
    return &loader;
}

static RamSaver::Flags loaderFlags(const RamLoader& loader) {
    if (loader.compressed()) {
        return RamSaver::Flags::Compress;
    } else {
        return RamSaver::Flags::None;
    }
}

RamSaver::RamSaver(RamLoader& loader, const std::string& fileName)
    : mLoader(prepareLoader(loader)),
      mStream(fopen(fileName.c_str(), "rb+")),
      mStreamFd(fileno(mStream.get())),
      mFlags(loaderFlags(loader)),
      mLoaderOnDemand(mLoader->onDemandEnabled()) {
    if (nonzero(mFlags & Flags::Compress)) {
        mIndex.flags |= int32_t(FileIndex::Flags::CompressedPages);
        mCompressor.emplace(
                [this](QueuedPageInfo&& pi, const uint8_t* data, int32_t size) {
                    writePage(pi, data, size);
                });
    }

    // Seek to the old index position and start overwriting from there.
    fseeko64(mStream.get(), loader.indexOffset(), SEEK_SET);
    mCurrentStreamPos.store(loader.indexOffset(), std::memory_order_relaxed);
}

RamSaver::~RamSaver() {
    join();
}

void RamSaver::registerBlock(const RamBlock& block) {
    mIndex.blocks.push_back({block, {}});
}

void RamSaver::savePage(int64_t blockOffset,
                        int64_t /*pageOffset*/,
                        int32_t /*pageSize*/) {
    assert(!mIndex.blocks.empty());
    assert(mLastBlockIndex >= 0 && mLastBlockIndex < int(mIndex.blocks.size()));
    if (blockOffset !=
        mIndex.blocks[size_t(mLastBlockIndex)].ramBlock.startOffset) {
        mLastBlockIndex = int(std::distance(
                mIndex.blocks.begin(),
                std::find_if(mIndex.blocks.begin(), mIndex.blocks.end(),
                             [blockOffset](const FileIndex::Block& block) {
                                 return block.ramBlock.startOffset ==
                                        blockOffset;
                             })));
        assert(mLastBlockIndex < int(mIndex.blocks.size()));
    }

    auto& block = mIndex.blocks[size_t(mLastBlockIndex)];
    if (block.pages.empty()) {
        // First time we see a page for this block - save all its pages now.
        auto& ramBlock = block.ramBlock;
        assert(ramBlock.totalSize % ramBlock.pageSize == 0);
        auto numPages = int32_t(ramBlock.totalSize / ramBlock.pageSize);
        mIndex.blocks[size_t(mLastBlockIndex)].pages.resize(size_t(numPages));
        for (int32_t i = 0; i != numPages; ++i) {
            passToSaveHandler({mLastBlockIndex, i});
        }
    }
}

static constexpr int kStopMarkerIndex = -1;

void RamSaver::join() {
    if (mJoined) {
        return;
    }
    passToSaveHandler({kStopMarkerIndex, 0});
    mSavingWorker.clear();
    mJoined = true;
    mIndex.clear();
}

void RamSaver::passToSaveHandler(QueuedPageInfo&& pi) {
    if (mSavingWorker) {
        mSavingWorker->enqueue(std::move(pi));
    } else {
        handlePageSave(std::move(pi));
    }
}

bool RamSaver::handlePageSave(QueuedPageInfo&& pi) {
    if (pi.blockIndex == kStopMarkerIndex) {
        mCompressor.clear();
        mIndex.startPosInFile =
                mCurrentStreamPos.load(std::memory_order_relaxed);
        fseeko64(mStream.get(), mIndex.startPosInFile, SEEK_SET);
        writeIndex();

        mHasError = ferror(mStream.get()) != 0;
        mIndex.clear();

#if SNAPSHOT_PROFILE > 1
        printf("RAM saving time: %.03f\n",
               (System::get()->getHighResTimeUs() - mStartTime) / 1000.0);
#endif
        return false;
    }

    FileIndex::Block& block = mIndex.blocks[size_t(pi.blockIndex)];
    FileIndex::Block::Page& page = block.pages[size_t(pi.pageIndex)];
    ++mIndex.totalPages;

    ++total;

    auto ptr = block.ramBlock.hostPtr +
               int64_t(pi.pageIndex) * block.ramBlock.pageSize;
    base::Optional<bool> isZeroed;
    page.same = false;
    if (const RamLoader::Page* loaderPage =
                (mLoader ? mLoader->findPage(pi.blockIndex, block.ramBlock.id,
                                             pi.pageIndex)
                         : nullptr)) {
        if (mLoaderOnDemand &&
            loaderPage->state.load(std::memory_order_relaxed) <
                    int(RamLoader::State::Filled)) {
            // not loaded yet: definitely not changed
            ++notyet;
            page.same = true;
            page.filePos = loaderPage->filePos;
            page.sizeOnDisk = loaderPage->sizeOnDisk;
            if (page.sizeOnDisk) {
                if (mLoader->version() >= 2) {
                    page.hash = loaderPage->hash;
                } else {
                    MurmurHash3_x64_128(ptr, block.ramBlock.pageSize, 0,
                                        page.hash.data());
                }
                page.hashFilled = true;
            }
        } else if (loaderPage->sizeOnDisk == 0) {
            isZeroed = isBufferZeroed(ptr, block.ramBlock.pageSize);
            if (*isZeroed) {
                ++still0;
                page.same = true;
                page.filePos = page.sizeOnDisk = 0;
            }
        } else if (mLoader->version() >= 2) {
            MurmurHash3_x64_128(ptr, block.ramBlock.pageSize, 0,
                                page.hash.data());
            page.hashFilled = true;
            if (page.hash == loaderPage->hash) {
                ++samehash;
                page.same = true;
                page.filePos = loaderPage->filePos;
                page.sizeOnDisk = loaderPage->sizeOnDisk;
            }
        }
    } else if (mLoader) {
        printf("Failed to find page %d/%d\n", pi.blockIndex, pi.pageIndex);
    }
    if (page.same) {
        ++same;
    } else {
        if (!isZeroed) {
            isZeroed = isBufferZeroed(ptr, block.ramBlock.pageSize);
        }
        if (*isZeroed) {
            page.sizeOnDisk = 0;
        } else {
            if (!page.hashFilled) {
                MurmurHash3_x64_128(ptr, block.ramBlock.pageSize, 0,
                                    page.hash.data());
                page.hashFilled = true;
            }
            if (mCompressor) {
                mCompressor->enqueue(std::move(pi), ptr,
                                     block.ramBlock.pageSize);
            } else {
                writePage(pi, ptr, block.ramBlock.pageSize);
            }
        }
    }

    return true;
}

base::WorkerProcessingResult RamSaver::savePageInWorker(QueuedPageInfo&& pi) {
    if (handlePageSave(std::move(pi))) {
        return base::WorkerProcessingResult::Continue;
    }
    return base::WorkerProcessingResult::Stop;
}

// Write a (usually) small delta between two numbers to the |stream|. Use the
// lowest bit as a marker for a negative number.
static void putDelta(base::Stream* stream, int64_t delta) {
    if (delta >= 0) {
        assert((uint64_t(delta) & (1ULL << 63)) == 0);
        stream->putPackedNum(uint64_t(delta) << 1);
    } else {
        assert((uint64_t(-delta) & (1ULL << 63)) == 0);
        stream->putPackedNum((uint64_t(-delta) << 1) | 1);
    }
}

void RamSaver::writeIndex() {
#if SNAPSHOT_PROFILE > 1
    auto start = mIndex.startPosInFile;
#endif

    MemStream stream(512 + 2 * mIndex.totalPages);
    bool compressed = (mIndex.flags & int(IndexFlags::CompressedPages)) != 0;
    stream.putBe32(uint32_t(mIndex.version));
    stream.putBe32(uint32_t(mIndex.flags));
    stream.putBe32(uint32_t(mIndex.totalPages));
    int64_t prevFilePos = 8;
    int32_t prevPageSizeOnDisk = 0;
    for (const FileIndex::Block& b : mIndex.blocks) {
        auto id = base::StringView(b.ramBlock.id);
        stream.putByte(uint8_t(id.size()));
        stream.write(id.data(), id.size());
        stream.putBe32(uint32_t(b.pages.size()));
        for (const FileIndex::Block::Page& page : b.pages) {
            stream.putPackedNum(uint64_t(
                    compressed ? page.sizeOnDisk
                               : (page.sizeOnDisk / b.ramBlock.pageSize)));
            if (page.sizeOnDisk) {
                auto deltaPos = page.filePos - prevFilePos;
                if (compressed) {
                    deltaPos -= prevPageSizeOnDisk;
                } else {
                    assert(deltaPos % b.ramBlock.pageSize == 0);
                    deltaPos /= b.ramBlock.pageSize;
                }
                putDelta(&stream, deltaPos);
                assert(page.hashFilled);
                stream.write(page.hash.data(), page.hash.size());
                prevFilePos = page.filePos;
                prevPageSizeOnDisk = page.sizeOnDisk;
            }
        }
    }

    auto end = ftello64(mStream.get()) + stream.writtenSize();
    mDiskSize = uint64_t(end);
#if SNAPSHOT_PROFILE > 1
    printf("RAM: index size: %d bytes (%d pages, %d same [not loaded %d; still "
           "empty %d; same hash %d], %d new)\n",
           int(end - start), total, same, notyet, still0, samehash,
           total - same);
#endif

    mStream.write(stream.buffer().data(), stream.buffer().size());
    fseeko64(mStream.get(), 0, SEEK_SET);
    mStream.putBe64(uint64_t(mIndex.startPosInFile));
}

void RamSaver::writePage(const QueuedPageInfo& pi,
                         const void* dataPtr,
                         int32_t dataSize) {
    FileIndex::Block& block = mIndex.blocks[size_t(pi.blockIndex)];
    FileIndex::Block::Page& page = block.pages[size_t(pi.pageIndex)];
    page.sizeOnDisk = dataSize;
    page.filePos =
            mCurrentStreamPos.fetch_add(dataSize, std::memory_order_relaxed);
    base::pwrite(mStreamFd, dataPtr, size_t(dataSize), page.filePos);
}

}  // namespace snapshot
}  // namespace android
