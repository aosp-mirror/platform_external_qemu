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

#include <api/video/video_frame.h>             // for VideoFrame::Builder
#include <api/video/video_frame_buffer.h>      // for VideoFrameBuffer
#include <api/video/video_rotation.h>          // for kVideoRotation_0
#include <api/video/video_source_interface.h>  // for VideoSinkWants
#include <grpcpp/grpcpp.h>                     // for ClientContext, ClientR...
#include <rtc_base/logging.h>                  // for RTC_LOG
#include <rtc_base/time_utils.h>               // for TimeMicros
#include <stdint.h>                            // for uint8_t
#include <memory>                              // for unique_ptr, operator==
#include <string>                              // for operator==, stoi, string
#include <thread>                              // for thread
#include <tuple>                               // for make_tuple, tuple_elem...

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

GrpcVideoSource::GrpcVideoSource(EmulatorGrpcClient client) : mClient(client) {}

std::tuple<int, int> getScreenDimensions(EmulatorGrpcClient client) {
    ::google::protobuf::Empty empty;

    auto context = client.newContext();
    int w = 1080, h = 1920;
    EmulatorStatus emuState;
    auto status = client.stub()->getStatus(context.get(), empty, &emuState);
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

    mContext = mClient.newContext();
    RTC_LOG(INFO) << "Requesting video stream " << w << "x" << h
                  << ", from:" << handle;
    std::unique_ptr<grpc::ClientReaderInterface<Image>> stream =
            mClient.stub()->streamScreenshot(mContext.get(), request);

    if (stream == nullptr) {
        RTC_LOG(WARNING) << "Video: unable to obtain stream.." << handle;
        tempfile_close(tmp);
        return;
    }

    bool warned = false;
    Image img;
    while (stream->Read(&img)) {
        if (img.format().rotation().rotation() != Rotation::PORTRAIT &&
            !warned) {
            RTC_LOG(WARNING) << "Rotation not yet properly supported.";
            warned = true;
        };

        uint8_t* data = (uint8_t*)img.image().data();
        size_t cData = img.image().size();

        if (img.format().has_transport() &&
            img.format().transport().channel() == ImageTransport::MMAP) {
            data = (uint8_t*)shm.get();
            cData = shm.size();
        }

        auto converted = libyuv::ConvertToI420(
                data, cData, mI420Buffer.get()->MutableDataY(),
                mI420Buffer.get()->StrideY(), mI420Buffer.get()->MutableDataU(),
                mI420Buffer.get()->StrideU(), mI420Buffer.get()->MutableDataV(),
                mI420Buffer.get()->StrideV(),
                /*crop_x=*/0,
                /*crop_y=*/0, w, h, w, h, libyuv::kRotate0,
                libyuv::FourCC::FOURCC_RGB3);
        auto frame = ::webrtc::VideoFrame::Builder()
                             .set_video_frame_buffer(mI420Buffer)
                             .set_timestamp_rtp(0)
                             .set_timestamp_us(rtc::TimeMicros())
                             .set_rotation(::webrtc::kVideoRotation_0)
                             .build();

        mBroadcaster.OnFrame(frame);
    }

    RTC_LOG(INFO) << "Completed video stream.";
    tempfile_close(tmp);
}

GrpcVideoSource::~GrpcVideoSource() {
    if (mContext) {
        mContext->TryCancel();
    }
    if (mCaptureVideo) {
        mVideoThread.join();
    }
    mCaptureVideo = false;
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

    bool expected = false;
    if (mClient.stub() &&
        mCaptureVideo.compare_exchange_strong(expected, true)) {
        mVideoThread = std::thread([this]() { captureFrames(); });
    }
    mVideoAdapter.OnSinkWants(wants);
}

}  // namespace webrtc
}  // namespace emulator
