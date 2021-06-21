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
#pragma once
#include <stdint.h>                                          // for uint32_t
#include <chrono>                                            // for operator...
#include <ios>                                               // for streamsize
#include <memory>                                            // for unique_ptr

#include "android/emulation/AudioCapture.h"                  // for AudioCap...
#include "android/emulation/control/logcat/RingStreambuf.h"  // for millisec...
#include "emulator_controller.pb.h"                          // for AudioFormat

using namespace std::chrono_literals;
using std::chrono::milliseconds;
namespace android {
namespace emulation {
namespace control {

// An audio stream contains a ringbuffer that can be used to
// send and receive audio from qemu.
class AudioStream {
public:
    AudioStream(const AudioFormat fmt,
                milliseconds blockingTime = 30ms,
                milliseconds bufferSize = 500ms);

    virtual ~AudioStream() = default;

    const AudioFormat getFormat() const { return mFormat; }

protected:
    uint32_t bytesPerMillisecond(const AudioFormat fmt) const;
    RingStreambuf mAudioBuffer;

private:
    AudioFormat mFormat;
};

// An input audio stream will register an audio receiver with QEMU.
// all the incoming data will be placed on the ringbuffer.
//
// Typically you would use this like this:
//
// AudioPacket pkt;
// AudioInputSteam ais(desiredFormat, 30ms, 500ms);
// while(wantAudio) {
//    int bytesRec = ais.read(&pkt);
//    if (bytesRec > 0)
//      sendThePacket(pkt);
// }
class AudioInputStream : public AudioStream {
public:
    AudioInputStream(const AudioFormat fmt,
                     milliseconds blockingTime = 30ms,
                     milliseconds bufferSize = 500ms);

    ~AudioInputStream() = default;
    // Blocks and waits until we can fill an audio packet with audio data.
    // This will block for at most the configured blocking time during
    // construction.
    std::streamsize read(AudioPacket* packet);

private:
    std::unique_ptr<emulation::AudioCapturer> mAudioCapturer;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
