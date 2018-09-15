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

#include "android/base/async/ThreadLooper.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/crashreport/crash-handler.h"
#include "android/emulation/AndroidMessagePipe.h"
#include "android/metrics/MetricsLogging.h"
#include "android/multi-instance.h"
#include "android/offworld/proto/offworld.pb.h"
#include "android/snapshot/common.h"
#include "android/snapshot/interface.h"

#include <assert.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <sstream>
#include <vector>

extern const QAndroidVmOperations* const gQAndroidVmOperations;

namespace {

struct SnapshotCrossSession {
    android::AndroidMessagePipe::DataBuffer sMetaData = {};
    android::base::Lock sLock = {};
    int sForkTotal = 0;
    int sForkId = 0;
};

android::base::LazyInstance<SnapshotCrossSession> sSnapshotCrossSession;

static void setResponseAfterLoad(const google::protobuf::Message& message) {
    std::vector<uint8_t> output;

    uint32_t recvSize = message.ByteSize();
    output.resize(recvSize + sizeof(recvSize));
    memcpy(output.data(), (void*)&recvSize, sizeof(recvSize));
    message.SerializeToArray(output.data() + sizeof(recvSize), recvSize);

    sSnapshotCrossSession->sMetaData = std::move(output);
}

void setCheckpointMetadataResponse(android::base::StringView metadata) {
    // Note: metadata are raw bytes, not necessary a string. It is
    // just protobuf encodes them into strings. The data should be
    // handled as raw bytes (no extra '\0' at the end). Please try
    // not to use metadata.c_str().
    offworld::Response response;
    response.set_result(offworld::Response::RESULT_NO_ERROR);
    response.mutable_snapshot()->mutable_create_checkpoint()->set_metadata(
            metadata);
    setResponseAfterLoad(response);
}

void setForkIdResponse(int forkId) {
    offworld::Response response;
    response.set_result(offworld::Response::RESULT_NO_ERROR);
    response.mutable_snapshot()
            ->mutable_fork_read_only_instances()
            ->set_instance_id(forkId);
    setResponseAfterLoad(response);
}

}  // namespace

namespace android {
namespace snapshot {

void createCheckpoint(android::base::StringView name) {
    const std::string snapshotName = name;

    gQAndroidVmOperations->vmStop();
    android::base::ThreadLooper::runOnMainLooper([snapshotName]() {
        androidSnapshot_save(snapshotName.c_str());
        gQAndroidVmOperations->vmStart();
    });
}

void gotoCheckpoint(
        android::base::StringView name,
        android::base::StringView metadata,
        android::base::Optional<android::base::FileShare> shareMode) {
    const std::string snapshotName = name;

    setCheckpointMetadataResponse(metadata);

    gQAndroidVmOperations->vmStop();
    android::base::ThreadLooper::runOnMainLooper([snapshotName, shareMode]() {
        if (shareMode) {
            bool res = android::multiinstance::updateInstanceShareMode(
                    snapshotName.c_str(), *shareMode);
            if (!res) {
                fprintf(stderr, "WARNING: share mode update failure\n");
            }
        }
        androidSnapshot_load(snapshotName.c_str());
        gQAndroidVmOperations->vmStart();
    });
}

void forkReadOnlyInstances(int forkTotal) {
    if (android::multiinstance::getInstanceShareMode() !=
        android::base::FileShare::Write) {
        return;
    }

    sSnapshotCrossSession->sForkTotal = forkTotal;
    sSnapshotCrossSession->sForkId = 0;

    setForkIdResponse(0);

    gQAndroidVmOperations->vmStop();
    android::base::ThreadLooper::runOnMainLooper([]() {
        // snapshotRemap triggers a snapshot save.
        // It must happen before changing disk backend
        bool res =
                gQAndroidVmOperations->snapshotRemap(false, nullptr, nullptr);
        if (!res) {
            fprintf(stderr, "WARNING: RAM share mode update failure\n");
        }
        // Update share mode flag and disk backend
        res = android::multiinstance::updateInstanceShareMode(
                android::snapshot::kDefaultBootSnapshot,
                android::base::FileShare::Read);
        if (!res) {
            fprintf(stderr, "WARNING: share mode update failure\n");
        }
        assert(res);
        androidSnapshot_load(android::snapshot::kDefaultBootSnapshot);
        gQAndroidVmOperations->vmStart();
    });
}

void doneInstance() {
    if (sSnapshotCrossSession->sForkId <
        sSnapshotCrossSession->sForkTotal - 1) {
        sSnapshotCrossSession->sForkId++;

        setForkIdResponse(sSnapshotCrossSession->sForkId);

        if (sSnapshotCrossSession->sForkId <
            sSnapshotCrossSession->sForkTotal - 1) {
            gQAndroidVmOperations->vmStop();
            // Load back to write mode for the last run
            android::base::FileShare mode =
                    sSnapshotCrossSession->sForkId <
                                    sSnapshotCrossSession->sForkTotal - 2
                            ? android::base::FileShare::Read
                            : android::base::FileShare::Write;
            android::base::ThreadLooper::runOnMainLooper([mode]() {
                bool res = android::multiinstance::updateInstanceShareMode(
                        android::snapshot::kDefaultBootSnapshot, mode);
                if (!res) {
                    fprintf(stderr, "WARNING: share mode update failure\n");
                }
                res = gQAndroidVmOperations->snapshotRemap(
                        mode == android::base::FileShare::Write, nullptr,
                        nullptr);
                gQAndroidVmOperations->vmStart();
            });
        }
    } else if (sSnapshotCrossSession->sForkId ==
               sSnapshotCrossSession->sForkTotal - 1) {
        // We don't load after the last run
        sSnapshotCrossSession->sForkId = 0;
        sSnapshotCrossSession->sForkTotal = 0;

        // TODO(yahan): Send a DoneInstance response.

    } else {
        fprintf(stderr, "WARNING: bad fork session ID\n");
    }
}

std::vector<uint8_t> getLoadMetadata() {
    // sMetaData should be cleared after the move
    return std::move(sSnapshotCrossSession->sMetaData);
}

}  // namespace snapshot
}  // namespace android
