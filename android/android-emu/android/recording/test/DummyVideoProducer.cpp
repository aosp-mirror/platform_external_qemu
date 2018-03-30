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

#include "android/recording/test/DummyVideoProducer.h"

#include "android/base/system/System.h"
#include "android/recording/Frame.h"

namespace android {
namespace recording {

namespace {

// Real implementation of Producer for video data
// There are two threads at work: one is grabbing the frames at a user-specified
// fps and another thread is sending each frame received to the callback. We
// separate the frame fetching and encoding because sometimes, the encoder will
// need extra time (usually frames marked as key frames). By separating the
// fetching and encoding and also using multiple buffers we can feed the encoder
// a relatively constant rate of frames.
class DummyVideoProducer : public Producer {
public:
    DummyVideoProducer(uint32_t fbWidth,
                  uint32_t fbHeight,
                  uint8_t fps,
                  uint8_t durationSecs,
                  VideoFormat fmt,
                  std::function<void()> onFinishedCb)
        : mFbWidth(fbWidth), mFbHeight(fbHeight), mFps(fps), mDurationSecs(durationSecs), mOnFinishedCb(std::move(onFinishedCb)) {
        mTimeDeltaUs = 1000000 / mFps;
        mFormat.videoFormat = fmt;
    }

    virtual ~DummyVideoProducer() {}

    intptr_t main() final {
        assert(mCallback);

        uint64_t startTimeUs = android::base::System::get()->getHighResTimeUs();
        auto sz = mFbWidth * mFbHeight * getVideoFormatSize(mFormat.videoFormat);
        Frame frame(sz);
        frame.format.videoFormat = mFormat.videoFormat;

        for (int i = 0; i < mDurationSecs * mFps; ++i) {
            /* Fill frame with dummy data */
            for (int j = 0; j < frame.dataVec.size() / 4; ++j) {
                frame.dataVec[j] = (j % 4 ? 0xFF : 0);
            }
            for (int j = frame.dataVec.size() / 4; j < frame.dataVec.size() / 2; ++j) {
                frame.dataVec[j] = ((j - 1) % 4 ? 0xFF : 0);
            }
            for (int j = frame.dataVec.size() / 2; j < 3 * frame.dataVec.size() / 4; ++j) {
                frame.dataVec[j] = ((j - 2) % 4 ? 0xFF : 0);
            }
            for (int j = 3 * frame.dataVec.size() / 4; j < frame.dataVec.size(); ++j) {
                frame.dataVec[j] = ((j - 3) % 4 ? 0xFF : 0);
            }
            frame.tsUs = startTimeUs + (i * mTimeDeltaUs);
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
    uint32_t mFbWidth = 0;
    uint32_t mFbHeight = 0;
    uint8_t mFps = 0;
    uint8_t mDurationSecs = 0;
    uint64_t mTimeDeltaUs = 0;
    std::function<void()> mOnFinishedCb;
};  // class VideoProducer
}  // namespace

std::unique_ptr<Producer> createDummyVideoProducer(
        uint32_t fbWidth,
        uint32_t fbHeight,
        uint8_t fps,
        uint8_t durationSecs,
        VideoFormat fmt,
        std::function<void()> onFinishedCb) {
    return std::unique_ptr<Producer>(
            new DummyVideoProducer(fbWidth, fbHeight, fps, durationSecs, fmt, std::move(onFinishedCb)));
}

}  // namespace recording
}  // namespace android

