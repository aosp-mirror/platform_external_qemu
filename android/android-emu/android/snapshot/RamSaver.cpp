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

#include "android/base/ContiguousRangeMapper.h"
#include "android/base/Profiler.h"
#include "android/base/Stopwatch.h"
#include "android/base/EintrWrapper.h"
#include "android/base/files/FileShareOpen.h"
#include "android/base/files/MemStream.h"
#include "android/base/files/preadwrite.h"
#include "android/base/memory/MemoryHints.h"
#include "android/base/memory/OnDemand.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/system/System.h"
#include "android/snapshot/MemoryWatch.h"
#include "android/snapshot/RamLoader.h"
#include "android/utils/debug.h"

#include "MurmurHash3.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility>

#ifdef __APPLE__

#include <sys/types.h>
#include <sys/mman.h>

#include <Hypervisor/hv.h>
#endif

namespace android {
namespace snapshot {

using android::base::ContiguousRangeMapper;
using android::base::MemStream;
using android::base::MemoryHint;
using android::base::ScopedMemoryProfiler;
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
        mFlags = loader->compressed() ? RamSaver::Flags::Compress :
                                        RamSaver::Flags::None;
        if (nonzero(preferredFlags & RamSaver::Flags::Async)) {
            mFlags |= RamSaver::Flags::Async;
        }

        mLoader = loader;
        mLoaderOnDemand = loader->onDemandEnabled();
        mStream = base::StdioStream(
                android::base::fsopen(fileName.c_str(), "rb+",
                                      android::base::FileShare::Write),
                base::StdioStream::kOwner);
        if (mStream.get()) {
            // Seek to the old index position and start overwriting from there.
            fseeko64(mStream.get(), loader->indexOffset(), SEEK_SET);
            mCurrentStreamPos = loader->indexOffset();
        }
    } else {
        mFlags = preferredFlags;
        mStream = base::StdioStream(
                android::base::fsopen(fileName.c_str(), "wb",
                                      android::base::FileShare::Write),
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

    if (nonzero(mFlags & Flags::Async)) {
        mIndex.flags |= int32_t(FileIndex::Flags::SeparateBackingStore);
    }

    if (nonzero(mFlags & Flags::Compress)) {
        mIndex.flags |= int32_t(FileIndex::Flags::CompressedPages);

        auto compressBuffers = new CompressBuffer[kCompressBufferCount];
        mCompressBufferMemory.reset(compressBuffers);
        mCompressBuffers.emplace(compressBuffers,
                                 compressBuffers + kCompressBufferCount);
    }

    mWriteCombineBuffer.resize(kCompressBufferBatchSize * kDefaultPageSize);

    mWorkers.emplace(
            std::min(System::get()->getCpuCoreCount() - 1, 2),
            [this](QueuedPageInfo&& pi) {
                mIncStats.measure(StatTime::TotalHandlingPageSave, [&] {
                    handlePageSave(std::move(pi));
                });
            });
    if (!mWorkers->start()) {
        mHasError = true;
        return;
    }
    mWriter.emplace([this](WriteInfo&& wi) {
        if (wi.blockIndex == -1) {
            return base::WorkerProcessingResult::Stop;
        }
        writePage(std::move(wi));
        return base::WorkerProcessingResult::Continue;
    });
    if (!mWriter->start()) {
        mHasError = true;
        return;
    }
}

RamSaver::~RamSaver() {
    join();
    mIndex.clear();
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

    if (block.ramBlock.readonly) {
        // No need to save this block as it's readonly
        return;
    }

    if (block.ramBlock.flags & SNAPSHOT_RAM_USER_BACKED) {
        // No need to save this block as it's readonly
        return;
    }

    if ((block.ramBlock.flags & SNAPSHOT_RAM_MAPPED_SHARED) &&
        nonzero(mFlags & RamSaver::Flags::Async)) {
        // No need to save this block as it's mapped as shared
        // and we are in async saving mode.
        return;
    }

    if (block.pages.empty()) {
        // First time we see a page for this block - save all its pages now.
        auto& ramBlock = block.ramBlock;

        // bug: 113126623
        // TODO: Figure out how to deal with pages sizes != 4k
        ramBlock.pageSize = kDefaultPageSize;

        assert(ramBlock.totalSize % ramBlock.pageSize == 0);
        auto numPages = int32_t(ramBlock.totalSize / ramBlock.pageSize);
        mIndex.blocks[size_t(mLastBlockIndex)].pages.resize(size_t(numPages));
        mIndex.totalPages += numPages;

        // Short-circuit the fastest cases right here.

        // Stats counting vars (for speed, avoid atomic ops)
        int totalZero = 0;
        int changedTotal = 0;
        int samePage = 0;
        int notLoadedPage = 0;
        int stillZero = 0;
        int sameHash = 0;

        mIncStats.countMultiple(StatAction::TotalPages, numPages);

        mIncStats.measure(StatTime::ZeroCheck, [&] {

            // Hint that we will access sequentially.
            android::base::memoryHint(
                block.ramBlock.hostPtr,
                numPages * block.ramBlock.pageSize,
                MemoryHint::Sequential);

            // Initialize Pages and check for all-zero pages.
            uint8_t* zeroCheckPtr = block.ramBlock.hostPtr;

            {

                // RAM decommit: when checking for zero pages or hashing, we need to make sure
                // that the memory does not become resident, or useful memory might
                // get paged out and the save itself will have to compete with
                // paging out, which can slow things down.
                //
                // Track continguous 16mb ranges to decommit.  This is so that zero
                // check causes extra RAM to be resident only up to 16 mb, while
                // avoiding issuing frequent system calls.

                // Zero pages can actually be zeroed out and MADV_FREE'ed.
                ContiguousRangeMapper zeroPageDeleter([](uintptr_t start, uintptr_t size) {
                    android::base::memoryHint((void*)start, size, MemoryHint::DontNeed);
                }, kDecommitChunkSize);

#if SNAPSHOT_PROFILE > 1
                ScopedMemoryProfiler mem("zeroCheck");
#endif

                for (int32_t i = 0; i < numPages;
                     ++i,
                     zeroCheckPtr += (uintptr_t)block.ramBlock.pageSize) {

                    bool isZero = isBufferZeroed(zeroCheckPtr,
                                                 block.ramBlock.pageSize);

                    auto& page = block.pages[size_t(i)];
                    page.same = false;
                    page.hashFilled = false;
                    page.filePos = 0;
                    page.loaderPage = nullptr;

                    // Don't branch for the isZero decision
                    page.sizeOnDisk = kDefaultPageSize * !isZero;
                    totalZero += isZero;

                    // Decommit or free in chunks of 16 mb.
                    if (page.sizeOnDisk == 0) {
                        zeroPageDeleter.add((uintptr_t)zeroCheckPtr, block.ramBlock.pageSize);
                    }
                }
            }

            changedTotal = totalZero;

            // Initialize the incremental save case
            if (mLoader) {

                // Check for not-yet-loaded pages if we are doing
                // on-demand RAM loading
                if (mLoaderOnDemand) {
                    for (int32_t i = 0; i < numPages; ++i) {
                        auto& page = block.pages[size_t(i)];
                        // Find all corresponding loader pages
                        page.loaderPage =
                            mLoader->findPage(mLastBlockIndex, block.ramBlock.id, i);
                        auto loaderPage = page.loaderPage;
                        if (loaderPage &&
                            loaderPage->state.load(std::memory_order_relaxed) <
                            int(RamLoader::State::Filled)) {
                            // not loaded yet: definitely not changed
                            samePage++;
                            notLoadedPage++;
                            page.same = true;
                            page.filePos = loaderPage->filePos;
                            page.sizeOnDisk = loaderPage->sizeOnDisk;
                            if (page.sizeOnDisk) {
                                page.hash = loaderPage->hash;
                                page.hashFilled = true;
                            }
                        }
                    }

                } else {
                    // Find all corresponding loader pages
                    for (int32_t i = 0; i < numPages; ++i) {
                        auto& page = block.pages[size_t(i)];
                        page.loaderPage =
                            mLoader->findPage(mLastBlockIndex, block.ramBlock.id, i);
                    }
                }
            }
        });

        // Calculate all hashes and if applicable, compare with previous
        // snapshot, computing all changed nonzero pages
        mIncStats.measure(StatTime::Hashing, [&] {

#if SNAPSHOT_PROFILE > 1
            ScopedMemoryProfiler mem("hashing");
#endif

            uint8_t* hashPtr = block.ramBlock.hostPtr;
            for (int32_t i = 0; i < numPages; ++i,
                 hashPtr += (uintptr_t)block.ramBlock.pageSize) {
                auto& page = block.pages[size_t(i)];
                if (page.sizeOnDisk && !page.hashFilled) {
                    calcHash(page, block, hashPtr);
                }
            }


            // Comparison with previous snapshot
            if (mLoader) {
                mIncStats.measure(StatTime::Hashing, [&] {

                for (int32_t i = 0; i < numPages; ++i) {
                    auto& page = block.pages[size_t(i)];
                    auto loaderPage = page.loaderPage;
                    if (loaderPage && loaderPage->zeroed() && !page.sizeOnDisk) {
                        ++stillZero;
                        page.same = true;
                        page.sizeOnDisk = 0;
                    } else if (page.hash == loaderPage->hash) {
                        ++sameHash;
                        page.same = true;
                        page.filePos = loaderPage->filePos;
                        page.sizeOnDisk = loaderPage->sizeOnDisk;
                    }
                }

                // Don't count stillZero pages in the total changed pages set.
                changedTotal -= stillZero;

                });
            }

            // These are the pages that will actually be written to disk;
            // the nonzero and changed pages.
            for (int32_t i = 0; i < numPages; ++i) {
                auto& page = block.pages[size_t(i)];
                if (!page.same && page.sizeOnDisk) {
                    block.nonzeroChangedPages.push_back(i);
                }
            }

            changedTotal += block.nonzeroChangedPages.size();

        });

        // Pass them to the save handler in chunks of kCompressBufferBatchSize.
        int32_t start = 0;
        int32_t end = 0;
        for (int32_t i = 0; i < block.nonzeroChangedPages.size(); ++i) {
            if (i == block.nonzeroChangedPages.size() - 1 ||
                (i - start + 1) == kCompressBufferBatchSize) {
                end = i + 1;
                passToSaveHandler({mLastBlockIndex, start, end});
                start = end;
            }
        }

        // Record most stats right here.
        mIncStats.countMultiple(StatAction::SamePage, samePage);
        mIncStats.countMultiple(StatAction::NotLoadedPage, notLoadedPage);
        mIncStats.countMultiple(StatAction::ChangedPage, changedTotal);
        mIncStats.countMultiple(StatAction::StillZeroPage, stillZero);
        mIncStats.countMultiple(StatAction::NewZeroPage, totalZero - stillZero);
        mIncStats.countMultiple(StatAction::SameHashPage, sameHash);
        mIncStats.countMultiple(StatAction::SamePage, sameHash + stillZero);
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
    if (pi.blockIndex != kStopMarkerIndex &&
        !mCanceled.load(std::memory_order_acquire)) {
        mWorkers->enqueue(std::move(pi));
    } else {
        if (mStopping.load(std::memory_order_acquire))
            return;
        mStopping.store(true, std::memory_order_release);

        mWorkers.clear();
        if (mWriter) {
            mWriter->enqueue({-1});
            mWriter.clear();
            mIndex.startPosInFile = mCurrentStreamPos;
            writeIndex();
        }

        mEndTime = System::get()->getHighResTimeUs();

#if SNAPSHOT_PROFILE > 1
        printf("RAM saving time: %.03f\n", (mEndTime - mStartTime) / 1000.0);
#endif
    }
}

bool RamSaver::handlePageSave(QueuedPageInfo&& pi) {

    assert(pi.blockIndex != kStopMarkerIndex);
    FileIndex::Block& block = mIndex.blocks[size_t(pi.blockIndex)];


    WriteInfo wi = {pi.blockIndex, pi.nonzeroChangedIndexStart,
                    pi.nonzeroChangedIndexEnd,
                    nullptr };

    if (compressed()) {
        CompressBuffer* compressBuffer =
            mIncStats.measure(StatTime::WaitingForDisk, [&] {
                return mCompressBuffers->allocate();
            });

        wi.toRelease = compressBuffer;
        uint8_t* compressBufferData = compressBuffer->data();
        uintptr_t compressBufferOffset = 0;

        mIncStats.measure(StatTime::Compressing, [&] {

            for (int32_t nzcIndex = pi.nonzeroChangedIndexStart;
                 nzcIndex < pi.nonzeroChangedIndexEnd; ++nzcIndex) {

                int32_t pageIndex = block.nonzeroChangedPages[size_t(nzcIndex)];
                auto& page = block.pages[size_t(pageIndex)];
                auto ptr = block.ramBlock.hostPtr +
                    int64_t(pageIndex) * block.ramBlock.pageSize;

                auto compressedSize =
                    compress::compress(
                            ptr, block.ramBlock.pageSize,
                            compressBufferData + compressBufferOffset,
                            compress::maxCompressedSize(kDefaultPageSize));

                assert(compressedSize > 0);

                // Invariant: The page is compressed iff
                // its sizeOnDisk is strictly less than the page size.
                if (compressedSize >= block.ramBlock.pageSize) {
                    // Screw this, the page is better off uncompressed.
                    page.sizeOnDisk = block.ramBlock.pageSize;
                    page.writePtr = ptr;
                } else {
                    page.sizeOnDisk = compressedSize;
                    page.writePtr = compressBufferData + compressBufferOffset;
                    compressBufferOffset += compressedSize;
                }
            }
        });

    } else {
        for (int32_t nzcIndex = pi.nonzeroChangedIndexStart; nzcIndex < pi.nonzeroChangedIndexEnd; ++nzcIndex) {
            int32_t pageIndex = block.nonzeroChangedPages[size_t(nzcIndex)];
            auto& page = block.pages[size_t(pageIndex)];
            auto ptr = block.ramBlock.hostPtr +
                int64_t(pageIndex) * block.ramBlock.pageSize;
            page.sizeOnDisk = block.ramBlock.pageSize;
            page.writePtr = ptr;
        }
    }

    mWriter->enqueue(std::move(wi));

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

    mIncStats.measure(StatTime::DiskIndexWrite, [&] {
        for (const FileIndex::Block& b : mIndex.blocks) {
            auto id = base::StringView(b.ramBlock.id);
            stream.putByte(uint8_t(id.size()));
            stream.write(id.data(), id.size());
            stream.putBe32(uint32_t(b.pages.size()));
            stream.putBe32(uint32_t(b.ramBlock.pageSize));

            if (nonzero(mFlags & RamSaver::Flags::Async)) {
                stream.putBe32(b.ramBlock.flags);
                stream.putString(b.ramBlock.path);
            } else {
                stream.putBe32(0);
                stream.putString("");
            }

            if (b.ramBlock.readonly) {
                continue;
            }

            if (b.ramBlock.flags & SNAPSHOT_RAM_USER_BACKED) {
                continue;
            }

            if ((b.ramBlock.flags & SNAPSHOT_RAM_MAPPED_SHARED) &&
                nonzero(mFlags & RamSaver::Flags::Async)) {
                continue;
            }

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
                    assert(page.hashFilled ||
                           mCanceled.load(std::memory_order_acquire));
                    stream.write(page.hash.data(), page.hash.size());
                    prevFilePos = page.filePos;
                    prevPageSizeOnDisk = page.sizeOnDisk;
                }
            }

            // Make our RAM pages normal again
            android::base::memoryHint(
                b.ramBlock.hostPtr,
                b.pages.size() * b.ramBlock.pageSize,
                MemoryHint::Normal);
        }
    });

    mIncStats.measure(StatTime::GapTrackingWriter, [&] {
        incremental() ? mGaps->save(stream) : OneSizeGapTracker().save(stream);
    });

    auto end = mIncStats.measure(StatTime::DiskIndexWrite, [&] {
        auto end = mIndex.startPosInFile + stream.writtenSize();
        mDiskSize = uint64_t(end);

        base::pwrite(mStreamFd, stream.buffer().data(), stream.buffer().size(),
                     mIndex.startPosInFile);
        setFileSize(mStreamFd, int64_t(mDiskSize));
        HANDLE_EINTR(fseeko64(mStream.get(), 0, SEEK_SET));
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

void RamSaver::writePage(WriteInfo&& wi) {
    int64_t nextStreamPos = mCurrentStreamPos;

    FileIndex::Block& block = mIndex.blocks[size_t(wi.blockIndex)];

    // Two stats are being counted here;
    // new pages in reused disk locations and
    // new pages appended at the end of the current set of pages.
    int64_t reusedPos = 0;
    int64_t appendedPos = 0;

    if (incremental()) {

        // First add many possible gaps, then take them away,
        // to increase coherent access to gap tracker
        // and decrease fragmentation / wasted space.
        mIncStats.measure(StatTime::GapTrackingWorker, [&] {
            for (int32_t nzcIndex = wi.nonzeroChangedIndexStart;
                 nzcIndex < wi.nonzeroChangedIndexEnd; ++nzcIndex) {

                int32_t pageIndex = block.nonzeroChangedPages[size_t(nzcIndex)];
                auto& page = block.pages[size_t(pageIndex)];

                if (page.loaderPage) {
                    if (page.sizeOnDisk <= page.loaderPage->sizeOnDisk) {
                        page.filePos = page.loaderPage->filePos;
                        ++reusedPos;
                        if (page.sizeOnDisk < page.loaderPage->sizeOnDisk) {
                            mGaps->add(
                                    page.loaderPage->filePos + page.sizeOnDisk,
                                    page.loaderPage->sizeOnDisk - page.sizeOnDisk);
                        }
                    } else if (!page.loaderPage->zeroed()) {
                        mGaps->add(page.loaderPage->filePos, page.loaderPage->sizeOnDisk);
                    }
                }
            }

        });

        mIncStats.measure(StatTime::GapTrackingWriter, [&] {
            for (int32_t nzcIndex = wi.nonzeroChangedIndexStart;
                 nzcIndex < wi.nonzeroChangedIndexEnd; ++nzcIndex) {

                int32_t pageIndex = block.nonzeroChangedPages[size_t(nzcIndex)];
                auto& page = block.pages[size_t(pageIndex)];

                if (page.filePos == 0) {
                    if (auto gapPos = mGaps->allocate(page.sizeOnDisk)) {
                        page.filePos = *gapPos;
                        ++reusedPos;
                    }
                }
            }

        });

    }

    mIncStats.measure(StatTime::DiskWriteCombine, [&] {

        for (int32_t nzcIndex = wi.nonzeroChangedIndexStart;
             nzcIndex < wi.nonzeroChangedIndexEnd; ++nzcIndex) {

            int32_t pageIndex = block.nonzeroChangedPages[size_t(nzcIndex)];
            auto& page = block.pages[size_t(pageIndex)];

            if (page.filePos == 0) {
                page.filePos = nextStreamPos;
                nextStreamPos += page.sizeOnDisk;
                ++appendedPos;
            }
        }

        char* writeCombinePtr = mWriteCombineBuffer.data();
        int64_t currStart = -1;
        int64_t currEnd = -1;
        int64_t contigBytes = 0;

        for (int32_t nzcIndex = wi.nonzeroChangedIndexStart;
             nzcIndex < wi.nonzeroChangedIndexEnd; ++nzcIndex) {

            int32_t pageIndex = block.nonzeroChangedPages[size_t(nzcIndex)];
            auto& page = block.pages[size_t(pageIndex)];

            int64_t pos = page.filePos;
            int64_t sz = page.sizeOnDisk;

            if (currEnd == -1) {
                currStart = pos;
                currEnd = pos + sz;
                contigBytes = sz;
            } else if (currEnd == page.filePos) {
                currEnd += sz;
                contigBytes += sz;
            } else {
                base::pwrite(mStreamFd,
                             mWriteCombineBuffer.data(),
                             contigBytes,
                             currStart);
                writeCombinePtr = mWriteCombineBuffer.data();
                currStart = pos;
                currEnd = pos + sz;
                contigBytes = sz;
            }

            memcpy(writeCombinePtr, page.writePtr, sz);
            writeCombinePtr += sz;
        }

        if (wi.toRelease) {
            mCompressBuffers->release(wi.toRelease);
        }

        base::pwrite(mStreamFd,
                     mWriteCombineBuffer.data(),
                     contigBytes,
                     currStart);
        mCurrentStreamPos = nextStreamPos;

    });

    mIncStats.countMultiple(StatAction::ReusedPos, reusedPos);
    mIncStats.countMultiple(StatAction::AppendedPos, appendedPos);
}

}  // namespace snapshot
}  // namespace android
