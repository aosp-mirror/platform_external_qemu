// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/offworld/OffworldPipe.h"

#include "android/base/Log.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/emulation/AndroidAsyncMessagePipe.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/metrics/MetricsLogging.h"
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

static std::vector<uint8_t> protoToVector(
        const google::protobuf::Message& message) {
    const int size = message.ByteSize();
    std::vector<uint8_t> result(static_cast<size_t>(size));
    message.SerializeToArray(result.data(), size);
    return result;
}

class OffworldPipe : public android::AndroidAsyncMessagePipe {
public:
    class Service
        : public android::AndroidAsyncMessagePipe::Service<OffworldPipe> {
        typedef android::AndroidAsyncMessagePipe::Service<OffworldPipe> Super;

    public:
        Service() : Super("OffworldPipe") {}
        ~Service() {
            android::base::AutoLock lock(sOffworldLock);
            sOffworldInstance = nullptr;
        }

        static Service* get() {
            android::base::AutoLock lock(sOffworldLock);
            if (!sOffworldInstance) {
                sOffworldInstance = new Service();
            }
            return sOffworldInstance;
        }

        virtual void postSave(android::base::Stream* stream) override {
            android::snapshot::onOffworldSave(stream);
            Super::postSave(stream);
        }

        virtual void postLoad(android::base::Stream* stream) override {
            android::snapshot::onOffworldLoad(stream);
            Super::postLoad(stream);
        }

    private:
        // Since OffworldPipe::Service may be destroyed in unittests, we cannot
        // use LazyInstance. Instead store the pointer and clear it on destruct.
        static Service* sOffworldInstance;
        static android::base::StaticLock sOffworldLock;
    };

    OffworldPipe(AndroidPipe::Service* service, PipeArgs&& pipeArgs)
        : android::AndroidAsyncMessagePipe(service, std::move(pipeArgs)) {}

private:
    void onMessage(const std::vector<uint8_t>& input) override {
        if (!mHandshakeComplete) {
            offworld::ConnectHandshake request;
            offworld::ConnectHandshakeResponse response;
            response.set_result(
                    offworld::ConnectHandshakeResponse::RESULT_NO_ERROR);

            bool error = false;
            if (request.ParseFromArray(input.data(), input.size())) {
                if (request.version() != android::offworld::kProtocolVersion) {
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

            send(std::move(protoToVector(response)));
            mHandshakeComplete = !error;
            if (error) {
                queueCloseFromHost();
            }

        } else {
            offworld::Request request;

            if (!request.ParseFromArray(input.data(), input.size())) {
                LOG(ERROR) << "Offworld lib message parsing failed.";
            } else {
                switch (request.module_case()) {
                    case offworld::Request::ModuleCase::kSnapshot:
                        handleSnapshotPb(request.snapshot());
                        break;
                    case offworld::Request::ModuleCase::kAutomation:
                        // TODO
                        break;
                    default:
                        LOG(ERROR) << "Offworld lib received unrecognized "
                                      "message!";
                }
            }
        }
    }

    void handleSnapshotPb(const offworld::SnapshotRequest& request) {
        using MS = offworld::SnapshotRequest;

        switch (request.function_case()) {
            case MS::FunctionCase::kCreateCheckpoint: {
                android::snapshot::createCheckpoint(
                        getHandle(),
                        request.create_checkpoint().snapshot_name());
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
                        getHandle(), gotoCheckpoint.snapshot_name(),
                        gotoCheckpoint.metadata(), shareMode);
                break;
            }
            case MS::FunctionCase::kForkReadOnlyInstances: {
                android::snapshot::forkReadOnlyInstances(
                        getHandle(),
                        request.fork_read_only_instances().num_instances());
                break;
            }
            case MS::FunctionCase::kDoneInstance: {
                android::snapshot::doneInstance(getHandle());
                break;
            }
            default:
                LOG(ERROR) << "Unrecognized offworld snapshot message";
        }
    }

    bool mHandshakeComplete = false;
};

OffworldPipe::Service* OffworldPipe::Service::sOffworldInstance = nullptr;
android::base::StaticLock OffworldPipe::Service::sOffworldLock;

}  // namespace

namespace android {
namespace offworld {

void registerOffworldPipeService() {
    if (android::featurecontrol::isEnabled(android::featurecontrol::Offworld)) {
        android::AndroidPipe::Service::add(OffworldPipe::Service::get());
    }
}

void registerOffworldPipeServiceForTest() {
    android::AndroidPipe::Service::add(OffworldPipe::Service::get());
}

// Send a response to an Offworld pipe.
bool sendResponse(android::AsyncMessagePipeHandle pipe,
                  const ::offworld::Response& response) {
    OffworldPipe::Service* service = OffworldPipe::Service::get();
    auto pipeRefOpt = service->getPipe(pipe);
    if (pipeRefOpt) {
        pipeRefOpt->pipe->send(std::move(protoToVector(response)));
        return true;
    } else {
        return false;
    }
}

}  // namespace offworld
}  // namespace android
