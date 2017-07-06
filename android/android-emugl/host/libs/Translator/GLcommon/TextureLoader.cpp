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

#include "GLcommon/TextureLoader.h"

#include <assert.h>

TextureLoader::TextureLoader(android::base::StdioStream&& stream)
    : mStream(std::move(stream)) {}

bool TextureLoader::start() {
    return readIndex();
}

void TextureLoader::loadTexture(uint32_t texId, loader_t loader) {
    android::base::AutoLock scopedLock(mLock);
    assert(mIndex.count(texId));
    fseek(mStream.get(), mIndex[texId], SEEK_SET);
    loader(&mStream);
}

bool TextureLoader::readIndex() {
#if SNAPSHOT_PROFILE > 1
    auto start = android::base::System::get()->getHighResTimeUs();
#endif
    assert(mIndex.size() == 0);
    uint64_t indexPos = mStream.getBe64();
    fseek(mStream.get(), indexPos, SEEK_SET);
    int version = mStream.getBe32();
    if (version != 1) {
        return false;
    }
    uint32_t texCount = mStream.getBe32();
    mIndex.reserve(texCount);
    for (uint32_t i = 0; i < texCount; i++) {
        uint32_t tex = mStream.getBe32();
        int64_t filePos = mStream.getBe64();
        mIndex.emplace(tex, filePos);
    }
#if SNAPSHOT_PROFILE > 1
    printf("Texture readIndex() time: %.03f\n",
           (android::base::System::get()->getHighResTimeUs() - start) / 1000.0);
#endif
    return true;
}
