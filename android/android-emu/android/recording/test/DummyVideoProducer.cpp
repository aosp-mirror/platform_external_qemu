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

// Generates dummy video data for unit-testing purposes
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

        int offset = 0;
        auto szBytes = getVideoFormatSize(mFormat.videoFormat);

        for (int i = 0; i < mDurationSecs * mFps; ++i) {
            /* Fill frame with dummy data */
            switch (mFormat.videoFormat) {
                case VideoFormat::RGBA8888:
                case VideoFormat::BGRA8888: {
                    // Four horizontal bars with different colors moving
                    // downwards and wrapping around at the top.
                    auto ptr = reinterpret_cast<int32_t*>(&(frame.dataVec[0]));
                    auto ptrSize = frame.dataVec.size() / szBytes;
                    const int bars = 4;
                    for (int q = 0; q < bars; ++q) {
                        int begin = (offset + (q * ptrSize / bars)) % ptrSize;
                        int end =
                                (offset + ((q + 1) * ptrSize / bars)) % ptrSize;
                        int32_t color = 0xff000000 >> (8 * q);
                        if (begin < end) {
                            memset_4(ptr + begin, color, end - begin);
                        } else {
                            memset_4(ptr, color, end);
                            memset_4(ptr + begin, color, ptrSize - begin);
                        }
                    }
                    break;
                }
                case VideoFormat::RGB565: {
                    // Three bars moving downwards and wrapping around at the
                    // top.
                    auto ptr = reinterpret_cast<int16_t*>(&(frame.dataVec[0]));
                    auto ptrSize = frame.dataVec.size() / szBytes;
                    const int bars = 3;
                    for (int q = 0; q < bars; ++q) {
                        int begin = (offset + (q * ptrSize / bars)) % ptrSize;
                        int end =
                                (offset + ((q + 1) * ptrSize / bars)) % ptrSize;
                        int16_t color = 0xF800;  // red
                        switch (q) {
                            case 1:
                                color = 0x07E0;  // green
                                break;
                            case 2:
                                color = 0x001F;  // blue
                                break;
                            default:
                                break;
                        }
                        if (begin < end) {
                            memset_2(ptr + begin, color, end - begin);
                        } else {
                            memset_2(ptr, color, end);
                            memset_2(ptr + begin, color, ptrSize - begin);
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            frame.tsUs = startTimeUs + (i * mTimeDeltaUs);
            mCallback(&frame);
            offset += 5 * mFbWidth;
        }

        mOnFinishedCb();
        return 0;
    }

    virtual void stop() override {
        // Don't need to to anything here because main() stops after
        // mDurationSecs.
    }

private:
    // helper to set by 4 bytes at a time
    void memset_4(int32_t* ptr, int32_t value, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            *(ptr + i) = value;
        }
    }

    // helper to set by 2 bytes at a time
    void memset_2(int16_t* ptr, int16_t value, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            *(ptr + i) = value;
        }
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

