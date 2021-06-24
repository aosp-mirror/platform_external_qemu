/*
* Copyright (C) 2017 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#pragma once

#include "android/base/containers/SmallVector.h"
#include "android/base/export.h"
#include "android/base/files/StdioStream.h"
#include "android/base/system/System.h"
#include "android/snapshot/common.h"

#include <functional>
#include <vector>

namespace android {
namespace snapshot {

class ITextureSaver {
    DISALLOW_COPY_AND_ASSIGN(ITextureSaver);

protected:
    ~ITextureSaver() = default;

public:
    ITextureSaver() = default;

    using Buffer = android::base::SmallVector<unsigned char>;
    using saver_t = std::function<void(android::base::Stream*, Buffer*)>;

    // Save texture to a stream as well as update the index
    virtual void saveTexture(uint32_t texId, const saver_t& saver) = 0;
    virtual bool hasError() const = 0;
    virtual uint64_t diskSize() const = 0;
    virtual bool compressed() const = 0;
    virtual bool getDuration(base::System::Duration* duration) = 0;
};

class TextureSaver final : public ITextureSaver {
    DISALLOW_COPY_AND_ASSIGN(TextureSaver);

public:
    AEMU_EXPORT TextureSaver(android::base::StdioStream&& stream);
    AEMU_EXPORT ~TextureSaver();
    AEMU_EXPORT void saveTexture(uint32_t texId, const saver_t& saver) override;
    AEMU_EXPORT void done();

    AEMU_EXPORT bool hasError() const override { return mHasError; }
    AEMU_EXPORT uint64_t diskSize() const override { return mDiskSize; }
    AEMU_EXPORT bool compressed() const override { return mIndex.version > 1; }

    // getDuration():
    // Returns true if there was save with measurable time
    // (and writes it to |duration| if |duration| is not null),
    // otherwise returns false.
    AEMU_EXPORT bool getDuration(base::System::Duration* duration) override {
        if (mEndTime < mStartTime) {
            return false;
        }

        if (duration) {
            *duration = mEndTime - mStartTime;
        }
        return true;
    }

private:
    struct FileIndex {
        struct Texture {
            uint32_t texId;
            int64_t filePos;
        };

        int64_t startPosInFile;
        int32_t version = 2;
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

    android::base::System::Duration mStartTime = 0;
    android::base::System::Duration mEndTime = 0;
};

}  // namespace snapshot
}  // namespace android
