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

#include "android/recording/video/VideoFrameSharer.h"

#include <string.h>                            // for memcpy, size_t
#include <sys/types.h>                         // for mode_t
#include <functional>                          // for _1, _2, _3

#include "android/base/Log.h"                  // for LogStream, LOG, LogMes...
#include "android/base/StringView.h"           // for StringView
#include "android/base/memory/SharedMemory.h"  // for SharedMemory, StringView
#include "android/base/system/System.h"        // for System
#include "android/gpu_frame.h"                 // for gpu_set_shared_memory_...
#include "android/recording/Producer.h"        // for Producer


#if DEBUG >= 2
#define DD(fmt,...) fprintf(stderr, "VideoFrameSharer: %s: " fmt "\n", __func__, ##__VA_ARGS__);
#else
#define DD(...) (void)0
#endif

namespace android {
namespace recording {

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

std::atomic<uint64_t> VideoFrameSharer::sFrameCounter{1};

VideoFrameSharer::VideoFrameSharer(uint32_t fbWidth,
                                   uint32_t fbHeight,
                                   const std::string& handle)
    : mVideo({fbWidth, fbHeight, 60}),
      mHandle(handle),
      mPixelBufferSize(getPixelBytes(mVideo)),
      mMemory(handle, sizeof(mVideo) + getPixelBytes(mVideo)) {}

VideoFrameSharer::~VideoFrameSharer() {
    stop();
    mMemory.close(true);
}
bool VideoFrameSharer::initialize() {
    // Prepare the shared memory with some basic info.
    const mode_t user_read_only = 0600;
    int err = mMemory.create(user_read_only);
    if (err != 0) {
        LOG(ERROR) << "Unable to open shared memory of size: " << mMemory.size()
                   << ", due to " << -err;
        mMemory.close(true);
#ifdef __APPLE__
        // TODO(jansene): Shared memory is not the right approach on macos,
        // revisit this and use: https://news.ycombinator.com/item?id=20313751
        // once we are ready.
        const int PageSize = 4096;
         LOG(ERROR) << "Update your kern.sysv.shmmax=" <<
         (mMemory.size() + PageSize & ~(PageSize-1));
         LOG(ERROR) << "settings and restart your machine.";
#endif
        return false;
    }

    memcpy(mMemory.get(), &mVideo, sizeof(mVideo));
    mReadPixels = android_getReadPixelsFunc();
    LOG(INFO) << "Initialized handle: " << mHandle;
    return true;
}

void VideoFrameSharer::frameCallbackForwarder(void* opaque) {
    VideoFrameSharer* pThis = reinterpret_cast<VideoFrameSharer*>(opaque);
    pThis->frameAvailable();
}

void VideoFrameSharer::start() {
    gpu_register_shared_memory_callback(&VideoFrameSharer::frameCallbackForwarder, this);
}

void VideoFrameSharer::stop() {
    gpu_unregister_shared_memory_callback(this);
}

void VideoFrameSharer::frameAvailable() {
    VideoInfo* info = (VideoInfo*)mMemory.get();
    uint8_t* bPixels = (uint8_t*)mMemory.get() + sizeof(mVideo);
    // TODO: enable displayId > 0
    mReadPixels(bPixels, mPixelBufferSize, 0);

    // Update frame information.
    info->frameNumber = sFrameCounter++;
    info->tsUs = base::System::get()->getUnixTimeUs();
    DD("Marshall, frame: %d, ts: %d", info->frameNumber, info->tsUs);
}

size_t VideoFrameSharer::getPixelBytes(VideoInfo info) {
    // ARGB8888 = 4 bytes per pixel.
    return info.width * info.height * 4;
}

}  // namespace recording
}  // namespace android
