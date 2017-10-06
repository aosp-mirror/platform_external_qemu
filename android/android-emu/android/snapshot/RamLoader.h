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

#pragma once

#include "android/base/Compiler.h"
#include "android/base/Optional.h"
#include "android/base/files/StdioStream.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/base/threads/ThreadPool.h"
#include "android/snapshot/common.h"
#include "android/snapshot/MemoryWatch.h"
#include "android/snapshot/PageMap.h"

#include <atomic>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <vector>

namespace android {
namespace snapshot {

class RamLoader {
    DISALLOW_COPY_AND_ASSIGN(RamLoader);

public:
    RamLoader(base::StdioStream&& stream);
    ~RamLoader();

    bool isRegistered(void* ptr) {
        for (const auto& it: mPageMaps) {
            if (it.has(ptr)) return true;
        }
        return false;
    }

    bool isPageDirty(void* ptr) {
        if (!mIsDirtyTracking) return true;
        if (!isRegistered(ptr)) return true;
        for (const auto& it: mPageMaps) {
            if (it.has(ptr)) {
                return it.lookup(ptr);
            }
        }
        return true;
    }

    void loadRam(void* ptr, uint64_t size);
    void dirtyRam(void* ptr, uint64_t size);
    void registerBlock(const RamBlock& block);
    bool start(bool isQuickboot);
    bool wasStarted() const { return mWasStarted; }
    void join();

    bool hasError() const { return mHasError; }
    bool onDemandEnabled() const { return mAccessWatch; }
    bool onDemandLoadingComplete() const {
        return mLoadingCompleted.load(std::memory_order_relaxed);
    }
    bool compressed() const {
        return (mIndex.flags & IndexFlags::CompressedPages) != 0;
    }
    uint64_t diskSize() const { return mDiskSize; }

private:
    using DirtyPageMap = std::unordered_map<void*, uint64_t>;
    using RangeMap = std::map<uintptr_t, uint64_t>;

    enum class State : uint8_t { Empty, Reading, Read, Filling, Filled, Error };

    struct Page;
    using Pages = std::vector<Page>;

    struct FileIndex {
        struct Block {
            RamBlock ramBlock;
            Pages::iterator pagesBegin;
            Pages::iterator pagesEnd;
        };

        using Blocks = std::vector<Block>;

        IndexFlags flags;
        Blocks blocks;
        Pages pages;

        void clear();
    };

    bool readIndex();
    void readBlockPages(base::Stream* stream,
                        FileIndex::Blocks::iterator blockIt,
                        bool compressed,
                        int64_t* runningFilePos,
                        int32_t* prevPageSizeOnDisk);
    bool registerPageWatches();

    void zeroOutPage(const Page& page);
    uint8_t* pagePtr(const Page& page) const;
    uint32_t pageSize(const Page& page) const;
    Page& page(void* ptr);

    void loadRamPage(void* ptr);
    bool readDataFromDisk(Page* pagePtr, uint8_t* preallocatedBuffer = nullptr);
    void fillPageData(Page* pagePtr);

    void readerWorker();
    MemoryAccessWatch::IdleCallbackResult backgroundPageLoad();
    MemoryAccessWatch::IdleCallbackResult fillPageInBackground(Page* page);
    void interruptReading();

    bool readAllPages();
    void startDecompressor();

    base::StdioStream mStream;
    int mStreamFd;  // An FD for the |mStream|'s underlying open file.
    bool mWasStarted = false;
    std::atomic<bool> mHasError{false};

    base::Optional<MemoryAccessWatch> mAccessWatch;
    base::FunctorThread mReaderThread;
    Pages::iterator mBackgroundPageIt;
    bool mSentEndOfPagesMarker = false;
    bool mJoining = false;
    base::MessageChannel<Page*, 32> mReadingQueue;
    base::MessageChannel<Page*, 32> mReadDataQueue;

    base::Optional<base::ThreadPool<Page*>> mDecompressor;

    FileIndex mIndex;
    uint64_t mDiskSize = 0;

#if SNAPSHOT_PROFILE > 1
    base::System::WallDuration mStartTime;
#endif

    // Assumed to be a power of 2 for convenient
    // rounding and aligning
    uint64_t mPageSize = 4096;
    // Flag to stop lazy RAM loading if loading has
    // completed.
    std::atomic<bool> mLoadingCompleted{false};

    // Whether or not this ram load is part of
    // quickboot load.
    bool mIsQuickboot = false;

    // Information about dirty page tracking:
    // whether we are doing so, and bitmaps holding
    // the data.
    bool mIsDirtyTracking = false;
    std::vector<PageMap> mPageMaps;
};

}  // namespace snapshot
}  // namespace android
