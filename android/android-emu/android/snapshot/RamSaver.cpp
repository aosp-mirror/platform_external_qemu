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
    : mStream(std::move(stream)),
      mZeroChecker(zeroChecker),
      mFlags(flags),
      mSavingThread([this]() { savingThreadWorker(); }) {
    // Put a placeholder for the index offset right now.
    mStream.putBe64(0);
    if (nonzero(mFlags & Flags::Async)) {
        mSavingThread.start();
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
    } else {
        // We don't support different page sizes yet, at least inside a single
        // RAM block.
        assert(pageSize == block.pageSize);
    }

    assert(pageOffset % block.pageSize == 0);
    QueuedPageInfo pi = {mLastBlockIndex,
                         static_cast<int32_t>(pageOffset / block.pageSize)};
    passToSaveHandler(pi);
}

void RamSaver::join() {
    done();
    mSavingThread.wait();
}

static constexpr int kStopMarkerIndex = -1;

void RamSaver::done() {
    passToSaveHandler({kStopMarkerIndex});
}

void RamSaver::savingThreadWorker() {
    QueuedPageInfo pi;
    for (;;) {
        mPagesQueue.receive(&pi);
        if (!handlePageSave(pi)) {
            break;
        }
    }

    mPagesQueue.stop();

#if SNAPSHOT_PROFILE > 1
    printf("Async RAM saving time: %.03f\n",
           (System::get()->getHighResTimeUs() - mStartTime) / 1000.0);
#endif
}

void RamSaver::passToSaveHandler(const RamSaver::QueuedPageInfo& pi) {
    if (nonzero(mFlags & Flags::Async)) {
        mPagesQueue.send(pi);
    } else {
        handlePageSave(pi);
    }
}

bool RamSaver::handlePageSave(const QueuedPageInfo& pi) {
    if (pi.blockIndex == kStopMarkerIndex) {
        mIndex.startPosInFile = ftell(mStream.get());
        writeIndex();
        return false;
    }

    FileIndex::Block& block = mIndex.blocks[pi.blockIndex];
    auto ptr = block.ramBlock.hostPtr + pi.pageIndex * block.ramBlock.pageSize;
    if (mZeroChecker(ptr, block.ramBlock.pageSize)) {
        block.zeroPageIndexes.push_back(pi.pageIndex);
    } else {
        ++mIndex.totalNonzeroPages;
        block.nonZeroPages.push_back(
                {pi.pageIndex, (int64_t)ftell(mStream.get())});
        mStream.write(ptr, block.ramBlock.pageSize);
    }

    return true;
}

// Write a (usually) small delta between two numbers to the |stream|. If the
// delta happens to be <= 0, use '0' as a marker and then put the negated
// delta - this way Stream::putPackedNum() uses the least space.
static void putDelta(base::Stream* stream, int32_t value, int32_t prev) {
    if (value > prev) {
        stream->putPackedNum(value - prev);
    } else {
        stream->putByte(0);
        stream->putPackedNum(prev - value);
    }
}

void RamSaver::writeIndex() {
#if SNAPSHOT_PROFILE > 1
    auto start = ftell(mStream.get());
#endif

    mStream.putBe32(mIndex.version);
    mStream.putBe32(mIndex.totalNonzeroPages);
    for (const FileIndex::Block& b : mIndex.blocks) {
        mStream.putByte(b.ramBlock.id.size());
        mStream.write(b.ramBlock.id.data(), b.ramBlock.id.size());
        mStream.putBe32(b.zeroPageIndexes.size());
        if (!b.zeroPageIndexes.empty()) {
            auto it = b.zeroPageIndexes.begin();
            auto offset = *it++;
            mStream.putBe32(offset);
            for (; it != b.zeroPageIndexes.end(); ++it) {
                const auto nextOffset = *it;
                putDelta(&mStream, nextOffset, offset);
                offset = nextOffset;
            }
        }
        mStream.putBe32(b.nonZeroPages.size());
        if (!b.nonZeroPages.empty()) {
            auto it = b.nonZeroPages.begin();
            const FileIndex::Block::Page* page = &*it++;
            mStream.putBe32(page->index);
            mStream.putBe64(page->filePos);
            for (; it != b.nonZeroPages.end(); ++it) {
                const FileIndex::Block::Page& nextPage = *it;
                putDelta(&mStream, nextPage.index, page->index);
                assert(nextPage.filePos > page->filePos);
                assert(((nextPage.filePos - page->filePos) %
                        b.ramBlock.pageSize) == 0);
                mStream.putPackedNum((nextPage.filePos - page->filePos) /
                                     b.ramBlock.pageSize);
                page = &nextPage;
            }
        }
    }
#if SNAPSHOT_PROFILE > 1
    auto end = ftell(mStream.get());
    printf("ram: index size: %d\n", (int)(end - start));
#endif

    fseek(mStream.get(), 0, SEEK_SET);
    mStream.putBe64(mIndex.startPosInFile);
}

}  // namespace snapshot
}  // namespace android
