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

#include <assert.h>
#include <grpcpp/grpcpp.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <ratio>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "aemu/base/EventNotificationSupport.h"
#include "aemu/base/Stopwatch.h"
#include "aemu/base/Tracing.h"
#include "aemu/base/async/Looper.h"
#include "aemu/base/async/ThreadLooper.h"
#include "aemu/base/logging/Log.h"
#include "aemu/base/logging/LogSeverity.h"
#include "aemu/base/memory/ScopedPtr.h"
#include "aemu/base/memory/SharedMemory.h"
#include "android/avd/info.h"
#include "android/base/system/System.h"
#include "android/console.h"
#include "android/emulation/LogcatPipe.h"
#include "android/emulation/control/ScreenCapturer.h"
#include "android/emulation/control/ServiceUtils.h"
#include "android/emulation/control/audio/AudioStream.h"
#include "android/emulation/control/battery_agent.h"
#include "android/emulation/control/camera/VirtualSceneCamera.h"
#include "android/emulation/control/clipboard/Clipboard.h"
#include "android/emulation/control/finger_agent.h"
#include "android/emulation/control/hw_control_agent.h"
#include "android/emulation/control/interceptor/LoggingInterceptor.h"
#include "android/emulation/control/keyboard/AndroidEventSender.h"
#include "android/emulation/control/keyboard/KeyEventSender.h"
#include "android/emulation/control/keyboard/MouseEventSender.h"
#include "android/emulation/control/keyboard/PenEventSender.h"
#include "android/emulation/control/keyboard/TouchEventSender.h"
#include "android/emulation/control/keyboard/WheelEventSender.h"
#include "android/emulation/control/location_agent.h"
#include "android/emulation/control/logcat/LogcatParser.h"
#include "android/emulation/control/logcat/LogcatStream.h"
#include "android/emulation/control/notifications/NotificationStream.h"
#include "android/emulation/control/sensors_agent.h"
#include "android/emulation/control/telephony_agent.h"
#include "android/emulation/control/utils/AudioUtils.h"
#include "android/emulation/control/utils/EventWaiter.h"
#include "android/emulation/control/utils/ScreenshotUtils.h"
#include "android/emulation/control/utils/SharedMemoryLibrary.h"
#include "android/emulation/resizable_display_config.h"
#include "android/gpu_frame.h"
#include "android/grpc/utils/SimpleAsyncGrpc.h"
#include "android/hw-sensors.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/Percentiles.h"
#include "android/physics/Physics.h"
#include "android/skin/rect.h"
#include "android/skin/winsys.h"
#include "android/telephony/gsm.h"
#include "android/telephony/modem.h"
#include "android/telephony/sms.h"
#include "android/utils/debug.h"
#include "android/version.h"
#include "android_modem_v2.h"
#include "emulator_controller.grpc.pb.h"
#include "emulator_controller.pb.h"
#include "host-common/FeatureControl.h"
#include "host-common/Features.h"
#include "host-common/MultiDisplay.h"
#include "host-common/hw-config.h"
#include "host-common/misc.h"
#include "host-common/opengles.h"
#include "host-common/vm_operations.h"
#include "render-utils/Renderer.h"
#include "studio_stats.pb.h"

using ::google::protobuf::Empty;
using ::grpc::ServerContext;
using ::grpc::ServerWriter;
using ::grpc::Status;
using namespace android::base;
// using namespace android::control::interceptor;
using namespace std::chrono_literals;

namespace android {
namespace emulation {
namespace control {

// Logic and data behind the server's behavior.
class EmulatorControllerImpl final
    : public EmulatorController::WithCallbackMethod_streamLogcat<
              EmulatorController::WithCallbackMethod_streamClipboard<
                      EmulatorController::WithCallbackMethod_streamNotification<
                              EmulatorController::WithCallbackMethod_streamInputEvent<
                                      EmulatorController::
                                              WithCallbackMethod_injectWheel<
                                                      EmulatorController::
                                                              Service>>>>> {
public:
    EmulatorControllerImpl(const AndroidConsoleAgents* agents)
        : mAgents(agents),
          mKeyEventSender(agents),
          mAndroidEventSender(agents),
          mMouseEventSender(agents),
          mPenEventSender(agents),
          mTouchEventSender(agents),
          mWheelEventSender(agents),
          mCamera(agents->sensors),
          mClipboard(Clipboard::getClipboard(agents->clipboard)),
          mLooper(android::base::ThreadLooper::get()),
          mNotificationStream(&mCamera, agents) {}

    virtual ::grpc::ServerWriteReactor<
            ::android::emulation::control::LogMessage>*
    streamLogcat(
            ::grpc::CallbackServerContext* /*context*/,
            const ::android::emulation::control::LogMessage* request) override {
        auto stream =
                new LogcatEventStreamWriter(&AdbLogcat::instance(), *request);
        return stream;
    }

    Status setBattery(ServerContext* context,
                      const BatteryState* requestPtr,
                      ::google::protobuf::Empty* reply) override {
        auto agent = mAgents->battery;

        // Make a copy
        BatteryState request = *requestPtr;

        android::base::ThreadLooper::runOnMainLooper([agent, request]() {
            agent->setHasBattery(request.hasbattery());
            agent->setIsBatteryPresent(request.ispresent());
            agent->setIsCharging(request.status() == BatteryState::CHARGING);
            agent->setCharger((BatteryCharger)request.charger());
            agent->setChargeLevel(request.chargelevel());
            agent->setHealth((BatteryHealth)request.health());
            agent->setStatus((BatteryStatus)request.status());
        });

        return Status::OK;
    }

    Status getBattery(ServerContext* context,
                      const Empty* request,
                      BatteryState* reply) override {
        auto agent = mAgents->battery;

        android::base::ThreadLooper::runOnMainLooperAndWaitForCompletion(
                [agent, reply]() {
                    reply->set_hasbattery(agent->hasBattery());
                    reply->set_ispresent(agent->present());
                    reply->set_charger(
                            (BatteryState_BatteryCharger)agent->charger());
                    reply->set_chargelevel(agent->chargeLevel());
                    reply->set_health(
                            (BatteryState_BatteryHealth)agent->health());
                    reply->set_status(
                            (BatteryState_BatteryStatus)agent->status());
                });

        return Status::OK;
    }

    Status setGps(ServerContext* context,
                  const GpsState* requestPtr,
                  ::google::protobuf::Empty* reply) override {
        auto agent = mAgents->location;
        GpsState request = *requestPtr;

        android::base::ThreadLooper::runOnMainLooperAndWaitForCompletion(
                [agent, request]() {
                    struct timeval tVal;
                    memset(&tVal, 0, sizeof(tVal));
                    gettimeofday(&tVal, NULL);
                    agent->gpsSendLoc(request.latitude(), request.longitude(),
                                      request.altitude(), request.speed(),
                                      request.bearing(), request.satellites(),
                                      &tVal);
                });

        return Status::OK;
    }

    Status getGps(ServerContext* context,
                  const Empty* request,
                  GpsState* reply) override {
        auto agent = mAgents->location;

        android::base::ThreadLooper::runOnMainLooperAndWaitForCompletion(
                [agent, request, reply]() {
                    double lat, lon, speed, heading, elevation;
                    int32_t count;

                    // TODO(jansene):Implement in underlying agent.
                    reply->set_passiveupdate(agent->gpsGetPassiveUpdate());
                    agent->gpsGetLoc(&lat, &lon, &elevation, &speed, &heading,
                                     &count);

                    reply->set_latitude(lat);
                    reply->set_longitude(lon);
                    reply->set_speed(speed);
                    reply->set_bearing(heading);
                    reply->set_altitude(elevation);
                    reply->set_satellites(count);
                });

        return Status::OK;
    }

    Status setPhysicalModel(ServerContext* context,
                            const PhysicalModelValue* request,
                            Empty* reply) override {
        auto values = request->value();
        mAgents->sensors->setPhysicalParameterTarget(
                (int)request->target(), values.data().data(),
                values.data().size(),
                PhysicalInterpolation::PHYSICAL_INTERPOLATION_STEP);
        return Status::OK;
    }

    Status getPhysicalModel(ServerContext* context,
                            const PhysicalModelValue* request,
                            PhysicalModelValue* reply) override {
        size_t size;
        mAgents->sensors->getPhysicalParameterSize((int)request->target(),
                                                   &size);
        std::vector<float> val(size, 0);
        std::vector<float*> out;
        for (size_t i = 0; i < val.size(); i++) {
            out.push_back(&val[i]);
        }

        int state = mAgents->sensors->getPhysicalParameter(
                (int)request->target(), out.data(), out.size(),
                PARAMETER_VALUE_TYPE_CURRENT);
        auto value = reply->mutable_value();
        for (size_t i = 0; i < val.size(); i++) {
            value->add_data(val[i]);
        }
        reply->set_status((PhysicalModelValue_State)state);
        reply->set_target(request->target());
        return Status::OK;
    }

    Status setClipboard(ServerContext* context,
                        const ClipData* clipData,
                        ::google::protobuf::Empty* reply) override {
        android::base::ThreadLooper::runOnMainLooperAndWaitForCompletion(
                [=]() { mClipboard->sendContents(clipData->text(), context); });
        return Status::OK;
    }

    Status getClipboard(ServerContext* context,
                        const ::google::protobuf::Empty* empty,
                        ClipData* reply) override {
        reply->set_text(mClipboard->getCurrentContents());
        return Status::OK;
    }

    ::grpc::ServerWriteReactor<ClipData>* streamClipboard(
            ::grpc::CallbackServerContext* context,
            const ::google::protobuf::Empty* /*request*/) override {
        return mClipboard->eventWriter(context);
    }

    Status setSensor(ServerContext* context,
                     const SensorValue* requestPtr,
                     ::google::protobuf::Empty* reply) override {
        auto agent = mAgents->sensors;
        SensorValue request = *requestPtr;

        android::base::ThreadLooper::runOnMainLooper([agent, request]() {
            auto values = request.value();
            int size = values.data().size();
            agent->setSensorOverride((int)request.target(),
                                     values.data().data(),
                                     values.data().size());
        });
        return Status::OK;
    }

    Status getSensor(ServerContext* context,
                     const SensorValue* request,
                     SensorValue* reply) override {
        size_t size = 0;
        int state =
                mAgents->sensors->getSensorSize((int)request->target(), &size);
        VERBOSE_INFO(grpc, "Received sensor state: %d, size: %d (%s)", state,
                     size, state == SENSOR_STATUS_OK ? "ok" : "not ok");
        if (state != SENSOR_STATUS_OK) {
            derror("Unable to retrieve sensor status for %s",
                   request->ShortDebugString());
            return Status(grpc::StatusCode::INTERNAL,
                          "Failed to retrieve sensor size, error: " +
                                  std::to_string(state));
        }
        std::vector<float> val(size, 0);
        std::vector<float*> out;
        for (size_t i = 0; i < val.size(); i++) {
            out.push_back(&val[i]);
        }

        state = mAgents->sensors->getSensor((int)request->target(), out.data(),
                                            out.size());

        auto value = reply->mutable_value();
        for (size_t i = 0; i < size; i++) {
            value->add_data(val[i]);
        }
        reply->set_status((SensorValue_State)state);
        reply->set_target(request->target());
        VERBOSE_INFO(grpc, "Received sensor state: %s (%s)",
                     reply->ShortDebugString(),
                     state == SENSOR_STATUS_OK ? "ok" : "not ok");
        return Status::OK;
    }

    static const char* getBacklightName(BrightnessValue::LightType lightType) {
        switch (lightType) {
            case BrightnessValue_LightType_LCD:
                return "lcd_backlight";
            case BrightnessValue_LightType_KEYBOARD:
                return "keyboard_backlight";
            case BrightnessValue_LightType_BUTTON:
                return "button_backlight";
            default:
                DCHECK(false) << "Unreachable";
                return "INVALID";
        }
    }

    Status setBrightness(ServerContext* context,
                         const BrightnessValue* requestPtr,
                         ::google::protobuf::Empty* reply) override {
        auto agent = mAgents->hw_control;
        BrightnessValue request = *requestPtr;

        if (agent == nullptr || agent->setBrightness == nullptr) {
            return Status(::grpc::StatusCode::FAILED_PRECONDITION,
                          "setBrightness: hw-control agent function not "
                          "available.",
                          "");
        }

        if (request.value() > UINT8_MAX) {
            return Status(::grpc::StatusCode::INVALID_ARGUMENT,
                          "setBrightness: brightness value out of boundary.",
                          "");
        }

        android::base::ThreadLooper::runOnMainLooper([agent, request]() {
            const char* light_name = getBacklightName(request.target());
            auto value = request.value();
            agent->setBrightness(light_name, value);
        });
        return Status::OK;
    }

    Status getBrightness(ServerContext* context,
                         const BrightnessValue* request,
                         BrightnessValue* reply) override {
        const char* light_name = getBacklightName(request->target());

        auto agent = mAgents->hw_control;
        if (agent == nullptr || agent->getBrightness == nullptr) {
            return Status(::grpc::StatusCode::FAILED_PRECONDITION,
                          "getBrightness: hw-control agent function not "
                          "available.",
                          "");
        }

        auto value = agent->getBrightness(light_name);
        reply->set_target(request->target());
        reply->set_value(value);
        return Status::OK;
    }

    Status setDisplayMode(ServerContext* context,
                          const DisplayMode* requestPtr,
                          ::google::protobuf::Empty* reply) override {
        if (!resizableEnabled()) {
            return Status(::grpc::StatusCode::FAILED_PRECONDITION,
                          ":setDisplayMode the AVD is not resizable.", "");
        }
        auto agent = mAgents->emu;
        if (agent == nullptr || agent->changeResizableDisplay == nullptr) {
            return Status(::grpc::StatusCode::INTERNAL,
                          "setDisplayMode: window agent function is not "
                          "available.",
                          "");
        }
        agent->changeResizableDisplay(
                static_cast<PresetEmulatorSizeType>(requestPtr->value()));
        return Status::OK;
    }

    Status getDisplayMode(ServerContext* context,
                          const ::google::protobuf::Empty* empty,
                          DisplayMode* reply) override {
        if (!resizableEnabled()) {
            return Status(::grpc::StatusCode::FAILED_PRECONDITION,
                          ":getDisplayMode the AVD is not resizable.", "");
        }
        reply->set_value(
                static_cast<DisplayModeValue>(getResizableActiveConfigId()));
        return Status::OK;
    }

    Status sendFingerprint(ServerContext* context,
                           const Fingerprint* requestPtr,
                           ::google::protobuf::Empty* reply) override {
        auto agent = mAgents->finger;
        Fingerprint request = *requestPtr;

        android::base::ThreadLooper::runOnMainLooper([agent, request]() {
            agent->setTouch(request.istouching(), request.touchid());
        });

        return Status::OK;
    }

    Status sendKey(ServerContext* context,
                   const KeyboardEvent* request,
                   ::google::protobuf::Empty* reply) override {
        mKeyEventSender.send(*request);
        return Status::OK;
    }

    Status sendMouse(ServerContext* context,
                     const MouseEvent* request,
                     ::google::protobuf::Empty* reply) override {
        mMouseEventSender.send(*request);
        return Status::OK;
    }

    Status sendTouch(ServerContext* context,
                     const TouchEvent* request,
                     ::google::protobuf::Empty* reply) override {
        mTouchEventSender.send(*request);
        return Status::OK;
    }

    virtual ::grpc::ServerReadReactor<WheelEvent>* injectWheel(
            ::grpc::CallbackServerContext* /*context*/,
            ::google::protobuf::Empty* /*response*/) override {
        return new SimpleServerLambdaReader<WheelEvent>(
                [this](auto request) { mWheelEventSender.send(*request); });
    }

    virtual ::grpc::ServerReadReactor<InputEvent>* streamInputEvent(
            ::grpc::CallbackServerContext* /*context*/,
            ::google::protobuf::Empty* /*response*/) override {
        SimpleServerLambdaReader<InputEvent>* eventReader =
                new SimpleServerLambdaReader<InputEvent>(
                        [this, &eventReader](auto request) {
                            VERBOSE_INFO(keys, "InputEvent: %s", request->ShortDebugString());
                            if (request->has_key_event()) {
                                mKeyEventSender.send(request->key_event());
                            } else if (request->has_mouse_event()) {
                                mMouseEventSender.send(request->mouse_event());
                            } else if (request->has_touch_event()) {
                                mTouchEventSender.send(request->touch_event());
                            } else if (request->has_android_event()) {
                                mAndroidEventSender.send(request->android_event());
                            } else if (request->has_pen_event()) {
                                mPenEventSender.send(request->pen_event());
                            } else if (request->has_wheel_event()) {
                                mWheelEventSender.send(request->wheel_event());
                            } else {
                                // Mark the stream as completed, this will
                                // result in setting that status and scheduling
                                // of a completion (onDone) event the async
                                // queue.
                                eventReader->Finish(Status(
                                        ::grpc::StatusCode::INVALID_ARGUMENT,
                                        "Unknown event, is the emulator out of date?."));
                            }
                        });
        // Note that the event reader will delete itself on completion of
        // the request.
        return eventReader;
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
    Status injectAudio(ServerContext* context,
                       ::grpc::ServerReader<AudioPacket>* reader,
                       ::google::protobuf::Empty* reply) override {
        // We currently only support one microphone, so make sure we
        // don't have multiple microphones registered.
        int8_t expectActive = 0;
        if (!mInjectAudioCount.compare_exchange_strong(expectActive, 1)) {
            return Status(::grpc::StatusCode::FAILED_PRECONDITION,
                          "There can be only one microphone active", "");
        }

        // Make sure we decrement the active microphone counter.
        assert(mInjectAudioCount == 1);
        auto decrementActiveCount = android::base::makeCustomScopedPtr(
                &mInjectAudioCount, [](auto counter) {
                    assert(*counter == 1);
                    counter->fetch_sub(1);
                    assert(*counter == 0);
                });

        AudioPacket pkt;
        if (!reader->Read(&pkt)) {
            return Status::OK;
        }

        if (pkt.format().samplingrate() > 48000) {
            return Status(::grpc::StatusCode::INVALID_ARGUMENT,
                          "The desired sampling rate is too high (>48khz) ",
                          "");
        }

        // Increasing the buffer size can result in longer waiting periods
        // before closing down the emulator.
        milliseconds audioQueueTime = 300ms;
        QemuAudioInputStream aos(pkt.format(), 100ms, audioQueueTime);
        if (!aos.good()) {
            return Status(::grpc::StatusCode::FAILED_PRECONDITION,
                          "Unable to register microphone.", "");
        }
        bool clientAlive = !context->IsCancelled();
        while (clientAlive) {
            // Let's not accept extremely large packets..
            if (pkt.audio().size() > aos.capacity()) {
                return Status(::grpc::StatusCode::INVALID_ARGUMENT,
                              "The audio buffer can store at most " +
                                      std::to_string(aos.capacity()) +
                                      " bytes, not: " +
                                      std::to_string(pkt.audio().size()),
                              "");
            }

            // Write out a packet while we are alive..
            const char* buf = pkt.audio().data();
            size_t cBuf = pkt.audio().size();
            while (cBuf > 0 && !context->IsCancelled()) {
                size_t written = aos.write(buf, cBuf);
                buf = buf + written;
                cBuf -= written;
            }

            // Get the next packet if we are alive.
            clientAlive = !context->IsCancelled() && reader->Read(&pkt);
        }

        // The circular buffer contains at most audioQueueTime ms of
        // samples, We need to deliver those before disconnecting the audio
        // stack to guarantee we deliver all the samples.
        const char silence[512] = {0};
        size_t toWrite = aos.capacity();
        auto timeout = std::chrono::system_clock::now() + audioQueueTime;

        // 2 cases:
        // - The emulator is requesting audio, we deliver the whole queue
        // - The emulator is not requesting audio, we time out.
        while (toWrite > 0 && std::chrono::system_clock::now() < timeout) {
            toWrite -= aos.write(silence, sizeof(silence));
        }

        return Status::OK;
    }

    Status streamAudio(ServerContext* context,
                       const AudioFormat* request,
                       ServerWriter<AudioPacket>* writer) override {
        // The source number of samples per audio frame in Qemu, 512 samples
        // is fixed
        constexpr int kSrcNumSamples = 512;

        // Translate external settings to internal qemu settings.
        int channels = AudioUtils::getChannels(*request);
        int sampleRate = request->samplingrate();

        // Set the default sample rate when it is not set.
        if (sampleRate == 0) {
            sampleRate = 44100;
        }

        AudioPacket packet;
        AudioFormat* format = packet.mutable_format();
        format->set_channels(request->channels());
        format->set_format(request->format());
        format->set_samplingrate(sampleRate);

        auto kTimeToWaitForAudioFrame = std::chrono::milliseconds(30);
        QemuAudioOutputStream audioStream(*format, kTimeToWaitForAudioFrame);

        int32_t kBytesPerSample = AudioUtils::getBytesPerSample(*format);
        int32_t kSamplesPerFrame =
                (sampleRate / 1000) * kTimeToWaitForAudioFrame.count();
        size_t kBytesPerFrame = kBytesPerSample * kSamplesPerFrame * channels;

        // Pre allocate the packet buffer.
        auto buffer = new std::string(kBytesPerFrame, 0);
        packet.set_allocated_audio(buffer);

        bool clientAlive = true;
        do {
            buffer->resize(kBytesPerFrame);
            auto size = audioStream.read(&packet);
            if (size > 0) {
                clientAlive = writer->Write(packet);
            }
            clientAlive = clientAlive && !context->IsCancelled();
        } while (clientAlive);

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
        Image reply;

        // cPixels is used to verify the invariant that retrieved image
        // is not shrinking over subsequent calls, as this might result
        // in unexpected behavior for clients.
        int cPixels = reply.image().size();
        bool clientAvailable = !context->IsCancelled();

        if (clientAvailable) {
            auto status = getScreenshot(context, request, &reply);
            if (!status.ok()) {
                return status;
            }

            assert(reply.image().size() >= cPixels);
            cPixels = reply.image().size();
            clientAvailable = !context->IsCancelled() && writer->Write(reply);
        }

        bool lastFrameWasEmpty = reply.format().width() == 0;
        int frame = 0;

        // TODO(jansen): Move to ScreenshotUtils.
        std::unique_ptr<EventWaiter> frameEvent;
        std::unique_ptr<RaiiEventListener<gfxstream::Renderer,
                                          gfxstream::FrameBufferChangeEvent>>
                frameListener;
        // Screenshots can come from either the gl renderer, or the guest.
        const auto& renderer = android_getOpenglesRenderer();
        if (renderer.get()) {
            // Fast mode..
            frameEvent = std::make_unique<EventWaiter>();
            frameListener = std::make_unique<RaiiEventListener<
                    gfxstream::Renderer, gfxstream::FrameBufferChangeEvent>>(
                    renderer.get(),
                    [&](const gfxstream::FrameBufferChangeEvent state) {
                        frameEvent->newEvent();
                    });
        } else {
            // slow mode, you are likely using older api..
            LOG(DEBUG) << "Reverting to slow callbacks";
            frameEvent = std::make_unique<EventWaiter>(
                    &gpu_register_shared_memory_callback,
                    &gpu_unregister_shared_memory_callback);
        }

        std::unique_ptr<EventWaiter> sensorEvent;
        sensorEvent = std::make_unique<EventWaiter>(
                &android_hw_sensors_register_callback,
                &android_hw_sensors_unregister_callback);

        // Track percentiles, and report if we have seen at least 32 frames.
        metrics::Percentiles perfEstimator(32, {0.5, 0.95});
        while (clientAvailable) {
            const auto kTimeToWaitForFrame = std::chrono::milliseconds(125);

            // The next call will return the number of frames that are
            // available. 0 means no frame was made available in the given
            // time interval. Since this is a synchronous call we want to
            // wait at most kTimeToWaitForFrame so we can check if the
            // client is still there. (All clients get disconnected on
            // emulator shutdown).
            auto framesArrived = frameEvent->next(kTimeToWaitForFrame);

            // Also check for sensor change events, and send a new frame
            // if needed. It is possible for the sensor state to change
            // without a new frame being rendered.
            auto sensorsChanged = sensorEvent->next(std::chrono::milliseconds(0));

            if ((framesArrived || sensorsChanged) && !context->IsCancelled()) {
                AEMU_SCOPED_TRACE("streamScreenshot::frame\r\n");
                frame += framesArrived;                Stopwatch sw;
                auto status = getScreenshot(context, request, &reply);
                if (!status.ok()) {
                    return status;
                }

                // The invariant that the pixel buffer does not decrease
                // should hold. Clients likely rely on the buffer size to
                // match the actual number of available pixels.
                assert(reply.image().size() >= cPixels);
                cPixels = reply.image().size();

                reply.set_seq(frame);
                // We send the first empty frame, after that we wait for
                // frames to come, or until the client gives up on us. So
                // for a screen that comes in and out the client will see
                // this timeline: (0 is empty frame. F is frame) [0, ...
                // <nothing> ..., F1, F2, F3, 0, ...<nothing>... ]
                bool emptyFrame = reply.format().width() == 0;
                if (!context->IsCancelled() &&
                    (!lastFrameWasEmpty || !emptyFrame)) {
                    AEMU_SCOPED_TRACE("streamScreenshot::write");
                    clientAvailable = writer->Write(reply);
                    perfEstimator.addSample(sw.elapsedUs());
                }
                lastFrameWasEmpty = emptyFrame;
            }
            clientAvailable = !context->IsCancelled() && clientAvailable;
        }

        // Only report metrics if we have a constant size per image, and
        // if we delivered sufficient amount of frames.
        if (perfEstimator.isBucketized() &&
            (request->format() == ImageFormat::RGB888 ||
             request->format() == ImageFormat::RGBA8888)) {
            auto size = reply.format().width() * reply.format().height() *
                        ScreenshotUtils::getBytesPerPixel(*request);
            android::metrics::MetricsReporter::get().report(
                    [size, frame,
                     perfEstimator](android_studio::AndroidStudioEvent* event) {
                        auto screenshot = event->mutable_emulator_details()
                                                  ->mutable_screenshot();
                        screenshot->set_size(size);
                        screenshot->set_frames(frame);
                        // We care about median, 95%, and 100%.
                        // (max enables us to calculate # of dropped
                        // frames.)
                        perfEstimator.fillMetricsEvent(
                                screenshot->mutable_delivery_delay(),
                                {0.5, 0.95, 1.0});
                    });
        }
        return Status::OK;
    }

    Status getScreenshot(ServerContext* context,
                         const ImageFormat* request,
                         Image* reply) override {
        AEMU_SCOPED_TRACE_CALL();
        uint32_t width, height;
        bool enabled = true;
        bool multiDisplayQueryWorks = mAgents->emu->getMultiDisplay(
                request->display(), nullptr, nullptr, &width, &height, nullptr,
                nullptr, &enabled);

        // b/248394093 Initialize the value of width and height to the
        // default hw settings regardless of the display ID when multi
        // display agent doesn't work.
        if (!multiDisplayQueryWorks) {
            width = getConsoleAgents()->settings->hw()->hw_lcd_width;
            height = getConsoleAgents()->settings->hw()->hw_lcd_height;
        }

        int myDisplayId = -1;
        const bool is_pixel_fold = android_foldable_is_pixel_fold();
        const bool not_pixel_fold = !is_pixel_fold;
        bool isFolded = android_foldable_is_folded();

        if (is_pixel_fold && isFolded) {
            width = getConsoleAgents()
                            ->settings->hw()
                            ->hw_displayRegion_0_1_width;
            height = getConsoleAgents()
                             ->settings->hw()
                             ->hw_displayRegion_0_1_height;
            myDisplayId = android_foldable_pixel_fold_second_display_id();
        }

        if (!enabled) {
            return Status(
                    ::grpc::StatusCode::INVALID_ARGUMENT,
                    "Invalid display: " + std::to_string(request->display()),
                    "");
        }

        // The folded screen is represented as a rectangle within the full
        // screen.
        SkinRect rect = {{0, 0}, {0, 0}};
        if (isFolded) {
            android_foldable_get_folded_area(&rect.pos.x, &rect.pos.y,
                                             &rect.size.w, &rect.size.h);
            auto foldedDisplay =
                    reply->mutable_format()->mutable_foldeddisplay();
            foldedDisplay->set_width(rect.size.w);
            foldedDisplay->set_height(rect.size.h);
            foldedDisplay->set_xoffset(rect.pos.x);
            foldedDisplay->set_yoffset(rect.pos.y);
            if (is_pixel_fold) {
                // reset rect
                rect = {{0, 0}, {0, 0}};
            }
        } else {
            reply->mutable_format()->clear_foldeddisplay();
        }
        if (resizableEnabled()) {
            reply->mutable_format()->set_displaymode(
                    static_cast<DisplayModeValue>(
                            getResizableActiveConfigId()));
        }

        reply->set_timestampus(System::get()->getUnixTimeUs());
        android::emulation::ImageFormat desiredFormat =
                ScreenshotUtils::translate(request->format());

        float xaxis = 0, yaxis = 0, zaxis = 0;
        auto out = {&xaxis, &yaxis, &zaxis};

        // TODO(jansene): Not clear if the rotation impacts displays other
        // than 0.
        mAgents->sensors->getPhysicalParameter(PHYSICAL_PARAMETER_ROTATION,
                                               out.begin(), out.size(),
                                               PARAMETER_VALUE_TYPE_CURRENT);
        auto rotation = ScreenshotUtils::deriveRotation(mAgents->sensors);

        int desiredWidth = request->width();
        int desiredHeight = request->height();

        // User wants to use device width/height
        if (desiredWidth == 0 || desiredHeight == 0) {
            desiredWidth = width;
            desiredHeight = height;

            if (rotation == Rotation::LANDSCAPE ||
                rotation == Rotation::REVERSE_LANDSCAPE) {
                std::swap(desiredWidth, desiredHeight);
            }
        }

        // Depending on the rotation state width and height need to be
        // reversed. as our apsect ration depends on how we are holding our
        // phone..
        if (rotation == Rotation::LANDSCAPE ||
            rotation == Rotation::REVERSE_LANDSCAPE) {
            std::swap(width, height);
            if (not_pixel_fold && isFolded) {
                std::swap(rect.pos.x, rect.pos.y);
                std::swap(rect.size.w, rect.size.h);
            }
        }

        // Note that we will never scale above the device width and height.
        desiredWidth = std::min<int>(desiredWidth, width);
        desiredHeight = std::min<int>(desiredHeight, height);

        // Calculate width and height, keeping aspect ratio in mind.
        int newWidth, newHeight;
        if (not_pixel_fold && isFolded) {
            // When screen is folded, resize based on the aspect ratio of
            // the folded screen instead of full screen.
            int newFoldedWidth, newFoldedHeight;
            std::tie(newFoldedWidth, newFoldedHeight) =
                    ScreenshotUtils::resizeKeepAspectRatio(
                            rect.size.w, rect.size.h, desiredWidth,
                            desiredHeight);
            rect.pos.x = rect.pos.x * newFoldedWidth / rect.size.w;
            rect.pos.y = rect.pos.y * newFoldedHeight / rect.size.h;
            newWidth = width * newFoldedWidth / rect.size.w;
            newHeight = height * newFoldedHeight / rect.size.h;
            rect.size.w = newFoldedWidth;
            rect.size.h = newFoldedHeight;
        } else {
            std::tie(newWidth, newHeight) =
                    ScreenshotUtils::resizeKeepAspectRatio(
                            width, height, desiredWidth, desiredHeight);
        }

        // The screenshot produces a rotated result and will just simply
        // flip width and height.
        if (rotation == Rotation::LANDSCAPE ||
            rotation == Rotation::REVERSE_LANDSCAPE) {
            std::swap(newWidth, newHeight);
            if (not_pixel_fold && isFolded) {
                std::swap(rect.pos.x, rect.pos.y);
                std::swap(rect.size.w, rect.size.h);
            }
        }

        // This is an expensive operation, that frequently can get
        // cancelled, so check availability of the client first.
        if (context->IsCancelled()) {
            return Status::CANCELLED;
        }

        uint8_t* pixels;
        size_t cPixels = 0;
        bool isPNG = request->format() ==
                     ImageFormat_ImgFormat::ImageFormat_ImgFormat_PNG;
        android::emulation::Image img(
                0, 0, 0, android::emulation::ImageFormat::RGB888, {});
        // For png format, only make one getScreenshot call because the
        // cPixels value can change for each call.
        if (isPNG) {
            img = ScreenshotUtils::getScreenshot(
                    myDisplayId >= 0 ? myDisplayId : request->display(),
                    request->format(), rotation, newWidth, newHeight, &width,
                    &height, rect);
            cPixels = img.getPixelCount();
        } else {  // Let's make a fast call to learn how many pixels we need
                  // to reserve.
            while (1) {
                width = 0;
                height = 0;
                cPixels = 0;
                ScreenshotUtils::getScreenshot(
                        myDisplayId >= 0 ? myDisplayId : request->display(),
                        request->format(), rotation, newWidth, newHeight, pixels,
                        &cPixels, &width, &height, rect);
                if (width > 0 && height > 0 && cPixels > 0) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        auto format = reply->mutable_format();
        format->set_format(request->format());
        format->set_display(myDisplayId >= 0 ? myDisplayId
                                             : request->display());

        auto rotation_reply = format->mutable_rotation();
        rotation_reply->set_xaxis(xaxis);
        rotation_reply->set_yaxis(yaxis);
        rotation_reply->set_zaxis(zaxis);
        rotation_reply->set_rotation(rotation);
        if (request->transport().channel() == ImageTransport::MMAP) {
            auto shm = mSharedMemoryLibrary.borrow(
                    request->transport().handle(), cPixels);
            if (shm->isOpen() && shm->isMapped()) {
                if (cPixels > shm->size()) {
                    // Oh oh, this will not work.
                    return Status(::grpc::StatusCode::FAILED_PRECONDITION,
                                  "The shared memory region needs to have a "
                                  "size of at least: " +
                                          std::to_string(cPixels),
                                  "");
                }
                cPixels = shm->size();
                pixels = reinterpret_cast<uint8_t*>(shm->get());

                auto transport = format->mutable_transport();
                transport->set_handle(request->transport().handle());
                transport->set_channel(ImageTransport::MMAP);
            }
        } else {
            // Make sure the image field has a string that is large enough.
            // Usually there are 2 cases:
            // - A single screenshot is requested, we need to allocate a
            // buffer of sufficient size
            // - The first screenshot in a series is requested and we need
            // to allocate the first frame.
            if (reply->mutable_image()->size() != cPixels) {
                LOG(DEBUG) << "Allocation of string object. "
                           << reply->mutable_image()->size() << " < "
                           << cPixels;
                auto buffer = new std::string(cPixels, 0);
                // The protobuf message takes ownership of the pointer.
                reply->set_allocated_image(buffer);
            }
            // The underlying char* is r/w since c++17.
            char* unsafe = reply->mutable_image()->data();
            pixels = reinterpret_cast<uint8_t*>(unsafe);
        }

        Stopwatch sw;
        if (isPNG) {
            memcpy(pixels, img.getPixelBuf(), cPixels);
        } else {
            ScreenshotUtils::getScreenshot(
                    myDisplayId >= 0 ? myDisplayId : request->display(), request->format(), rotation, newWidth,
                    newHeight, pixels, &cPixels, &width, &height, rect);
        }
        // Update format information with the retrieved width, height.
        format->set_height(height);
        format->set_width(width);
        VERBOSE_PRINT(grpc, "Screenshot %dx%d (%s), pixels: %d in %d us.",
                      width, height, rotation_reply->ShortDebugString().c_str(),
                      cPixels, sw.elapsedUs());

        if (isFolded) {
            format->set_display(request->display());
        }
        return Status::OK;
    }

    Status getVmState(ServerContext* context,
                      const ::google::protobuf::Empty* request,
                      VmRunState* reply) override {
        switch (mAgents->vm->getRunState()) {
            case QEMU_RUN_STATE_PAUSED:
            case QEMU_RUN_STATE_SUSPENDED:
                reply->set_state(VmRunState::PAUSED);
                break;
            case QEMU_RUN_STATE_RESTORE_VM:
                reply->set_state(VmRunState::RESTORE_VM);
                break;
            case QEMU_RUN_STATE_RUNNING:
                reply->set_state(VmRunState::RUNNING);
                break;
            case QEMU_RUN_STATE_SAVE_VM:
                reply->set_state(VmRunState::SAVE_VM);
                break;
            case QEMU_RUN_STATE_SHUTDOWN:
                reply->set_state(VmRunState::SHUTDOWN);
                break;
            case QEMU_RUN_STATE_GUEST_PANICKED:
            case QEMU_RUN_STATE_INTERNAL_ERROR:
            case QEMU_RUN_STATE_IO_ERROR:
                reply->set_state(VmRunState::INTERNAL_ERROR);
                break;
            default:
                reply->set_state(VmRunState::UNKNOWN);
                break;
        };

        return Status::OK;
    }

    void fillDisplayConfigurations(DisplayConfigurations* reply) {
        uint32_t width, height, dpi, flags;
        bool enabled;
        for (int i = 0; i < MultiDisplay::s_maxNumMultiDisplay; i++) {
            if (mAgents->multi_display->getMultiDisplay(i, nullptr, nullptr,
                                                        &width, &height, &dpi,
                                                        &flags, &enabled)) {
                auto cfg = reply->add_displays();
                cfg->set_width(width);
                cfg->set_height(height);
                cfg->set_dpi(dpi);
                cfg->set_display(i);
                cfg->set_flags(flags);
            }
            const bool is_pixel_fold = android_foldable_is_pixel_fold();
            if (is_pixel_fold)
                break;
        }

        reply->set_maxdisplays(android::MultiDisplay::s_maxNumMultiDisplay);
        reply->set_userconfigurable(avdInfo_maxMultiDisplayEntries());
    }

    Status getDisplayConfigurations(ServerContext* context,
                                    const ::google::protobuf::Empty* request,
                                    DisplayConfigurations* reply) override {
        if (!featurecontrol::isEnabled(android::featurecontrol::MultiDisplay)) {
            return Status(::grpc::StatusCode::FAILED_PRECONDITION,
                          "The multi-display feature is not available", "");
        }
        fillDisplayConfigurations(reply);

        return Status::OK;
    }

    void deleteDisplay(int displayId) {
        if (displayId == 0)
            return;

        android::base::ThreadLooper::runOnMainLooperAndWaitForCompletion(
                [displayId, this]() {
                    if (mAgents->multi_display->setMultiDisplay(
                                displayId, -1, -1, 0, 0, 0, 0, false) >= 0) {
                        mAgents->emu->updateUIMultiDisplayPage(displayId);
                    }
                });
    }

    Status setDisplayConfigurations(ServerContext* context,
                                    const DisplayConfigurations* request,
                                    DisplayConfigurations* reply) override {
        // Check preconditions, do we have multi display and no duplicate
        // ids?
        if (!featurecontrol::isEnabled(android::featurecontrol::MultiDisplay)) {
            return Status(::grpc::StatusCode::FAILED_PRECONDITION,
                          "The multi-display feature is not available", "");
        }

        std::set<int> updatingDisplayIds;
        for (const auto& display : request->displays()) {
            if (updatingDisplayIds.count(display.display())) {
                return Status(::grpc::StatusCode::INVALID_ARGUMENT,
                              "Duplicate display: " +
                                      std::to_string(display.display()) +
                                      " detected.",
                              "");
            }

            if (!mAgents->multi_display->multiDisplayParamValidate(
                        display.display(), display.width(), display.height(),
                        display.dpi(), display.flags())) {
                return Status(::grpc::StatusCode::INVALID_ARGUMENT,
                              "Display: " + std::to_string(display.display()) +
                                      " is an invalid configuration.",
                              "");
            }

            if (display.display() > avdInfo_maxMultiDisplayEntries() ||
                display.display() < 1) {
                return Status(
                        ::grpc::StatusCode::INVALID_ARGUMENT,
                        "Display id has to be in the set: [1, " +
                                std::to_string(
                                        avdInfo_maxMultiDisplayEntries()) +
                                "].",
                        "");
            }
            updatingDisplayIds.insert(display.display());
        }

        std::set<int> updatedDisplays;
        // Create a snapshot of the current display state.
        DisplayConfigurations previousState;
        getDisplayConfigurations(context, nullptr, &previousState);
        auto existingDisplays = previousState.displays();

        int failureDisplay = 0;
        // Reconfigure to the desired state
        for (const auto& display : request->displays()) {
            // Check if this display is in the set of displays that can be
            // modified and is different than any existing one.
            auto unchanged = std::find_if(
                    std::begin(existingDisplays), std::end(existingDisplays),
                    [&display](const DisplayConfiguration& cfg) {
                        return display.display() < 1 ||
                               display.display() >
                                       avdInfo_maxMultiDisplayEntries() ||
                               ScreenshotUtils::equals(cfg, display);
                    });

            if (unchanged != std::end(existingDisplays)) {
                // We are trying to change a display outside the set [1,3],
                // or we are trying to update a display with the exact same
                // configuration, however we do not want to delete it, as it
                // is unchanged..
                VERBOSE_INFO(grpc, "Display: %d is unchanged.",
                             display.display());
                updatedDisplays.insert(display.display());
                continue;
            }

            int updated = -1;
            android::base::ThreadLooper::runOnMainLooperAndWaitForCompletion(
                    [&updated, &display, this]() {
                        updated = mAgents->multi_display->setMultiDisplay(
                                display.display(), -1, -1, display.width(),
                                display.height(), display.dpi(),
                                display.flags(), true);
                    });
            if (updated < 0) {
                // oh, oh, failure.
                failureDisplay = display.display();
                break;
            };

            VERBOSE_INFO(grpc, "Updated display: %d, to %dx%d (%d), flags: %d",
                         display.display(), display.width(), display.height(),
                         display.dpi(), display.flags());
            updatedDisplays.insert(display.display());

            mAgents->emu->updateUIMultiDisplayPage(display.display());
        }

        // Rollback if we didn't modify all the screens.
        if (failureDisplay != 0) {
            LOG(WARNING) << "Rolling back failed display updates.";
            // Bring back the old state of the displays we added.
            for (const auto& display : previousState.displays()) {
                if (updatedDisplays.erase(display.display()) == 0) {
                    continue;
                }
                int updated = -1;
                android::base::ThreadLooper::
                        runOnMainLooperAndWaitForCompletion([&updated, &display,
                                                             this]() {
                            updated = mAgents->multi_display->setMultiDisplay(
                                    display.display(), -1, -1, display.width(),
                                    display.height(), display.dpi(),
                                    display.flags(), true);
                        });

                if (updated >= 0) {
                    mAgents->emu->updateUIMultiDisplayPage(display.display());
                };
            }

            // Delete the displays we added..
            for (int displayId : updatedDisplays) {
                deleteDisplay(displayId);
            }

            // TODO(jansene): Extract detailed messages from setMultiDisplay
            return Status(::grpc::StatusCode::INTERNAL,
                          "Internal error while trying to modify display: " +
                                  std::to_string(failureDisplay),
                          "");
        }

        // Delete displays we don't want.
        for (const auto& display : previousState.displays()) {
            if (updatedDisplays.count(display.display()) == 0) {
                VERBOSE_INFO(grpc, "Deleting display: %d", display.display());
                deleteDisplay(display.display());
            }
        }

        // Make sure third-party subscribers are notfied of a display change
        // event.
        android::base::ThreadLooper::runOnMainLooperAndWaitForCompletion(
                [this]() { mAgents->multi_display->notifyDisplayChanges(); });
        // Get the actual status and return it.
        return getDisplayConfigurations(context, nullptr, reply);
    }

    Status setVmState(ServerContext* context,
                      const VmRunState* request,
                      ::google::protobuf::Empty* reply) override {
        const std::chrono::milliseconds kWaitToDie = std::chrono::seconds(60);

        // These need to happen on the qemu looper as these transitions
        // will require io locks.
        android::base::ThreadLooper::runOnMainLooper([state = request->state(),
                                                      vm = mAgents->vm,
                                                      ui = mAgents->libui]() {
            switch (state) {
                case VmRunState::RESET:
                    vm->vmReset();
                    break;
                case VmRunState::SHUTDOWN:
                    skin_winsys_quit_request();
                    break;
                case VmRunState::TERMINATE: {
                    LOG(INFO) << "Terminating the emulator.";
                    auto pid = System::get()->getCurrentProcessId();
                    System::get()->killProcess(pid);
                }; break;
                case VmRunState::PAUSED:
                    vm->vmPause();
                    break;
                case VmRunState::RUNNING:
                    vm->vmResume();
                    break;
                case VmRunState::RESTART:
                    ui->requestRestart(0, "External gRPC request");
                    break;
                case VmRunState::START:
                    vm->vmStart();
                    break;
                case VmRunState::STOP:
                    vm->vmStop();
                    break;
                default:
                    break;
            };
        });

        return Status::OK;
    }

    Status sendSms(ServerContext* context,
                   const SmsMessage* smsMessage,
                   PhoneResponse* reply) override {
        SmsAddressRec sender;

        if (sms_address_from_str(&sender, smsMessage->srcaddress().c_str(),
                                 smsMessage->srcaddress().size()) < 0) {
            reply->set_response(PhoneResponse::BadNumber);
            return Status::OK;
        }

        auto modem = mAgents->telephony->getModem();
        if (!modem) {
            reply->set_response(PhoneResponse::ActionFailed);
            return Status::OK;
        }

        // create a list of SMS PDUs, then send them
        SmsPDU* pdus = smspdu_create_deliver_utf8(
                (cbytes_t)smsMessage->text().c_str(), smsMessage->text().size(),
                &sender, NULL);

        if (pdus == NULL) {
            reply->set_response(PhoneResponse::ActionFailed);
            return Status::OK;
        }

        for (int nn = 0; pdus[nn] != NULL; nn++) {
            amodem_receive_sms(modem, pdus[nn]);
        }
        smspdu_free_list(pdus);

        return Status::OK;
    }

    Status setPhoneNumber(ServerContext* context,
                          const PhoneNumber* numberPtr,
                          PhoneResponse* reply) override {
        (void)context;
        PhoneNumber number = *numberPtr;
        auto modem = mAgents->telephony->getModem();
        if (!modem) {
            reply->set_response(PhoneResponse::ActionFailed);
            return Status::OK;
        }
        if (amodem_update_phone_number(modem, number.number().c_str())) {
            reply->set_response(PhoneResponse::BadNumber);
        }
        return Status::OK;
    }

    Status sendPhone(ServerContext* context,
                     const PhoneCall* requestPtr,
                     PhoneResponse* reply) override {
        (void)context;
        PhoneCall request = *requestPtr;

        // We're going to end up waiting for the reply anyway, so
        // wait for it.
        android::base::ThreadLooper::runOnMainLooperAndWaitForCompletion(
                [this, request, reply]() {
                    // We assume that the int mappings are consistent..
                    TelephonyOperation operation =
                            (TelephonyOperation)request.operation();
                    std::string phoneNr = request.number();
                    TelephonyResponse response =
                            mAgents->telephony->telephonyCmd(operation,
                                                             phoneNr.c_str());
                    reply->set_response((PhoneResponse_Response)response);
                });

        return Status::OK;
    }

    ::grpc::ServerWriteReactor<Notification>* streamNotification(
            ::grpc::CallbackServerContext* /*context*/,
            const ::google::protobuf::Empty* /*request*/) override {
        return mNotificationStream.notificationStream();
    }

    Status rotateVirtualSceneCamera(ServerContext* context,
                                    const RotationRadian* request,
                                    ::google::protobuf::Empty* reply) override {
        mCamera.rotate({request->x(), request->y(), request->z()});
        return Status::OK;
    }

    Status setVirtualSceneCameraVelocity(
            ServerContext* context,
            const Velocity* request,
            ::google::protobuf::Empty* reply) override {
        mCamera.setVelocity({request->x(), request->y(), request->z()});

        return Status::OK;
    }

    Status setPosture(ServerContext* context,
                      const Posture* requestPtr,
                      ::google::protobuf::Empty* reply) override {
        auto agent = mAgents->emu;

        if (agent->setPosture((int)requestPtr->value()) == false) {
            return Status(::grpc::StatusCode::FAILED_PRECONDITION,
                          "Unable to set posture.", "");
        }

        return Status::OK;
    }

private:
    const AndroidConsoleAgents* mAgents;
    keyboard::KeyEventSender mKeyEventSender;
    MouseEventSender mMouseEventSender;
    PenEventSender mPenEventSender;
    TouchEventSender mTouchEventSender;
    AndroidEventSender mAndroidEventSender;
    WheelEventSender mWheelEventSender;
    SharedMemoryLibrary mSharedMemoryLibrary;
    EventWaiter mNotificationWaiter;
    NotificationStream mNotificationStream;

    VirtualSceneCamera mCamera;
    Clipboard* mClipboard;
    Looper* mLooper;

    std::atomic_int8_t mInjectAudioCount{0};  // # of active inject audio.
    static constexpr uint32_t k128KB = (128 * 1024) - 1;
    static constexpr std::chrono::milliseconds k5SecondsWait = 5s;
    const std::chrono::milliseconds kNoWait = 0ms;
};

grpc::Service* getEmulatorController(const AndroidConsoleAgents* agents) {
    return new EmulatorControllerImpl(agents);
}

}  // namespace control
}  // namespace emulation
}  // namespace android
