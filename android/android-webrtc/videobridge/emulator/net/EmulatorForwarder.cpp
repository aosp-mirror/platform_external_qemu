
// Copyright (C) 2021 The Android Open Source Project
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
#include "emulator/net/EmulatorForwarder.h"

#include <assert.h>         // for assert
#include <grpcpp/grpcpp.h>  // for Status
#include <libyuv.h>
#include <atomic>              // for atomic
#include <memory>              // for unique_ptr
#include <ratio>               // for ratio
#include <thread>              // for thread
#include <unordered_map>       // for unorder...
#include <utility>             // for move
#include "android/base/Log.h"  // for LogStre...
#include "android/base/async/AsyncSocketServer.h"
#include "android/emulation/control/EmulatorAdvertisement.h"  // for Emulato...
#include "android/emulation/control/GrpcServices.h"           // for Emulato...
#include "android/emulation/control/utils/ScreenshotUtils.h"
#include "android/emulation/control/utils/SharedMemoryLibrary.h"
#include "android/utils/Random.h"  // for generat...
#include "api/video/i420_buffer.h"
#include "emulator/net/EmulatorGrcpClient.h"  // for Emulato...
#include "emulator_controller.grpc.pb.h"      // for Emulato...
#include "emulator_controller.pb.h"           // for AudioPa...
#ifdef _WIN32
#include <windows.h>

#undef ERROR
#include <errno.h>
#include <stdio.h>
#endif

#include <stdlib.h>  // for size_t
#include <chrono>    // for operator+

extern "C" {
#include "android/proxy/proxy_int.h"  // for proxy_b...

namespace android {
namespace base {
class PathUtils;
class System;
}  // namespace base
namespace emulation {
namespace control {
class AEMUControllerForwarderImpl;
}  // namespace control
}  // namespace emulation
}  // namespace android
namespace google {
namespace protobuf {
class Empty;
}  // namespace protobuf
}  // namespace google
}

#define FWD_DEBUG 0
#if FWD_DEBUG >= 1
#define DD(fmt, ...) \
    printf("Fwd: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define DD(fmt, ...) (void)0
#endif

using android::base::PathUtils;
using android::base::System;
using emulator::webrtc::EmulatorGrpcClient;
using ::google::protobuf::Empty;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using grpc::StatusCode;

#define fwdOne(fn, in, out)                                                 \
    Status fn(ServerContext* context, const in* request, out* reply)        \
            override {                                                      \
        auto ctx = mClient->newContext();                                   \
        ctx->set_deadline(std::chrono::system_clock::now() +                \
                          std::chrono::seconds(1));                         \
        return checkAlive(mClient->stub()->fn(ctx.get(), *request, reply)); \
    }

#define fwdMany(fn, in, out)                                    \
    Status fn(ServerContext* context, const in* request,        \
              ServerWriter<out>* writer) override {             \
        auto ctx = mClient->newContext();                       \
        auto stream = mClient->stub()->fn(ctx.get(), *request); \
        return forwardCall(writer, stream.get());               \
    }
namespace android {
namespace emulation {
namespace control {

class AEMUControllerForwarderImpl final : public EmulatorController::Service {
private:
    // 1 -> Many
    template <class T>
    Status forwardCall(ServerWriter<T>* origin, ClientReader<T>* destination) {
        T msg;
        while (destination->Read(&msg) && origin->Write(msg)) {
            DD("1 --> Many");
        }
        DD("1 --> Many: completed");
        return checkAlive(destination->Finish());
    }

public:
    AEMUControllerForwarderImpl(EmulatorGrpcClient* client,
                                EmulatorForwarder* fwd)
        : mClient(client), mForwarder(fwd){};

    fwdOne(getLogcat, LogMessage, LogMessage);
    fwdMany(streamLogcat, LogMessage, LogMessage);

    fwdOne(setBattery, BatteryState, Empty);
    fwdOne(getBattery, Empty, BatteryState);

    fwdOne(getGps, Empty, GpsState);
    fwdOne(setGps, GpsState, Empty);

    fwdOne(setPhysicalModel, PhysicalModelValue, Empty);
    fwdOne(getPhysicalModel, PhysicalModelValue, PhysicalModelValue);

    fwdOne(getClipboard, Empty, ClipData);
    fwdOne(setClipboard, ClipData, Empty);
    fwdMany(streamClipboard, Empty, ClipData);

    fwdOne(getSensor, SensorValue, SensorValue);
    fwdOne(setSensor, SensorValue, Empty);

    fwdOne(sendFingerprint, Fingerprint, Empty);

    fwdOne(getStatus, Empty, EmulatorStatus);

    fwdOne(getDisplayConfigurations, Empty, DisplayConfigurations);
    fwdOne(setDisplayConfigurations,
           DisplayConfigurations,
           DisplayConfigurations);

    fwdOne(setVmState, VmRunState, Empty);
    fwdOne(getVmState, Empty, VmRunState);

    fwdOne(sendSms, SmsMessage, PhoneResponse);
    fwdOne(sendPhone, PhoneCall, PhoneResponse);

    fwdMany(streamNotification, Empty, Notification);

    fwdOne(getBrightness, BrightnessValue, BrightnessValue);
    fwdOne(setBrightness, BrightnessValue, Empty);

    Status getScreenshot(ServerContext* context,
                         const ImageFormat* request,
                         Image* reply) override {
        uint32_t width, height;
        bool enabled;

        auto receiver = mForwarder->webrtcConnection()->videoReceiver();
        if (receiver == nullptr || receiver->currentFrame() == nullptr) {
            // No WebRTC connection.. Fall back to gRPC?
            return Status::OK;
        }
        width = receiver->currentFrame()->width();
        height = receiver->currentFrame()->height();
        reply->set_timestampus(System::get()->getUnixTimeUs());
        android::emulation::ImageFormat desiredFormat =
                ScreenshotUtils::translate(request->format());

        float xaxis = 0, yaxis = 0, zaxis = 0;

        // TODO(jansene): WebRTC does not yet support multi displays..

        int desiredWidth = request->width();
        int desiredHeight = request->height();

        // TODO(jansene): Get actual rotation from webrtc endpoint?
        auto rotation = Rotation::PORTRAIT;
        SkinRotation desiredRotation = ScreenshotUtils::translate(rotation);

        // User wants to use device width/height
        if (desiredWidth == 0 || desiredHeight == 0) {
            desiredWidth = width;
            desiredHeight = height;

            if (rotation == Rotation::LANDSCAPE ||
                rotation == Rotation::REVERSE_LANDSCAPE) {
                std::swap(desiredWidth, desiredHeight);
            }
        }

        // Calculate width and height, keeping aspect ration in mind.
        auto [newWidth, newHeight] = ScreenshotUtils::resizeKeepAspectRatio(
                width, height, desiredWidth, desiredHeight);

        auto frame = ::webrtc::I420Buffer::Create(newWidth, newHeight);
        frame->CropAndScaleFrom(*receiver->currentFrame()->GetI420());

        // Update format information with the retrieved width, height..
        auto format = reply->mutable_format();
        format->set_format(request->format());
        format->set_height(frame->height());
        format->set_width(frame->width());

        auto pixelCount = request->width() * request->height() *
                          ScreenshotUtils::getBytesPerPixel(*request);
        uint8_t* dest = nullptr;
        bool shipBytes = false;

        if (request->transport().channel() == ImageTransport::MMAP) {
            auto shm = mSharedMemoryLibrary.borrow(
                    request->transport().handle(), pixelCount);
            if (shm->isOpen() && shm->isMapped()) {
                dest = (uint8_t*)shm.get();
                auto transport = format->mutable_transport();
                transport->set_handle(request->transport().handle());
                transport->set_channel(ImageTransport::MMAP);
            }
        }

        // Make sure we have a dest. memory region.
        if (dest == nullptr) {
            dest = (uint8_t*)malloc(pixelCount);
            shipBytes = true;
        }

        auto fmt = request->format() == ImageFormat::RGB888
                           ? libyuv::FOURCC_RAW
                           : libyuv::FOURCC_ARGB;

        auto result = ConvertFromI420(frame->DataY(), frame->StrideY(),
                                      frame->DataU(), frame->StrideU(),
                                      frame->DataV(), frame->StrideV(), dest, 0,
                                      frame->width(), frame->height(), fmt);

        auto rotation_reply = format->mutable_rotation();
        rotation_reply->set_xaxis(xaxis);
        rotation_reply->set_yaxis(yaxis);
        rotation_reply->set_zaxis(zaxis);
        rotation_reply->set_rotation(rotation);

        if (shipBytes) {
            reply->set_image(dest, pixelCount);
            free(dest);
        }

        return Status::OK;
    }

    Status streamScreenshot(ServerContext* context,
                            const ImageFormat* request,
                            ServerWriter<Image>* writer) override {
        SharedMemoryLibrary::LibraryEntry entry;
        if (request->transport().channel() == ImageTransport::MMAP) {
            entry = mSharedMemoryLibrary.borrow(
                    request->transport().handle(),
                    request->width() * request->height() *
                            ScreenshotUtils::getBytesPerPixel(*request));
        }

        // Make sure we always write the first frame, this can be
        // a completely empty frame if the screen is not active.
        Image first;
        bool clientAvailable = !context->IsCancelled();

        if (clientAvailable) {
            getScreenshot(context, request, &first);
            clientAvailable = !context->IsCancelled() && writer->Write(first);
        }

        auto receiver = mForwarder->webrtcConnection()->videoReceiver();
        if (receiver == nullptr || receiver->currentFrame() == nullptr) {
            // Oh oh, no webrtc connection (yet?). Fallback to forwarder?
            return Status::OK;
        }

        bool lastFrameWasEmpty = first.format().width() == 0;
        int frame = 0;
        // Track percentiles, and report if we have seen at least 32 frames.
        while (clientAvailable) {
            Image reply;
            const auto kTimeToWaitForFrame = std::chrono::milliseconds(125);

            // The next call will return the number of frames that are
            // available. 0 means no frame was made available in the given
            // time interval. Since this is a synchronous call we want to
            // wait at most kTimeToWaitForFrame so we can check if the
            // client is still there. (All clients get disconnected on
            // emulator shutdown).
            auto arrived = receiver->next(kTimeToWaitForFrame);
            if (arrived > 0 && !context->IsCancelled()) {
                frame += arrived;
                getScreenshot(context, request, &reply);
                reply.set_seq(frame);

                // We send the first empty frame, after that we wait for
                // frames to come, or until the client gives up on us. So
                // for a screen that comes in and out the client will see
                // this timeline: (0 is empty frame-> F is frame) [0, ...
                // <nothing> ..., F1, F2, F3, 0, ...<nothing>... ]
                bool emptyFrame = reply.format().width() == 0;
                if (!context->IsCancelled() &&
                    (!lastFrameWasEmpty || !emptyFrame)) {
                    clientAvailable = writer->Write(reply);
                }
                lastFrameWasEmpty = emptyFrame;
            }
            clientAvailable = !context->IsCancelled() && clientAvailable;
        }

        return Status::OK;
    }

    fwdMany(streamAudio, AudioFormat, AudioPacket);

    Status sendKey(ServerContext* context,
                   const KeyboardEvent* keyEvent,
                   ::google::protobuf::Empty* reply) override {
        mForwarder->webrtcConnection()->sendKey(keyEvent);
        return Status::OK;
    }

    Status sendMouse(ServerContext* context,
                     const MouseEvent* mouseEvent,
                     ::google::protobuf::Empty* reply) override {
        mForwarder->webrtcConnection()->sendMouse(mouseEvent);
        return Status::OK;
    }

    Status sendTouch(ServerContext* context,
                     const TouchEvent* touchEvent,
                     ::google::protobuf::Empty* reply) override {
        mForwarder->webrtcConnection()->sendTouch(touchEvent);
        return Status::OK;
    }

private:
    Status checkAlive(const Status& state) {
        bool alive = true;
        if (state.error_code() == StatusCode::UNAVAILABLE ||
            !mClient->hasOpenChannel(false) &&
                    mAlive.compare_exchange_strong(alive, true)) {
            // Notify closing on a separate thread.
            std::thread disconnect([=]() { mForwarder->close(); });
            disconnect.detach();
        }

        return state;
    }

    EmulatorGrpcClient* mClient;
    EmulatorForwarder* mForwarder;
    SharedMemoryLibrary mSharedMemoryLibrary;
    std::atomic<bool> mAlive{true};
    std::atomic<int> mCtxId;
};

static std::unique_ptr<EmulatorAdvertisement> advertiser;

// Generates a secure base64 encoded token of
// |cnt| bytes.
static std::string generateToken(int cnt) {
    char buf[cnt];
    if (!android::generateRandomBytes(buf, sizeof(buf))) {
        return "";
    }

    const size_t kBase64Len = 4 * ((cnt + 2) / 3);
    char base64[kBase64Len];
    int len = proxy_base64_encode(buf, cnt, base64, kBase64Len);
    // len < 0 can only happen if we calculate kBase64Len incorrectly..
    assert(len > 0);
    return std::string(base64, len);
}

void EmulatorForwarder::wait() {
    if (mFwdService) {
        mFwdService->wait();
    }
}

EmulatorForwarder::EmulatorForwarder(EmulatorGrpcClient* client, int adbPort, TurnConfig turnConfig)
    : mClient(client), mWebRtcConnection(client, adbPort, turnConfig), mAvd(client){};

EmulatorForwarder::~EmulatorForwarder() {}

void EmulatorForwarder::close() {
    const std::lock_guard<std::mutex> lock(mCloseMutex);
    if (!mAlive) {
        return;
    }
    mAlive = false;
    LOG(INFO) << "Stopping webrtc..";
    mWebRtcConnection.close();
}

void EmulatorForwarder::stopService() {
    LOG(INFO) << "Stopping service..";
    mFwdService->stop();
}

bool EmulatorForwarder::createRemoteConnection() {
    mAvd.garbageCollect();
    if (!mAvd.create()) {
        return false;
    }

    // Establish a webrtc connection, and wait for it to be available.
    // TODO(jansene): Make it lazy, connect to getScreenshot?
    mWebRtcConnection.connect();
    mWebRtcConnection.registerConnectionClosedListener([&] { stopService(); });

    // Wait at most a few seconds, for a frame.
    auto frame =
            mWebRtcConnection.videoReceiver()->next(std::chrono::seconds(25));
    if (frame == 0) {
        LOG(ERROR) << "No frames received after 5 seconds, bailing out!";
        return false;
    } else {
        LOG(INFO) << "First frame arrived, registering discovery mechanism.";
    }

    const int of64Bytes = 64;
    auto token = generateToken(of64Bytes);
    EmulatorProperties props{
            {"port.serial", "5554"},  // TODO(jansene) remove?
            {"port.adb", "5555"},     // TODO(jansene): Setup forwarder.
            {"avd.name", mAvd.name()},
            {"avd.dir", mAvd.dir()},
            {"avd.id", mAvd.id()},
            {"grpc.token", token},
            // Note the commandline below is merely to convince studio to
            // see this as an emulator.
            {"cmdline",
             R"#("qemu-system-x86_64-headless" "-netdelay" "none" "-netspeed" "full" "-avd" "P" "-qt-hide-window" "-grpc-use-token" "-idle-grpc-timeout" "300")#"}};

    std::string address = "localhost";
    int grpc_start = 9000;
    int grpc_end = 10000;
    auto builder = EmulatorControllerService::Builder()
                           .withPortRange(grpc_start, grpc_end)
                           .withVerboseLogging(true)
                           .withAddress(address)
                           .withAuthToken(token)
                           .withService(new AEMUControllerForwarderImpl(mClient,
                                                                        this));

    mFwdService = builder.build();
    if (!mFwdService) {
        LOG(ERROR) << "Unable to start gRPC service";
    }

    mAlive = true;
    auto port = mFwdService->port();
    props["grpc.port"] = std::to_string(port);

    mAdvertiser = std::make_unique<EmulatorAdvertisement>(std::move(props));
    mAdvertiser->garbageCollect();
    if (!mAdvertiser->write()) {
        LOG(ERROR) << "Failed to write advertising file.";
        return false;
    }

    return true;
}

}  // namespace control
}  // namespace emulation
}  // namespace android
