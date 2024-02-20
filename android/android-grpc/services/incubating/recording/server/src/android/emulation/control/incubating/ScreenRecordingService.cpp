// Copyright (C) 2022 The Android Open Source Project
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

#include <grpcpp/grpcpp.h>

#include "android/console.h"
#include "android/emulation/control/utils/EventSupport.h"
#include "android/grpc/utils/SimpleAsyncGrpc.h"
#include "android/utils/debug.h"
#include "host-common/screen-recorder.h"
#include "screen_recording_service.grpc.pb.h"

// #define DEBUG 1
#if DEBUG >= 1
#define DD(fmt, ...)                                                       \
    printf("ScreenRecordingService: %s:%d| " fmt "\n", __func__, __LINE__, \
           ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

namespace android {
namespace emulation {
namespace control {
namespace incubating {

EventChangeSupport<RecordingInfo> s_activeRecordingListeners;

extern "C" {
static void recording_callback(void* userData, RecordingStatus status);
}

using ScreenRecordingEventStream = GenericEventStreamWriter<RecordingInfo>;

RecordingInfo toRecordingInfo(::RecordingInfo* source) {
    RecordingInfo info;
    if (source->fileName)
        info.set_file_name(source->fileName);
    info.set_width(source->width);
    info.set_height(source->height);
    info.set_bit_rate(source->videoBitrate);
    info.set_time_limit(source->timeLimit);
    info.set_fps(source->fps);
    info.set_display(source->displayId);
    return info;
}

class ScreenRecordingImpl final
    : public ScreenRecording::WithCallbackMethod_ReceiveRecordingEvents<
              ScreenRecording::Service>

{
public:
    ScreenRecordingImpl(const AndroidConsoleAgents* agents)
        : mRecordAgent(agents->record) {}

    ::grpc::Status StartRecording(
            ::grpc::ServerContext* context,
            const ::android::emulation::control::incubating::RecordingInfo*
                    request,
            ::android::emulation::control::incubating::RecordingInfo* response)
            override {
        auto currentState = mRecordAgent->getRecorderState();
        VERBOSE_INFO(grpc, "Starting recording from state: %s ",
                     recorderStateToString(currentState.state));
        if (currentState.state != RECORDER_STOPPED) {
            return ::grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                                  "The recorder is not in a stopped state.");
        }

        mFileName = request->file_name();
        mActiveRecording.fileName = mFileName.c_str();
        mActiveRecording.width = request->width();
        mActiveRecording.height = request->height();
        mActiveRecording.videoBitrate = request->bit_rate();
        mActiveRecording.timeLimit = request->time_limit();
        mActiveRecording.fps = request->fps();
        mActiveRecording.displayId = request->display();
        mActiveRecording.cb = &recording_callback;
        mActiveRecording.opaque = &mActiveRecording;

        if (!mRecordAgent->startRecording(&mActiveRecording)) {
            mActiveRecording = {nullptr};
            return ::grpc::Status(
                    grpc::StatusCode::INTERNAL,
                    "The recorder failed to start the recording.");
        };

        auto info = toRecordingInfo(&mActiveRecording);
        response->CopyFrom(info);
        VERBOSE_INFO(grpc, "Recording to: %s", response->ShortDebugString());
        return ::grpc::Status::OK;
    };

    ::grpc::Status StopRecording(
            ::grpc::ServerContext* context,
            const ::android::emulation::control::incubating::RecordingInfo*
                    request,
            ::android::emulation::control::incubating::RecordingInfo* response)
            override {
        VERBOSE_INFO(grpc, "Stopping the recording agent.");
        mRecordAgent->stopRecording();
        auto info = toRecordingInfo(&mActiveRecording);
        response->CopyFrom(info);
        VERBOSE_INFO(grpc, "Current state: %s", response->ShortDebugString());
        return ::grpc::Status::OK;
    };

    ::grpc::Status ListRecordings(
            ::grpc::ServerContext* context,
            const ::android::emulation::control::incubating::RecordingInfo*
                    request,
            ::android::emulation::control::incubating::RecordingInfoList*
                    response) override {
        auto currentState = mRecordAgent->getRecorderState();
        if (currentState.state == RECORDER_RECORDING) {
            auto info = toRecordingInfo(&mActiveRecording);
            response->add_recordings()->CopyFrom(info);
        }

        VERBOSE_INFO(grpc, "Recording list:%s", response->ShortDebugString());
        return ::grpc::Status::OK;
    };

    ::grpc::ServerWriteReactor<
            ::android::emulation::control::incubating::RecordingInfo>*
    ReceiveRecordingEvents(
            ::grpc::CallbackServerContext* /*context*/,
            const ::google::protobuf::Empty* /*request*/) override {
        return new ScreenRecordingEventStream(&s_activeRecordingListeners);
    }

private:
    static std::string recorderStateToString(RecorderState state) {
        switch (state) {
            case RECORDER_STARTING:
                return "Starting";
            case RECORDER_RECORDING:
                return "Recording";
            case RECORDER_STOPPING:
                return "Stopping";
            case RECORDER_STOPPED:
                return "Stopped";
            default:
                return "Unknown State";
        }
    }

    const QAndroidRecordScreenAgent* mRecordAgent;

    ::RecordingInfo mActiveRecording{0};
    std::string mFileName;
};

extern "C" {
static void recording_callback(void* userData, RecordingStatus status) {
    if (s_activeRecordingListeners.size() == 0) {
        // No listeners, so no need to fire events.
        return;
    }

    ::RecordingInfo* data = reinterpret_cast<::RecordingInfo*>(userData);
    RecordingInfo info = toRecordingInfo(data);

    switch (status) {
        case RECORD_START_INITIATED:
            info.set_state(RecordingInfo::RECORDER_STATE_STARTING);
            break;
        case RECORD_STARTED:
            info.set_state(RecordingInfo::RECORDER_STATE_RECORDING);
            break;
        case RECORD_START_FAILED:
            info.set_state(RecordingInfo::RECORDER_STATE_START_FAILED);
            break;
        case RECORD_STOP_INITIATED:
            info.set_state(RecordingInfo::RECORDER_STATE_STOPPING);
            break;
        case RECORD_STOPPED:
            info.set_state(RecordingInfo::RECORDER_STATE_STOPPED);
            break;
        case RECORD_STOP_FAILED:
            info.set_state(RecordingInfo::RECORDER_STOP_FAILED);
            break;
    }

    VERBOSE_INFO(grpc, "Broadcast %s", info.ShortDebugString());
    s_activeRecordingListeners.fireEvent(info);
}
}

grpc::Service* getScreenRecordingService(const AndroidConsoleAgents* agents) {
    return new ScreenRecordingImpl(agents);
}

}  // namespace incubating
}  // namespace control
}  // namespace emulation
}  // namespace android