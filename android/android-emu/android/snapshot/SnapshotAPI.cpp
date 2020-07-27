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

#include "android/snapshot/SnapshotAPI.h"

#include "android/base/async/ThreadLooper.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/crashreport/crash-handler.h"
#include "android/emulation/AndroidAsyncMessagePipe.h"
#include "android/globals.h"
#include "android/metrics/MetricsLogging.h"
#include "android/multi-instance.h"
#include "android/offworld/OffworldPipe.h"
#include "android/snapshot/common.h"
#include "android/snapshot/interface.h"
#include "android/opengl/emugl_config.h"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

namespace {

enum class RequestType : uint32_t {
    CreateCheckpoint,
    Fork,
};

struct SnapshotCrossSession {
    // Created before saving a snapshot, and saved into the snapshot, keeps
    // track of any pending requests that need a response.  Based on the request
    // type, a default response will be sent to the pipe on snapshot load unless
    // it has been overridden by mOverrideResponse.
    std::map<android::AsyncMessagePipeHandle, RequestType>
            mPipesAwaitingResponse;
    // Override responses from the snapshot load, not serialized.  Lasts only
    // until the next snapshot load completes, at which point it is cleared.
    std::map<RequestType, offworld::Response> mOverrideResponse;
    int sForkTotal = 0;
    int sForkId = 0;
};

android::base::LazyInstance<SnapshotCrossSession> sSnapshotCrossSession;

offworld::Response createNoErrorResponse() {
    offworld::Response response;
    response.set_result(offworld::Response::RESULT_NO_ERROR);
    return response;
}

offworld::Response createErrorResponse() {
    LOG(WARNING) << "Respond snapshot error";
    offworld::Response response;
    response.set_result(offworld::Response::RESULT_ERROR_UNKNOWN);
    return response;
}

offworld::Response createCheckpointMetadataResponse(
        android::base::StringView metadata) {
    // Note: metadata are raw bytes, not necessary a string. It is
    // just protobuf encodes them into strings. The data should be
    // handled as raw bytes (no extra '\0' at the end). Please try
    // not to use metadata.c_str().
    offworld::Response response;
    response.set_result(offworld::Response::RESULT_NO_ERROR);
    response.mutable_snapshot()->mutable_create_checkpoint()->set_metadata(
            metadata);
    return response;
}

offworld::Response createForkIdResponse(int forkId,
        android::base::StringView metadata = nullptr) {
    offworld::Response response;
    response.set_result(offworld::Response::RESULT_NO_ERROR);
    auto forkReadOnlyInstance = response.mutable_snapshot()
            ->mutable_fork_read_only_instances();
    forkReadOnlyInstance->set_instance_id(forkId);
    if (metadata != nullptr) {
        forkReadOnlyInstance->set_metadata(metadata);
    }
    return response;
}

void sendDefaultResponse(android::AsyncMessagePipeHandle pipe,
                         RequestType type) {
    switch (type) {
        case RequestType::CreateCheckpoint: {
            android::offworld::sendResponse(pipe, createNoErrorResponse());
            break;
        }
        case RequestType::Fork: {
            android::offworld::sendResponse(pipe, createForkIdResponse(0));
            break;
        }
        default: {
            DLOG(WARNING) << "Unknown request type=" << uint32_t(type);
            offworld::Response response;
            response.set_result(offworld::Response::RESULT_ERROR_UNKNOWN);
            android::offworld::sendResponse(pipe, response);
            break;
        }
    }
}

}  // namespace

namespace android {
namespace snapshot {

void createCheckpoint(AsyncMessagePipeHandle pipe,
                      android::base::StringView name) {
    // BUG: 127849628
    if (!emuglConfig_current_renderer_supports_snapshot()   ||
            !engine_supports_snapshot) {
        android::offworld::sendResponse(pipe, createErrorResponse());
        return;
    }

    const std::string snapshotName = name;

    sSnapshotCrossSession->mPipesAwaitingResponse[pipe] =
            RequestType::CreateCheckpoint;

    gQAndroidVmOperations->vmStop();
    android::base::ThreadLooper::runOnMainLooper([pipe, snapshotName]() {
        android_snapshot_update_timer = 0;
        const AndroidSnapshotStatus result =
                androidSnapshot_save(snapshotName.c_str());
        gQAndroidVmOperations->vmStart();

        sSnapshotCrossSession->mPipesAwaitingResponse.erase(pipe);
        android::offworld::sendResponse(
                pipe, result == AndroidSnapshotStatus::SNAPSHOT_STATUS_OK
                              ? createNoErrorResponse()
                              : createErrorResponse());
    });
}

void gotoCheckpoint(
        AsyncMessagePipeHandle pipe,
        android::base::StringView name,
        android::base::StringView metadata,
        android::base::Optional<android::base::FileShare> shareMode) {
    const std::string snapshotName = name;

    sSnapshotCrossSession->mOverrideResponse[RequestType::CreateCheckpoint] =
            createCheckpointMetadataResponse(metadata);

    gQAndroidVmOperations->vmStop();
    android::base::ThreadLooper::runOnMainLooper([pipe, snapshotName,
                                                  shareMode]() {
        android_snapshot_update_timer = 0;
        if (shareMode) {
            bool res = android::multiinstance::updateInstanceShareMode(
                    snapshotName.c_str(), *shareMode);
            if (!res) {
                LOG(WARNING) << "Share mode update failure";
            }
        }

        const AndroidSnapshotStatus result =
                androidSnapshot_load(snapshotName.c_str());
        gQAndroidVmOperations->vmStart();
        if (result != AndroidSnapshotStatus::SNAPSHOT_STATUS_OK) {
            android::offworld::sendResponse(pipe, createErrorResponse());
        }
    });
}

void forkReadOnlyInstances(android::AsyncMessagePipeHandle pipe,
                           int forkTotal) {
    if (android::multiinstance::getInstanceShareMode() !=
        android::base::FileShare::Write ||
        !emuglConfig_current_renderer_supports_snapshot() ||
        !engine_supports_snapshot) {
        android::offworld::sendResponse(pipe, createErrorResponse());
        return;
    }

    sSnapshotCrossSession->sForkTotal = forkTotal;
    sSnapshotCrossSession->sForkId = 0;

    if (forkTotal <= 1) {
        android::offworld::sendResponse(pipe, createForkIdResponse(0));
        return;
    }

    sSnapshotCrossSession->mPipesAwaitingResponse[pipe] = RequestType::Fork;
    sSnapshotCrossSession->mOverrideResponse[RequestType::Fork] =
            createForkIdResponse(0);

    gQAndroidVmOperations->vmStop();
    android::base::ThreadLooper::runOnMainLooper([pipe]() {
        // snapshotRemap triggers a snapshot save.
        // It must happen before changing disk backend
        android_snapshot_update_timer = 0;
        bool res =
                gQAndroidVmOperations->snapshotRemap(false, nullptr, nullptr);
        LOG_IF(WARNING, !res) << "RAM share mode update failure";

        // Update share mode flag and disk backend
        res = android::multiinstance::updateInstanceShareMode(
                android::snapshot::kDefaultBootSnapshot,
                android::base::FileShare::Read);
        LOG_IF(WARNING, !res) << "Share mode update failure";

        assert(res);
        const AndroidSnapshotStatus result =
                androidSnapshot_load(android::snapshot::kDefaultBootSnapshot);
        gQAndroidVmOperations->vmStart();
        if (result != AndroidSnapshotStatus::SNAPSHOT_STATUS_OK) {
            sSnapshotCrossSession->mPipesAwaitingResponse.erase(pipe);
            android::offworld::sendResponse(pipe, createErrorResponse());
        }
    });
}

void doneInstance(android::AsyncMessagePipeHandle pipe,
        android::base::StringView metadata) {
    if (sSnapshotCrossSession->sForkId <
        sSnapshotCrossSession->sForkTotal - 1) {
        sSnapshotCrossSession->sForkId++;

        sSnapshotCrossSession->mOverrideResponse[RequestType::Fork] =
                createForkIdResponse(sSnapshotCrossSession->sForkId,
                        metadata);

        gQAndroidVmOperations->vmStop();
        // Load back to write mode for the last run
        android::base::FileShare mode =
                sSnapshotCrossSession->sForkId <
                                sSnapshotCrossSession->sForkTotal - 1
                        ? android::base::FileShare::Read
                        : android::base::FileShare::Write;
        android::base::ThreadLooper::runOnMainLooper([mode]() {
            android_snapshot_update_timer = 0;
            bool res = android::multiinstance::updateInstanceShareMode(
                    android::snapshot::kDefaultBootSnapshot, mode);
            LOG_IF(WARNING, !res) << "Share mode update failure";
            res = gQAndroidVmOperations->snapshotRemap(
                    mode == android::base::FileShare::Write, nullptr, nullptr);
            gQAndroidVmOperations->vmStart();
        });
    } else if (sSnapshotCrossSession->sForkId ==
               sSnapshotCrossSession->sForkTotal - 1) {
        // We don't load after the last run
        sSnapshotCrossSession->sForkId = 0;
        sSnapshotCrossSession->sForkTotal = 0;

        ::offworld::Response response;
        response.set_result(::offworld::Response::RESULT_NO_ERROR);
        response.mutable_snapshot()
                ->mutable_done_instance();  // Creates the done_instance member.
        offworld::sendResponse(pipe, response);

    } else {
        LOG(ERROR) << "Bad fork session id: " << sSnapshotCrossSession->sForkId
                   << ", expected total " << sSnapshotCrossSession->sForkTotal;
        offworld::sendResponse(pipe, createErrorResponse());
    }
}

void onOffworldSave(base::Stream* stream) {
    stream->putBe32(
            uint32_t(sSnapshotCrossSession->mPipesAwaitingResponse.size()));
    LOG(VERBOSE) << "Snapshot save pipe count " << sSnapshotCrossSession->mPipesAwaitingResponse.size();
    assert(sSnapshotCrossSession->mPipesAwaitingResponse.size() <= 1);
    for (auto it : sSnapshotCrossSession->mPipesAwaitingResponse) {
        stream->putBe32(it.first.id);
        stream->putBe32(uint32_t(it.second));
    }
}

void onOffworldLoad(base::Stream* stream) {
    sSnapshotCrossSession->mPipesAwaitingResponse.clear();

    const uint32_t pipeCount = stream->getBe32();
    LOG(VERBOSE) << "Snapshot load pipe count " << pipeCount;
    assert(pipeCount <= 1);
    for (uint32_t i = 0; i < pipeCount; ++i) {
        AsyncMessagePipeHandle pipe;
        pipe.id = static_cast<int>(stream->getBe32());

        const RequestType type = static_cast<RequestType>(stream->getBe32());

        auto overrideIt = sSnapshotCrossSession->mOverrideResponse.find(type);
        if (overrideIt != sSnapshotCrossSession->mOverrideResponse.end()) {
            LOG(VERBOSE) << "Sending response from load request pipe="
                         << pipe.id;
            offworld::sendResponse(pipe, overrideIt->second);
        } else {
            // No override response, send default.
            LOG(VERBOSE) << "Sending default response pipe=" << pipe.id
                         << ", type=" << uint32_t(type);
            sendDefaultResponse(pipe, type);
        }
    }

    sSnapshotCrossSession->mOverrideResponse.clear();
}

}  // namespace snapshot
}  // namespace android
