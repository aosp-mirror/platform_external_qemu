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

#include "android/snapshot/TextureSaver.h"

#include "android/base/files/CompressingStream.h"
#include "android/base/system/System.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility>

using android::base::CompressingStream;
using android::base::System;

namespace android {
namespace snapshot {

TextureSaver::TextureSaver(android::base::StdioStream&& stream)
    : mStream(std::move(stream)) {
    // Put a placeholder for the index offset right now.
    mStream.putBe64(0);
}

TextureSaver::~TextureSaver() {
    done();
}

void TextureSaver::saveTexture(uint32_t texId, const saver_t& saver) {

    if (!mStartTime) {
        mStartTime = System::get()->getHighResTimeUs();
    }

    assert(mIndex.textures.end() ==
           std::find_if(mIndex.textures.begin(), mIndex.textures.end(),
                        [texId](FileIndex::Texture& tex) {
                            return tex.texId == texId;
                        }));
    mIndex.textures.push_back({texId, ftello64(mStream.get())});

    CompressingStream stream(mStream);
    saver(&stream, &mBuffer);
}

void TextureSaver::done() {
    if (mFinished) {
        return;
    }
    mIndex.startPosInFile = ftello64(mStream.get());
    writeIndex();
    mEndTime = System::get()->getHighResTimeUs();
#if SNAPSHOT_PROFILE > 1
    printf("Texture saving time: %.03f\n",
           (mEndTime - mStartTime) / 1000.0);
#endif
    mHasError = ferror(mStream.get()) != 0;
    mFinished = true;
    mStream.close();
}

void TextureSaver::writeIndex() {
#if SNAPSHOT_PROFILE > 1
    auto start = ftello64(mStream.get());
#endif

    mStream.putBe32(static_cast<uint32_t>(mIndex.version));
    mStream.putBe32(static_cast<uint32_t>(mIndex.textures.size()));
    for (const FileIndex::Texture& b : mIndex.textures) {
        mStream.putBe32(b.texId);
        mStream.putBe64(static_cast<uint64_t>(b.filePos));
    }
    auto end = ftello64(mStream.get());
    mDiskSize = uint64_t(end);
#if SNAPSHOT_PROFILE > 1
    printf("texture: index size: %d\n", int(end - start));
#endif

    fseeko64(mStream.get(), 0, SEEK_SET);
    mStream.putBe64(static_cast<uint64_t>(mIndex.startPosInFile));
}

}  // namespace snapshot
}  // namespace android
