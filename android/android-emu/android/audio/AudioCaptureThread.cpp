// Copyright (C) 2017 The Android Open Source Project
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

#include "android/audio/AudioCaptureThread.h"

#include "android/audio/AudioPacket.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/emulation/AudioCapture.h"
#include "android/emulation/AudioCaptureEngine.h"
#include "android/utils/debug.h"

#include <memory>

namespace android {
namespace audio {

using android::base::MessageChannel;
using android::emulation::AudioCaptureEngine;
using android::emulation::AudioCapturer;

namespace {
class AudioCapturerImpl : public AudioCapturer {
public:
    typedef int (Callback)(void* opaque, void* buf, int size);
    explicit AudioCapturerImpl(AudioCapturerImpl::Callback cb,
                               void* opaque,
                               int freq,
                               int bits,
                               int nchannels)
        : AudioCapturer(freq, bits, nchannels),
          mCallback(cb),
          mOpaque(opaque) {}

    virtual ~AudioCapturerImpl() {}

    // Called from the qemu io thread with the audio data
    virtual int onSample(void* buf, int size) override {
        return mCallback(mOpaque, buf, size);
    }

private:
    AudioCapturerImpl::Callback* mCallback = nullptr;
    void* mOpaque = nullptr;
};  // class AudioCapturerImpl

// Real implementation of AudioCaptureThread
class AudioCaptureThreadImpl : public AudioCaptureThread {
public:
    AudioCaptureThreadImpl(AudioCaptureThread::Callback* cb,
                           void* opaque,
                           int sampleRate,
                           int bits,
                           int nchannels)
        : mCallback(cb),
          mOpaque(opaque),
          mSampleRate(sampleRate),
          mBits(bits),
          mChannels(nchannels) {}

    ~AudioCaptureThreadImpl() {}

    intptr_t main() override final {
        if (mCallback == nullptr) {
            return -1;
        }

        // Setup the audio capture engine
        mEngine = AudioCaptureEngine::get();
        if (mEngine == nullptr) {
            return -1;
        }

        mAudioCapturer.reset(
                new AudioCapturerImpl(&AudioCaptureThreadImpl::onSample, this,
                                      mSampleRate, mBits, mChannels));

        // Start the engine. The main thread will start calling onSample()
        // with the audio data.
        mEngine->start(mAudioCapturer.get());

        for (;;) {
            // Wait for data
            AudioPacket pkt;
            mAudioPackets.receive(&pkt);
            if (pkt.data) {
                mCallback(mOpaque, pkt.data.get(), pkt.size, pkt.tsUs);
            } else {
                // null data sent from stop() call
                break;
            }
        }

        return 0;
    }

    void stop() {
        if (mEngine != nullptr && mAudioCapturer != nullptr) {
            mEngine->stop(mAudioCapturer.get());
        }

        // Signal to stop the thread by sending null data
        AudioPacket pkt;
        mAudioPackets.send(std::move(pkt));
    }

private:
    enum { kMaxPackets = 16 };

    // Called from the qemu io thread. Keep this as fast as possible.
    static int onSample(void* opaque, void* buf, int size) {
        AudioCaptureThreadImpl* t =
                reinterpret_cast<AudioCaptureThreadImpl*>(opaque);

        if (buf == nullptr || size <= 0) {
            return -1;
        }

        AudioPacket pkt(buf, size, t->mBits);
        if (!t->mAudioPackets.trySend(std::move(pkt))) {
            dwarning("Audio queue full. Discarding packet.\n");
        }

        return 0;
    }

    AudioCaptureThread::Callback* mCallback = nullptr;
    void* mOpaque = nullptr;
    int mSampleRate = 0;
    AudioCaptureEngine* mEngine = nullptr;
    std::unique_ptr<AudioCapturerImpl> mAudioCapturer;
    int mBits = 0;
    int mChannels = 0;
    android::base::MessageChannel<AudioPacket, kMaxPackets> mAudioPackets;
};  // class AudioCaptureThreadImpl
}  // namespace

AudioCaptureThread* AudioCaptureThread::create(Callback* cb,
                                               void* opaque,
                                               int sampleRate,
                                               int bits,
                                               int nchannels) {
    return new AudioCaptureThreadImpl(cb, opaque, sampleRate, bits, nchannels);
}
}  // namespace audio
}  // namespace android
