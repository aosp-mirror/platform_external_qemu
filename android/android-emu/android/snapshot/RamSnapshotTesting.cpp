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
#include "android/utils/file_io.h"

#include <cstdlib>
#include <random>

using android::base::c_str;
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

static void mockQemuPageSave(RamSaver& saver, const RamBlock& block) {
    const int blockIndex = 0;

    for (int64_t i = block.startOffset; i < block.startOffset + block.totalSize;
         i += block.pageSize) {
        saver.savePage(blockIndex, i, block.pageSize);
    }
}

RamBlock makeRam(android::base::StringView name,
                 uint8_t* data,
                 int64_t size) {
    return {
        c_str(name),
        0x0,
        data,
        size,
        kTestingPageSize,
        0 /* flags */,
        "" /* path */,
        false /* readonly */,
        false /* initFromRamFile */ };
}

void saveRamSingleBlock(const RamSaver::Flags flags,
                        const RamBlock& block,
                        android::base::StringView filename) {
    RamSaver s(filename, flags, nullptr, true);

    s.registerBlock(block);

    mockQemuPageSave(s, block);

    s.join();
}

void loadRamSingleBlock(const RamBlock& block,
                        android::base::StringView filename) {
    auto ram = android_fopen(c_str(filename), "rb");

    RamLoader::RamBlockStructure emptyRamBlockStructure = {};

    // Disallow on-demand load for now.
    RamLoader ramLoader(StdioStream(ram, StdioStream::kOwner),
                        RamLoader::Flags::None, emptyRamBlockStructure);

    ramLoader.registerBlock(block);

    ramLoader.start(false);
    ramLoader.join();
}

void incrementalSaveSingleBlock(const RamSaver::Flags flags,
                                const RamBlock& blockToLoad,
                                const RamBlock& blockToSave,
                                android::base::StringView filename) {
    auto ram = android_fopen(c_str(filename), "rb");

    RamLoader::RamBlockStructure emptyRamBlockStructure = {};

    // Disallow on-demand load for now.
    RamLoader ramLoader(StdioStream(ram, StdioStream::kOwner),
                        RamLoader::Flags::None, emptyRamBlockStructure);

    ramLoader.registerBlock(blockToLoad);

    ramLoader.start(false);

    RamSaver s(filename, flags, &ramLoader, true);

    s.registerBlock(blockToSave);

    mockQemuPageSave(s, blockToSave);

    s.join();
}

TestRamBuffer generateRandomRam(size_t numPages, float zeroPageChance, int seed) {
    std::default_random_engine generator;
    // Use a consistent seed value to avoid flakes
    generator.seed(seed);

    // Distributions for the random patterns and zero pages
#ifdef _MSC_VER
    // MSVC throws the following error if type is char:
    //
    // invalid template argument for uniform_int_distribution: N4659 29.6.1.1 [rand.req.genl]/1e requires one of short, int, long, long long, unsigned short, unsigned int, unsigned long, or unsigned long long
    std::uniform_int_distribution<unsigned short> patternDistribution(0, 255);
#else
    std::uniform_int_distribution<uint8_t> patternDistribution(0, 255);
#endif
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

void randomMutateRam(TestRamBuffer& ram, float noChangeChance, float zeroPageChance, int seed) {
    std::default_random_engine generator;
    generator.seed(seed);

#ifdef _MSC_VER
    std::uniform_int_distribution<unsigned short> patternDistribution(0, 255);
#else
    std::uniform_int_distribution<uint8_t> patternDistribution(0, 255);
#endif
    std::bernoulli_distribution noChangeDistribution(noChangeChance);
    std::bernoulli_distribution zeroPageDistribution(zeroPageChance);

    for (size_t i = 0; i < ram.size(); i += kTestingPageSize) {
        if (noChangeDistribution(generator)) continue;

        if (zeroPageDistribution(generator)) {
            memset(ram.data() + i, 0x0, kTestingPageSize);
        } else {
            char pattern = patternDistribution(generator);
            memset(ram.data() + i, pattern, kTestingPageSize);
        }
    }
}

}  // namespace snapshot
}  // namespace android
