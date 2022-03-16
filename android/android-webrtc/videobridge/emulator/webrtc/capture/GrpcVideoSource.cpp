// Copyright (C) 2020 The Android Open Source Project
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
#include "emulator/webrtc/capture/GrpcVideoSource.h"

#include <api/video/video_frame.h>         // for VideoFrame::Builder
#include <api/video/video_frame_buffer.h>  // for VideoFrameBuffer
#include <api/video/video_rotation.h>      // for kVideoRotation_0
#include <grpcpp/grpcpp.h>                 // for ClientReaderInterface
#include <rtc_base/logging.h>              // for RTC_LOG
#include <rtc_base/time_utils.h>           // for TimeMicros
#include <stdint.h>                        // for uint8_t

#include <memory>  // for unique_ptr, operator==
#include <string>  // for string, operator==, stoi
#include <thread>  // for thread
#include <tuple>   // for make_tuple, tuple_elem...

#include "android/base/StringView.h"           // for StringView
#include "android/base/memory/SharedMemory.h"  // for SharedMemory, StringView
#include "android/utils/tempfile.h"            // for tempfile_close, tempfi...
#include "api/video/i420_buffer.h"             // for I420Buffer
#include "emulator_controller.grpc.pb.h"       // for EmulatorController::Stub
#include "emulator_controller.pb.h"            // for ImageFormat, EmulatorS...
#include "google/protobuf/empty.pb.h"          // for Empty
#include "libyuv/convert.h"                    // for ConvertToI420
#include "libyuv/rotate.h"                     // for kRotate0
#include "libyuv/video_common.h"               // for FOURCC_RGB3, FourCC

namespace rtc {
struct VideoSinkWants;
template <typename VideoFrameT>
class VideoSinkInterface;
}  // namespace rtc

namespace emulator {
namespace webrtc {

using ::android::emulation::control::EmulatorStatus;
using ::android::emulation::control::Entry;
using ::android::emulation::control::Image;
using ::android::emulation::control::ImageFormat;
using ::android::emulation::control::ImageTransport;

GrpcVideoSource::GrpcVideoSource(EmulatorGrpcClient* client)
    : mClient(client) {}

std::tuple<int, int> getScreenDimensions(EmulatorGrpcClient* client) {
    ::google::protobuf::Empty empty;

    auto context = client->newContext();
    int w = 1080, h = 1920;
    EmulatorStatus emuState;
    auto status = client->stub<android::emulation::control::EmulatorController>()->getStatus(context.get(), empty, &emuState);
    if (emuState.has_hardwareconfig()) {
        for (int i = 0; i < emuState.hardwareconfig().entry_size(); i++) {
            auto entry = emuState.hardwareconfig().entry(i);
            if (entry.key() == "hw.lcd.width") {
                w = std::stoi(entry.value());
            }
            if (entry.key() == "hw.lcd.height") {
                h = std::stoi(entry.value());
            }
        }
    }

    return std::make_tuple(w, h);
}

using android::emulation::control::Rotation;

static gpr_timespec grpc_timeout_to_deadline(std::chrono::microseconds us) {
    return gpr_time_add(gpr_now(GPR_CLOCK_MONOTONIC),
                        gpr_time_from_micros(us.count(), GPR_TIMESPAN));
}



void GrpcVideoSource::captureFrames() {
    auto [w, h] = getScreenDimensions(mClient);
    auto memsize = w * h * 3;
    mI420Buffer = ::webrtc::I420Buffer::Create(w, h);

    auto tmp = tempfile_create();
    std::string handle = std::string("file://") + tempfile_path(tmp);

    android::base::SharedMemory shm(handle, memsize);
    shm.create(0600);

    ImageFormat request;
    request.set_format(ImageFormat::RGB888);
    request.set_width(w);
    request.set_height(h);
    auto transport = request.mutable_transport();
    transport->set_channel(ImageTransport::MMAP);
    transport->set_handle(handle);

    std::shared_ptr<grpc::ClientContext> context = mClient->newContext();
    mContext = context;
    RTC_LOG(INFO) << "Requesting video stream " << w << "x" << h
                  << ", from:" << handle;


    // Video frames..
    ::webrtc::VideoFrame currentFrame =
            ::webrtc::VideoFrame::Builder()
                    .set_video_frame_buffer(mI420Buffer)
                    .set_timestamp_rtp(0)
                    .set_timestamp_us(rtc::TimeMicros())
                    .set_rotation(::webrtc::kVideoRotation_0)
                    .build();
    Image img;

    ::grpc::CompletionQueue cq;
    ::grpc::Status status;
    bool warned = false;
    void *got_tag, *my_tag = this;
    auto rpc = mClient->stub<android::emulation::control::EmulatorController>()->PrepareAsyncstreamScreenshot(context.get(),
                                                             request, &cq);

    // Start the async rpc.
    rpc->StartCall((void*) 1);
    rpc->Read(&img, my_tag);
    bool ok = false, completed = false;
    do {

        // Wait at most 500ms for the next frame, so we can deliver at least 2 fps to the decoder
        // if needed. The decoder will drop double frames as we don't modify timestamps.
        // new connections however will get an immediate frame.
        auto state =
                cq.AsyncNext(&got_tag, &ok,
                             grpc_timeout_to_deadline(std::chrono::milliseconds(500)));
        // RTC_LOG(INFO) << "Got state: " << state << " tag " << (uint64_t) got_tag << " == " << (uint64_t) my_tag;
        switch (state) {
            case ::grpc::CompletionQueue::NextStatus::GOT_EVENT:
                if (ok && got_tag == my_tag) {
                    if (img.format().rotation().rotation() !=
                                Rotation::PORTRAIT &&
                        !warned) {
                        RTC_LOG(WARNING)
                                << "Rotation not yet properly supported.";
                        warned = true;
                    };

                    uint8_t* data = (uint8_t*)img.image().data();
                    size_t cData = img.image().size();

                    if (img.format().has_transport() &&
                        img.format().transport().channel() ==
                                ImageTransport::MMAP) {
                        data = (uint8_t*)shm.get();
                        cData = shm.size();
                    }

                    auto converted = libyuv::ConvertToI420(
                            data, cData, mI420Buffer.get()->MutableDataY(),
                            mI420Buffer.get()->StrideY(),
                            mI420Buffer.get()->MutableDataU(),
                            mI420Buffer.get()->StrideU(),
                            mI420Buffer.get()->MutableDataV(),
                            mI420Buffer.get()->StrideV(),
                            /*crop_x=*/0,
                            /*crop_y=*/0, w, h, w, h, libyuv::kRotate0,
                            libyuv::FourCC::FOURCC_RGB3);
                    currentFrame =
                            ::webrtc::VideoFrame::Builder()
                                    .set_video_frame_buffer(mI420Buffer)
                                    .set_timestamp_rtp(0)
                                    .set_timestamp_us(rtc::TimeMicros())
                                    .set_rotation(::webrtc::kVideoRotation_0)
                                    .build();
                    rpc->Read(&img, my_tag);
                }
            case ::grpc::CompletionQueue::NextStatus::TIMEOUT:
                // We update the timetamp to make sure the encoder does not immediately rejects
                // the frame.
                currentFrame.set_timestamp_us(rtc::TimeMicros());
                assert(currentFrame.video_frame_buffer());
                mBroadcaster.OnFrame(currentFrame);
                break;
            case ::grpc::CompletionQueue::NextStatus::SHUTDOWN:
                completed = true;
                break;
        }
    } while (mCaptureVideo && !completed && ok);

    RTC_LOG(INFO) << "Completed video stream.";
    tempfile_close(tmp);
}

void GrpcVideoSource::cancel() {
    LOG(INFO) << "Cancelling stream";
    if (auto context = mContext.lock()) {
        context->TryCancel();
    }
    mCaptureVideo = false;
}

void GrpcVideoSource::run() {
    mCaptureVideo = true;
    captureFrames();
}

GrpcVideoSource::~GrpcVideoSource() {
    cancel();
}

bool GrpcVideoSource::GetStats(Stats* stats) {
    const std::lock_guard<std::mutex> lock(mStatsMutex);

    if (!mStats) {
        return false;
    }

    *stats = *mStats;
    return true;
}

void GrpcVideoSource::AddOrUpdateSink(
        rtc::VideoSinkInterface<::webrtc::VideoFrame>* sink,
        const rtc::VideoSinkWants& wants) {
    mBroadcaster.AddOrUpdateSink(sink, wants);
    OnSinkWantsChanged(mBroadcaster.wants());
}

void GrpcVideoSource::RemoveSink(
        rtc::VideoSinkInterface<::webrtc::VideoFrame>* sink) {
    mBroadcaster.RemoveSink(sink);
    OnSinkWantsChanged(mBroadcaster.wants());
}

void GrpcVideoSource::OnSinkWantsChanged(const rtc::VideoSinkWants& wants) {
    // TODO(jansene): Set the desired pixel count based on wants.
    RTC_LOG(WARNING) << "OnSinkWantsChanged desired pixels: "
                     << wants.target_pixel_count.value_or(0)
                     << ", max: " << wants.max_pixel_count
                     << ", fps: " << wants.max_framerate_fps
                     << ", align: " << wants.resolution_alignment;
    mVideoAdapter.OnSinkWants(wants);
}

}  // namespace webrtc
}  // namespace emulator
