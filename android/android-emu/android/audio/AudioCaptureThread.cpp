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
#include "android/audio/BufferQueue.h"
#include "android/base/synchronization/Lock.h"
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
    typedef int(Callback)(void* opaque, void* buf, int size);
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
                           int nbSamples,
                           int bits,
                           int nchannels)
        : mCallback(cb),
          mOpaque(opaque),
          mSampleRate(sampleRate),
          mNbSamples(nbSamples),
          mBits(bits),
          mChannels(nchannels) {
        assert(bits == 8 || bits == 16);
        size_t sz = bits == 8 ? sizeof(int8_t) : sizeof(int16_t);
        mBufferQueue.reset(new BufferQueue<AudioPacket, kMaxPackets>(
                mLock, mNbSamples * mChannels * sz, mBits));
    }

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

        bool finished = false;
        while (!finished) {
            // This will block until we get a buffer with type Data or Finish
            auto pkt = mBufferQueue->acquireBuffer([](AudioPacket* p) {
                return (p->type == PacketType::Data ||
                        p->type == PacketType::Finish);
            });
            if (!pkt) {
                derror("Audio BufferQueue acquireBuffer() returned null\n");
                break;
            }

            switch (pkt->type) {
                case PacketType::Data:
                    mCallback(mOpaque, pkt->data.get(), pkt->size, pkt->tsUs);
                    pkt->type = PacketType::None;
                    mBufferQueue->releaseBuffer(pkt);
                    break;
                default:
                    mBufferQueue->releaseBuffer(pkt);
                    finished = true;
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
        auto pkt = mBufferQueue->dequeueBuffer();
        if (!pkt) {
            derror("Audio BufferQueue dequeueBuffer() returned null\n");
            return;
        }

        // Send a special packet to notify thread of completion.
        pkt->type = PacketType::Finish;
        mBufferQueue->queueBuffer(pkt);
    }

private:
    enum { kMaxPackets = 3 };

    // Called from the qemu io thread. Keep this as fast as possible.
    static int onSample(void* opaque, void* buf, int size) {
        AudioCaptureThreadImpl* t =
                reinterpret_cast<AudioCaptureThreadImpl*>(opaque);

        if (buf == nullptr || size <= 0) {
            return -1;
        }

        int remaining = size;
        while (remaining > 0) {
            auto pkt = t->mBufferQueue->dequeueBuffer();
            if (!pkt) {
                derror("Audio BufferQueue dequeueBuffer() returned null\n");
                break;
            }

            if (pkt->type == PacketType::Finish) {
                // Don't write over a finish-type packet, or the encoding
                // thread will not know when to stop.
                t->mBufferQueue->queueBuffer(pkt);
                break;
            }
            int sz = remaining > pkt->size ? pkt->size : remaining;
            ::memcpy(pkt->data.get(), buf, sz);
            pkt->bitness = t->mBits;
            pkt->tsUs = android::base::System::get()->getHighResTimeUs();
            pkt->type = PacketType::Data;
            remaining -= sz;
            t->mBufferQueue->queueBuffer(pkt);
        }

        return 0;
    }

    AudioCaptureThread::Callback* mCallback = nullptr;
    void* mOpaque = nullptr;
    int mSampleRate = 0;
    int mNbSamples = 0;
    AudioCaptureEngine* mEngine = nullptr;
    std::unique_ptr<AudioCapturerImpl> mAudioCapturer;
    int mBits = 0;
    int mChannels = 0;
    std::unique_ptr<BufferQueue<AudioPacket, kMaxPackets>> mBufferQueue;
    android::base::Lock mLock;
};  // class AudioCaptureThreadImpl
}  // namespace

AudioCaptureThread* AudioCaptureThread::create(Callback* cb,
                                               void* opaque,
                                               int sampleRate,
                                               int nbSamples,
                                               int bits,
                                               int nchannels) {
    return new AudioCaptureThreadImpl(cb, opaque, sampleRate, nbSamples, bits,
                                      nchannels);
}
}  // namespace audio
}  // namespace android
