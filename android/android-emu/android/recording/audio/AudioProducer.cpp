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

#include "android/recording/audio/AudioProducer.h"

#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/emulation/AudioCapture.h"
#include "android/emulation/AudioCaptureEngine.h"
#include "android/utils/debug.h"

#include <memory>

#define D(...) VERBOSE_PRINT(audio_producer, __VA_ARGS__);

namespace android {
namespace recording {

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

// Implementation of Producer class for audio data
class AudioProducer : public Producer {
public:
    explicit AudioProducer(int sampleRate,
                           int nbSamples,
                           AudioFormat format,
                           int nchannels)
        : mSampleRate(sampleRate),
          mNbSamples(nbSamples),
          mAudioFormat(format),
          mChannels(nchannels) {
        size_t szBytes = getAudioFormatSize(mAudioFormat);
        // Prefill the free queue with empty packets
        for (int i = 0; i < kMaxFrames; ++i) {
            Frame f(mNbSamples * mChannels * szBytes);
            f.audioFormat = mAudioFormat;
            mFreeQueue.send(std::move(f));
        }
    }

    ~AudioProducer() { stop(); }

    intptr_t main() final {
        assert(mCallback);

        // Start the audio capturer to start receiving audio frames.
        mAudioCapturer.reset(new AudioCapturerImpl(
                [this](const void* buf, int size) -> int {
                    return onSample(buf, size);
                },
                mSampleRate, getAudioFormatSize(mAudioFormat) * CHAR_BIT,
                mChannels));
        if (!mAudioCapturer->start()) {
            derror("Unable to start audio capturer");
            return -1;
        }

        int i = 0;
        while (true) {
            // Consumer
            // Blocks until either we get a frame or the queue is closed.
            auto frame = mDataQueue.receive();
            if (!frame) {
                break;
            }

            D("Encoding audio frame %d (size=%u, tsUs=%lu)\n", i++,
              frame->dataVec.size(), frame->tsUs);
            mCallback(&*frame);
            // Blocks until there is space in the queue or is closed.
            if (!mFreeQueue.send(std::move(*frame))) {
                break;
            }
        }

        return 0;
    }

    virtual void stop() override {
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
            Frame frame;
            // Use non-blocking call here because we want to avoid blocking the
            // io thread. If we can't get a packet, just let this audio packet
            // go.
            if (!mFreeQueue.tryReceive(&frame)) {
                return -1;
            }

            int sz = remaining > frame.dataVec.capacity()
                             ? frame.dataVec.capacity()
                             : remaining;
            frame.dataVec.assign(p, p + sz);
            p += sz;
            frame.tsUs = tsUs;
            remaining -= sz;
            // MessageChannel::send is a blocking call, but since we should
            // never be exceeding the capacity of the channel (kMaxFrames),
            // this call should never block.
            mDataQueue.send(std::move(frame));
        }

        return 0;
    }

    int mSampleRate = 0;
    int mNbSamples = 0;
    AudioFormat mAudioFormat = AudioFormat::INVALID_FMT;
    int mChannels = 0;
    std::unique_ptr<AudioCapturerImpl> mAudioCapturer;
    // mDataQueue contains filled audio frames and mFreeQueue has available
    // audio frames that the producer can use. The workflow is as follows:
    // Producer:
    //   1) get an audio frame from mFreeQueue
    //   2) write data into frame
    //   3) push frame to mDataQueue
    // Consumer:
    //   1) wait for audio frame in mDataQueue
    //   2) process audio frame
    //   3) Push frame back to mFreeQueue
    static constexpr int kMaxFrames = 3;
    MessageChannel<Frame, kMaxFrames> mDataQueue;
    MessageChannel<Frame, kMaxFrames> mFreeQueue;
};  // class AudioProducer
}  // namespace

Producer* createAudioProducer(int sampleRate,
                              int nbSamples,
                              AudioFormat format,
                              int nchannels) {
    return new AudioProducer(sampleRate, nbSamples, format, nchannels);
}

}  // namespace recording
}  // namespace android
