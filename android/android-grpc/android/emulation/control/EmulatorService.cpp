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

#include "android/emulation/control/EmulatorService.h"

#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <sys/time.h>
#endif

#include <grpcpp/grpcpp.h>
#include <stdio.h>
#include <string.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "android/base/Log.h"
#include "android/base/system/System.h"
#include "android/console.h"
#include "android/emulation/LogcatPipe.h"
#include "android/emulation/control/RtcBridge.h"
#include "android/emulation/control/ScreenCapturer.h"
#include "android/emulation/control/battery_agent.h"
#include "android/emulation/control/display_agent.h"
#include "android/emulation/control/finger_agent.h"
#include "android/emulation/control/interceptor/LoggingInterceptor.h"
#include "android/emulation/control/keyboard/EmulatorKeyEventSender.h"
#include "android/emulation/control/keyboard/TouchEventSender.h"
#include "android/emulation/control/location_agent.h"
#include "android/emulation/control/logcat/LogcatParser.h"
#include "android/emulation/control/logcat/RingStreambuf.h"
#include "android/emulation/control/sensors_agent.h"
#include "android/emulation/control/telephony_agent.h"
#include "android/emulation/control/user_event_agent.h"
#include "android/emulation/control/utils/EventWaiter.h"
#include "android/emulation/control/utils/ScreenshotUtils.h"
#include "android/emulation/control/utils/ServiceUtils.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"
#include "android/gpu_frame.h"
#include "android/hw-sensors.h"
#include "android/opengles.h"
#include "android/physics/Physics.h"
#include "android/skin/rect.h"
#include "android/version.h"
#include "emulator_controller.grpc.pb.h"
#include "emulator_controller.pb.h"

namespace google {
namespace protobuf {
class Empty;
}  // namespace protobuf
}  // namespace google

using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using namespace android::base;
using namespace android::control::interceptor;
using ::google::protobuf::Empty;

namespace android {
namespace emulation {
namespace control {

// Logic and data behind the server's behavior.
class EmulatorControllerImpl final : public EmulatorController::Service {
public:
    EmulatorControllerImpl(const AndroidConsoleAgents* agents)
        : mAgents(agents),
          mLogcatBuffer(k128KB),
          mKeyEventSender(agents),
          mTouchEventSender(agents) {
        // the logcat pipe will take ownership of the created stream, and writes
        // to our buffer.
        LogcatPipe::registerStream(new std::ostream(&mLogcatBuffer));
    }

    Status getLogcat(ServerContext* context,
                     const LogMessage* request,
                     LogMessage* reply) override {
        auto message = mLogcatBuffer.bufferAtOffset(request->start(), kNoWait);
        if (request->sort() == LogMessage::Parsed) {
            auto parsed = LogcatParser::parseLines(message.second);
            reply->set_start(message.first);
            for (auto entry : parsed.second) {
                *reply->add_entries() = entry;
            }
            reply->set_next(message.first + parsed.first);
        } else {
            reply->set_start(message.first);
            reply->set_contents(message.second);
            reply->set_next(message.first + message.second.size());
        }
        return Status::OK;
    }

    Status streamLogcat(ServerContext* context,
                        const LogMessage* request,
                        ServerWriter<LogMessage>* writer) override {
        LogMessage log;
        log.set_next(request->start());
        do {
            // When streaming, block at most 5 seconds before sending any status
            // This also makes sure we check that the clients is still around at
            // least once every 5 seconds.
            auto message =
                    mLogcatBuffer.bufferAtOffset(log.next(), k5SecondsWait);
            if (request->sort() == LogMessage::Parsed) {
                auto parsed = LogcatParser::parseLines(message.second);
                log.clear_entries();
                log.set_start(message.first);
                for (auto entry : parsed.second) {
                    *log.add_entries() = entry;
                }
                log.set_next(message.first + parsed.first);
            } else {
                log.set_start(message.first);
                log.set_contents(message.second);
                log.set_next(message.first + message.second.size());
            }
        } while (writer->Write(log));
        return Status::OK;
    }

    Status setBattery(ServerContext* context,
                      const BatteryState* request,
                      Empty* reply) override {
        auto battery = mAgents->battery;
        battery->setHasBattery(request->hasbattery());
        battery->setIsBatteryPresent(request->ispresent());
        battery->setIsCharging(request->status() == BatteryState::CHARGING);
        battery->setCharger((BatteryCharger)request->charger());
        battery->setChargeLevel(request->chargelevel());
        battery->setHealth((BatteryHealth)request->health());
        battery->setStatus((BatteryStatus)request->status());
        return Status::OK;
    }

    Status getBattery(ServerContext* context,
                      const Empty* request,
                      BatteryState* reply) override {
        auto battery = mAgents->battery;
        reply->set_hasbattery(battery->hasBattery());
        reply->set_ispresent(battery->present());
        reply->set_charger((BatteryState_BatteryCharger)battery->charger());
        reply->set_chargelevel(battery->chargeLevel());
        reply->set_health((BatteryState_BatteryHealth)battery->health());
        reply->set_status((BatteryState_BatteryStatus)battery->status());
        return Status::OK;
    }

    Status setGps(ServerContext* context,
                  const GpsState* request,
                  Empty* reply) override {
        auto location = mAgents->location;
        struct timeval tVal;
        memset(&tVal, 0, sizeof(tVal));
        gettimeofday(&tVal, NULL);

        location->gpsSetPassiveUpdate(request->passiveupdate());
        location->gpsSendLoc(request->latitude(), request->longitude(),
                             request->altitude(), request->speed(),
                             request->bearing(), request->satellites(), &tVal);
        return Status::OK;
    }

    Status getGps(ServerContext* context,
                  const Empty* request,
                  GpsState* reply) override {
        auto location = mAgents->location;
        double lat, lon, speed, heading, elevation;
        int32_t count;

        // TODO(jansene):Implement in underlying agent.
        reply->set_passiveupdate(location->gpsGetPassiveUpdate());
        location->gpsGetLoc(&lat, &lon, &elevation, &speed, &heading, &count);

        reply->set_latitude(lat);
        reply->set_longitude(lon);
        reply->set_speed(speed);
        reply->set_bearing(heading);
        reply->set_altitude(elevation);
        reply->set_satellites(count);
        return Status::OK;
    }

    Status setPhysicalModel(ServerContext* context,
                            const PhysicalModelValue* request,
                            Empty* reply) override {
        auto values = request->value();
        int size = values.data().size();
        float a = size > 0 ? values.data(0) : 0;
        float b = size > 1 ? values.data(1) : 0;
        float c = size > 2 ? values.data(2) : 0;

        mAgents->sensors->setPhysicalParameterTarget(
                (int)request->target(), a, b, c,
                PhysicalInterpolation::PHYSICAL_INTERPOLATION_SMOOTH);
        return Status::OK;
    }

    Status getPhysicalModel(ServerContext* context,
                            const PhysicalModelValue* request,
                            PhysicalModelValue* reply) override {
        float a = 0, b = 0, c = 0;
        int state = mAgents->sensors->getPhysicalParameter(
                (int)request->target(), &a, &b, &c,
                PARAMETER_VALUE_TYPE_CURRENT);
        auto value = reply->mutable_value();
        value->add_data(a);
        value->add_data(b);
        value->add_data(c);
        reply->set_status((PhysicalModelValue_State)state);
        reply->set_target(request->target());
        return Status::OK;
    }

    Status setSensor(ServerContext* context,
                     const SensorValue* request,
                     Empty* reply) override {
        auto values = request->value();
        int size = values.data().size();
        float a = size > 0 ? values.data(0) : 0;
        float b = size > 1 ? values.data(1) : 0;
        float c = size > 2 ? values.data(2) : 0;

        mAgents->sensors->setSensorOverride((int)request->target(), a, b, c);
        return Status::OK;
    }

    Status getSensor(ServerContext* context,
                     const SensorValue* request,
                     SensorValue* reply) override {
        float a = 0, b = 0, c = 0;
        int state =
                mAgents->sensors->getSensor((int)request->target(), &a, &b, &c);
        auto value = reply->mutable_value();
        value->add_data(a);
        value->add_data(b);
        value->add_data(c);
        reply->set_status((SensorValue_State)state);
        reply->set_target(request->target());
        return Status::OK;
    }

    Status sendFingerprint(ServerContext* context,
                           const Fingerprint* request,
                           Empty* reply) override {
        mAgents->finger->setTouch(request->istouching(), request->touchid());
        return Status::OK;
    }

    Status sendKey(ServerContext* context,
                   const KeyboardEvent* request,
                   Empty* reply) override {
        mKeyEventSender.send(request);
        return Status::OK;
    }

    Status sendMouse(ServerContext* context,
                     const MouseEvent* request,
                     Empty* reply) override {
        mAgents->user_event->sendMouseEvent(request->x(), request->y(), 0,
                                            request->buttons(), 0);
        return Status::OK;
    }

    Status sendTouch(ServerContext* context,
                     const TouchEvent* request,
                     Empty* reply) override {
        mTouchEventSender.send(request);
        return Status::OK;
    }

    Status getStatus(ServerContext* context,
                     const Empty* request,
                     EmulatorStatus* reply) override {
        ::VmConfiguration config;
        mAgents->vm->getVmConfiguration(&config);
        reply->mutable_vmconfig()->set_hypervisortype(
                (VmConfiguration_VmHypervisorType)(config.hypervisorType));
        reply->mutable_vmconfig()->set_numberofcpucores(
                config.numberOfCpuCores);
        reply->mutable_vmconfig()->set_ramsizebytes(config.ramSizeBytes);
        reply->set_booted(bootCompleted());
        reply->set_uptime(System::get()->getProcessTimes().wallClockMs);
        reply->set_version(std::string(EMULATOR_VERSION_STRING) + " (" +
                           std::string(EMULATOR_FULL_VERSION_STRING) + ")");

        auto entries = reply->mutable_hardwareconfig();
        auto cnf = getQemuConfig();
        for (auto const& entry : cnf) {
            auto response_entry = entries->add_entry();
            response_entry->set_key(entry.first);
            response_entry->set_value(entry.second);
        };

        return Status::OK;
    }

    Status streamScreenshot(ServerContext* context,
                            const ImageFormat* request,
                            ServerWriter<Image>* writer) override {
        EventWaiter frameEvent(&gpu_register_shared_memory_callback,
                               &gpu_unregister_shared_memory_callback);
        bool clientAvailable = !context->IsCancelled();
        bool lastFrameWasEmpty = false;
        while (clientAvailable) {
            Image reply;
            const auto kTimeToWaitForFrame = std::chrono::milliseconds(500);

            // The next call will return the number of frames that are
            // available. 0 means no frame was made available in the given time
            // interval. Since this is a synchronous call we want to wait at
            // most kTimeToWaitForFrame so we can check if the client is still
            // there. (All clients get disconnected on emulator shutdown).
            auto frame = frameEvent.next(kTimeToWaitForFrame);
            if (frame > 0) {
                // TODO(jansene): Add metrics around dropped frames/timing?
                getScreenshot(context, request, &reply);

                // We send the first empty frame, after that we wait for frames
                // to come, or until the client gives up on us. So for a screen
                // that comes in and out the client will see this timeline: (0
                // is empty frame. F is frame) [0, ... <nothing> ..., F1, F2,
                // F3, 0, ...<nothing>... ]
                bool emptyFrame = reply.format().width() == 0;
                if (!lastFrameWasEmpty || !emptyFrame) {
                    clientAvailable = writer->Write(reply);
                }
                lastFrameWasEmpty = emptyFrame;
            }
            clientAvailable = !context->IsCancelled() && clientAvailable;
        }
        return Status::OK;
    };

    Status getScreenshot(ServerContext* context,
                         const ImageFormat* request,
                         Image* reply) override {
        uint32_t width, height;
        bool enabled;
        mAgents->emu->getMultiDisplay(request->display(), nullptr, nullptr,
                                      &width, &height, nullptr, nullptr,
                                      &enabled);
        if (!enabled)
            return Status::OK;

        android::emulation::ImageFormat desiredFormat =
                ScreenshotUtils::translate(request->format());

        float xaxis = 0, yaxis = 0, zaxis = 0;

        // TODO(jansene): Not clear if the rotation impacts displays other
        // than 0.
        if (request->display() == 0) {
            mAgents->sensors->getPhysicalParameter(
                    PHYSICAL_PARAMETER_ROTATION, &xaxis, &yaxis, &zaxis,
                    PARAMETER_VALUE_TYPE_CURRENT);
        }
        auto rotation = ScreenshotUtils::coarseRotation(zaxis);

        // Calculate the desired rotation and width we should use..
        int desiredWidth = request->width();
        int desiredHeight = request->height();
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

        // Depending on the rotation state width and height need to be reversed.
        // as our apsect ration depends on how we are holding our phone..
        if (rotation == Rotation::LANDSCAPE ||
            rotation == Rotation::REVERSE_LANDSCAPE) {
            std::swap(width, height);
        }

        // Calculate widht and height, keeping aspect ration in mind.
        auto [newWidth, newHeight] = ScreenshotUtils::resizeKeepAspectRatio(
                width, height, desiredWidth, desiredHeight);

        // The screenshot produces a rotated result and will just simply flip
        // width and height.
        if (rotation == Rotation::LANDSCAPE ||
            rotation == Rotation::REVERSE_LANDSCAPE) {
            std::swap(newWidth, newHeight);
        }

        // Screenshots can come from either the gl renderer, or the guest.
        const auto& renderer = android_getOpenglesRenderer();
        android::emulation::Image img = android::emulation::takeScreenshot(
                desiredFormat, desiredRotation, renderer.get(),
                mAgents->display->getFrameBuffer, request->display(), newWidth,
                newHeight);

        reply->set_image(img.getPixelBuf(), img.getPixelCount());

        // Update format information with the retrieved width, height..
        auto format = reply->mutable_format();
        format->set_format(ScreenshotUtils::translate(img.getImageFormat()));
        format->set_height(img.getHeight());
        format->set_width(img.getWidth());

        auto rotation_reply = format->mutable_rotation();
        rotation_reply->set_xaxis(xaxis);
        rotation_reply->set_yaxis(yaxis);
        rotation_reply->set_zaxis(zaxis);
        rotation_reply->set_rotation(rotation);

        return Status::OK;
    }

    Status sendPhone(ServerContext* context,
                     const PhoneCall* request,
                     PhoneResponse* reply) override {
        // We assume that the int mappings are consistent..
        TelephonyOperation operation = (TelephonyOperation)request->operation();
        std::string phoneNr = request->number();
        TelephonyResponse response =
                mAgents->telephony->telephonyCmd(operation, phoneNr.c_str());
        reply->set_response((PhoneResponse_Response)response);
        return Status::OK;
    }

private:
    const AndroidConsoleAgents* mAgents;
    keyboard::EmulatorKeyEventSender mKeyEventSender;
    TouchEventSender mTouchEventSender;
    RingStreambuf
            mLogcatBuffer;  // A ring buffer that tracks the logcat output.

    static constexpr uint32_t k128KB = (128 * 1024) - 1;
    static constexpr uint16_t k5SecondsWait = 5 * 1000;
    const uint16_t kNoWait = 0;
};

grpc::Service* getEmulatorController(const AndroidConsoleAgents* agents) {
    return  new EmulatorControllerImpl(agents);
}

}  // namespace control
}  // namespace emulation
}  // namespace android
