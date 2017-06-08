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
#include "android/snapshot/common.h"

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
    };

    using ZeroChecker = bool (*)(const uint8_t* ptr, int size);

    RamSaver(base::StdioStream&& stream,
             ZeroChecker zeroChecker,
             Flags flags = Flags::Async);
    ~RamSaver();

    void registerBlock(const RamBlock& block);
    void savePage(int64_t blockOffset, int64_t pageOffset, int32_t pageSize);
    void join();

private:
    struct QueuedPageInfo {
        int blockIndex;
        int32_t pageIndex;  // == pageOffset / block.pageSize
    };

    struct FileIndex {
        struct Block {
            RamBlock ramBlock;
            std::vector<int32_t> zeroPageIndexes;
            struct Page {
                int32_t index;
                int64_t filePos;
            };
            std::vector<Page> nonZeroPages;
        };

        int64_t startPosInFile;
        int32_t version = 1;
        int32_t totalNonzeroPages = 0;
        std::vector<Block> blocks;
    };

    void done();
    void savingThreadWorker();
    void passToSaveHandler(const QueuedPageInfo& pi);
    bool handlePageSave(const QueuedPageInfo& pi);
    void writeIndex();

    base::StdioStream mStream;
    ZeroChecker mZeroChecker;
    Flags mFlags;

    int mLastBlockIndex = 0;

    base::MessageChannel<QueuedPageInfo, 512 * 1024> mPagesQueue;
    base::FunctorThread mSavingThread;
    FileIndex mIndex;

#if SNAPSHOT_PROFILE > 1
    base::System::WallDuration mStartTime =
            base::System::get()->getHighResTimeUs();
#endif
};

}  // namespace snapshot
}  // namespace android
