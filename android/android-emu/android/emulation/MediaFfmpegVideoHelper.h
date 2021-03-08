// Copyright (C) 2019 The Android Open Source Project
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

#pragma once

#include "android/emulation/MediaSnapshotState.h"
#include "android/emulation/MediaVideoHelper.h"
#include "android/emulation/YuvConverter.h"

#include <cstdint>
#include <list>
#include <mutex>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>   // for AVCodecContext, AVPacket
#include <libavformat/avio.h>     // for avio_open, AVIO_FLAG_...
#include <libavutil/avutil.h>     // for AVMediaType, AVMEDIA_...
#include <libavutil/dict.h>       // for AVDictionary
#include <libavutil/error.h>      // for av_make_error_string
#include <libavutil/frame.h>      // for AVFrame, av_frame_alloc
#include <libavutil/log.h>        // for av_log_set_callback
#include <libavutil/pixfmt.h>     // for AVPixelFormat, AV_PIX...
#include <libavutil/rational.h>   // for AVRational
#include <libavutil/samplefmt.h>  // for AVSampleFormat
#include <libavutil/timestamp.h>
}

#include <stdio.h>
#include <string.h>

#include <stddef.h>

namespace android {
namespace emulation {

class MediaFfmpegVideoHelper : public MediaVideoHelper {
public:
    MediaFfmpegVideoHelper(int type, int threads);
    ~MediaFfmpegVideoHelper() override;

    // return true if success; false otherwise
    bool init() override;
    void decode(const uint8_t* frame,
                size_t szBytes,
                uint64_t inputPts) override;
    void flush() override;
    void deInit() override;

private:
    std::vector<uint8_t> mDecodedFrame;

    int mType = 0;
    int mThreadCount = 1;

    // ffmpeg stuff
    AVCodec* mCodec = nullptr;
    AVCodecContext* mCodecCtx = nullptr;
    AVFrame* mFrame = nullptr;
    AVPacket mPacket;

    void copyFrame();
    void fetchAllFrames();
};  // MediaFfmpegVideoHelper

}  // namespace emulation
}  // namespace android
