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
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/base/threads/Thread.h"
#include "android/snapshot/common.h"

#include <functional>
#include <memory>
#include <unordered_map>

namespace android {
namespace snapshot {

class TextureLoader {
    DISALLOW_COPY_AND_ASSIGN(TextureLoader);

public:
    using LoaderThreadPtr = std::shared_ptr<android::base::Thread>;

    TextureLoader(android::base::StdioStream&& stream);

    using loader_t = std::function<void(android::base::Stream*)>;
    bool start();
    // Move file position to texId and trigger loader
    void loadTexture(uint32_t texId, const loader_t& loader);

    bool hasError() const { return mHasError; }

    void acquireLoaderThread(LoaderThreadPtr thread) {
        mLoaderThread = std::move(thread);
    }

    void join() {
        if (mLoaderThread) {
            mLoaderThread->wait();
            mLoaderThread.reset();
        }
    }

private:
    bool readIndex();

    android::base::StdioStream mStream;
    std::unordered_map<uint32_t, int64_t> mIndex;
    android::base::Lock mLock;
    bool mStarted = false;
    bool mHasError = false;
#if SNAPSHOT_PROFILE > 1
    android::base::System::WallDuration mStartTime;
#endif
    LoaderThreadPtr mLoaderThread;
};

}  // namespace snapshot
}  // namespace android
