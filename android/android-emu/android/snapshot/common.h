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
#include <string>
#include <stdint.h>

struct SnapshotRamBlock {
    const char* id;
    int64_t startOffset;
    uint8_t* hostPtr;
    int64_t totalSize;
    int32_t pageSize;
    uint32_t flags;
    std::string path;
    bool readonly;
    bool needRestoreFromRamFile;
};

namespace android {
namespace snapshot {

class ITextureSaver;
class TextureSaver;
class ITextureLoader;
class TextureLoader;
using ITextureSaverPtr = std::shared_ptr<ITextureSaver>;
using ITextureLoaderPtr = std::shared_ptr<ITextureLoader>;
using ITextureLoaderWPtr = std::weak_ptr<ITextureLoader>;

// Taken from exec.c, these #defines
// are for the |flags| field in SnapshotRamBlock.
#define SNAPSHOT_RAM_MAPPED_SHARED (1 << 1)
#define SNAPSHOT_RAM_MAPPED (1 << 3)
#define SNAPSHOT_RAM_USER_BACKED (1 << 4)

using RamBlock = ::SnapshotRamBlock;

enum class IndexFlags {
    Empty = 0,
    CompressedPages = 0x01,
    SeparateBackingStore = 0x02,
};

enum class OperationStatus {
    NotStarted = SNAPSHOT_STATUS_NOT_STARTED,
    Ok = SNAPSHOT_STATUS_OK,
    Error = SNAPSHOT_STATUS_ERROR,
    Canceled = SNAPSHOT_STATUS_CANCELED,
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
    Canceled,
    Tombstone,

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
    OutOfDiskSpace,

    InProgressLimit = 30000,
};

FailureReason errnoToFailure(int error);
const char* failureReasonToString(FailureReason failure,
                                  SnapshotOperation op);

template <class Operation>
bool isComplete(const Operation& op) {
    return op.status() != OperationStatus::NotStarted;
}

bool isBufferZeroed(const void* ptr, int32_t size);

constexpr int32_t kDefaultPageSize = 4096;

constexpr int32_t kCancelTimeoutMs = 15000;

// Size in bytes of largest in-flight RAM region for decommitting.
constexpr uint64_t kDecommitChunkSize = 4096 * 4096; // 16 MB
constexpr const char* kDefaultBootSnapshot = "default_boot";
constexpr const char* kRamFileName = "ram.bin";
constexpr const char* kTexturesFileName = "textures.bin";
constexpr const char* kMappedRamFileName = "ram.img";
constexpr const char* kMappedRamFileDirtyName = "ram.img.dirty";

void resetSnapshotLiveness();
bool isSnapshotAlive();

}  // namespace snapshot
}  // namespace android
