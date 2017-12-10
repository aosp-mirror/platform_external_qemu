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

static std::atomic<int> total{0};
static std::atomic<int> same{0};
static std::atomic<int> notyet{0};
static std::atomic<int> still0{0};
static std::atomic<int> samehash{0};
static std::atomic<int> appended{0};
static std::atomic<int> reused{0};
static std::atomic<int> new_zero{0};
static std::atomic<int> write_time{0};
static std::atomic<int> hashing_time{0};
static std::atomic<int> zero_check_time{0};
static std::atomic<int> gap_tracker_time{0};
static std::atomic<int> compress_time{0};

using android::base::MemStream;
using android::base::System;

template <class Counter>
class RaiiTimeTracker {
public:
    RaiiTimeTracker(Counter& time) : mTime(time) {}
    ~RaiiTimeTracker() { mTime += mSw.elapsedUs(); }

private:
    base::Stopwatch mSw;
    Counter& mTime;
};

template <class Func, class Counter>
auto measure(Counter& time, Func&& f) -> decltype(f()) {
    RaiiTimeTracker<Counter> rtt(time);
    return f();
}

void RamSaver::FileIndex::clear() {
    decltype(blocks)().swap(blocks);
}

RamSaver::RamSaver(const std::string& fileName,
                   Flags preferredFlags,
                   RamLoader* loader)
    : mStream(nullptr) {
    bool incremental = false;
    if (loader) {
        // check if we're ok to proceed with incremental saving
        auto currentGaps = loader->releaseGapTracker();
        assert(currentGaps);
        const auto wastedSpace = currentGaps->wastedSpace();
        if (wastedSpace <= loader->diskSize() * 0.30) {
            incremental = true;
            mGaps = std::move(currentGaps);
        }
        VERBOSE_PRINT(snapshot,
                      "%s incremental RAM saving (currently wasting %llu"
                      " bytes of %llu bytes (%.2f%%)",
                      incremental ? "Enabled" : "Disabled",
                      (unsigned long long)wastedSpace,
                      (unsigned long long)loader->diskSize(),
                      wastedSpace * 100.0 / loader->diskSize());
    }

    if (loader) {
        loader->interrupt();
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
    }

    mWorkers.emplace(
            std::max(2, std::min(System::get()->getCpuCoreCount() - 1, 16)),
            [this](QueuedPageInfo&& pi) { handlePageSave(std::move(pi)); });
    mWorkers->start();
    mWriter.emplace([this](WriteInfo&& wi) {
        if (!wi.page) {
            return base::WorkerProcessingResult::Stop;
        }
        writePage(std::move(wi));
        return base::WorkerProcessingResult::Continue;
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
        for (int32_t i = 0; i != numPages; ++i) {
            passToSaveHandler({mLastBlockIndex, i});
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
    mIndex.clear();
}

void RamSaver::calcHash(FileIndex::Block::Page& page,
                        const FileIndex::Block& block,
                        const void* ptr) {
    base::Stopwatch sw;
    MurmurHash3_x64_128(ptr, block.ramBlock.pageSize, 0, page.hash.data());
    page.hashFilled = true;
    hashing_time += sw.elapsedUs();
}

void RamSaver::passToSaveHandler(QueuedPageInfo&& pi) {
    if (pi.blockIndex != kStopMarkerIndex) {
        ++total;

        // short-cirquit the fastest cases right here.
        if (mLoader && mLoaderOnDemand) {
            FileIndex::Block& block = mIndex.blocks[size_t(pi.blockIndex)];
            const RamLoader::Page* loaderPage = mLoader->findPage(
                    pi.blockIndex, block.ramBlock.id, pi.pageIndex);
            if (loaderPage &&
                loaderPage->state.load(std::memory_order_relaxed) <
                        int(RamLoader::State::Filled)) {
                // not loaded yet: definitely not changed
                ++same;
                ++notyet;
                FileIndex::Block::Page& page =
                        block.pages[size_t(pi.pageIndex)];
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
                                int64_t(pi.pageIndex) * block.ramBlock.pageSize;
                        calcHash(page, block, ptr);
                    }
                }
                return;
            }
        }
        mWorkers->enqueue(std::move(pi));
    } else {
        mWorkers.clear();
        mWriter->enqueue({});
        mWriter.clear();
        mIndex.startPosInFile = mCurrentStreamPos;
        writeIndex();

        mIndex.clear();

#if SNAPSHOT_PROFILE > 1
        printf("RAM saving time: %.03f\n",
               (System::get()->getHighResTimeUs() - mStartTime) / 1000.0);
#endif
    }
}

bool RamSaver::handlePageSave(QueuedPageInfo&& pi) {
    assert(pi.blockIndex != kStopMarkerIndex);

    FileIndex::Block& block = mIndex.blocks[size_t(pi.blockIndex)];
    FileIndex::Block::Page& page = block.pages[size_t(pi.pageIndex)];

    auto ptr = block.ramBlock.hostPtr +
               int64_t(pi.pageIndex) * block.ramBlock.pageSize;
    auto isZeroed = base::makeOnDemand<bool>([&] {
        return std::make_tuple(measure(zero_check_time, [&] {
            return isBufferZeroed(ptr, block.ramBlock.pageSize);
        }));
    });
    page.filePos = 0;
    page.same = false;
    page.hashFilled = false;
    const RamLoader::Page* loaderPage = nullptr;
    if (mLoader) {
        loaderPage = mLoader->findPage(pi.blockIndex, block.ramBlock.id,
                                       pi.pageIndex);
        assert(loaderPage);
        if (loaderPage->zeroed()) {
            if (*isZeroed) {
                ++still0;
                page.same = true;
                page.sizeOnDisk = 0;
            }
        } else if (mLoader->version() >= 2) {
            calcHash(page, block, ptr);
            if (page.hash == loaderPage->hash) {
                ++samehash;
                page.same = true;
                page.filePos = loaderPage->filePos;
                page.sizeOnDisk = loaderPage->sizeOnDisk;
            }
        }
    }
    if (page.same) {
        ++same;
    } else {
        if (*isZeroed) {
            ++new_zero;
            page.sizeOnDisk = 0;
            if (loaderPage) {
                measure(gap_tracker_time, [&] {
                    mGaps->add(loaderPage->filePos, loaderPage->sizeOnDisk);
                });
            }
        } else {
            if (!page.hashFilled) {
                calcHash(page, block, ptr);
            }

            if (compressed()) {
                auto compressedPage = measure(compress_time, [&] {
                    return CompressorBase::compress(ptr,
                                                    block.ramBlock.pageSize);
                });
                page.sizeOnDisk = compressedPage.second;
                if (loaderPage) {
                    if (page.sizeOnDisk <= loaderPage->sizeOnDisk) {
                        page.filePos = loaderPage->filePos;
                        if (page.sizeOnDisk < loaderPage->sizeOnDisk) {
                            measure(gap_tracker_time, [&] {
                                mGaps->add(
                                        loaderPage->filePos + page.sizeOnDisk,
                                        loaderPage->sizeOnDisk -
                                                page.sizeOnDisk);
                            });
                        }
                    } else {
                        measure(gap_tracker_time, [&] {
                            mGaps->add(loaderPage->filePos,
                                       loaderPage->sizeOnDisk);
                        });
                    }
                }
                mWriter->enqueue({&page, compressedPage.first, true});
            } else {
                page.sizeOnDisk = block.ramBlock.pageSize;
                if (loaderPage) {
                    if (loaderPage->sizeOnDisk == page.sizeOnDisk) {
                        page.filePos = loaderPage->filePos;
                    } else {
                        measure(gap_tracker_time, [&] {
                            mGaps->add(loaderPage->filePos,
                                       loaderPage->sizeOnDisk);
                        });
                    }
                }
                mWriter->enqueue({&page, ptr, false});
            }
        }
    }

    return true;
}

void RamSaver::writeIndex() {
#if SNAPSHOT_PROFILE > 1
    auto start = mIndex.startPosInFile;
#endif

    int zeroPages = 0;

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
            } else {
                ++zeroPages;
            }
        }
    }

    measure(gap_tracker_time, [&] {
        mGaps ? mGaps->save(stream) : OneSizeGapTracker().save(stream);
    });

    auto end = measure(write_time, [&] {
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

#if SNAPSHOT_PROFILE > 1
    auto bytes_wasted = mGaps ? mGaps->wastedSpace() : 0;
    printf("RAM: index %d bytes (is compressed: %d; %d pages, %d "
           "empty):\n"
           "\t%d same:\n"
           "\t\t[not loaded %d; still empty %d; same hash %d]\n"
           "\t%d new:\n"
           "\t\t[reused %d, empty %d, appended %d, %d bytes wasted]\n"
           "\tTimes: disk %.03f, hash %.03f, iszero %.03f, gaps %.03f, "
           "compress %.03f\n",
           int(end - start), compressed, total.load(), zeroPages, same.load(),
           notyet.load(), still0.load(), samehash.load(), total - same,
           reused.load(), new_zero.load(), appended.load(), int(bytes_wasted),
           write_time / 1000.0, hashing_time / 1000.0, zero_check_time / 1000.0,
           gap_tracker_time / 1000.0, compress_time / 1000.0);
#endif
}

void RamSaver::writePage(WriteInfo&& wi) {
    if (wi.page->filePos == 0) {
        if (mGaps) {
            if (auto gapPos = measure(gap_tracker_time, [&] {
                    return mGaps->allocate(wi.page->sizeOnDisk);
                })) {
                wi.page->filePos = *gapPos;
                ++reused;
            }
        }
        if (wi.page->filePos == 0) {
            wi.page->filePos = mCurrentStreamPos;
            mCurrentStreamPos += wi.page->sizeOnDisk;
            ++appended;
        }
    }

    measure(write_time, [&] {
        base::pwrite(mStreamFd, wi.ptr, size_t(wi.page->sizeOnDisk),
                     wi.page->filePos);
    });
    if (wi.allocated) {
        delete[] wi.ptr;
    }
}

}  // namespace snapshot
}  // namespace android
