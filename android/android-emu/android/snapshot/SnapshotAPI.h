// Copyright 2018 The Android Open Source Project
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

#include "aemu/base/Optional.h"

#include "aemu/base/files/FileShareOpen.h"
#include "android/emulation/AndroidAsyncMessagePipe.h"

#include <string_view>
#include <vector>

namespace android {
namespace snapshot {

void createCheckpoint(android::AsyncMessagePipeHandle pipe,
                      std::string_view name);

void gotoCheckpoint(
        AsyncMessagePipeHandle pipe,
        std::string_view name,
        std::string_view metadata,
        android::base::Optional<android::base::FileShare> shareMode);

void forkReadOnlyInstances(android::AsyncMessagePipeHandle pipe, int forkTotal);
void doneInstance(android::AsyncMessagePipeHandle pipe,
                  std::string_view metadata);

void onOffworldSave(base::Stream* stream);
void onOffworldLoad(base::Stream* stream);

}  // namespace snapshot
}  // namespace android
