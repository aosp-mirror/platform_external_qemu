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
#include "android/snapshot/MemoryWatch.h"
#include "android/snapshot/common.h"

#include <atomic>
#include <cstdint>
#include <vector>

namespace android {
namespace snapshot {

class RamLoader {
    DISALLOW_COPY_AND_ASSIGN(RamLoader);

public:
    RamLoader(base::StdioStream&& stream);
    ~RamLoader();

    void loadRam(void* ptr, uint64_t size);
    void registerBlock(const RamBlock& block);
    bool start(bool isQuickboot);
    bool wasStarted() const { return mWasStarted; }

    void join(); // Finishes the current load.
    void interrupt();  // Cancels the current load.

    bool hasError() const { return mHasError; }
    bool onDemandEnabled() const { return mAccessWatch; }
    bool onDemandLoadingComplete() const {
        return mLoadingCompleted.load(std::memory_order_relaxed);
    }
    bool compressed() const {
        return (mIndex.flags & IndexFlags::CompressedPages) != 0;
    }
    uint64_t diskSize() const { return mDiskSize; }

    // getDuration():
    // Returns true if there was save with measurable time
    // (and writes it to |duration| if |duration| is not null),
    // otherwise returns false.
    bool getDuration(base::System::Duration* duration) {
        if (mEndTime < mStartTime) {
            return false;
        }

        if (duration) {
            *duration = mEndTime - mStartTime;
        }
        return true;
    }

private:
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

    base::System::Duration mStartTime = 0;
    base::System::Duration mEndTime = 0;

    // Assumed to be a power of 2 for convenient
    // rounding and aligning
    uint64_t mPageSize = 4096;
    // Flag to stop lazy RAM loading if loading has
    // completed.
    std::atomic<bool> mLoadingCompleted{false};

    // Whether or not this ram load is part of
    // quickboot load.
    bool mIsQuickboot = false;
};

}  // namespace snapshot
}  // namespace android
