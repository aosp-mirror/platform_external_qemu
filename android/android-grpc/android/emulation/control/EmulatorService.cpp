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
#include <ratio>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "android/base/Log.h"
#include "android/base/Optional.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/console.h"
#include "android/emulation/LogcatPipe.h"
#include "android/emulation/control/RtcBridge.h"
#include "android/emulation/control/ScreenCapturer.h"
#include "android/emulation/control/battery_agent.h"
#include "android/emulation/control/clipboard/Clipboard.h"
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
#include "android/emulation/control/utils/AudioUtils.h"
#include "android/emulation/control/utils/EventWaiter.h"
#include "android/emulation/control/utils/ScreenshotUtils.h"
#include "android/emulation/control/utils/ServiceUtils.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"
#include "android/globals.h"
#include "android/gpu_frame.h"
#include "android/hw-sensors.h"
#include "android/opengles.h"
#include "android/physics/Physics.h"
#include "android/recording/Frame.h"
#include "android/recording/Producer.h"
#include "android/recording/audio/AudioProducer.h"
#include "android/skin/rect.h"
#include "android/telephony/gsm.h"
#include "android/telephony/modem.h"
#include "android/telephony/sms.h"
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
          mTouchEventSender(agents),
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

        android::base::ThreadLooper::runOnMainLooperAndWaitForCompletion([agent, request]() {
            struct timeval tVal;
            memset(&tVal, 0, sizeof(tVal));
            gettimeofday(&tVal, NULL);
            agent->gpsSetPassiveUpdate(request.passiveupdate());
            agent->gpsSendLoc(request.latitude(), request.longitude(),
                              request.altitude(), request.speed(),
                              request.bearing(), request.satellites(), &tVal);
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
        int size = values.data().size();
        float a = size > 0 ? values.data(0) : 0;
        float b = size > 1 ? values.data(1) : 0;
        float c = size > 2 ? values.data(2) : 0;

        mAgents->sensors->setPhysicalParameterTarget(
                (int)request->target(), a, b, c,
                PhysicalInterpolation::PHYSICAL_INTERPOLATION_STEP);
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
            float a = size > 0 ? values.data(0) : 0;
            float b = size > 1 ? values.data(1) : 0;
            float c = size > 2 ? values.data(2) : 0;
            agent->setSensorOverride((int)request.target(), a, b, c);
        });
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
                                  request.buttons(), 0);
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

        constexpr int kMaxAudioFrames = 50;
        using TimedAudioFrame = std::pair<System::Duration, std::string>;
        android::base::MessageChannel<TimedAudioFrame, kMaxAudioFrames>
                audioFrames;
        auto audioProducer = android::recording::createAudioProducer(
                sampleRate, kSrcNumSamples, sampleFormat, channels);

        // Callback that is responsible for copying the incoming packets
        // into the audioframe channel.
        audioProducer->attachCallback(
                [&audioFrames,
                 &packet](const android::recording::Frame* frame) {
                    auto audioFrame =
                            std::make_pair<System::Duration, std::string>(
                                    System::get()->getUnixTimeUs(),
                                    std::string((char*)frame->dataVec.data(),
                                                frame->dataVec.size()));
                    return audioFrames.trySend(std::move(audioFrame));
                });

        audioProducer->start();

        // Write out the incoming audio packets.
        constexpr std::chrono::microseconds kTimeToWaitForAudioFrame =
                std::chrono::milliseconds(125);
        bool clientAlive = true;
        do {
            auto audioFrame =
                    audioFrames.timedReceive(kTimeToWaitForAudioFrame.count());
            if (audioFrame) {
                packet.set_timestamp(audioFrame->first);
                packet.set_audio(std::move(audioFrame->second));
                clientAlive = writer->Write(packet);
            }
            clientAlive = clientAlive && !context->IsCancelled();
        } while (clientAlive);

        audioProducer->stop();
        return Status::OK;
    }

    Status streamScreenshot(ServerContext* context,
                            const ImageFormat* request,
                            ServerWriter<Image>* writer) override {
        EventWaiter frameEvent(&gpu_register_shared_memory_callback,
                               &gpu_unregister_shared_memory_callback);

        // Make sure we always write the first frame, this can be
        // a completely empty frame if the screen is not active.
        Image first;
        bool clientAvailable = !context->IsCancelled();

        if (clientAvailable) {
            getScreenshot(context, request, &first);
            clientAvailable = !context->IsCancelled() && writer->Write(first);
        }

        bool lastFrameWasEmpty = first.format().width() == 0;
        int frame = 0;
        while (clientAvailable) {
            Image reply;
            const auto kTimeToWaitForFrame = std::chrono::milliseconds(125);

            // The next call will return the number of frames that are
            // available. 0 means no frame was made available in the given time
            // interval. Since this is a synchronous call we want to wait at
            // most kTimeToWaitForFrame so we can check if the client is still
            // there. (All clients get disconnected on emulator shutdown).
            auto arrived = frameEvent.next(kTimeToWaitForFrame);
            if (arrived > 0 && !context->IsCancelled()) {
                frame += arrived;
                // TODO(jansene): Add metrics around dropped frames/timing?
                getScreenshot(context, request, &reply);
                reply.set_seq(frame);

                // We send the first empty frame, after that we wait for frames
                // to come, or until the client gives up on us. So for a screen
                // that comes in and out the client will see this timeline: (0
                // is empty frame. F is frame) [0, ... <nothing> ..., F1, F2,
                // F3, 0, ...<nothing>... ]
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

    Status getScreenshot(ServerContext* context,
                         const ImageFormat* request,
                         Image* reply) override {
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

        android::emulation::ImageFormat desiredFormat =
                ScreenshotUtils::translate(request->format());

        float xaxis = 0, yaxis = 0, zaxis = 0;

        // TODO(jansene): Not clear if the rotation impacts displays other
        // than 0.
        mAgents->sensors->getPhysicalParameter(PHYSICAL_PARAMETER_ROTATION,
                                               &xaxis, &yaxis, &zaxis,
                                               PARAMETER_VALUE_TYPE_CURRENT);

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

        // Screenshot backend uses the _skin layout_'s orientation,
        // which is clockwise instead of counterclockwise.
        // This makes the 90/270 cases flipped.
        if (desiredRotation == SKIN_ROTATION_270) {
            desiredRotation = SKIN_ROTATION_90;
        } else if (desiredRotation == SKIN_ROTATION_90) {
            desiredRotation = SKIN_ROTATION_270;
        }

        // Screenshots can come from either the gl renderer, or the guest.
        const auto& renderer = android_getOpenglesRenderer();

        // This is an expensive operation, that frequently can get cancelled, so
        // check availability of the client first.
        if (context->IsCancelled()) {
            return Status::CANCELLED;
        }

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
                    mAgents->vm->vmShutdown();
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

private:
    const AndroidConsoleAgents* mAgents;
    keyboard::EmulatorKeyEventSender mKeyEventSender;
    TouchEventSender mTouchEventSender;

    Clipboard* mClipboard;
    Looper* mLooper;
    RingStreambuf
            mLogcatBuffer;  // A ring buffer that tracks the logcat output.

    static constexpr uint32_t k128KB = (128 * 1024) - 1;
    static constexpr uint16_t k5SecondsWait = 5 * 1000;
    const uint16_t kNoWait = 0;
};

grpc::Service* getEmulatorController(const AndroidConsoleAgents* agents) {
    return new EmulatorControllerImpl(agents);
}

}  // namespace control
}  // namespace emulation
}  // namespace android
