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
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <new>
#include <ratio>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "android/avd/info.h"
#include "android/base/EventNotificationSupport.h"
#include "android/base/Log.h"
#include "android/base/Optional.h"
#include "android/base/Stopwatch.h"
#include "android/base/Tracing.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/memory/SharedMemory.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/console.h"
#include "android/emulation/LogcatPipe.h"
#include "android/emulation/MultiDisplay.h"
#include "android/emulation/control/RtcBridge.h"
#include "android/emulation/control/ScreenCapturer.h"
#include "android/emulation/control/ServiceUtils.h"
#include "android/emulation/control/audio/AudioStream.h"
#include "android/emulation/control/battery_agent.h"
#include "android/emulation/control/camera/VirtualSceneCamera.h"
#include "android/emulation/control/clipboard/Clipboard.h"
#include "android/emulation/control/display_agent.h"
#include "android/emulation/control/finger_agent.h"
#include "android/emulation/control/interceptor/LoggingInterceptor.h"
#include "android/emulation/control/keyboard/EmulatorKeyEventSender.h"
#include "android/emulation/control/keyboard/TouchEventSender.h"
#include "android/emulation/control/location_agent.h"
#include "android/emulation/control/logcat/LogcatParser.h"
#include "android/emulation/control/logcat/RingStreambuf.h"
#include "android/emulation/control/multi_display_agent.h"
#include "android/emulation/control/sensors_agent.h"
#include "android/emulation/control/telephony_agent.h"
#include "android/emulation/control/user_event_agent.h"
#include "android/emulation/control/utils/AudioUtils.h"
#include "android/emulation/control/utils/EventWaiter.h"
#include "android/emulation/control/utils/ScreenshotUtils.h"
#include "android/emulation/control/utils/SharedMemoryLibrary.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"
#include "android/emulation/resizable_display_config.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/featurecontrol/Features.h"
#include "android/globals.h"
#include "android/gpu_frame.h"
#include "android/hw-sensors.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/Percentiles.h"
#include "android/opengles.h"
#include "android/physics/Physics.h"
#include "android/recording/Frame.h"
#include "android/recording/Producer.h"
#include "android/recording/audio/AudioProducer.h"
#include "android/skin/rect.h"
#include "android/skin/winsys.h"
#include "android/telephony/gsm.h"
#include "android/telephony/modem.h"
#include "android/telephony/sms.h"
#include "android/version.h"
#include "emulator_controller.grpc.pb.h"
#include "emulator_controller.pb.h"
#include "studio_stats.pb.h"

namespace android {
namespace base {
class Looper;
}  // namespace base
}  // namespace android

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
using namespace std::chrono_literals;
using ::google::protobuf::Empty;
using namespace std::chrono_literals;

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
          mTouchEventSender(agents),
          mCamera(agents->sensors),
          mClipboard(Clipboard::getClipboard(agents->clipboard)),
          mLooper(android::base::ThreadLooper::get()) {
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
                [=]() { mClipboard->sendContents(clipData->text()); });
        return Status::OK;
    }

    Status getClipboard(ServerContext* context,
                        const ::google::protobuf::Empty* empty,
                        ClipData* reply) override {
        reply->set_text(mClipboard->getCurrentContents());
        return Status::OK;
    }

    Status streamClipboard(ServerContext* context,
                           const ::google::protobuf::Empty* empty,
                           ServerWriter<ClipData>* writer) override {
        ClipData reply;
        bool clientAvailable = !context->IsCancelled();

        if (clientAvailable) {
            reply.set_text(mClipboard->getCurrentContents());
            clientAvailable = writer->Write(reply);
        }

        int frame = 0;
        while (clientAvailable) {
            const auto kTimeToWaitForFrame = std::chrono::milliseconds(125);
            auto arrived = mClipboard->eventWaiter()->next(kTimeToWaitForFrame);
            if (arrived > 0 && !context->IsCancelled()) {
                reply.set_text(mClipboard->getCurrentContents());
                frame += arrived;
                clientAvailable = writer->Write(reply);
            }
            clientAvailable = !context->IsCancelled() && clientAvailable;
        }
        return Status::OK;
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
        size_t size;
        mAgents->sensors->getSensorSize((int)request->target(), &size);
        std::vector<float> val(size, 0);
        std::vector<float*> out;
        for (size_t i = 0; i < val.size(); i++) {
            out.push_back(&val[i]);
        }

        int state = mAgents->sensors->getSensor((int)request->target(),
                                                out.data(), out.size());

        auto value = reply->mutable_value();
        for (size_t i = 0; i < size; i++) {
            value->add_data(val[i]);
        }
        reply->set_status((SensorValue_State)state);
        reply->set_target(request->target());
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
                   const KeyboardEvent* requestPtr,
                   ::google::protobuf::Empty* reply) override {
        KeyboardEvent request = *requestPtr;
        android::base::ThreadLooper::runOnMainLooper([this, request]() {
            mKeyEventSender.sendOnThisThread(&request);
        });
        return Status::OK;
    }

    Status sendMouse(ServerContext* context,
                     const MouseEvent* requestPtr,
                     ::google::protobuf::Empty* reply) override {
        MouseEvent request = *requestPtr;
        auto agent = mAgents->user_event;

        android::base::ThreadLooper::runOnMainLooper([agent, request]() {
            agent->sendMouseEvent(request.x(), request.y(), 0,
                                  request.buttons(), request.display());
        });

        return Status::OK;
    }

    Status sendTouch(ServerContext* context,
                     const TouchEvent* requestPtr,
                     ::google::protobuf::Empty* reply) override {
        TouchEvent request = *requestPtr;
        android::base::ThreadLooper::runOnMainLooper([this, request]() {
            mTouchEventSender.sendOnThisThread(&request);
        });
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
                    assert(*counter == 0); });

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

        // The circular buffer contains at most audioQueueTime ms of samples,
        // We need to deliver those before disconnecting the audio stack to
        // guarantee we deliver all the samples.
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
        // The source number of samples per audio frame in Qemu, 512 samples is
        // fixed
        constexpr int kSrcNumSamples = 512;

        // Translate external settings to internal qemu settings.
        auto sampleFormat = AudioUtils::getSampleFormat(*request);
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
            getScreenshot(context, request, &reply);
            assert(reply.image().size() >= cPixels);
            cPixels = reply.image().size();
            clientAvailable = !context->IsCancelled() && writer->Write(reply);
        }

        bool lastFrameWasEmpty = reply.format().width() == 0;
        int frame = 0;

        // TODO(jansen): Move to ScreenshotUtils.
        std::unique_ptr<EventWaiter> frameEvent;
        std::unique_ptr<RaiiEventListener<emugl::Renderer,
                                          emugl::FrameBufferChangeEvent>>
                frameListener;
        // Screenshots can come from either the gl renderer, or the guest.
        const auto& renderer = android_getOpenglesRenderer();
        if (renderer.get()) {
            // Fast mode..
            frameEvent = std::make_unique<EventWaiter>();
            frameListener = std::make_unique<RaiiEventListener<
                    emugl::Renderer, emugl::FrameBufferChangeEvent>>(
                    renderer.get(),
                    [&](const emugl::FrameBufferChangeEvent state) {
                        frameEvent->newEvent();
                    });
        } else {
            // slow mode, you are likely using older api..
            LOG(VERBOSE) << "Reverting to slow callbacks";
            frameEvent = std::make_unique<EventWaiter>(
                    &gpu_register_shared_memory_callback,
                    &gpu_unregister_shared_memory_callback);
        }
        // Track percentiles, and report if we have seen at least 32 frames.
        metrics::Percentiles perfEstimator(32, {0.5, 0.95});
        while (clientAvailable) {
            const auto kTimeToWaitForFrame = std::chrono::milliseconds(125);

            // The next call will return the number of frames that are
            // available. 0 means no frame was made available in the given time
            // interval. Since this is a synchronous call we want to wait at
            // most kTimeToWaitForFrame so we can check if the client is still
            // there. (All clients get disconnected on emulator shutdown).
            auto arrived = frameEvent->next(kTimeToWaitForFrame);
            if (arrived > 0 && !context->IsCancelled()) {
                AEMU_SCOPED_TRACE("streamScreenshot::frame");
                frame += arrived;
                Stopwatch sw;
                getScreenshot(context, request, &reply);

                // The invariant that the pixel buffer does not decrease should
                // hold. Clients likely rely on the buffer size to match the
                // actual number of available pixels.
                assert(reply.image().size() >= cPixels);
                cPixels = reply.image().size();

                reply.set_seq(frame);
                // We send the first empty frame, after that we wait for frames
                // to come, or until the client gives up on us. So for a screen
                // that comes in and out the client will see this timeline: (0
                // is empty frame. F is frame) [0, ... <nothing> ..., F1, F2,
                // F3, 0, ...<nothing>... ]
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
            android::metrics::MetricsReporter::get().report(
                    [=](android_studio::AndroidStudioEvent* event) {
                        int bpp = ScreenshotUtils::getBytesPerPixel(*request);
                        auto screenshot = event->mutable_emulator_details()
                                                  ->mutable_screenshot();
                        screenshot->set_size(request->width() *
                                             request->height() * bpp);
                        screenshot->set_frames(frame);
                        // We care about median, 95%, and 100%.
                        // (max enables us to calculate # of dropped frames.)
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
        bool enabled;
        bool multiDisplayQueryWorks = mAgents->emu->getMultiDisplay(
                request->display(), nullptr, nullptr, &width, &height, nullptr,
                nullptr, &enabled);

        // TODO(b/151387266): Use new query that uses a shared state for
        // multidisplay
        if (!multiDisplayQueryWorks) {
            width = android_hw->hw_lcd_width;
            height = android_hw->hw_lcd_height;
            enabled = true;
        }

        if (!enabled)
            return Status::OK;
        bool isFolded = android_foldable_is_folded();
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
        // Calculate the desired rotation and width we should use..
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

        // Depending on the rotation state width and height need to be reversed.
        // as our apsect ration depends on how we are holding our phone..
        if (rotation == Rotation::LANDSCAPE ||
            rotation == Rotation::REVERSE_LANDSCAPE) {
            std::swap(width, height);
            if (isFolded) {
                std::swap(rect.pos.x, rect.pos.y);
                std::swap(rect.size.w, rect.size.h);
            }
        }

        // Calculate width and height, keeping aspect ratio in mind.
        int newWidth, newHeight;
        if (isFolded) {
            // When screen is folded, resize based on the aspect ratio of the
            // folded screen instead of full screen.
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

        // The screenshot produces a rotated result and will just simply flip
        // width and height.
        if (rotation == Rotation::LANDSCAPE ||
            rotation == Rotation::REVERSE_LANDSCAPE) {
            std::swap(newWidth, newHeight);
            if (isFolded) {
                std::swap(rect.pos.x, rect.pos.y);
                std::swap(rect.size.w, rect.size.h);
            }
        }

        // This is an expensive operation, that frequently can get cancelled, so
        // check availability of the client first.
        if (context->IsCancelled()) {
            return Status::CANCELLED;
        }

        uint8_t* pixels;
        size_t cPixels = 0;
        bool isPNG = request->format() ==
                     ImageFormat_ImgFormat::ImageFormat_ImgFormat_PNG;
        android::emulation::Image img(
                0, 0, 0, android::emulation::ImageFormat::RGB888, {});
        // For png format, only make one getScreenshot call because the cPixels
        // value can change for each call.
        if (isPNG) {
            img = ScreenshotUtils::getScreenshot(
                    request->display(), request->format(), rotation, newWidth,
                    newHeight, &width, &height, rect);
            cPixels = img.getPixelCount();
        } else {  // Let's make a fast call to learn how many pixels we need to
                  // reserve.

            ScreenshotUtils::getScreenshot(
                    request->display(), request->format(), rotation, newWidth,
                    newHeight, pixels, &cPixels, &width, &height, rect);
        }

        auto format = reply->mutable_format();
        format->set_format(request->format());

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
            // - A single screenshot is requested, we need to allocate a buffer
            // of sufficient size
            // - The first screenshot in a series is requested and we need to
            // allocate the first frame.
            if (reply->mutable_image()->size() != cPixels) {
                LOG(VERBOSE)
                        << "Allocation of string object. "
                        << reply->mutable_image()->size() << " < " << cPixels;
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
                    request->display(), request->format(), rotation, newWidth,
                    newHeight, pixels, &cPixels, &width, &height, rect);
        }
        // Update format information with the retrieved width, height.
        format->set_height(height);
        format->set_width(width);
        LOG(VERBOSE) << "Screenshot " << width << "x" << height
                     << ", cPixels: " << cPixels << ", in: " << sw.elapsedUs()
                     << " us";

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

    Status getDisplayConfigurations(ServerContext* context,
                                    const ::google::protobuf::Empty* request,
                                    DisplayConfigurations* reply) override {
        if (!featurecontrol::isEnabled(android::featurecontrol::MultiDisplay)) {
            return Status(::grpc::StatusCode::FAILED_PRECONDITION,
                          "The multi-display feature is not available", "");
        }

        uint32_t width, height, dpi, flags;
        bool enabled;
        for (int i = 0; i <= MultiDisplay::s_maxNumMultiDisplay; i++) {
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
        }

        reply->set_maxdisplays(android::MultiDisplay::s_maxNumMultiDisplay);
        reply->set_userconfigurable(avdInfo_maxMultiDisplayEntries());
        return Status::OK;
    }

    void deleteDisplay(int displayId) {
        if (displayId != 0 &&
            mAgents->multi_display->setMultiDisplay(displayId, -1, -1, 0, 0, 0,
                                                    0, false) >= 0) {
            mAgents->emu->updateUIMultiDisplayPage(displayId);
        }
    }

    Status setDisplayConfigurations(ServerContext* context,
                                    const DisplayConfigurations* request,
                                    DisplayConfigurations* reply) override {
        // Check preconditions, do we have multi display and no duplicate ids?
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
                // We are trying to change a display outside the set [1,3], or
                // we are trying to update a display with the exact same
                // configuration.
                continue;
            }

            // TODO(jansene): This can result in UI messages if invalid values
            // are presented.
            if (mAgents->multi_display->setMultiDisplay(
                        display.display(), -1, -1, display.width(),
                        display.height(), display.dpi(), display.flags(),
                        true) < 0) {
                // oh, oh, failure.
                failureDisplay = display.display();
                break;
            };

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
                if (mAgents->multi_display->setMultiDisplay(
                            display.display(), -1, -1, display.width(),
                            display.height(), display.dpi(), display.flags(),
                            true) >= 0) {
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
                deleteDisplay(display.display());
            }
        }

        // Get the actual status and return it.
        return getDisplayConfigurations(context, nullptr, reply);
    }

    Status setVmState(ServerContext* context,
                      const VmRunState* request,
                      ::google::protobuf::Empty* reply) override {
        const std::chrono::milliseconds kWaitToDie = std::chrono::seconds(60);

        // These need to happen on the qemu looper as these transitions
        // will require io locks.
        android::base::ThreadLooper::runOnMainLooper([=]() {
            switch (request->state()) {
                case VmRunState::RESET:
                    mAgents->vm->vmReset();
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
                    mAgents->vm->vmPause();
                    break;
                case VmRunState::RUNNING:
                    mAgents->vm->vmResume();
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

    Notification getCameraNotificationEvent() {
        Notification reply;
        if (mCamera.isConnected()) {
            reply.set_event(Notification::VIRTUAL_SCENE_CAMERA_ACTIVE);
        } else {
            reply.set_event(Notification::VIRTUAL_SCENE_CAMERA_INACTIVE);
        }
        return reply;
    }

    Status streamNotification(ServerContext* context,
                              const Empty* request,
                              ServerWriter<Notification>* writer) override {
        bool clientAvailable = !context->IsCancelled();
        if (clientAvailable) {
            clientAvailable = writer->Write(getCameraNotificationEvent());
        }

        // The event waiter will be unlocked when a change event is received.
        EventWaiter notifier;
        int eventCount = 0;
        enum class EventTypes { CameraEvent, DisplayEvent };

        // This is the set of notifications that need to be delivered.
        std::vector<Notification> activeNotifications;
        std::mutex notificationLock;

        // Register temporary listeners for multi display & camera changes.
        RaiiEventListener<VirtualSceneCamera, bool> cameraListener(
                &mCamera, [&](auto state) {
                    std::lock_guard<std::mutex> lock(notificationLock);
                    activeNotifications.push_back(getCameraNotificationEvent());
                    notifier.newEvent();
                });

        RaiiEventListener<MultiDisplay, android::DisplayChangeEvent>
                displayListener(
                        MultiDisplay::getInstance(),
                        [&](const android::DisplayChangeEvent state) {
                            if (state.change ==
                                DisplayChange::DisplayTransactionCompleted) {
                                Notification display;
                                display.set_event(
                                        Notification::
                                                DISPLAY_CONFIGURATIONS_CHANGED_UI);

                                std::lock_guard<std::mutex> lock(
                                        notificationLock);
                                activeNotifications.push_back(display);
                                notifier.newEvent();
                            }
                        });

        // And deliver the events as they come in.
        while (clientAvailable) {
            const auto kTimeToWaitForFrame = std::chrono::milliseconds(500);
            // This will not block if events came in while we were processing
            // them below.
            auto arrived = notifier.next(eventCount, kTimeToWaitForFrame);
            if (arrived > 0 && !context->IsCancelled()) {
                eventCount += arrived;
                // We now have a non empty set of events we have to deliver.
                std::vector<Notification> current;
                {
                    // Make sure we deliver events only once.
                    std::lock_guard<std::mutex> lock(notificationLock);
                    std::swap(current, activeNotifications);
                }

                // Deliver each individual event.
                for (const auto& event : current) {
                    clientAvailable = writer->Write(event);
                    if (!clientAvailable) {
                        break;
                    }
                }
            }
            clientAvailable = !context->IsCancelled() && clientAvailable;
        }
        return Status::OK;
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
    keyboard::EmulatorKeyEventSender mKeyEventSender;
    TouchEventSender mTouchEventSender;
    SharedMemoryLibrary mSharedMemoryLibrary;
    EventWaiter mNotificationWaiter;

    VirtualSceneCamera mCamera;
    Clipboard* mClipboard;
    Looper* mLooper;
    RingStreambuf
            mLogcatBuffer;  // A ring buffer that tracks the logcat output.

    std::atomic_int8_t mInjectAudioCount{0}; // # of active inject audio.
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
