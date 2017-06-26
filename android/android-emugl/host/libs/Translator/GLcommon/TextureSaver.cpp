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

#include "GLcommon/TextureSaver.h"

#include "android/base/system/System.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility>

using android::base::System;

TextureSaver::TextureSaver(android::base::StdioStream&& stream)
    : mStream(std::move(stream)) {
    // Put a placeholder for the index offset right now.
    mStream.putBe64(0);
}

TextureSaver::~TextureSaver() {
    done();
}

void TextureSaver::saveTexture(uint32_t texId, saver_t saver) {
    assert(mIndex.texturs.end() ==
           std::find_if(
                   mIndex.texture.begin(),
                   mIndex.texture.end(), [texId](FileIndex::Texture & tex) {
                       return tex.texId == texId;
                   }));
    mIndex.textures.push_back({texId, (int64_t)ftell(mStream.get())});
    saver(&mStream, &mBuffer);
}

void TextureSaver::done() {
    mIndex.startPosInFile = ftell(mStream.get());
    writeIndex();
#if SNAPSHOT_PROFILE > 1
    printf("Texture saving time: %.03f\n",
           (System::get()->getHighResTimeUs() - mStartTime) / 1000.0);
#endif
}

void TextureSaver::writeIndex() {
#if SNAPSHOT_PROFILE > 1
    auto start = ftell(mStream.get());
#endif

    mStream.putBe32(mIndex.version);
    mStream.putBe32(mIndex.textures.size());
    for (const FileIndex::Texture& b : mIndex.textures) {
        mStream.putBe32(b.texId);
        mStream.putBe64(b.filePos);
    }
#if SNAPSHOT_PROFILE > 1
    auto end = ftell(mStream.get());
    printf("texture: index size: %d\n", (int)(end - start));
#endif

    fseek(mStream.get(), 0, SEEK_SET);
    mStream.putBe64(mIndex.startPosInFile);
}
