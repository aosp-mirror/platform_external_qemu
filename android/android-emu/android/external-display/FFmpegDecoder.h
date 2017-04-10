// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details
//

// FFmpegDecoder.h

#pragma once

extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/avassert.h"
#include "libavutil/channel_layout.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavutil/timestamp.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

namespace android {
namespace display {

class DisplayWindow;

class FFmpegDecoder {
public:
    FFmpegDecoder() = default;
    ~FFmpegDecoder();

    static FFmpegDecoder* create(int is_vp8,
                                 const char* extradata,
                                 int extradata_size,
                                 int width,
                                 int height);
    int decode(DisplayWindow* window, const char* data, int data_size);

private:
    void* xzalloc(size_t size);
    bool decode_video(DisplayWindow* window,
                      const char* data,
                      int data_size,
                      int output_width,
                      int output_height);

private:
    int mMediaType;
    enum AVCodecID mCodecId;
    AVCodecContext* mCodecContext = nullptr;
    AVCodec* mCodec = nullptr;
    AVFrame* mFrame = nullptr;
    struct SwsContext* mImgConvertCtx = nullptr;

    AVPacket mPacket;

    enum AVPixelFormat mDecodedFormat;
    char* mDecodedData = nullptr;
    int mDecodedSize = 0;

    char* mExtraData = nullptr;
    int mExtraDataSize = 0;
};

}  // namespace display
}  // namespace android
