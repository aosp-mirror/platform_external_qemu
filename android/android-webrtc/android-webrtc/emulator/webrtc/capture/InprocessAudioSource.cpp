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
#include "emulator/webrtc/capture/InprocessAudioSource.h"

#include <stdio.h>    // for size_t
#include <algorithm>  // for min
#include <chrono>
#include <memory>  // for unique_ptr, operator==
#include <string>  // for string, basic_string<>:...

#include "aemu/base/Log.h"
#include "android/emulation/control/audio/AudioStream.h"
#include "android/emulation/control/utils/AudioUtils.h"
#include "emulator_controller.pb.h"  // for AudioPacket, AudioFormat

namespace emulator {
namespace webrtc {

using ::android::emulation::control::AudioFormat;
using ::android::emulation::control::AudioPacket;
using ::android::emulation::control::AudioUtils;
using ::android::emulation::control::QemuAudioOutputStream;

constexpr int32_t kBitRateHz = 44100;
constexpr int32_t kChannels = 2;
constexpr int32_t kBytesPerSample = 2;

// WebRTC requires each audio packet to be 10ms long.
constexpr int32_t kSamplesPerFrame = kBitRateHz / 100 /*(10ms/1s)*/;
constexpr size_t kBytesPerFrame =
        kBytesPerSample * kSamplesPerFrame * kChannels;

InprocessAudioSource::InprocessAudioSource() {
    mPartialFrame.reserve(kBytesPerFrame);
}

void InprocessAudioSource::setAudioDumpFile(const std::string& audioDumpFile) {
    if (mAudioDump.is_open()) {
        LOG(INFO) << "Already dumpping, ignoring the request";
        return;
    }
    if (audioDumpFile.empty()) {
        return;
    }
    mAudioDump.open(audioDumpFile.c_str(), std::ios::out | std::ios::binary);
    if (!mAudioDump.is_open()) {
        LOG(WARNING) << "Could not open " << audioDumpFile
                     << " for write. Ignoring the audio dump request";
    }
}

void InprocessAudioSource::run() {
    StreamAudio();
}

void InprocessAudioSource::cancel() {
    LOG(INFO) << "Cancelling audio stream";
    mCaptureAudio = false;
}

void InprocessAudioSource::StreamAudio() {
    AudioFormat request;
    request.set_format(AudioFormat::AUD_FMT_S16);
    request.set_channels(AudioFormat::Stereo);
    request.set_samplingrate(kBitRateHz);
    AudioPacket packet;
    AudioFormat* format = packet.mutable_format();
    format->set_channels(request.channels());
    format->set_format(request.format());
    format->set_samplingrate(request.samplingrate());

    auto kTimeToWaitForAudioFrame = std::chrono::milliseconds(30);
    QemuAudioOutputStream audioStream(*format, kTimeToWaitForAudioFrame);

    // Pre allocate the packet buffer.
    auto buffer = new std::string(kBytesPerFrame, 0);
    packet.set_allocated_audio(buffer);
    while (mCaptureAudio == true) {
        buffer->resize(kBytesPerFrame);
        auto size = audioStream.read(&packet);
        if (size > 0 && mCaptureAudio == true) {
            ConsumeAudioPacket(packet);
            if (mAudioDump.is_open()) {
                mAudioDump.write(packet.audio().data(), packet.audio().size());
                mAudioDump.flush();
            }
        }
    }
}

void InprocessAudioSource::ConsumeAudioPacket(const AudioPacket& audio_packet) {
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

InprocessAudioSource::~InprocessAudioSource() {
    if (mAudioDump.is_open()) {
        mAudioDump.close();
    }
    cancel();
}

const cricket::AudioOptions InprocessAudioSource::options() const {
    cricket::AudioOptions options;
    options.echo_cancellation = false;
    options.auto_gain_control = false;
    options.noise_suppression = false;
    options.highpass_filter = false;
    return options;
}

}  // namespace webrtc
}  // namespace emulator
