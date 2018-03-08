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
#include "android/base/files/MemStream.h"
#include "android/base/files/StdioStream.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/base/threads/ThreadPool.h"
#include "android/snapshot/GapTracker.h"
#include "android/snapshot/MemoryWatch.h"
#include "android/snapshot/Snapshot.h"
#include "android/snapshot/common.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace android {
namespace snapshot {

class RamLoader {
    DISALLOW_COPY_AND_ASSIGN(RamLoader);

public:
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

        int version;
        RamIndexFlags flags;
        int64_t offset;
        std::vector<bool> usedParents;
        Blocks blocks;
        Pages pages;

        void clear();

        bool hasParents() const {
            return nonzero(flags & RamIndexFlags::Incremental);
        }
        bool compressed() const {
            return nonzero(flags & RamIndexFlags::CompressedPages);
        }

        const Page* findPage(int blockIdx, int pageIdx) const;
    };

    RamLoader(const Snapshot& snapshot, OperationFlags flags);
    ~RamLoader();

    void loadRam(void* ptr, uint64_t size);
    void registerBlock(const RamBlock& block);
    bool start();
    bool wasStarted() const { return mWasStarted; }
    void join();
    void interrupt();

    bool hasError() const { return mHasError; }
    void invalidateGaps() { mGaps.reset(nullptr); }
    bool hasGaps() const { return mGaps != nullptr; }
    bool onDemandEnabled() const { return mOnDemandEnabled; }
    bool onDemandLoadingComplete() const {
        return mLoadingCompleted.load(std::memory_order_relaxed);
    }
    bool compressed() const { return mIndex.compressed(); }
    uint64_t diskSize() const { return mDiskSize; }
    int version() const { return mIndex.version; }

    const FileIndex& index() const { return mIndex; }

    void acquireGapTracker(GapTracker::Ptr gaps) { mGaps = std::move(gaps); }
    GapTracker::Ptr releaseGapTracker() { return std::move(mGaps); }

    bool getDuration(base::System::Duration* duration) {
        if (mEndTime < mStartTime) {
            return false;
        }

        if (duration) {
            *duration = mEndTime - mStartTime;
        }
        return true;
    }

    static std::pair<FileIndex, GapTracker::Ptr> loadIndex(
            const Snapshot& snapshot,
            base::System::FileSize* outSize = nullptr);

private:
    bool readIndex();

    static bool readIndex(const Snapshot& snapshot,
                          base::Stream& indexStream,
                          FileIndex& index,
                          GapTracker::Ptr& gaps,
                          std::vector<base::StdioStream>* streams);
    static bool readIndexPages(base::Stream& indexStream,
                               uint8_t fileNo,
                               FileIndex& index);
    static void readBlockPages(base::Stream* stream,
                               FileIndex& index,
                               FileIndex::Blocks::iterator blockIt,
                               int64_t* runningFilePos,
                               int32_t* prevPageSizeOnDisk,
                               int fileIndex,
                               bool incremental);
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

    std::vector<base::StdioStream> mStreams;
    // FDs for the |mStreams|' underlying open files.
    std::vector<int> mStreamFds;

    bool mWasStarted = false;
    std::atomic<bool> mHasError{false};

    base::Optional<MemoryAccessWatch> mAccessWatch;
    base::FunctorThread mReaderThread;
    Pages::iterator mBackgroundPageIt;
    bool mSentEndOfPagesMarker = false;
    bool mJoining = false;
    bool mOnDemandEnabled = false;
    base::MessageChannel<Page*, 32> mReadingQueue;
    base::MessageChannel<Page*, 32> mReadDataQueue;

    base::Optional<base::ThreadPool<Page*>> mDecompressor;

    FileIndex mIndex;
    GapTracker::Ptr mGaps;
    uint64_t mDiskSize = 0;
    uint64_t mIndexPos = 0;

    base::System::Duration mStartTime = 0;
    base::System::Duration mEndTime = 0;

    // Assumed to be a power of 2 for convenient rounding and aligning
    uint64_t mPageSize = kDefaultPageSize;
    // Flag to stop lazy RAM loading if loading has completed.
    std::atomic<bool> mLoadingCompleted{false};

    OperationFlags mFlags;
    const Snapshot& mSnapshot;
};

struct RamLoader::Page {
    std::atomic<uint8_t> state{uint8_t(State::Empty)};
    bool filled = false;
    uint8_t fileIndex;
    uint16_t blockIndex;
    uint16_t sizeOnDisk;
    uint64_t filePos;
    std::array<char, 16> hash;
    uint8_t* data;

    Page() = default;
    Page(RamLoader::State state) : state(uint8_t(state)) {}
    Page(Page&& other)
        : state(other.state.load(std::memory_order_relaxed)),
          filled(other.filled),
          fileIndex(other.fileIndex),
          blockIndex(other.blockIndex),
          sizeOnDisk(other.sizeOnDisk),
          filePos(other.filePos),
          hash(other.hash),
          data(other.data) {}

    Page& operator=(Page&& other) {
        state.store(other.state.load(std::memory_order_relaxed),
                    std::memory_order_relaxed);
        filled = other.filled;
        fileIndex = other.fileIndex;
        blockIndex = other.blockIndex;
        sizeOnDisk = other.sizeOnDisk;
        filePos = other.filePos;
        hash = other.hash;
        data = other.data;
        return *this;
    }

    bool zeroed() const { return sizeOnDisk == 0; }
};

}  // namespace snapshot
}  // namespace android
