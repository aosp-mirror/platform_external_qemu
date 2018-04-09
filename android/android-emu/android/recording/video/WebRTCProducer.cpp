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

#include "android/recording/video/WebRTCProducer.h"

#include "android/base/Log.h"
#include "android/base/memory/SharedMemory.h"
#include "android/base/system/System.h"
#include "android/base/threads/Async.h"
#include "android/gpu_frame.h"
#include "android/opengles.h"
#include "android/recording/Frame.h"
#include "android/utils/debug.h"
#include "android/utils/misc.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>

#include <memory>
#include <cstring>

#ifdef __linux__
#include <linux/videodev2.h>
#endif

#include <libyuv.h>

#define D(...) VERBOSE_PRINT(record, __VA_ARGS__);

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
class WebRTCProducer : public Producer {
public:
 struct VideoInfo {
     uint32_t width;
     uint32_t height;
     uint8_t fps;
    };

    WebRTCProducer(uint32_t fbWidth,
                  uint32_t fbHeight,
                  uint8_t fps,
                  const std::string& handle)
        : mVideo({fbWidth, fbHeight, fps}), mMemory(handle, sizeof(mVideo) + getPixelBytes(mVideo)) {
        mTimeDeltaMs = 1000 / fps;
    }


    virtual ~WebRTCProducer() { }

    intptr_t main() final {
        // Let's not let others mess with this,
        fprintf(stderr, "Started WebRTCProducer\n");
        const mode_t user_read_only = 0600;
        mRecording = true;

        int err = mMemory.open(O_CREAT | O_RDWR | O_TRUNC, user_read_only);
        if (err != 0) {
            LOG(ERROR) << "Unable to open shared memory due to " << errno;
        }

        memcpy(mMemory.get(), &mVideo, sizeof(mVideo));
        uint8_t* bPixels = (uint8_t*) mMemory.get() + sizeof(mVideo);


        // Force a repost
        gpu_frame_set_record_mode(true);
        android_redrawOpenglesWindow();

        unsigned int i = 0;
        const int cPixelBytes = getPixelBytes(mVideo);

        LOG(info) << "Started WebRTC producer of " << mVideo.width << "x"
                  << mVideo.height << " at: " << mVideo.fps;
        while (mRecording) {
            long long currTimeMs =
                    android::base::System::get()->getHighResTimeUs() / 1000;

            auto px = (unsigned char*)gpu_frame_get_record_frame();

            if (px) {
                D("sending video frame %d", i++);
                // We need to convert this to I420, otherwise the WebRTC engine
                // will get confused. So we do that here.

                const size_t y_stride = mVideo.width;
                const size_t u_or_v_stride = y_stride / 2;

                const size_t y_size = y_stride * mVideo.height;
                const size_t u_or_v_size = u_or_v_stride * ((mVideo.height + 1) / 2);

                const size_t required_framebuffer_size = y_size + 2 * u_or_v_size;

                if (cPixelBytes < required_framebuffer_size) {
                    /* D("%s: Staging framebuffer too small, %d bytes required, %d provided", */
                    /*   __FUNCTION__, required_framebuffer_size, cPixelBytes); */
                    exit(1);
                }

                uint8_t* y_staging = bPixels;
                uint8_t* u_staging = y_staging + y_size;
                uint8_t* v_staging = u_staging + u_or_v_size;
                int result = ConvertToI420( (uint8_t*) px,           // src_frame
                                       cPixelBytes,       // src_size
                                       y_staging,         // dst_y
                                       y_stride,          // dst_stride_y
                                       u_staging,         // dst_u
                                       u_or_v_stride,     // dst_stride_u
                                       v_staging,         // dst_v
                                       u_or_v_stride,     // dst_stride_v
                                       0,                 // crop_x
                                       0,                 // crop_y
                                       mVideo.width,          // src_width
                                       mVideo.height,         // src_height
                                       mVideo.width,          // crop_width
                                       mVideo.height,         // crop_height
                                       libyuv::kRotate0,  // rotation
                                       libyuv::FOURCC_ABGR); // source format.

                if (result != 0) {
                   fprintf(stderr, "Conversion failed\n");
                }
            }

            // Need to do some calculation here so we are calling
            // gpu_frame_get_record_frame() at mFPS.
            long long newTimeMs =
                    android::base::System::get()->getHighResTimeUs() / 1000;


            if (px) {
                D("Video frame sent in [%lld] ms", newTimeMs - currTimeMs);
            }

            if (newTimeMs - currTimeMs < mTimeDeltaMs) {
                D("Video sending thread sleeping for [%lld] ms\n",
                  mTimeDeltaMs - newTimeMs + currTimeMs);
                android::base::System::get()->sleepMs(mTimeDeltaMs - newTimeMs +
                                                      currTimeMs);
            }
        }

        D("Finished sending video frames");

        return 0;
    }

    virtual void stop() override {
        gpu_frame_set_record_mode(false);
        mRecording = false;
    }

private:
    size_t getPixelBytes(VideoInfo info) {
        // I420 = 12 bits per pixel.
        return info.width * info.height * 12 / 8;
    }

    VideoInfo mVideo;
    base::NamedSharedMemory mMemory;
    uint32_t mTimeDeltaMs = 0;
    uint8_t mTimeLimitSecs = 0;
    bool mRecording;
};  // class WebRTCProducer
}  // namespace

std::unique_ptr<Producer> createWebRTCProducer(
        uint32_t fbWidth,
        uint32_t fbHeight,
        uint8_t fps,
        const std::string& handle) {
    return std::unique_ptr<Producer>(
            new WebRTCProducer(fbWidth, fbHeight, fps, handle));
}

}  // namespace recording
}  // namespace android

