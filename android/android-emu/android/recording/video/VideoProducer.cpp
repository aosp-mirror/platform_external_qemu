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

#include "android/recording/video/VideoProducer.h"

#include <assert.h>                                       // for assert
#include <cstdint>                                        // for uint8_t
#include <functional>                                     // for __base, fun...
#include <memory>                                         // for unique_ptr
#include <new>                                            // for operator new
#include <utility>                                        // for move
#include <vector>                                         // for vector

#include "aemu/base/Optional.h"                        // for Optional
#include "aemu/base/synchronization/MessageChannel.h"  // for MessageChannel
#include "android/base/system/System.h"                   // for System
#include "aemu/base/threads/Async.h"                   // for async
#include "android/emulation/control/display_agent.h"      // for QAndroidDis...
#include "android/gpu_frame.h"                            // for gpu_frame_s...
#include "android/opengles.h"                             // for android_red...
#include "android/recording/Frame.h"                      // for Frame, AVFo...
#include "android/recording/Producer.h"                   // for Producer
#include "android/recording/video/GuestReadbackWorker.h"  // for GuestReadba...
#include "android/utils/debug.h"                          // for VERBOSE_PRINT

#define D(...) VERBOSE_PRINT(record, __VA_ARGS__);

namespace android {
namespace recording {

using android::base::MessageChannel;

namespace {

// Real implementation of Producer for video data
// There are two threads at work: one is grabbing the frames at a user-specified
// fps and another thread is sending each frame received to the callback. We
// separate the frame fetching and encoding because sometimes, the encoder will
// need extra time (usually frames marked as key frames). By separating the
// fetching and encoding and also using multiple buffers we can feed the encoder
// a relatively constant rate of frames.
class VideoProducer : public Producer {
public:
    VideoProducer(uint32_t fbWidth,
                  uint32_t fbHeight,
                  uint8_t fps,
                  uint32_t displayId,
                  const QAndroidDisplayAgent* dpyAgent)
        : mFbWidth(fbWidth), mFbHeight(fbHeight), mFps(fps), mDisplayId(displayId) {
        mTimeDeltaMs = 1000 / mFps;
        // Is |dpyAgent| was supplied, then assume we are in guest mode.
        if (dpyAgent) {
            mIsGuestMode = true;
            mGuestReadbackWorker =
                    GuestReadbackWorker::create(dpyAgent, mFbWidth, mFbHeight);
            mFormat.videoFormat = mGuestReadbackWorker->getPixelFormat();
        } else {
            // The host framebuffer is always using RGBA8888.
            mFormat.videoFormat = VideoFormat::BGRA8888;
        }

        // Prefill the free queue with empty frames
        auto sz =
                mFbWidth * mFbHeight * getVideoFormatSize(mFormat.videoFormat);
        for (int i = 0; i < kMaxFrames; ++i) {
            Frame f(sz);
            f.format.videoFormat = mFormat.videoFormat;
            mFreeQueue.send(std::move(f));
        }
    }

    virtual ~VideoProducer() {}

    intptr_t main() final {
        assert(mCallback);
        mIsStopped = false;
        if (!mIsGuestMode) {
            // Force a repost
            gpu_frame_set_record_mode(true, mDisplayId);
            android_redrawOpenglesWindow();
        }

        android::base::async([this]() { sendFramesWorker(); });

        while (true) {
            // Consumer
            // Blocks until either we get a frame or the queue is closed.
            auto frame = mDataQueue.receive();
            if (!frame) {
                break;
            }

            mCallback(&*frame);

            // Blocks until there is space in the queue or is closed.
            if (!mFreeQueue.send(std::move(*frame))) {
                break;
            }
        }

        return 0;
    }

    void pokeReceiver() {
        mSignal.send(1);
    }

    void waitForPoke() {
        mSignal.receive();
    }

    virtual void stop() override {
        mIsStopped = true;
        mDataQueue.stop();
        mFreeQueue.stop();
        waitForPoke();
        // disable Gpu record when sendFramesWorker() thread exits.
        if (!mIsGuestMode) {
            gpu_frame_set_record_mode(false, mDisplayId);
        }
    }

private:
    // Helper to send frames at the user-specified FPS
    void sendFramesWorker() {
        int i = 0;

        while (true) {
            if (mIsStopped) {
              break;
            }
            long long currTimeMs =
                    android::base::System::get()->getHighResTimeUs() / 1000;

            auto frame = mFreeQueue.receive();
            if (!frame) {
                if (mFreeQueue.isStopped()) {
                    break;
                }
                continue;
            }

            frame->tsUs = android::base::System::get()->getHighResTimeUs();
            bool gotFrame = false;
            if (!mIsGuestMode) {
                auto px = (unsigned char*)gpu_frame_get_record_frame(mDisplayId);
                if (px) {
                    frame->dataVec.assign(px, px + frame->dataVec.size());
                    gotFrame = true;
                }
            } else {
                auto f = mGuestReadbackWorker->getFrame();
                if (f) {
                    frame->dataVec.assign(f->dataVec.begin(), f->dataVec.end());
                    gotFrame = true;
                }
            }

            if (gotFrame) {
                D("sending video frame %d", i++);
                mDataQueue.send(std::move(*frame));
            } else {
                mFreeQueue.send(std::move(*frame));
            }

            // TODO(joshuaduong): Replace this with high precision timers.
            //
            // Need to do some calculation here so we are calling
            // gpu_frame_get_record_frame() at mFPS.
            long long newTimeMs =
                    android::base::System::get()->getHighResTimeUs() / 1000;

            // Stop recording if time limit is reached
            //            if (newTimeMs - startMs >= mTimeLimitSecs * 1000) {
            //                D("Time limit reached (%lld ms). Stopping the
            //                recording",
            //                  newTimeMs - startMs);
            //                stop();
            //                break;
            //            }

            if (gotFrame) {
                D("Video frame sent in [%u] ms", newTimeMs - currTimeMs);
            }

            if (newTimeMs - currTimeMs < mTimeDeltaMs) {
                D("Video sending thread sleeping for [%u] ms\n",
                  mTimeDeltaMs - newTimeMs + currTimeMs);
                android::base::System::get()->sleepMs(mTimeDeltaMs - newTimeMs +
                                                      currTimeMs);
            }
        }

        D("Finished sending video frames");
        pokeReceiver();
    }

    uint32_t mFbWidth = 0;
    uint32_t mFbHeight = 0;
    uint32_t mTimeDeltaMs = 0;
    uint32_t mDisplayId = 0;
    uint8_t mFps = 0;
    uint8_t mTimeLimitSecs = 0;
    bool mIsGuestMode = false;

    // mDataQueue contains filled video frames and mFreeQueue has available
    // video frames that the producer can use. The workflow is as follows:
    // Producer:
    //   1) get an video frame from mFreeQueue
    //   2) write data into frame
    //   3) push frame to mDataQueue
    // Consumer:
    //   1) wait for video frame in mDataQueue
    //   2) process video frame
    //   3) Push frame back to mFreeQueue
    static constexpr int kMaxFrames = 3;
    MessageChannel<Frame, kMaxFrames> mDataQueue;
    MessageChannel<Frame, kMaxFrames> mFreeQueue;
    MessageChannel<int, 1> mSignal;
    bool mIsStopped = false;

    // In guest mode, the frames are already triple-buffered in the readback
    // worker, so we don't need to use the messaage channel.
    std::unique_ptr<GuestReadbackWorker> mGuestReadbackWorker;
};  // class VideoProducer
}  // namespace

std::unique_ptr<Producer> createVideoProducer(
        uint32_t fbWidth,
        uint32_t fbHeight,
        uint8_t fps,
        uint32_t displayId,
        const QAndroidDisplayAgent* dpyAgent) {
    return std::unique_ptr<Producer>(
            new VideoProducer(fbWidth, fbHeight, fps, displayId, dpyAgent));
}

}  // namespace recording
}  // namespace android
