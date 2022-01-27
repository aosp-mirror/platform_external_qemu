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
#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint32_t
#include <chrono>    // for operator...
#include <ios>       // for streamsize
#include <memory>    // for unique_ptr

#include "android/emulation/AudioCapture.h"                  // for AudioCap...
#include "android/emulation/AudioCaptureEngine.h"            // for AudioCap...
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

    std::streamsize capacity() { return mAudioBuffer.capacity(); }

    virtual bool good() = 0;

protected:
    virtual int onSample(void* buf, int n) = 0;
    virtual int available() { return 0; }
    uint32_t bytesPerMillisecond(const AudioFormat fmt) const;

    RingStreambuf mAudioBuffer;
    std::streamsize mAudioBufferSize;

    friend class AudioStreamCapturer;
    AudioFormat mFormat;
};

// A qemu audio output stream will register an audio receiver with QEMU.
// all the incoming data will be placed on the ringbuffer.
//
// Typically you would use this like this:
//
// AudioPacket pkt;
// QemuAudioOutputStream ais(desiredFormat, 30ms, 500ms);
// while(wantAudio) {
//    int bytesRec = ais.read(&pkt);
//    if (bytesRec > 0)
//      sendThePacket(pkt);
// }
class QemuAudioOutputStream : public AudioStream {
public:
    QemuAudioOutputStream(const AudioFormat fmt,
                          milliseconds blockingTime = 300ms,
                          milliseconds bufferSize = 500ms);

    ~QemuAudioOutputStream() = default;
    // Blocks and waits until we can fill an audio packet with audio data.
    // This will block for at most the configured blocking time during
    // construction.
    std::streamsize read(AudioPacket* packet);

    bool good() override { return mAudioCapturer->good(); };

protected:
    int onSample(void* buf, int n) override;

private:
    std::unique_ptr<emulation::AudioCapturer> mAudioCapturer;
};

// A QemuAudioInputStream will register a microphone with QEMU.
// all the incoming data will be placed on the ringbuffer and
// given to QEMU when requested.
//
// Typically you would use this like this:
//
//  QemuAudioInputStream ais(pkt.format());
//  while(reader->Read(&pkt)) {
//   ais.write(pkt);
// }
class QemuAudioInputStream : public AudioStream {
public:
    QemuAudioInputStream(const AudioFormat fmt,
                         milliseconds blockingTime = 300ms,
                         milliseconds bufferSize = 1000ms);
    ~QemuAudioInputStream() = default;

    // Blocks and waits until we can write an entire audio packet.
    void write(const AudioPacket* packet);

    // Blocks and waits until we can write cBuffer bytes from
    // the buffer. Returns the number of bytes written.
    // Note: This can be 0, in case qemu is not requesting any audio!
    size_t write(const char* buffer, size_t cBuffer);

    bool good() override { return mAudioCapturer->good(); };

    // The capacity (in bytes) of the underlying circular audio buffer.
    // This buffer should likely be flushed before closing the audio stack.
    size_t capacity();

protected:
    int onSample(void* buf, int n) override;
    int available() override;

private:
    std::unique_ptr<emulation::AudioCapturer> mAudioCapturer;
};

class AudioStreamCapturer : public android::emulation::AudioCapturer {
public:
    // the audio packets to the given stream.
    AudioStreamCapturer(const AudioFormat& fmt,
                        AudioStream* stream,
                        AudioCaptureEngine::AudioMode mode =
                                AudioCaptureEngine::AudioMode::AUDIO_OUTPUT);
    ~AudioStreamCapturer();

    int onSample(void* buf, int size) override;
    bool good() override;
    int available() override;

private:
    AudioStream* mStream;
    AudioCaptureEngine::AudioMode mAudioMode;
    bool mRunning;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
