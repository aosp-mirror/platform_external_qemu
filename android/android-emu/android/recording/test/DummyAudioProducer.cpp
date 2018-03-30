// Copyright (C) 2018 The Android Open Source Project
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

#include "android/recording/test/DummyAudioProducer.h"

#include "android/base/system/System.h"
#include "android/recording/Frame.h"

namespace android {
namespace recording {

namespace {

// Class to produce dummy audio data for testing purposes.
class DummyAudioProducer : public Producer {
public:
    DummyAudioProducer(uint32_t sampleRate,
                  uint32_t nbSamples,
                  uint8_t nChannels,
                  uint8_t durationSecs,
                  AudioFormat fmt,
                  std::function<void()> onFinishedCb)
        : mSampleRate(sampleRate), mNbSamples(nbSamples), mChannels(nChannels), mDurationSecs(durationSecs), mOnFinishedCb(std::move(onFinishedCb)) {
        mFormat.audioFormat = fmt;
        mMicroSecondsPerFrame = (uint64_t)(((float)mNbSamples / mSampleRate) * 1000000);
    }

    virtual ~DummyAudioProducer() {}

    intptr_t main() final {
        assert(mCallback);

        uint64_t startTimeUs = android::base::System::get()->getHighResTimeUs();
        auto sz = mNbSamples * mChannels * getAudioFormatSize(mFormat.audioFormat);
        Frame frame(sz);
        frame.format.audioFormat = mFormat.audioFormat;

        /* init signal generator */
        float t = 0;
        float tincr = 2 * M_PI * 110.0 / mSampleRate;

        float t_stereo = 0;
        float tincr_stereo = 2 * M_PI * 130.0 / mSampleRate;
        /* increment frequency by 110 Hz per second */
        float tincr2 = tincr / mSampleRate;

        for (uint64_t elapsedUs = 0; elapsedUs < mDurationSecs * 1000000; elapsedUs += mMicroSecondsPerFrame) {
            /* Fill frame with dummy data */
            if (mFormat.audioFormat == AudioFormat::AUD_FMT_S16) {
                auto ptr = reinterpret_cast<int16_t*>(&(frame.dataVec[0]));
                auto ptrSize = frame.dataVec.size() / 2;
                for (int j = 0; j < ptrSize / mChannels; ++j) {
                    int v[2] = {(int)(sin(t) * 0x7fff),
                                (int)(sin(t_stereo) * 0x7fff)};
                    for (int i = 0; i < mChannels; i++) {
                        ptr[j * mChannels + i] = v[i];
                    }
                    t += tincr;
                    tincr += tincr2;
                    t_stereo += tincr_stereo;
                    tincr_stereo += tincr2;
                }
            } else if (mFormat.audioFormat == AudioFormat::AUD_FMT_U8) {
                auto ptr = reinterpret_cast<int8_t*>(&(frame.dataVec[0]));
                auto ptrSize = frame.dataVec.size();
                for (int j = 0; j < ptrSize / mChannels; ++j) {
                    int v[2] = {(int)(sin(t) * 0xFF),
                                (int)(sin(t_stereo) * 0xFF)};
                    for (int i = 0; i < mChannels; i++) {
                        ptr[j * mChannels + i] = v[i];
                    }
                    t += tincr;
                    tincr += tincr2;
                    t_stereo += tincr_stereo;
                    tincr_stereo += tincr2;
                }
            }
            frame.tsUs = startTimeUs + elapsedUs;
            mCallback(&frame);
        }

        mOnFinishedCb();
        return 0;
    }

    virtual void stop() override {
        // Don't need to to anything here because main() stops after
        // mDurationSecs.
    }

private:
    uint32_t mSampleRate = 0;
    uint32_t mNbSamples = 0;
    uint8_t mChannels = 0;
    uint8_t mDurationSecs = 0;
    uint64_t mMicroSecondsPerFrame = 0;
    std::function<void()> mOnFinishedCb;
};  // class VideoProducer
}  // namespace

std::unique_ptr<Producer> createDummyAudioProducer(
        uint32_t sampleRate,
        uint32_t nbSamples,
        uint8_t nChannels,
        uint8_t durationSecs,
        AudioFormat fmt,
        std::function<void()> onFinishedCb) {
    return std::unique_ptr<Producer>(
            new DummyAudioProducer(sampleRate, nbSamples, nChannels, durationSecs, fmt, std::move(onFinishedCb)));
}

}  // namespace recording
}  // namespace android


