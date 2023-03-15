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

#include <algorithm>   // for min
#include <functional>  // for function
#include <string>      // for string

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
      mAudioBuffer(bytesPerMillisecond(fmt) * bufferSize.count() * 2,
                   blockingTime) {}

uint32_t AudioStream::bytesPerMillisecond(const AudioFormat fmt) const {
    return fmt.samplingrate() / 1000 * AudioUtils::getBytesPerSample(fmt) *
           AudioUtils::getChannels(fmt);
}

using AudioCallback = std::function<int(char*, std::streamsize)>;

QemuAudioOutputStream::QemuAudioOutputStream(const AudioFormat fmt,
                                             milliseconds blockingTime,
                                             milliseconds bufferSize)
    : AudioStream(fmt, blockingTime, bufferSize) {
    mAudioCapturer = std::make_unique<AudioStreamCapturer>(fmt, this);
}

// Blocks and waits until we can fill an audio packet with the received packet.
std::streamsize QemuAudioOutputStream::read(AudioPacket* packet) {
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

int QemuAudioOutputStream::onSample(void* buf, int n) {
    return mAudioBuffer.sputn(reinterpret_cast<char*>(buf), n);
}

QemuAudioInputStream::QemuAudioInputStream(const AudioFormat fmt,
                                           milliseconds blockingTime,
                                           milliseconds bufferSize)
    : AudioStream(fmt, blockingTime, bufferSize) {
    mAudioCapturer = std::make_unique<AudioStreamCapturer>(
            fmt, this, AudioCaptureEngine::AudioMode::AUDIO_INPUT);
}

void QemuAudioInputStream::write(const AudioPacket* pkt) {
    const char* buf = pkt->audio().data();
    size_t cBuf = pkt->audio().size();
    while (cBuf > 0) {
        size_t written = write(buf, cBuf);
        buf = buf + written;
        cBuf -= written;
    }
}

size_t QemuAudioInputStream::write(const char* buffer, size_t cBuffer) {
    if (mFormat.mode() == AudioFormat::MODE_REAL_TIME) {
        return mAudioBuffer.sputn(buffer, cBuffer);
    }
    size_t toWrite = mAudioBuffer.waitForAvailableSpace(cBuffer);
    toWrite = std::min(toWrite, cBuffer);
    return mAudioBuffer.sputn(buffer, toWrite);
}

size_t QemuAudioInputStream::capacity() {
    return mAudioBuffer.capacity();
}

int QemuAudioInputStream::onSample(void* buf, int n) {
    return mAudioBuffer.sgetn(reinterpret_cast<char*>(buf), n);
}

int QemuAudioInputStream::available() {
    return mAudioBuffer.in_avail();
}

AudioStreamCapturer::AudioStreamCapturer(const AudioFormat& fmt,
                                         AudioStream* stream,
                                         AudioCaptureEngine::AudioMode mode)
    : AudioCapturer(fmt.samplingrate(),
                    fmt.format() == AudioFormat::AUD_FMT_U8 ? 8 : 16,
                    fmt.channels() == AudioFormat::Mono ? 1 : 2),
      mStream(stream),
      mAudioMode(mode) {
    mRunning = (AudioCaptureEngine::get(mAudioMode)->start(this) == 0);
}

AudioStreamCapturer::~AudioStreamCapturer() {
    AudioCaptureEngine::get(mAudioMode)->stop(this);
}

int AudioStreamCapturer::onSample(void* buf, int size) {
    return mStream->onSample(buf, size);
}

bool AudioStreamCapturer::good() {
    return mRunning;
}

int AudioStreamCapturer::available() {
    return mStream->available();
};

}  // namespace control
}  // namespace emulation
}  // namespace android
