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

#include "android/automation/AutomationController.h"
#include "android/base/Log.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/emulation/AndroidAsyncMessagePipe.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/metrics/MetricsLogging.h"
#include "android/sensor_mock/SensorMockUtils.h"
#include "android/snapshot/SnapshotAPI.h"
#include "android/snapshot/common.h"
#include "android/snapshot/interface.h"
#include "android/videoinjection/VideoInjectionController.h"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <memory>
#include <sstream>
#include <vector>

using android::automation::AutomationController;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::Optional;
using android::videoinjection::VideoInjectionController;

namespace {

static std::vector<uint8_t> protoToVector(
        const google::protobuf::Message& message) {
    const int size = message.ByteSize();
    std::vector<uint8_t> result(static_cast<size_t>(size));
    message.SerializeToArray(result.data(), size);
    return result;
}

static offworld::Response createOkResponse() {
    offworld::Response response;
    response.set_result(offworld::Response::RESULT_NO_ERROR);
    return response;
}

static offworld::Response createErrorResponse(
    const offworld::Response_Result& result,
    std::string str = std::string()) {
    offworld::Response response;
    response.set_result(result);
    if (!str.empty()) {
        response.set_error_string(str);
    }
    return response;
}

class OffworldPipe : public android::AndroidAsyncMessagePipe {
public:
    class Service
        : public android::AndroidAsyncMessagePipe::Service<OffworldPipe> {
        typedef android::AndroidAsyncMessagePipe::Service<OffworldPipe> Super;

    public:
        Service(AutomationController& automationController,
                VideoInjectionController& videoInjectionController)
            : Super("OffworldPipe"),
              mAutomationController(automationController),
              mVideoInjectionController(videoInjectionController) {}
        ~Service() {
            android::base::AutoLock lock(sOffworldLock);
            sOffworldInstance = nullptr;
        }

        static std::unique_ptr<Service> create(
            AutomationController& automationController,
            VideoInjectionController& videoInjectionController) {
            android::base::AutoLock lock(sOffworldLock);
            CHECK(!sOffworldInstance) << "OffworldPipe already exists";
            sOffworldInstance = new Service(automationController,
                                            videoInjectionController);
            return std::unique_ptr<Service>(sOffworldInstance);
        }

        static Service* get() {
            android::base::AutoLock lock(sOffworldLock);
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

        AutomationController& getAutomationController() {
            return mAutomationController;
        }

        VideoInjectionController& getVideoInjectionController() {
            return mVideoInjectionController;
        }

    private:
        // Since OffworldPipe::Service may be destroyed in unittests, we cannot
        // use LazyInstance. Instead store the pointer and clear it on destruct.
        static Service* sOffworldInstance;
        static android::base::StaticLock sOffworldLock;

        AutomationController& mAutomationController;
        VideoInjectionController& mVideoInjectionController;
    };

    OffworldPipe(AndroidPipe::Service* service, PipeArgs&& pipeArgs)
        : android::AndroidAsyncMessagePipe(service, std::move(pipeArgs)),
          mService(static_cast<Service*>(service)) {}

    virtual ~OffworldPipe() {
        getAutomationController().pipeClosed(getHandle());
        getVideoInjectionController().pipeClosed(getHandle());
    }

    void onSave(android::base::Stream* stream) override {
        android::AndroidAsyncMessagePipe::onSave(stream);
        stream->putByte(mHandshakeComplete);
        stream->putBe32(mNextAsyncId);
    }

    void onLoad(android::base::Stream* stream) override {
        android::AndroidAsyncMessagePipe::onLoad(stream);
        mHandshakeComplete = static_cast<bool>(stream->getByte());
        mNextAsyncId = stream->getBe32();
    }

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
                sendError(offworld::Response::RESULT_ERROR_UNKNOWN);
            } else {
                switch (request.module_case()) {
                    case offworld::Request::ModuleCase::kSnapshot:
                        handleSnapshotRequest(request.snapshot());
                        break;
                    case offworld::Request::ModuleCase::kAutomation:
                        handleAutomationRequest(request.automation());
                        break;
                    case offworld::Request::ModuleCase::kVideoInjection:
                        handleVideoInjectionRequest(request.video_injection());
                        break;
                    case offworld::Request::ModuleCase::kSensorMock:
                        handleSensorMockRequest(request.sensor_mock());
                        break;
                    default:
                        LOG(ERROR) << "Offworld lib received unrecognized "
                                      "message!";
                        sendError(offworld::Response::
                                          RESULT_ERROR_NOT_IMPLEMENTED);
                }
            }
        }
    }

    void handleSnapshotRequest(const offworld::SnapshotRequest& request) {
        using sr = offworld::SnapshotRequest;

        switch (request.function_case()) {
            case sr::FunctionCase::kCreateCheckpoint: {
                android::snapshot::createCheckpoint(
                        getHandle(),
                        request.create_checkpoint().snapshot_name());
                break;
            }
            case sr::FunctionCase::kGotoCheckpoint: {
                const sr::GotoCheckpoint& gotoCheckpoint =
                        request.goto_checkpoint();

                Optional<android::base::FileShare> shareMode;

                if (gotoCheckpoint.has_share_mode()) {
                    switch (gotoCheckpoint.share_mode()) {
                        case sr::GotoCheckpoint::UNKNOWN:
                        case sr::GotoCheckpoint::UNCHANGED:
                            break;
                        case sr::GotoCheckpoint::READ_ONLY:
                            shareMode = android::base::FileShare::Read;
                            break;
                        case sr::GotoCheckpoint::WRITABLE:
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
            case sr::FunctionCase::kForkReadOnlyInstances: {
                android::snapshot::forkReadOnlyInstances(
                        getHandle(),
                        request.fork_read_only_instances().num_instances());
                break;
            }
            case sr::FunctionCase::kDoneInstance: {
                android::snapshot::doneInstance(getHandle(),
                        request.done_instance().metadata());
                break;
            }
            default:
                LOG(ERROR) << "Unrecognized offworld snapshot message";
                sendError(offworld::Response::RESULT_ERROR_NOT_IMPLEMENTED);
                break;
        }
    }

    void handleAutomationRequest(const offworld::AutomationRequest& request) {
        using autor = offworld::AutomationRequest;
        using namespace android::automation;

        switch (request.function_case()) {
            case autor::FunctionCase::kReplayInitialState: {
                auto result = getAutomationController().replayInitialState(
                        request.replay_initial_state().state());

                if (result.ok()) {
                    sendResponse(createOkResponse());
                } else {
                    std::stringstream ss;
                    ss << result.unwrapErr();
                    sendError(offworld::Response::RESULT_ERROR_UNKNOWN,
                              ss.str());
                }
                break;
            }
            case autor::FunctionCase::kReplay: {
                const uint32_t asyncId = mNextAsyncId++;
                auto result = getAutomationController().replayEvent(
                        getHandle(), request.replay().event(), asyncId);

                if (result.ok()) {
                    offworld::Response response = createOkResponse();
                    response.set_pending_async_id(asyncId);
                    sendResponse(response);
                } else {
                    std::stringstream ss;
                    ss << result.unwrapErr();
                    sendError(offworld::Response::RESULT_ERROR_UNKNOWN,
                              ss.str());
                }
                break;
            }
            case autor::FunctionCase::kListen: {
                const uint32_t asyncId = mNextAsyncId++;
                auto result =
                        getAutomationController().listen(getHandle(), asyncId);

                if (result.ok()) {
                    offworld::Response response = createOkResponse();
                    response.set_pending_async_id(asyncId);
                    response.mutable_automation()
                            ->mutable_listen()
                            ->set_initial_state(result.unwrap());

                    sendResponse(response);
                } else {
                    std::stringstream ss;
                    ss << result.unwrapErr();
                    sendError(offworld::Response::RESULT_ERROR_UNKNOWN,
                              ss.str());
                }
                break;
            }
            case autor::FunctionCase::kStopListening: {
                getAutomationController().stopListening();
                sendResponse(createOkResponse());
                break;
            }
            default:
                LOG(ERROR) << "Unrecognized offworld automation message";
                sendError(offworld::Response::RESULT_ERROR_NOT_IMPLEMENTED);
                break;
        }
    }

    void handleVideoInjectionRequest(
        const offworld::VideoInjectionRequest& request) {

        const uint32_t asyncId = mNextAsyncId++;
        auto result = getVideoInjectionController().handleRequest(
            getHandle(), request, asyncId);

        if (result.ok()) {
            offworld::Response response = createOkResponse();
            response.set_pending_async_id(asyncId);
            response.mutable_video_injection()->set_sequence_id(
                request.sequence_id());
            sendResponse(response);
        } else {
            std::stringstream ss;
            ss << result.unwrapErr();
            offworld::Response response = createErrorResponse(
                offworld::Response::RESULT_ERROR_UNKNOWN,
                ss.str());
            response.mutable_video_injection()->set_sequence_id(
                request.sequence_id());
            sendResponse(response);
        }
    }

    void handleSensorMockRequest(
        const offworld::SensorMockRequest& request) {
      LOG(ERROR) << "Received Sensor Mock Event";
      getAutomationController().sendEvent(
          android::sensor_mock::toEvent(request));
      sendResponse(createOkResponse());
    }

    void sendResponse(const offworld::Response& response) {
        send(std::move(protoToVector(response)));
    }

    void sendError(const offworld::Response_Result& result,
                   std::string str = std::string()) {
        offworld::Response response = createErrorResponse(result, str);
        sendResponse(response);
    }

    AutomationController& getAutomationController() {
        return mService->getAutomationController();
    }

    VideoInjectionController& getVideoInjectionController() {
        return mService->getVideoInjectionController();
    }

    Service* const mService;

    bool mHandshakeComplete = false;
    std::atomic<std::uint32_t> mNextAsyncId { 1 };
};

OffworldPipe::Service* OffworldPipe::Service::sOffworldInstance = nullptr;
android::base::StaticLock OffworldPipe::Service::sOffworldLock;

}  // namespace

namespace android {
namespace offworld {

void registerOffworldPipeService() {
    if (android::featurecontrol::isEnabled(android::featurecontrol::Offworld)) {
        registerAsyncMessagePipeService(
                OffworldPipe::Service::create(
                    AutomationController::get(),
                    VideoInjectionController::get()));
    }
}

void registerOffworldPipeServiceForTest(
    AutomationController* automationController,
    VideoInjectionController* videoInjectionController) {
    registerAsyncMessagePipeService(OffworldPipe::Service::create(
        *automationController, *videoInjectionController));
}

// Send a response to an Offworld pipe.
bool sendResponse(android::AsyncMessagePipeHandle pipe,
                  const pb::Response& response) {
    OffworldPipe::Service* service = OffworldPipe::Service::get();
    if (!service) {
        LOG(INFO) << "Dropping Offworld response, service does not exist.";
        return false;
    }

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
