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
#include "android/emulation/control/audio/AudioStream.h"

#include <functional>                                    // for __base, func...
#include <string>                                        // for string

#include "android/base/system/System.h"                  // for System
#include "android/emulation/AudioCapture.h"              // for AudioCapturer
#include "android/emulation/AudioCaptureEngine.h"        // for AudioCapture...
#include "android/emulation/control/utils/AudioUtils.h"  // for AudioUtils

namespace android {
namespace emulation {
namespace control {

AudioStream::AudioStream(const AudioFormat fmt,
                         milliseconds blockingTime,
                         milliseconds bufferSize)
    : mFormat(fmt),
      mAudioBuffer(bytesPerMillisecond(fmt) * bufferSize.count(),
                   blockingTime) {}

uint32_t AudioStream::bytesPerMillisecond(const AudioFormat fmt) const {
    return fmt.samplingrate() / 1000 * AudioUtils::getBytesPerSample(fmt) *
           AudioUtils::getChannels(fmt);
}

using AudioCallback = std::function<int(char*, std::streamsize)>;

class AudioStreamCapturer : public android::emulation::AudioCapturer {
public:
    // create an instance with the specified audio stream, qemu will publish
    // the audio packets to the given stream.
    AudioStreamCapturer(const AudioFormat& fmt, AudioCallback audioCallback)
        : AudioCapturer(fmt.samplingrate(),
                        fmt.format() == AudioFormat::AUD_FMT_U8 ? 8 : 16,
                        fmt.channels() == AudioFormat::Mono ? 1 : 2),
          mAudioReceivedCallback(audioCallback) {
        AudioCaptureEngine::get()->start(this);
    }

    ~AudioStreamCapturer() { AudioCaptureEngine::get()->stop(this); }
    virtual int onSample(void* buf, int size) {
        return mAudioReceivedCallback((char*)buf, (std::streamsize)size);
    }

private:
    AudioCallback mAudioReceivedCallback;
};

AudioInputStream::AudioInputStream(const AudioFormat fmt,
                                   milliseconds blockingTime,
                                   milliseconds bufferSize)
    : AudioStream(fmt, blockingTime, bufferSize) {
    mAudioCapturer = std::make_unique<AudioStreamCapturer>(
            fmt, [this](char* data, std::streamsize len) {
                return mAudioBuffer.sputn(data, len);
            });
}

// Blocks and waits until we can fill an audio packet with
std::streamsize AudioInputStream::read(AudioPacket* packet) {
    if (packet->mutable_audio()->size() == 0) {
        // Oh, oh we have to make up a buffer size..
        packet->mutable_audio()->resize(4096);
    }
    // C++17 strings are writable
    char* unsafe = packet->mutable_audio()->data();
    auto future = base::System::get()->getUnixTimeUs();
    auto actualSize = mAudioBuffer.sgetn(unsafe, packet->audio().size());
    auto now = base::System::get()->getUnixTimeUs();
    packet->set_timestamp(base::System::get()->getUnixTimeUs());
    packet->mutable_audio()->resize(actualSize);
    return actualSize;
}

}  // namespace control
}  // namespace emulation
}  // namespace android
