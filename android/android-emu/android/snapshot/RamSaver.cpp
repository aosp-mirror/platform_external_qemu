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

#include "android/base/system/System.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility>

namespace android {
namespace snapshot {

using android::base::System;

RamSaver::RamSaver(base::StdioStream&& stream, Flags flags)
    : mStream(std::move(stream)), mFlags(flags) {
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

RamSaver::~RamSaver() {
    join();
}

void RamSaver::registerBlock(const RamBlock& block) {
    mIndex.blocks.push_back({block, {}});
}

void RamSaver::savePage(int64_t blockOffset,
                        int64_t /*pageOffset*/,
                        int32_t pageSize) {
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

    RamBlock& block = mIndex.blocks[size_t(mLastBlockIndex)].ramBlock;
    if (block.pageSize == 0) {
        block.pageSize = pageSize;
        // First time we see a page for this block - save all its pages now.
        assert(block.totalSize % block.pageSize == 0);
        auto numPages = int32_t(block.totalSize / block.pageSize);
        mIndex.blocks[size_t(mLastBlockIndex)].pages.resize(numPages);
        for (int32_t i = 0; i != numPages; ++i) {
            passToSaveHandler({mLastBlockIndex, i});
        }
    } else {
        // We don't support different page sizes.
        assert(pageSize == block.pageSize);
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
    mHasError = ferror(mStream.get()) != 0;
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
        mIndex.startPosInFile = ftello64(mStream.get());
        writeIndex();

#if SNAPSHOT_PROFILE > 1
        printf("RAM saving time: %.03f\n",
               (System::get()->getHighResTimeUs() - mStartTime) / 1000.0);
#endif
        return false;
    }

    FileIndex::Block& block = mIndex.blocks[size_t(pi.blockIndex)];
    FileIndex::Block::Page& page = block.pages[size_t(pi.pageIndex)];
    ++mIndex.totalPages;

    auto ptr = block.ramBlock.hostPtr + int64_t(pi.pageIndex) * block.ramBlock.pageSize;
    if (isBufferZeroed(ptr, block.ramBlock.pageSize)) {
        page.sizeOnDisk = 0;
    } else {
        if (mCompressor) {
            mCompressor->enqueue(std::move(pi), ptr, block.ramBlock.pageSize);
        } else {
            writePage(pi, ptr, block.ramBlock.pageSize);
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

    bool compressed = (mIndex.flags & int(IndexFlags::CompressedPages)) != 0;
    mStream.putBe32(uint32_t(mIndex.version));
    mStream.putBe32(uint32_t(mIndex.flags));
    mStream.putBe32(uint32_t(mIndex.totalPages));
    int64_t prevFilePos = 8;
    int32_t prevPageSizeOnDisk = 0;
    for (const FileIndex::Block& b : mIndex.blocks) {
        auto id = base::StringView(b.ramBlock.id);
        mStream.putByte(uint8_t(id.size()));
        mStream.write(id.data(), id.size());
        mStream.putBe32(uint32_t(b.pages.size()));
        for (const FileIndex::Block::Page& page : b.pages) {
            mStream.putPackedNum(uint64_t(
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
                putDelta(&mStream, deltaPos);
                prevFilePos = page.filePos;
                prevPageSizeOnDisk = page.sizeOnDisk;
            }
        }
    }
    auto end = ftello64(mStream.get());
    mDiskSize = uint64_t(end);
#if SNAPSHOT_PROFILE > 1
    printf("RAM: index size: %d bytes\n", int(end - start));
#endif

    fseeko64(mStream.get(), 0, SEEK_SET);
    mStream.putBe64(uint64_t(mIndex.startPosInFile));
}

void RamSaver::writePage(const QueuedPageInfo& pi,
                         const void* dataPtr,
                         int32_t dataSize) {
    FileIndex::Block& block = mIndex.blocks[size_t(pi.blockIndex)];
    FileIndex::Block::Page& page = block.pages[size_t(pi.pageIndex)];
    page.sizeOnDisk = dataSize;
    page.filePos = int64_t(ftello64(mStream.get()));
    mStream.write(dataPtr, size_t(dataSize));
}

}  // namespace snapshot
}  // namespace android
