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

RamSaver::RamSaver(base::StdioStream&& stream,
                   ZeroChecker zeroChecker,
                   Flags flags)
    : mStream(std::move(stream)), mZeroChecker(zeroChecker), mFlags(flags) {
    // Put a placeholder for the index offset right now.
    mStream.putBe64(0);
    if (nonzero(mFlags & Flags::Compress)) {
        mIndex.flags |= (int32_t)FileIndex::Flags::CompressedPages;
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
    mIndex.blocks.push_back({block});
}

void RamSaver::savePage(int64_t blockOffset,
                        int64_t pageOffset,
                        int32_t pageSize) {
    assert(!mIndex.blocks.empty());
    assert(mLastBlockIndex >= 0 && mLastBlockIndex < (int)mIndex.blocks.size());
    if (blockOffset != mIndex.blocks[mLastBlockIndex].ramBlock.startOffset) {
        mLastBlockIndex = std::distance(
                mIndex.blocks.begin(),
                std::find_if(mIndex.blocks.begin(), mIndex.blocks.end(),
                             [blockOffset](const FileIndex::Block& block) {
                                 return block.ramBlock.startOffset ==
                                        blockOffset;
                             }));
        assert(mLastBlockIndex < (int)mIndex.blocks.size());
    }

    RamBlock& block = mIndex.blocks[mLastBlockIndex].ramBlock;
    if (block.pageSize == 0) {
        block.pageSize = pageSize;
        // First time we see a page for this block - save all its pages now.
        assert(block.totalSize % block.pageSize == 0);
        auto numPages = int32_t(block.totalSize / block.pageSize);
        mIndex.blocks[mLastBlockIndex].pages.resize(numPages);
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
    passToSaveHandler({kStopMarkerIndex});
    mSavingWorker.clear();
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
        mIndex.startPosInFile = ftell(mStream.get());
        writeIndex();

#if SNAPSHOT_PROFILE > 1
        printf("RAM saving time: %.03f\n",
               (System::get()->getHighResTimeUs() - mStartTime) / 1000.0);
#endif
        return false;
    }

    FileIndex::Block& block = mIndex.blocks[pi.blockIndex];
    FileIndex::Block::Page& page = block.pages[pi.pageIndex];
    ++mIndex.totalPages;

    auto ptr = block.ramBlock.hostPtr + pi.pageIndex * block.ramBlock.pageSize;

    if (mZeroChecker(ptr, block.ramBlock.pageSize)) {
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
        assert((delta & (1LL<<63)) == 0);
        stream->putPackedNum(delta << 1);
    } else {
        assert((-delta & (1LL<<63)) == 0);
        stream->putPackedNum(((-delta) << 1) | 1);
    }
}

void RamSaver::writeIndex() {
#if SNAPSHOT_PROFILE > 1
    auto start = mIndex.startPosInFile;
#endif

    bool compressed = (mIndex.flags & (int)IndexFlags::CompressedPages) != 0;
    mStream.putBe32(mIndex.version);
    mStream.putBe32(mIndex.flags);
    mStream.putBe32(mIndex.totalPages);
    int64_t prevFilePos = 8;
    int32_t prevPageSizeOnDisk = 0;
    for (const FileIndex::Block& b : mIndex.blocks) {
        mStream.putByte(b.ramBlock.id.size());
        mStream.write(b.ramBlock.id.data(), b.ramBlock.id.size());
        mStream.putBe32(b.pages.size());
        for (const FileIndex::Block::Page& page : b.pages) {
            mStream.putPackedNum(
                    compressed ? page.sizeOnDisk
                               : (page.sizeOnDisk / b.ramBlock.pageSize));
            if (page.sizeOnDisk) {
                auto deltaPos = page.filePos - prevFilePos;
                if (compressed) {
                    deltaPos -= prevPageSizeOnDisk;
                } else {
                    if (deltaPos % b.ramBlock.pageSize != 0) {
                        printf("%d %d\n", (int)deltaPos, (int)b.ramBlock.pageSize);
                    }
                    deltaPos /= b.ramBlock.pageSize;
                }
                putDelta(&mStream, deltaPos);
                prevFilePos = page.filePos;
                prevPageSizeOnDisk = page.sizeOnDisk;
            }
        }
    }
#if SNAPSHOT_PROFILE > 1
    auto end = ftell(mStream.get());
    printf("RAM: index size: %d bytes\n", (int)(end - start));
#endif

    fseek(mStream.get(), 0, SEEK_SET);
    mStream.putBe64(mIndex.startPosInFile);
}

void RamSaver::writePage(const QueuedPageInfo& pi,
                         const void* dataPtr,
                         int32_t dataSize) {
    FileIndex::Block& block = mIndex.blocks[pi.blockIndex];
    FileIndex::Block::Page& page = block.pages[pi.pageIndex];
    page.sizeOnDisk = dataSize;
    page.filePos = int64_t(ftell(mStream.get()));
    mStream.write(dataPtr, dataSize);
}

}  // namespace snapshot
}  // namespace android
