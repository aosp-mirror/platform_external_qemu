
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

#include <assert.h>                                           // for assert
#include <grpcpp/grpcpp.h>                                    // for Status
#include <atomic>                                             // for atomic
#include <memory>                                             // for unique_ptr
#include <ratio>                                              // for ratio
#include <thread>                                             // for thread
#include <unordered_map>                                      // for unorder...
#include <utility>                                            // for move

#include "android/base/Log.h"                                 // for LogStre...
#include "android/emulation/control/EmulatorAdvertisement.h"  // for Emulato...
#include "android/emulation/control/GrpcServices.h"           // for Emulato...
#include "android/utils/Random.h"                             // for generat...
#include "emulator/net/EmulatorGrcpClient.h"                  // for Emulato...
#include "emulator_controller.grpc.pb.h"                      // for Emulato...
#include "emulator_controller.pb.h"                           // for AudioPa...

#ifdef _WIN32
#include <windows.h>

#undef ERROR
#include <errno.h>
#include <stdio.h>
#endif

#include <stdlib.h>                                           // for size_t
#include <chrono>                                             // for operator+

extern "C" {
#include "android/proxy/proxy_int.h"                          // for proxy_b...

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

    // TODO: These should go over WebRTC..
    fwdOne(getScreenshot, ImageFormat, Image);
    fwdMany(streamScreenshot, ImageFormat, Image);
    fwdMany(streamAudio, AudioFormat, AudioPacket);
    fwdOne(sendKey, KeyboardEvent, Empty);
    fwdOne(sendMouse, MouseEvent, Empty);
    fwdOne(sendTouch, TouchEvent, Empty);

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
    std::atomic<bool> mAlive{true};
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

EmulatorForwarder::EmulatorForwarder(EmulatorGrpcClient* client)
    : mClient(client), mAvd(client){};

EmulatorForwarder::~EmulatorForwarder() {
    close();
}

void EmulatorForwarder::close() {
    const std::lock_guard<std::mutex> lock(mCloseMutex);
    if (!mAlive) {
        return;
    }
    mAlive = false;

    LOG(INFO) << "Stopping service..";
    mFwdService->stop();
}

bool EmulatorForwarder::createRemoteConnection() {
    mAvd.garbageCollect();
    if (!mAvd.create()) {
        return false;
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
            // Note the commandline below is merely to convince studio to see
            // this as an emulator.
            {"cmdline",
             R"#("qemu-system-x86_64-headless" "-netdelay" "none" "-netspeed" "full" "-avd" "P" "-qt-hide-window" "-grpc-use-token" "-idle-grpc-timeout" "300")#"}};

    std::string address = "127.0.0.1";
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
