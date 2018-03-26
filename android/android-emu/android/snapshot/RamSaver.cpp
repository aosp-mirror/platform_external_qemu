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

#include "android/base/Stopwatch.h"
#include "android/base/files/MemStream.h"
#include "android/base/files/preadwrite.h"
#include "android/base/memory/OnDemand.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/system/System.h"
#include "android/snapshot/RamLoader.h"
#include "android/utils/debug.h"

#include "MurmurHash3.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility>

namespace android {
namespace snapshot {

using android::base::MemStream;
using android::base::System;

using StatAction = IncrementalStats::Action;
using StatTime = IncrementalStats::Time;

void RamSaver::FileIndex::clear() {
    decltype(blocks)().swap(blocks);
}

RamSaver::RamSaver(const std::string& fileName,
                   Flags preferredFlags,
                   RamLoader* loader,
                   bool isOnExit)
    : mStream(nullptr) {
    bool incremental = false;
    if (loader) {
        // check if we're ok to proceed with incremental saving
        auto currentGaps = loader->releaseGapTracker();
        assert(currentGaps);
        const auto wastedSpace = currentGaps->wastedSpace();
        if (wastedSpace <= loader->diskSize() * 0.30) {
            incremental = true;
            if (isOnExit) {
                loader->interrupt();
            }
            mGaps = std::move(currentGaps);
        } else {
            loader->join();
        }
        VERBOSE_PRINT(snapshot,
                      "%s incremental RAM saving (currently wasting %llu"
                      " bytes of %llu bytes (%.2f%%)",
                      incremental ? "Enabled" : "Disabled",
                      (unsigned long long)wastedSpace,
                      (unsigned long long)loader->diskSize(),
                      wastedSpace * 100.0 / loader->diskSize());
    }

    if (incremental) {
        mFlags = loader->compressed() ? RamSaver::Flags::Compress
                                      : RamSaver::Flags::None;
        mLoader = loader;
        mLoaderOnDemand = loader->onDemandEnabled();
        mStream = base::StdioStream(fopen(fileName.c_str(), "rb+"),
                                    base::StdioStream::kOwner);
        if (mStream.get()) {
            // Seek to the old index position and start overwriting from there.
            fseeko64(mStream.get(), loader->indexOffset(), SEEK_SET);
            mCurrentStreamPos = loader->indexOffset();
        }
    } else {
        mFlags = preferredFlags;
        mStream = base::StdioStream(fopen(fileName.c_str(), "wb"),
                                    base::StdioStream::kOwner);
        if (mStream.get()) {
            // Put a placeholder for the index offset right now.
            mStream.putBe64(0);
        }
    }

    if (!mStream.get()) {
        mHasError = true;
        return;
    }

    mStreamFd = fileno(mStream.get());

    if (nonzero(mFlags & Flags::Compress)) {
        mIndex.flags |= int32_t(FileIndex::Flags::CompressedPages);

        auto compressBuffers = new CompressBuffer[kCompressBufferCount];
        mCompressBufferMemory.reset(compressBuffers);
        mCompressBuffers.emplace(compressBuffers,
                                 compressBuffers + kCompressBufferCount);
    }

    mWorkers.emplace(
            std::min(System::get()->getCpuCoreCount() - 1, 2),
            [this](QueuedPageInfo&& pi) {
                mIncStats.measure(StatTime::HandlingPageSaves, [&] {
                    handlePageSave(std::move(pi));
                });
            });
    mWorkers->start();
    mPendingWrites.reserve(kMaxPendingWrites);
    mWriteCombineBuffer.resize(kDefaultPageSize * kMaxPendingWrites);
    mWriter.emplace([this](WriteInfo&& wi) {
        bool shouldStop = false;

        if (wi.page) {
            mPendingWrites.push_back(std::move(wi));
        } else {
            shouldStop = true;
        }

        if (shouldStop || mPendingWrites.size() == kMaxPendingWrites) {
            mIncStats.measure(StatTime::WriteCombining, [this] {
                flushWrites();
            });
        }

        return shouldStop ? base::WorkerProcessingResult::Stop :
                            base::WorkerProcessingResult::Continue;
    });
    mWriter->start();
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
    if (mLastBlockIndex < 0) {
        mLastBlockIndex = 0;
#if SNAPSHOT_PROFILE > 1
        printf("From ctor to first savePage: %.03f\n",
               (mSystem->getHighResTimeUs() - mStartTime) / 1000.0);
#endif
    }
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
        mIndex.totalPages += numPages;
        // Split it up into work units of kMaxPendingWrites.
        int32_t start = 0;
        int32_t end = 0;
        for (int32_t i = 0; i != numPages; ++i) {
            if (i == numPages - 1 || (i - start) == kCompressBufferCount) {
                end = i + 1;
                passToSaveHandler({mLastBlockIndex, start, end});
                start = end;
            }
        }
    }
}

void RamSaver::complete() {
    mWorkers->done();
}

static constexpr int kStopMarkerIndex = -1;

void RamSaver::join() {
    if (mJoined) {
        return;
    }
    passToSaveHandler({kStopMarkerIndex, 0});
    mJoined = true;
}

void RamSaver::cancel() {
    mCanceled.store(true, std::memory_order_release);
    join();
}

void RamSaver::calcHash(FileIndex::Block::Page& page,
                        const FileIndex::Block& block,
                        const void* ptr) {
    MurmurHash3_x64_128(ptr, block.ramBlock.pageSize, 0, page.hash.data());
    page.hashFilled = true;
}

void RamSaver::passToSaveHandler(QueuedPageInfo&& pi) {
    if (pi.blockIndex != kStopMarkerIndex) {
        mIncStats.count(StatAction::TotalPages);
        mWorkers->enqueue(std::move(pi));
    } else {
        if (mStopping.load(std::memory_order_acquire))
            return;
        mStopping.store(true, std::memory_order_release);

        mWorkers.clear();
        mWriter->enqueue({});
        mWriter.clear();
        mIndex.startPosInFile = mCurrentStreamPos;
        writeIndex();

        mIndex.clear();

        mEndTime = System::get()->getHighResTimeUs();

#if SNAPSHOT_PROFILE > 1
        printf("RAM saving time: %.03f\n", (mEndTime - mStartTime) / 1000.0);
#endif
    }
}

bool RamSaver::handlePageSave(QueuedPageInfo&& pi) {
    if (mCanceled.load(std::memory_order_acquire))
        return false;

    assert(pi.blockIndex != kStopMarkerIndex);

    FileIndex::Block& block = mIndex.blocks[size_t(pi.blockIndex)];

    size_t numToProcess = pi.pageIndexEnd - pi.pageIndexStart;
    std::vector<int32_t> changedPages;
    std::vector<int32_t> changedNonzeroPages;
    changedPages.reserve(numToProcess);
    changedNonzeroPages.reserve(numToProcess);

    for (int pageIndex = pi.pageIndexStart; pageIndex < pi.pageIndexEnd; ++pageIndex) {
        FileIndex::Block::Page& page = block.pages[size_t(pageIndex)];

        // short-cirquit the fastest cases right here.
        if (mLoader && mLoaderOnDemand) {
            const RamLoader::Page* loaderPage = mLoader->findPage(
                    pi.blockIndex, block.ramBlock.id, pageIndex);
            if (loaderPage &&
                    loaderPage->state.load(std::memory_order_relaxed) <
                    int(RamLoader::State::Filled)) {
                // not loaded yet: definitely not changed
                mIncStats.count(StatAction::SamePage);
                mIncStats.count(StatAction::NotLoadedPage);

                page.same = true;
                page.filePos = loaderPage->filePos;
                page.sizeOnDisk = loaderPage->sizeOnDisk;
                if (page.sizeOnDisk) {
                    if (mLoader->version() >= 2) {
                        page.hash = loaderPage->hash;
                        page.hashFilled = true;
                    } else {
                        const auto ptr =
                            block.ramBlock.hostPtr +
                            int64_t(pageIndex) * block.ramBlock.pageSize;
                        calcHash(page, block, ptr);
                    }
                }
                continue;
            }
        }

        auto ptr = block.ramBlock.hostPtr +
            int64_t(pageIndex) * block.ramBlock.pageSize;
        auto isZeroed = base::makeOnDemand<bool>([&] {
                return std::make_tuple(mIncStats.measure(StatTime::ZeroCheck, [&] {
                            return isBufferZeroed(ptr, block.ramBlock.pageSize);
                            }));
                });
        page.filePos = 0;
        page.same = false;
        page.hashFilled = false;
        const RamLoader::Page* loaderPage = nullptr;
        if (mLoader) {
            loaderPage = mLoader->findPage(pi.blockIndex, block.ramBlock.id,
                    pageIndex);
            assert(loaderPage);
            if (loaderPage->zeroed()) {
                if (*isZeroed) {
                    mIncStats.count(StatAction::StillZeroPage);
                    page.same = true;
                    page.sizeOnDisk = 0;
                }
            } else if (mLoader->version() >= 2) {
                calcHash(page, block, ptr);
                if (page.hash == loaderPage->hash) {
                    mIncStats.count(StatAction::SameHashPage);
                    page.same = true;
                    page.filePos = loaderPage->filePos;
                    page.sizeOnDisk = loaderPage->sizeOnDisk;
                }
            }
        }
        if (page.same) {
            mIncStats.count(StatAction::SamePage);
        } else {
            changedPages.push_back(pageIndex);
            mIncStats.count(StatAction::ChangedPage);
        }
    }

    mIncStats.measure(StatTime::ZeroCheck, [&] {

    for (auto pageIndex : changedPages) {
        auto& page = block.pages[size_t(pageIndex)];
        auto ptr = block.ramBlock.hostPtr +
            int64_t(pageIndex) * block.ramBlock.pageSize;
        bool isZero = isBufferZeroed(ptr, block.ramBlock.pageSize);
        if (isZero) {
            page.sizeOnDisk = 0;
        } else {
            changedNonzeroPages.push_back(pageIndex);
        }
    }

    });

    mIncStats.measure(StatTime::Hashing, [&] {

    for (auto pageIndex : changedNonzeroPages) {
        auto& page = block.pages[size_t(pageIndex)];
        auto ptr = block.ramBlock.hostPtr +
            int64_t(pageIndex) * block.ramBlock.pageSize;
        if (!page.hashFilled) {
            calcHash(page, block, ptr);
        }
    }

    });

    mIncStats.measure(StatTime::Compressing, [&] {

    if (compressed()) {
        for (auto pageIndex : changedNonzeroPages) {
            auto& page = block.pages[size_t(pageIndex)];
            auto ptr = block.ramBlock.hostPtr +
                int64_t(pageIndex) * block.ramBlock.pageSize;
            // TODO: deal with loader page
            WriteInfo wi = {&page, nullptr, ptr, false};
            auto buffer = mCompressBuffers->allocate();
            auto compressedSize =
                        compress::compress(
                                ptr, block.ramBlock.pageSize,
                                buffer->data(), buffer->size());
            assert(compressedSize > 0);
            if (compressedSize >= block.ramBlock.pageSize) {
                // Screw this, the page is better off uncompressed.
                page.sizeOnDisk = block.ramBlock.pageSize;
                mCompressBuffers->release(buffer);
            } else {
                page.sizeOnDisk = compressedSize;
                wi.ptr = buffer->data();
                wi.allocated = true;
            }
            mWriter->enqueue(std::move(wi));
        }
    } else {
        for (auto pageIndex : changedNonzeroPages) {
            auto& page = block.pages[size_t(pageIndex)];
            page.sizeOnDisk = block.ramBlock.pageSize;
        }
    }

    });

    return true;
}

void RamSaver::writeIndex() {
    auto start = mIndex.startPosInFile;

    MemStream stream(512 + 16 * mIndex.totalPages);
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
            if (!page.zeroed()) {
                auto deltaPos = page.filePos - prevFilePos;
                if (compressed) {
                    deltaPos -= prevPageSizeOnDisk;
                } else {
                    assert(deltaPos % b.ramBlock.pageSize == 0);
                    deltaPos /= b.ramBlock.pageSize;
                }
                stream.putPackedSignedNum(deltaPos);
                assert(page.hashFilled);
                stream.write(page.hash.data(), page.hash.size());
                prevFilePos = page.filePos;
                prevPageSizeOnDisk = page.sizeOnDisk;
            }
        }
    }

    mIncStats.measure(StatTime::GapTrackingWriter, [&] {
        incremental() ? mGaps->save(stream) : OneSizeGapTracker().save(stream);
    });

    auto end = mIncStats.measure(StatTime::Disk, [&] {
        auto end = mIndex.startPosInFile + stream.writtenSize();
        mDiskSize = uint64_t(end);

        base::pwrite(mStreamFd, stream.buffer().data(), stream.buffer().size(),
                     mIndex.startPosInFile);
        setFileSize(mStreamFd, int64_t(mDiskSize));
        fseeko64(mStream.get(), 0, SEEK_SET);
        mStream.putBe64(uint64_t(mIndex.startPosInFile));
        mHasError = ferror(mStream.get()) != 0;
        mStream.close();
        return end;
    });

    auto bytesWasted = incremental() ? mGaps->wastedSpace() : 0;
    mIncStats.print(
            "RAM: index %d, total %lld bytes, wasted %d (compressed: %s)\n",
            int(end - start), (long long)mDiskSize, int(bytesWasted),
            compressed ? "yes" : "no");
}

void RamSaver::flushWrites() {
    int64_t nextStreamPos = mCurrentStreamPos;

    if (incremental()) {
        for (auto& i : mPendingWrites) {
            if (i.loaderPage) {
                if (i.page->sizeOnDisk <= i.loaderPage->sizeOnDisk) {
                    i.page->filePos = i.loaderPage->filePos;
                    mIncStats.count(StatAction::ReusedPos);
                    if (i.page->sizeOnDisk < i.loaderPage->sizeOnDisk) {
                        mIncStats.measure(StatTime::GapTrackingWorker, [&] {
                                mGaps->add(
                                        i.loaderPage->filePos + i.page->sizeOnDisk,
                                        i.loaderPage->sizeOnDisk - i.page->sizeOnDisk);
                                });
                    }
                } else if (!i.loaderPage->zeroed()) {
                    mIncStats.measure(StatTime::GapTrackingWorker, [&] {
                            mGaps->add(i.loaderPage->filePos, i.loaderPage->sizeOnDisk);
                            });
                }
            }

            if (i.page->filePos == 0) {
                if (auto gapPos = mIncStats.measure(StatTime::GapTrackingWriter, [&] {
                            return mGaps->allocate(i.page->sizeOnDisk);
                            })) {
                    i.page->filePos = *gapPos;
                    mIncStats.count(StatAction::ReusedPos);
                }
            }
        }
    }

    for (auto& i : mPendingWrites) {
        if (i.page->filePos == 0) {
            i.page->filePos = nextStreamPos;
            nextStreamPos += i.page->sizeOnDisk;
            mIncStats.count(StatAction::AppendedPos);
        }
    }

    if (incremental()) {
        std::sort(mPendingWrites.begin(), mPendingWrites.end(),
                  WriteInfoFilePosCompare());
    }

    char* writeCombinePtr = mWriteCombineBuffer.data();
    int64_t currStart = -1;
    int64_t currEnd = -1;
    int64_t contigBytes = 0;
    for (auto& i : mPendingWrites) {
        int64_t pos = i.page->filePos;
        int64_t sz = i.page->sizeOnDisk;

        if (currEnd == -1) {
            currStart = pos;
            currEnd = pos + sz;
            contigBytes = sz;
        } else if (currEnd == i.page->filePos) {
            currEnd += sz;
            contigBytes += sz;
        } else {
            mIncStats.measure(StatTime::Disk,
                    [&] { base::pwrite(mStreamFd,
                            mWriteCombineBuffer.data(),
                            contigBytes,
                            currStart); });
            writeCombinePtr = mWriteCombineBuffer.data();
            currStart = pos;
            currEnd = pos + sz;
            contigBytes = sz;
        }

        memcpy(writeCombinePtr, i.ptr, sz);

        if (i.allocated) {
            mCompressBuffers->release((CompressBuffer*)i.ptr);
        }

        writeCombinePtr += sz;
    }

    mIncStats.measure(StatTime::Disk,
            [&] { base::pwrite(mStreamFd,
                    mWriteCombineBuffer.data(),
                    contigBytes,
                    currStart); });

    mPendingWrites.clear();
    mCurrentStreamPos = nextStreamPos;
}

}  // namespace snapshot
}  // namespace android
