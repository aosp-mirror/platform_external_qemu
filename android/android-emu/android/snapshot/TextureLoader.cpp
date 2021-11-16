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

#include "android/snapshot/TextureLoader.h"

#include "android/base/EintrWrapper.h"
#include "android/base/files/DecompressingStream.h"

#include <assert.h>

using android::base::DecompressingStream;

namespace android {
namespace snapshot {

TextureLoader::TextureLoader(android::base::StdioStream&& stream)
    : mStream(std::move(stream)) {}

bool TextureLoader::start() {
    if (mStarted) {
        return !mHasError;
    }

    mStarted = true;
    bool res = readIndex();
    if (!res) {
        mHasError = true;
        return false;
    }
    return true;
}

void TextureLoader::loadTexture(uint32_t texId, const loader_t& loader) {
    android::base::AutoLock scopedLock(mLock);
    assert(mIndex.count(texId));
    HANDLE_EINTR(fseeko64(mStream.get(), mIndex[texId], SEEK_SET));
    switch (mVersion) {
        case 1:
            loader(&mStream);
            break;
        case 2: {
            DecompressingStream stream(mStream);
            loader(&stream);
        }
    }
    if (ferror(mStream.get())) {
        mHasError = true;
    }
}

bool TextureLoader::readIndex() {
#if SNAPSHOT_PROFILE > 1
    auto start = android::base::System::get()->getHighResTimeUs();
#endif
    assert(mIndex.size() == 0);
    base::System::FileSize size;
    if (base::System::get()->fileSize(fileno(mStream.get()), &size)) {
        mDiskSize = size;
    }
    auto indexPos = mStream.getBe64();
    HANDLE_EINTR(fseeko64(mStream.get(), static_cast<int64_t>(indexPos), SEEK_SET));
    mVersion = mStream.getBe32();
    if (mVersion < 1 || mVersion > 2) {
        return false;
    }
    uint32_t texCount = mStream.getBe32();
    mIndex.reserve(texCount);
    for (uint32_t i = 0; i < texCount; i++) {
        uint32_t tex = mStream.getBe32();
        uint64_t filePos = mStream.getBe64();
        mIndex.emplace(tex, filePos);
    }
#if SNAPSHOT_PROFILE > 1
    dprint("Texture readIndex() time: %.03f",
           (android::base::System::get()->getHighResTimeUs() - start) / 1000.0);
#endif
    return true;
}

}  // namespace snapshot
}  // namespace android
