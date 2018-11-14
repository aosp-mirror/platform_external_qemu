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

#include "android/base/Optional.h"
#include "android/base/StringView.h"
#include "android/base/files/FileShareOpen.h"
#include "android/emulation/AndroidAsyncMessagePipe.h"

#include <vector>

namespace android {
namespace snapshot {

void createCheckpoint(android::AsyncMessagePipeHandle pipe,
                      android::base::StringView name);

void gotoCheckpoint(
        AsyncMessagePipeHandle pipe,
        android::base::StringView name,
        android::base::StringView metadata,
        android::base::Optional<android::base::FileShare> shareMode);

void forkReadOnlyInstances(android::AsyncMessagePipeHandle pipe, int forkTotal);
void doneInstance(android::AsyncMessagePipeHandle pipe,
        android::base::StringView metadata);

void onOffworldSave(base::Stream* stream);
void onOffworldLoad(base::Stream* stream);

}  // namespace snapshot
}  // namespace android
