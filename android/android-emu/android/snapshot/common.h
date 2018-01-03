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

#pragma once

#include "android/base/StringView.h"
#include "android/emulation/control/vm_operations.h"
#include "android/snapshot/interface.h"

#include <memory>
#include <stdint.h>

namespace android {
namespace snapshot {

class ITextureSaver;
class TextureSaver;
class ITextureLoader;
class TextureLoader;
using ITextureSaverPtr = std::shared_ptr<ITextureSaver>;
using ITextureLoaderPtr = std::shared_ptr<ITextureLoader>;
using ITextureLoaderWPtr = std::weak_ptr<ITextureLoader>;

using RamBlock = ::SnapshotRamBlock;

enum class IndexFlags {
    Empty = 0,
    CompressedPages = 0x01,
};

enum class OperationStatus {
    NotStarted = SNAPSHOT_STATUS_NOT_STARTED,
    Ok = SNAPSHOT_STATUS_OK,
    Error = SNAPSHOT_STATUS_ERROR
};

enum class FailureReason {
    Empty = 0,

    CorruptedData,
    NoSnapshotPb,
    BadSnapshotPb,
    IncompatibleVersion,
    NoRamFile,
    NoTexturesFile,
    SnapshotsNotSupported,

    UnrecoverableErrorLimit = 10000,

    NoSnapshotInImage,
    ConfigMismatchHostHypervisor,
    ConfigMismatchHostGpu,
    ConfigMismatchRenderer,
    ConfigMismatchFeatures,
    ConfigMismatchAvd,
    SystemImageChanged,

    ValidationErrorLimit = 20000,

    InternalError,
    EmulationEngineFailed,
    RamFailed,
    TexturesFailed,
    AdbOffline,

    InProgressLimit = 30000,
};

template <class Operation>
bool isComplete(const Operation& op) {
    return op.status() != OperationStatus::NotStarted;
}

bool isBufferZeroed(const void* ptr, int32_t size);

constexpr int32_t kDefaultPageSize = 4096;

}  // namespace snapshot
}  // namespace android
