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
#include "android/base/files/StdioStream.h"
#include "android/base/Optional.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/snapshot/common.h"
#include "android/snapshot/MemoryWatch.h"

#include <cstdint>
#include <vector>

namespace android {
namespace snapshot {

class RamLoader {
    DISALLOW_COPY_AND_ASSIGN(RamLoader);
public:
    RamLoader(ZeroChecker zeroChecker);
    ~RamLoader();

    void registerBlock(const RamBlock& block);
    bool startLoading(base::StdioStream&& stream);
    bool wasStarted() const { return mWasStarted; }

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

        std::vector<Block> blocks;
        Pages pages;
    };

    bool readIndex();
    bool registerPageWatches();

    void zeroOutPage(const RamBlock& block, uint32_t offset);
    uint8_t* pagePtr(const Page& page) const;
    uint32_t pageSize(const Page& page) const;
    Page& page(void* ptr);

    bool readDataFromDisk(Page* pagePtr, uint8_t* preallocatedBuffer = nullptr);
    void fillPageData(Page* pagePtr);

    void readerWorker();
    MemoryAccessWatch::IdleCallbackResult backgroundPageLoad();
    void interruptReading();

    bool readAllPages();

    base::StdioStream mStream;
    int mStreamFd;  // An FD for the |mStream|'s underlying open file.
    ZeroChecker mZeroChecker;
    bool mWasStarted = false;

    base::Optional<MemoryAccessWatch> mAccessWatch;
    base::FunctorThread mReaderThread;
    Pages::iterator mBackgroundPageIt;
    bool mSentEndOfPagesMarker = false;
    base::MessageChannel<Page*, 32> mReadingQueue;
    base::MessageChannel<Page*, 32> mReadDataQueue;

    FileIndex mIndex;

#if SNAPSHOT_PROFILE > 1
    base::System::WallDuration mStartTime;
#endif
};

}  // namespace snapshot
}  // namespace android
