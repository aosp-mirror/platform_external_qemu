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
#include "VideoShareCapture.h"
#include <rtc_base/logging.h>
#include <sys/time.h>
#include "VideoShareCapture.h"
#include "VideoShareInfo.h"
#include "rtc_base/criticalsection.h"

#include "android/base/memory/SharedMemory.h"

using android::base::SharedMemory;

namespace emulator {
namespace webrtc {

namespace videocapturemodule {

static int64_t getCurrentTimeInUs() {
    timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
}

VideoShareCapture::~VideoShareCapture() {
    RTC_LOG(INFO) << "~VideoShareCapture";
}

bool VideoShareCapture::CaptureProcess() {
    rtc::CritScope cs(&mCaptureCS);

    // Sleep up to mMaxFrameDelay, as we don't want to
    // slam the encoder.
    int64_t now = getCurrentTimeInUs();
    int64_t sleeptime = mMaxFrameDelay - (now - mPrevTimestamp);
    if (sleeptime > 0)
        usleep(sleeptime);

    if (mCaptureStarted) {
        // convert to to I420 if needed
        IncomingFrame((uint8_t*)mPixelBuffer, mPixelBufferSize, mSettings);
    }
    mPrevTimestamp = getCurrentTimeInUs();
    return true;
}

int32_t VideoShareCapture::StopCapture() {
    if (mCaptureThread) {
        // Make sure the capture thread stop stop using the critsect.
        mCaptureThread->Stop();
        mCaptureThread.reset();
    }

    rtc::CritScope cs(&mCaptureCS);
    mCaptureStarted = false;

    return 0;
};

int32_t VideoShareCapture::StartCapture(
        const VideoCaptureCapability& capability) {
    if (mCaptureStarted)
        return 0;

    rtc::CritScope cs(&mCaptureCS);

    // Let's always try to capture at the maximum supported FPS..
    uint8_t minFps = std::min(capability.maxFPS, kInitialFrameRate);
    mMaxFrameDelay = (1 * 1000 * 1000) / capability.maxFPS;

    // start capture thread;
    if (!mCaptureThread) {
        mCaptureThread.reset(new rtc::PlatformThread(
                VideoShareCapture::CaptureThread, this, "CaptureThread"));
        mCaptureThread->Start();
        mCaptureThread->SetPriority(rtc::kHighPriority);
    }

    mCaptureStarted = true;
    RTC_LOG(INFO) << "Started cature thread with minFps: " << minFps << ", frameDelay: " << mMaxFrameDelay;
    return 0;
};

static size_t getPixelCount(const VideoCaptureCapability& capability) {
    return capability.width * capability.height * 12 / 8;
}

int32_t VideoShareCapture::Init(std::string handle) {
    mSharedMemory =
            SharedMemory(handle, getPixelCount(mSettings) +
                                         sizeof(VideoShareInfo::VideoInfo));
    int err = mSharedMemory.open(SharedMemory::AccessMode::READ_ONLY);
    if (err != 0) {
        RTC_LOG(LERROR) << "Unable to open memory mapped handle: [" << handle
                        << "], due to:" << err;
        return -1;
    }
    mPixelBuffer = (uint8_t*)*mSharedMemory + sizeof(VideoShareInfo::VideoInfo);
    mPixelBufferSize = getPixelCount(mSettings);
    return 0;
};

}  // namespace videocapturemodule
}  // namespace webrtc
}  // namespace emulator
