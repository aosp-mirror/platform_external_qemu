// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/mp4/MP4Dataset.h"

#include <string>

#include "android/base/Log.h"
#include "android/recording/AVScopedPtr.h"
#include "android/recording/video/player/PacketQueue.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

using android::recording::AVScopedPtr;
using android::recording::makeAVScopedPtr;

namespace android {
namespace mp4 {

class Mp4DatasetImpl : public Mp4Dataset {
public:
    Mp4DatasetImpl(std::string filepath) : mDatasetFilepath(filepath) {}
    virtual ~Mp4DatasetImpl(){};
    int getAudioStreamIndex() { return mAudioStreamIdx; }
    int getVideoStreamIndex() { return mVideoStreamIdx; }
    int getSensorDataStreamIndex(AndroidSensor sensor) {
        return mSensorStreamsIdx[sensor];
    }
    int getLocationDataStreamIndex() { return mLocationStreamIdx; }
    AVFormatContext* getFormatContext() { return mFormatCtx.get(); }
    void clearFormatContext() { mFormatCtx.reset(); }

private:
    std::string mDatasetFilepath;
    AVScopedPtr<AVFormatContext> mFormatCtx;
    int mAudioStreamIdx = -1;
    int mVideoStreamIdx = -1;
    int mSensorStreamsIdx[MAX_SENSORS] = {-1, -1, -1, -1, -1, -1,
                                          -1, -1, -1, -1, -1};
    int mLocationStreamIdx = -1;

private:
    int init();
};

std::unique_ptr<Mp4Dataset> Mp4Dataset::create(std::string filepath) {
    std::unique_ptr<Mp4Dataset> dataset;
    dataset.reset(new Mp4DatasetImpl(filepath));
    if (dataset->init() < 0) {
        LOG(ERROR) << ": Failed to initialize mp4 dataset!";
        return nullptr;
    }
    return std::move(dataset);
}

// TODO(haipengwu): Add data stream indices setting logic whith proto
// integration
int Mp4DatasetImpl::init() {
    const char* filename = mDatasetFilepath.c_str();

    // Register all formats and codecs
    av_register_all();
    avformat_network_init();

    AVFormatContext* inputCtx = nullptr;
    if (avformat_open_input(&inputCtx, filename, NULL, NULL) != 0) {
        LOG(ERROR) << ": Failed to open input context";
        return -1;
    }

    mFormatCtx = makeAVScopedPtr(inputCtx);

    if (avformat_find_stream_info(mFormatCtx.get(), NULL) < 0) {
        LOG(ERROR) << ": Failed to find stream info";
        return -1;
    }

    av_dump_format(mFormatCtx.get(), 0, filename, false);

    for (int i = 0; i < mFormatCtx->nb_streams; i++) {
        if (mFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            mAudioStreamIdx = i;
        } else if (mFormatCtx->streams[i]->codec->codec_type ==
                   AVMEDIA_TYPE_VIDEO) {
            mVideoStreamIdx = i;
        }
    }

    if (mVideoStreamIdx == -1) {
        LOG(ERROR) << ": No video stream found";
        return -1;
    }

    return 0;
}

}  // namespace mp4
}  // namespace android