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

#include "android/base/containers/SmallVector.h"
#include "android/base/files/StdioStream.h"
#include "android/base/system/System.h"
#include "android/snapshot/common.h"

#include <functional>
#include <vector>

namespace android {
namespace snapshot {

class TextureSaver {
    DISALLOW_COPY_AND_ASSIGN(TextureSaver);

public:
    TextureSaver(android::base::StdioStream&& stream);
    ~TextureSaver();
    using Buffer = android::base::SmallVector<unsigned char>;
    using saver_t = std::function<void(android::base::Stream*, Buffer*)>;
    // Save texture to mStream as well as update the index
    void saveTexture(uint32_t texId, const saver_t& saver);
    void done();

    bool hasError() const { return mHasError; }
    uint64_t diskSize() const { return mDiskSize; }

private:
    struct FileIndex {
        struct Texture {
            uint32_t texId;
            int64_t filePos;
        };

        int64_t startPosInFile;
        int32_t version = 1;
        std::vector<Texture> textures;
    };

    void writeIndex();

    android::base::StdioStream mStream;
    // A buffer for fetching data from GPU memory to RAM.
    android::base::SmallFixedVector<unsigned char, 128> mBuffer;

    FileIndex mIndex;
    uint64_t mDiskSize = 0;
    bool mFinished = false;
    bool mHasError = false;

#if SNAPSHOT_PROFILE > 1
    android::base::System::WallDuration mStartTime =
            android::base::System::get()->getHighResTimeUs();
#endif
};

}  // namespace snapshot
}  // namespace android
