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
#include "android/offworld/proto/offworld.pb.h"
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

typedef ::offworld::DatasetInfo DatasetInfo;

class Mp4DatasetImpl : public Mp4Dataset {
public:
    Mp4DatasetImpl(){};
    virtual ~Mp4DatasetImpl(){};
    int init(std::string filepath, DatasetInfo* datasetInfo);
    int getAudioStreamIndex() { return mAudioStreamIdx; }
    int getVideoStreamIndex() { return mVideoStreamIdx; }
    int getSensorDataStreamIndex(AndroidSensor sensor) {
        return mSensorStreamsIdx[sensor];
    }
    int getLocationDataStreamIndex() { return mLocationStreamIdx; }
    AVFormatContext* getFormatContext() { return mFormatCtx.get(); }
    void clearFormatContext() { mFormatCtx.reset(); }

private:
    AVScopedPtr<AVFormatContext> mFormatCtx;
    int mAudioStreamIdx = -1;
    int mVideoStreamIdx = -1;
    int mSensorStreamsIdx[MAX_SENSORS] = {-1, -1, -1, -1, -1, -1,
                                          -1, -1, -1, -1, -1};
    int mLocationStreamIdx = -1;
};

std::unique_ptr<Mp4Dataset> Mp4Dataset::create(std::string filepath,
                                               DatasetInfo* datasetInfo) {
    auto dataset = new Mp4DatasetImpl();
    if (dataset->init(filepath, datasetInfo) < 0) {
        LOG(ERROR) << ": Failed to initialize mp4 dataset!";
        return nullptr;
    }
    std::unique_ptr<Mp4Dataset> ret;
    ret.reset(dataset);
    return std::move(ret);
}

// TODO(haipengwu): Add data stream indices setting logic whith proto
// integration
int Mp4DatasetImpl::init(std::string filepath, DatasetInfo* datasetInfo) {
    const char* filename = filepath.c_str();

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

    // We expect a dataset to at least have one video stream
    if (mVideoStreamIdx == -1) {
        LOG(ERROR) << ": No video stream found";
        return -1;
    }

    if (datasetInfo != nullptr) {
        if (datasetInfo->has_location()) {
            mLocationStreamIdx = datasetInfo->location().stream_index();
        }
        if (datasetInfo->has_accelerometer()) {
            mSensorStreamsIdx[ANDROID_SENSOR_ACCELERATION] =
                    datasetInfo->accelerometer().stream_index();
        }
        if (datasetInfo->has_gyroscope()) {
            mSensorStreamsIdx[ANDROID_SENSOR_GYROSCOPE] =
                    datasetInfo->gyroscope().stream_index();
        }
        if (datasetInfo->has_magnetic_field()) {
            mSensorStreamsIdx[ANDROID_SENSOR_MAGNETIC_FIELD] =
                    datasetInfo->magnetic_field().stream_index();
        }
    }

    return 0;
}

}  // namespace mp4
}  // namespace android