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

#include "android/base/Log.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/emulation/AndroidMessagePipe.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/metrics/MetricsLogging.h"
#include "android/offworld/proto/offworld.pb.h"
#include "android/snapshot/SnapshotAPI.h"
#include "android/snapshot/common.h"
#include "android/snapshot/interface.h"

#include <assert.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <sstream>
#include <vector>

using android::base::LazyInstance;
using android::base::Lock;
using android::base::Optional;

namespace {

struct OffworldState {
    android::base::Lock sLock = {};
};

static constexpr uint32_t kProtocolVersion = 1;
android::base::LazyInstance<OffworldState> sOffworldState;

static std::vector<uint8_t> protoToVector(
        const google::protobuf::Message& message) {
    const int size = message.ByteSize();
    std::vector<uint8_t> result(static_cast<size_t>(size));
    message.SerializeToArray(result.data(), size);
    return result;
}

class OffworldPipe : public android::AndroidMessagePipe {
public:
    class Service : public android::AndroidPipe::Service {
    public:
        Service() : android::AndroidPipe::Service("OffworldPipe") {}
        bool canLoad() const override { return true; }

        virtual android::AndroidPipe* create(void* hwPipe, const char* args)
                override {
            // To avoid complicated synchronization issues, only 1 instance
            // of a OffworldPipe is allowed at a time
            if (sOffworldState->sLock.tryLock()) {
                return new OffworldPipe(hwPipe, this);
            } else {
                return nullptr;
            }
        }

        android::AndroidPipe* load(void* hwPipe,
                                   const char* args,
                                   android::base::Stream* stream) override {
            __attribute__((unused)) bool success =
                    sOffworldState->sLock.tryLock();
            assert(success);
            return new OffworldPipe(hwPipe, this, stream);
        }
    };
    OffworldPipe(void* hwPipe,
                 Service* service,
                 android::base::Stream* loadStream = nullptr)
        : android::AndroidMessagePipe(hwPipe,
                                      service,
                                      std::bind(&OffworldPipe::decodeAndExecute,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2),
                                      loadStream) {
        ANDROID_IF_DEBUG(assert(sOffworldState->sLock.isLocked()));
        const bool isLoad = static_cast<bool>(loadStream);
        if (isLoad) {
            // TODO(jwmcglynn): Store state on the stream to make sure handshake
            // was successful.
            mHandshakeComplete = true;
            resetRecvPayload(android::snapshot::getLoadMetadata());
        }
    }
    ~OffworldPipe() { sOffworldState->sLock.unlock(); }

private:
    void decodeAndExecute(const std::vector<uint8_t>& input,
                          std::vector<uint8_t>* output) {
        output->clear();

        if (!mHandshakeComplete) {
            offworld::ConnectHandshake request;
            offworld::ConnectHandshakeResponse response;
            response.set_result(
                    offworld::ConnectHandshakeResponse::RESULT_NO_ERROR);

            bool error = false;
            if (request.ParseFromArray(input.data(), input.size())) {
                if (request.version() != kProtocolVersion) {
                    VLOG(offworld) << "Unsupported offworld version: "
                                   << request.version();
                    error = true;
                    response.set_result(offworld::ConnectHandshakeResponse::
                                                RESULT_ERROR_VERSION_MISMATCH);
                }
            } else {
                LOG(ERROR) << "Failed to parse offworld handshake.";
                error = true;
                response.set_result(offworld::ConnectHandshakeResponse::
                                            RESULT_ERROR_UNKNOWN);
            }

            // TODO(jwmcglynn): Close the pipe if there was a handshake error.
            if (!error) {
                mHandshakeComplete = true;
            }

            *output = std::move(protoToVector(response));

        } else {
            offworld::Request request;
            offworld::Response response;
            bool shouldReply = false;

            if (!request.ParseFromArray(input.data(), input.size())) {
                LOG(ERROR) << "Offworld lib message parsing failed.";
            } else {
                switch (request.module_case()) {
                    case offworld::Request::ModuleCase::kSnapshot:
                        handleSnapshotPb(request.snapshot(), &shouldReply,
                                         &response);
                        break;
                    case offworld::Request::ModuleCase::kAutomation:
                        // TODO
                        break;
                    default:
                        LOG(ERROR) << "Offworld lib received unrecognized "
                                      "message!";
                }
            }
            if (shouldReply) {
                *output = std::move(protoToVector(response));
            }
        }
    }

    static void handleSnapshotPb(const offworld::SnapshotRequest& request,
                                 bool* shouldReply,
                                 offworld::Response* response) {
        using MS = offworld::SnapshotRequest;

        switch (request.function_case()) {
            case MS::FunctionCase::kCreateCheckpoint: {
                android::snapshot::createCheckpoint(
                        request.create_checkpoint().snapshot_name());
                *shouldReply = true;
                response->Clear();
                break;
            }
            case MS::FunctionCase::kGotoCheckpoint: {
                const MS::GotoCheckpoint& gotoCheckpoint =
                        request.goto_checkpoint();

                Optional<android::base::FileShare> shareMode;

                if (gotoCheckpoint.has_share_mode()) {
                    switch (gotoCheckpoint.share_mode()) {
                        case MS::GotoCheckpoint::UNKNOWN:
                        case MS::GotoCheckpoint::UNCHANGED:
                            break;
                        case MS::GotoCheckpoint::READ_ONLY:
                            shareMode = android::base::FileShare::Read;
                            break;
                        case MS::GotoCheckpoint::WRITABLE:
                            shareMode = android::base::FileShare::Write;
                            break;
                        default:
                            LOG(WARNING) << "Unsupported share mode, "
                                            "defaulting to unchanged.";
                            break;
                    }
                }

                android::snapshot::gotoCheckpoint(
                        gotoCheckpoint.snapshot_name(),
                        gotoCheckpoint.metadata(), shareMode);
                *shouldReply = false;
                break;
            }
            case MS::FunctionCase::kForkReadOnlyInstances: {
                android::snapshot::forkReadOnlyInstances(
                        request.fork_read_only_instances().num_instances());
                *shouldReply = false;
                break;
            }
            case MS::FunctionCase::kDoneInstance: {
                android::snapshot::doneInstance();
                *shouldReply = false;
                break;
            }
            default:
                LOG(ERROR) << "Unrecognized offworld snapshot message";
                *shouldReply = false;
        }
    }

    bool mHandshakeComplete = false;
};

}  // namespace

namespace android {
namespace offworld {

void registerOffworldPipeService() {
    if (android::featurecontrol::isEnabled(android::featurecontrol::Offworld)) {
        android::AndroidPipe::Service::add(new OffworldPipe::Service());
    }
}

}  // namespace offworld
}  // namespace android
