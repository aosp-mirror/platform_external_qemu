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

    // Start the audio capturer
    bool start() {
        auto engine = AudioCaptureEngine::get();
        if (!engine) {
            return false;
        }

        return engine->start(this) == 0;
    }

    // Stop the audio capturer
    bool stop() {
        auto engine = AudioCaptureEngine::get();
        if (!engine) {
            return false;
        }

        return engine->stop(this) == 0;
    }

    virtual ~AudioCapturerImpl() {
        // Doesn't hurt to stop the audio capture engine even if we stopped it
        // already
        auto engine = AudioCaptureEngine::get();
        if (engine) {
            engine->stop(this);
        }
    }

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
                mLock, mNbSamples * mChannels * sz));
    }

    ~AudioCaptureThreadImpl() {}

    intptr_t main() override final {
        if (mCallback == nullptr) {
            derror("No audio callback supplied");
            return -1;
        }

        // Start the audio capturer to start receiving audio frames.
        mAudioCapturer.reset(
                new AudioCapturerImpl(&AudioCaptureThreadImpl::onSample, this,
                                      mSampleRate, mBits, mChannels));
        if (!mAudioCapturer->start()) {
            derror("Unable to start audio capturer");
            return -1;
        }

        bool finished = false;
        while (!finished) {
            // This will block until we get a buffer with type Data or Finish
            auto pkt = mBufferQueue->acquireBuffer([](AudioPacket* p) {
                return (p->type == PacketType::Data ||
                        p->type == PacketType::Finish);
            });
            assert(pkt);

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
        if (!mBufferQueue || !mAudioCapturer) {
            return;
        }

        mAudioCapturer->stop();

        // Set all buffers' type to finish so the audio capture thread
        // is guaranteed to pick it up if waiting or has not yet acquired a
        // frame yet.
        for (int i = 0; i < kMaxPackets; ++i) {
            auto pkt = mBufferQueue->dequeueBuffer();
            assert(pkt);
            // Send a special packet to notify thread of completion.
            pkt->type = PacketType::Finish;
            mBufferQueue->queueBuffer(pkt);
        }
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
            // Get the timestamp first as dequeuing the buffer and memcpying
            // will take some time.
            auto tsUs = android::base::System::get()->getHighResTimeUs();
            auto pkt = t->mBufferQueue->dequeueBuffer();
            assert(pkt);

            if (pkt->type == PacketType::Finish) {
                // Don't write over a finish-type packet, or the encoding
                // thread will not know when to stop.
                t->mBufferQueue->queueBuffer(pkt);
                break;
            }
            int sz = remaining > pkt->capacity ? pkt->capacity : remaining;
            ::memcpy(pkt->data.get(), buf, sz);
            pkt->tsUs = tsUs;
            pkt->type = PacketType::Data;
            pkt->size = sz;
            remaining -= sz;
            t->mBufferQueue->queueBuffer(pkt);
        }

        return 0;
    }

    AudioCaptureThread::Callback* mCallback = nullptr;
    void* mOpaque = nullptr;
    int mSampleRate = 0;
    int mNbSamples = 0;
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
