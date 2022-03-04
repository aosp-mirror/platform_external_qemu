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
#include "emulator/webrtc/capture/GrpcAudioSource.h"

#include <grpcpp/grpcpp.h>  // for ClientReaderInterface
#include <stdio.h>          // for size_t
#include <algorithm>        // for min
#include <memory>           // for unique_ptr, operator==
#include <string>           // for string, basic_string<>:...

#include "android/emulation/control/utils/EmulatorGrcpClient.h"  // for EmulatorGrpcClient
#include "emulator_controller.grpc.pb.h"      // for EmulatorController::Stub
#include "emulator_controller.pb.h"           // for AudioPacket, AudioFormat

namespace emulator {
namespace webrtc {

using ::android::emulation::control::AudioFormat;
using ::android::emulation::control::AudioPacket;

constexpr int32_t kBitRateHz = 44100;
constexpr int32_t kChannels = 2;
constexpr int32_t kBytesPerSample = 2;

// WebRTC requires each audio packet to be 10ms long.
constexpr int32_t kSamplesPerFrame = kBitRateHz / 100 /*(10ms/1s)*/;
constexpr size_t kBytesPerFrame =
        kBytesPerSample * kSamplesPerFrame * kChannels;

GrpcAudioSource::GrpcAudioSource(EmulatorGrpcClient* client)
    : mClient(client) {
    mPartialFrame.reserve(kBytesPerFrame);
}

void GrpcAudioSource::setAudioDumpFile(const std::string & audioDumpFile) {
    if (mAudioDump.is_open()) {
        RTC_LOG(INFO) << "Already dumpping, ignoring the request";
        return;
    }
    if (audioDumpFile.empty()) {
        return;
    }
    mAudioDump.open(audioDumpFile.c_str(), std::ios::out|std::ios::binary);
    if (!mAudioDump.is_open()) {
        RTC_LOG(WARNING) << "Could not open " << audioDumpFile << " for write. Ignoring the audio dump request";
    }
}
void GrpcAudioSource::run() {
    StreamAudio();
}

void GrpcAudioSource::cancel() {
    if (auto context = mContext.lock()) {
        context->TryCancel();
    }
    mCaptureAudio = false;
}

void GrpcAudioSource::StreamAudio() {
    std::shared_ptr<grpc::ClientContext> context = mClient->newContext();
    mContext = context;
    AudioFormat request;
    request.set_format(AudioFormat::AUD_FMT_S16);
    request.set_channels(AudioFormat::Stereo);
    request.set_samplingrate(kBitRateHz);
    std::unique_ptr<grpc::ClientReaderInterface<AudioPacket>> stream =
            mClient->stub<android::emulation::control::EmulatorController>()->streamAudio(context.get(), request);
    if (stream == nullptr) {
        return;
    }
    AudioPacket audio_packet;
    while (stream->Read(&audio_packet)) {
        ConsumeAudioPacket(audio_packet);
        if (mAudioDump.is_open()) {
            mAudioDump.write(audio_packet.audio().data(), audio_packet.audio().size());
            mAudioDump.flush();
        }
    }
}

void GrpcAudioSource::ConsumeAudioPacket(const AudioPacket& audio_packet) {
    size_t bytes_consumed = 0;
    if (!mPartialFrame.empty()) {
        size_t bytes_to_append = std::min(kBytesPerFrame - mPartialFrame.size(),
                                          audio_packet.audio().size());
        mPartialFrame.insert(mPartialFrame.end(), audio_packet.audio().data(),
                             audio_packet.audio().data() + bytes_to_append);
        if (mPartialFrame.size() < kBytesPerFrame) {
            return;
        }
        bytes_consumed += bytes_to_append;
        OnData(&mPartialFrame.front(), kBytesPerSample * 8, kBitRateHz,
               kChannels, kSamplesPerFrame);
    }

    while (bytes_consumed + kBytesPerFrame <= audio_packet.audio().size()) {
        OnData(audio_packet.audio().data() + bytes_consumed,
               kBytesPerSample * 8, kBitRateHz, kChannels, kSamplesPerFrame);
        bytes_consumed += kBytesPerFrame;
    }

    mPartialFrame.assign(
            audio_packet.audio().data() + bytes_consumed,
            audio_packet.audio().data() + audio_packet.audio().size());
}

GrpcAudioSource::~GrpcAudioSource() {
    if (mAudioDump.is_open()) {
        mAudioDump.close();
    }
}

const cricket::AudioOptions GrpcAudioSource::options() const {
    cricket::AudioOptions options;
    options.echo_cancellation = false;
    options.auto_gain_control = false;
    options.noise_suppression = false;
    options.highpass_filter = false;
    options.typing_detection = false;
    options.residual_echo_detector = false;
    return options;
}

}  // namespace webrtc
}  // namespace emulator
