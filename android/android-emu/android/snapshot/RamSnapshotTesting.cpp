// Copyright (C) 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/snapshot/RamSnapshotTesting.h"

#include "android/base/files/StdioStream.h"

#include <cstdlib>
#include <random>

using android::base::StdioStream;

#ifndef userSettingIsDontSaveSnapshot

// Needed because Quickboot statically depends on Qt and there is a link
// failure if the function is not defined.
bool userSettingIsDontSaveSnapshot() {
    return false;
}

#endif

namespace android {
namespace snapshot {

using TestRamBuffer = AlignedBuf<uint8_t, kTestingPageSize>;

void saveRamSingleBlock(const RamSaver::Flags flags,
                        const RamBlock& block,
                        android::base::StringView filename) {
    RamSaver s(filename.c_str(), flags, nullptr, true);

    const int blockIndex = 0;
    s.registerBlock(block);

    for (int64_t i = block.startOffset; i < block.startOffset + block.totalSize;
         i += block.pageSize) {
        s.savePage(blockIndex, i, block.pageSize);
    }

    s.join();
}

void loadRamSingleBlock(const RamBlock& block,
                        android::base::StringView filename) {
    auto ram = fopen(filename.c_str(), "rb");

    RamLoader::RamBlockStructure emptyRamBlockStructure = {};

    // Disallow on-demand load for now.
    RamLoader ramLoader(StdioStream(ram, StdioStream::kOwner),
                        RamLoader::Flags::None, emptyRamBlockStructure);

    ramLoader.registerBlock(block);

    ramLoader.start(false);
    ramLoader.join();
}

TestRamBuffer generateRandomRam(size_t numPages, float zeroPageChance) {
    std::default_random_engine generator;
    // Use a consistent seed value to avoid flakes
    generator.seed(0);

    // Distributions for the random patterns and zero pages
    std::uniform_int_distribution<char> patternDistribution(0, 255);
    std::bernoulli_distribution zeroPageDistribution(zeroPageChance);

    TestRamBuffer res(numPages * kTestingPageSize);

    uint8_t* ram = res.data();

    for (size_t i = 0; i < numPages; ++i) {
        uint8_t* currentPage = ram + i * kTestingPageSize;
        if (zeroPageDistribution(generator)) {
            memset(currentPage, 0x0, kTestingPageSize);
        } else {
            char pattern = patternDistribution(generator);
            memset(ram + i * kTestingPageSize, pattern, kTestingPageSize);
        }
    }

    return res;
}

}  // namespace snapshot
}  // namespace android
