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
    using Callback = std::function<int(const void* buf, int size)>;
    explicit AudioCapturerImpl(Callback cb, int freq, int bits, int nchannels)
        : AudioCapturer(freq, bits, nchannels), mCallback(std::move(cb)) {
        assert(mCallback);
    }

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
        return mCallback(buf, size);
    }

private:
    Callback mCallback;
};  // class AudioCapturerImpl

// Real implementation of AudioCaptureThread
class AudioCaptureThreadImpl : public AudioCaptureThread {
public:
    AudioCaptureThreadImpl(Callback cb,
                           int sampleRate,
                           int nbSamples,
                           int bits,
                           int nchannels)
        : mCallback(std::move(cb)),
          mSampleRate(sampleRate),
          mNbSamples(nbSamples),
          mBits(bits),
          mChannels(nchannels) {
        assert(bits == 8 || bits == 16);
        assert(mCallback);
        size_t szBytes = bits / CHAR_BIT;

        mFrameSize = mNbSamples * mChannels * szBytes;
        double bytes_per_second = (double)mSampleRate * mChannels * szBytes;
        double sec_per_frame = ((double)mFrameSize) / bytes_per_second;
        mMicroSecondsPerFrame = (uint64_t)(sec_per_frame * 1000000);

        // 4 silent frames makes Chrome very happy
        mNbSilentFrames = 4;
        mSilentPkt.reset(new AudioPacket(mNbSilentFrames * mFrameSize, 0));

        // Prefill the free queue with empty packets
        for (int i = 0; i < kMaxPackets; ++i) {
            mFreeQueue.send(
                    std::move(AudioPacket(mFrameSize)));
        }
    }

    ~AudioCaptureThreadImpl() {}

    intptr_t main() final {
        // Start the audio capturer to start receiving audio frames.
        mAudioCapturer.reset(new AudioCapturerImpl(
                [this](const void* buf, int size) -> int {
                    return onSample(buf, size);
                },
                mSampleRate, mBits, mChannels));
        if (!mAudioCapturer->start()) {
            derror("Unable to start audio capturer");
            return -1;
        }

        uint64_t last_pts = android::base::System::get()->getHighResTimeUs();
        while (true) {
            // Consumer
            auto start = android::base::System::get()->getHighResTimeUs();
            auto future = base::System::get()->getUnixTimeUs() + mNbSilentFrames * mMicroSecondsPerFrame;
            // Blocks until either we get a packet, timeout or the queue is closed.
            auto pkt = mDataQueue.timedReceive(future);
            if (!pkt) {
                if (mDataQueue.isStopped()) {
                    break;
                }
                // insert silent audio
                uint64_t diff = android::base::System::get()->getHighResTimeUs() - start;
                last_pts += diff;
                mCallback(mSilentPkt->dataVec.data(), mSilentPkt->dataVec.size(), last_pts);
            } else {
                last_pts = pkt->tsUs;
                mCallback(pkt->dataVec.data(), pkt->dataVec.size(), pkt->tsUs);

                // Blocks until there is space in the queue or is closed.
                if (!mFreeQueue.send(std::move(*pkt))) {
                    break;
                }
            }
        }

        return 0;
    }

    void stop() {
        if (!mAudioCapturer) {
            return;
        }

        mAudioCapturer->stop();
        mDataQueue.stop();
        mFreeQueue.stop();
    }

private:
    // Called from the qemu io thread. Keep this as fast as possible.
    int onSample(const void* buf, int size) {
        if (buf == nullptr || size <= 0) {
            return -1;
        }

        // Producer
        int remaining = size;
        auto p = static_cast<const uint8_t*>(buf);
        while (remaining > 0) {
            // Get the timestamp first as dequeuing the buffer and memcpying
            // will take some time.
            auto tsUs = android::base::System::get()->getHighResTimeUs();
            AudioPacket pkt;
            // Use non-blocking call here because we want to avoid blocking the
            // io thread. If we can't get a packet, just let this audio packet
            // go.
            if (!mFreeQueue.tryReceive(&pkt)) {
                return -1;
            }

            int sz = remaining > pkt.dataVec.capacity() ? pkt.dataVec.capacity()
                                                        : remaining;
            pkt.dataVec.assign(p, p + sz);
            p += sz;
            pkt.tsUs = tsUs;
            remaining -= sz;
            // MessageChannel::send is a blocking call, but since we should
            // never be exceeding the capacity of the channel (kMaxPackets),
            // this call should never block.
            mDataQueue.send(std::move(pkt));
        }

        return 0;
    }

    Callback mCallback;
    int mSampleRate = 0;
    int mNbSamples = 0;
    int mBits = 0;
    int mChannels = 0;
    int mFrameSize = 0;
    // micro seconds per frame
    uint64_t mMicroSecondsPerFrame = 0;

    int mNbSilentFrames = 0;
    std::unique_ptr<AudioPacket> mSilentPkt;

    std::unique_ptr<AudioCapturerImpl> mAudioCapturer;
    // mDataQueue contains filled audio packets and mFreeQueue has available
    // audio packets that the producer can use. The workflow is as follows:
    // Producer:
    //   1) get an audio packet from mFreeQueue
    //   2) write data into packet
    //   3) push packet to mDataQueue
    // Consumer:
    //   1) wait for audio packet in mDataQueue
    //   2) process audio packet
    //   3) Push packet back to mFreeQueue
    static constexpr int kMaxPackets = 3;
    MessageChannel<AudioPacket, kMaxPackets> mDataQueue;
    MessageChannel<AudioPacket, kMaxPackets> mFreeQueue;
};  // class AudioCaptureThreadImpl
}  // namespace

AudioCaptureThread* AudioCaptureThread::create(Callback cb,
                                               int sampleRate,
                                               int nbSamples,
                                               int bits,
                                               int nchannels) {
    return new AudioCaptureThreadImpl(std::move(cb), sampleRate, nbSamples,
                                      bits, nchannels);
}
}  // namespace audio
}  // namespace android
