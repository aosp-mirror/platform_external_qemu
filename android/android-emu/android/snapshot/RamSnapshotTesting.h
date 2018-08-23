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

#pragma once

#include "android/base/AlignedBuf.h"
#include "android/base/StringView.h"
#include "android/snapshot/RamLoader.h"
#include "android/snapshot/RamSaver.h"

#include <vector>

namespace android {
namespace snapshot {

constexpr int kTestingPageSize = 4096;

using TestRamBuffer = AlignedBuf<uint8_t, kTestingPageSize>;

RamBlock makeRam(android::base::StringView name,
                 uint8_t* hostPtr,
                 int64_t size);

void saveRamSingleBlock(const RamSaver::Flags flags,
                        const RamBlock& block,
                        android::base::StringView filename);

void loadRamSingleBlock(const RamBlock& block,
                        android::base::StringView filename);

void incrementalSaveSingleBlock(const RamSaver::Flags flags,
                                const RamBlock& blockToLoad,
                                const RamBlock& blockToSave,
                                android::base::StringView filename);

TestRamBuffer generateRandomRam(size_t numPages, float zeroPageChance, int seed = 0);

void randomMutateRam(TestRamBuffer& ram, float noChangeChance, float zeroPageChance, int seed = 0);

}  // namespace snapshot
}  // namespace android
