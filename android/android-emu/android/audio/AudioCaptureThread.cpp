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

#include "android/base/memory/ScopedPtr.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/sockets/SocketWaiter.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/emulation/AudioCapture.h"
#include "android/emulation/AudioCaptureEngine.h"
#include "android/utils/debug.h"

#include <memory>

namespace android {
namespace audio {

using android::base::MessageChannel;
using android::base::ScopedPtr;
using android::base::SocketWaiter;
using android::emulation::AudioCaptureEngine;
using android::emulation::AudioCapturer;

namespace {

// A small structure to encapsulate the audio data to
// pass between the main loop thread and another thread.
struct AudioPacket {
    uint8_t* data;
    int size;
    int bitness;  // either uint8_t(AUD_FMT_U8) or int16_t(AUD_FMT_S16)

    AudioPacket(void* buf, int size_bytes, int bits)
        : data(nullptr), size(size_bytes), bitness(bits) {
        if (buf != nullptr && size > 0 && (bits == 8 || bits == 16)) {
            data = new uint8_t[size];
            ::memcpy(data, buf, size);
        }
    }

    ~AudioPacket() {
        if (data)
            delete[] data;
    }
};

class AudioCapturerImpl : public AudioCapturer {
public:
    explicit AudioCapturerImpl(AudioCaptureThread::Callback cb,
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
    AudioCaptureThread::Callback* mCallback = nullptr;
    void* mOpaque = nullptr;
};  // class AudioCapturerImpl

// Real implementation of AudioCaptureThread
class AudioCaptureThreadImpl : public AudioCaptureThread {
public:
    AudioCaptureThreadImpl(Callback* cb,
                           void* opaque,
                           int sampleRate,
                           int bits,
                           int nchannels)
        : mCallback(cb),
          mOpaque(opaque),
          mSampleRate(sampleRate),
          mBits(bits),
          mChannels(nchannels) {
        if (::android::base::socketCreatePair(&mInSocket, &mOutSocket) < 0) {
            derror("Could not create socket pair\n");
            return;
        }
    }

    ~AudioCaptureThreadImpl() {
        android::base::socketClose(mOutSocket);
        android::base::socketClose(mInSocket);
    }

    intptr_t main() override final {
        if (mCallback == nullptr || mInSocket == -1 || mOutSocket == -1) {
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

        // Wait for data
        ScopedPtr<SocketWaiter> waiter(SocketWaiter::create());
        waiter->update(mOutSocket, SocketWaiter::kEventRead);

        int ret;
        while ((ret = waiter->wait(INT64_MAX)) != -1) {
            unsigned events = 0;
            if (mOutSocket == waiter->nextPendingFd(&events) &&
                events & SocketWaiter::kEventRead) {
                char c = 0;
                android::base::socketRecv(mOutSocket, &c, 1);
                if (!c) {
                    break;
                }

                AudioPacket* pkt = nullptr;
                mAudioPackets.receive(&pkt);
                if (pkt) {
                    mCallback(mOpaque, pkt->data, pkt->size);
                    delete pkt;
                }
            }
        }

        if (ret == -1) {
            derror("Error while waiting for audio data.\n");
        }

        waiter.reset();
        return 0;
    }

    void stop() {
        if (mEngine != nullptr && mAudioCapturer != nullptr) {
            mEngine->stop(mAudioCapturer.get());
        }

        // Signal to stop sending audio
        char c = 0;
        android::base::socketSend(mInSocket, &c, 1);
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

        AudioPacket* pkt = new AudioPacket(buf, size, t->mBits);
        if (!t->mAudioPackets.trySend(pkt)) {
            dwarning("Audio queue full. Discarding packet.\n");
        } else {
            char c = 1;
            android::base::socketSend(t->mInSocket, &c, 1);
        }

        return 0;
    }

    Callback* mCallback = nullptr;
    void* mOpaque = nullptr;
    int mSampleRate = 0;
    AudioCaptureEngine* mEngine = nullptr;
    std::unique_ptr<AudioCapturerImpl> mAudioCapturer;
    int mBits = 0;
    int mChannels = 0;
    int mInSocket = -1;
    int mOutSocket = -1;
    android::base::MessageChannel<AudioPacket*, kMaxPackets> mAudioPackets;
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
