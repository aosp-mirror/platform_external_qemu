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
#include "android/base/EnumFlags.h"
#include "android/base/files/StdioStream.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/snapshot/Compressor.h"
#include "android/snapshot/common.h"

#include <atomic>
#include <cstdint>
#include <vector>

namespace android {
namespace snapshot {

using namespace ::android::base::EnumFlags;

class RamSaver {
    DISALLOW_COPY_AND_ASSIGN(RamSaver);

public:
    enum class Flags {
        None = 0,
        Async = 0x1,
        // TODO: add "CopyOnWrite = 0x3  // implies |Async|"
        Compress = 0x4,
    };

    RamSaver(base::StdioStream&& stream, Flags flags);
    ~RamSaver();

    void registerBlock(const RamBlock& block);
    void savePage(int64_t blockOffset, int64_t pageOffset, int32_t pageSize);
    void join();
    bool hasError() const { return mHasError; }
    bool compressed() const { return mIndex.flags & int32_t(IndexFlags::CompressedPages); }
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
    struct QueuedPageInfo {
        int blockIndex;
        int32_t pageIndex;  // == pageOffset / block.pageSize
    };

    // The file structure is as follows:
    //
    // 0: 8 bytes, index offset in the file (indexOffset)
    // 8: first nonzero page as struct FileIndex::Page
    // 8 + first page size: second nonzero page
    // ....
    // indexOffset: struct FileIndex
    // EOF

    struct FileIndex {
        struct Block {
            RamBlock ramBlock;
            struct Page {
                int32_t sizeOnDisk; // 0 -> page is all zeroes
                int64_t filePos;
            };
            std::vector<Page> pages;
        };

        using Flags = IndexFlags;

        int64_t startPosInFile;
        int32_t version = 1;
        int32_t flags = int32_t(Flags::Empty);
        int32_t totalPages = 0;
        std::vector<Block> blocks;

        void clear();
    };

    void passToSaveHandler(QueuedPageInfo&& pi);
    base::WorkerProcessingResult savePageInWorker(QueuedPageInfo&& pi);
    bool handlePageSave(QueuedPageInfo&& pi);
    void writeIndex();
    void writePage(const QueuedPageInfo& pi,
                   const void* dataPtr,
                   int32_t dataSize);

    base::StdioStream mStream;
    int mStreamFd;
    Flags mFlags;
    bool mJoined = false;
    bool mHasError = false;
    int mLastBlockIndex = 0;
    std::atomic<int64_t> mCurrentStreamPos {8};

    base::Optional<Compressor<QueuedPageInfo>> mCompressor;
    base::Optional<base::WorkerThread<QueuedPageInfo>> mSavingWorker;

    FileIndex mIndex;
    uint64_t mDiskSize = 0;

    base::System::Duration mStartTime =
            base::System::get()->getHighResTimeUs();
    base::System::Duration mEndTime = 0;
};

}  // namespace snapshot
}  // namespace android
