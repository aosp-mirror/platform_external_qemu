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

#include "android/mp4/VideoMetadataProvider.h"

#include <libavcodec/avcodec.h>                  // for AVPacket
#include <memory>                                // for unique_ptr
#include <queue>                                 // for queue
#include <utility>                               // for move

#include "android/base/Log.h"                    // for LogMessage, LOG, CHECK
#include "android/mp4/FieldDecodeInfo.h"         // for FieldDecodeInfo
#include "offworld.pb.h"  // for DataStreamInfo, Fiel...

namespace android {
namespace mp4 {

typedef ::offworld::DatasetInfo DatasetInfo;
typedef ::offworld::FieldInfo::Type Type;
typedef std::queue<VideoMetadata> VideoMetadataQueue;

struct VideoMetadataDecodeInfo {
    int streamIndex = -1;
    FieldDecodeInfo timestamp;
    FieldDecodeInfo frameNumber;
};

class VideoMetadataProviderImpl : public VideoMetadataProvider {
public:
    VideoMetadataProviderImpl() {
        mVideoMetadataQueue.reset(new VideoMetadataQueue());
    };
    virtual ~VideoMetadataProviderImpl(){};
    int init(const DatasetInfo& datasetInfo);
    int createVideoMetadata(const AVPacket* packet) override;
    bool hasNextFrameMetadata() override {
        return mVideoMetadataQueue->size() > 0;
    }
    VideoMetadata getNextFrameMetadata() override;
    VideoMetadata peekNextFrameMetadata() override;

private:
    std::unique_ptr<VideoMetadataQueue> mVideoMetadataQueue;
    VideoMetadataDecodeInfo mVideoMetadataDecodeInfo;
};

std::unique_ptr<VideoMetadataProvider> VideoMetadataProvider::create(
        const DatasetInfo& datasetInfo) {
    std::unique_ptr<VideoMetadataProviderImpl> provider(
            new VideoMetadataProviderImpl());
    if (provider->init(datasetInfo) < 0) {
        LOG(ERROR) << "Failed to initialize video metadata provider!";
        return nullptr;
    }
    return std::unique_ptr<VideoMetadataProvider>(provider.release());
}

int VideoMetadataProviderImpl::init(const DatasetInfo& datasetInfo) {
    if (!datasetInfo.has_video_metadata()) {
        LOG(ERROR)
                << "Dataset info does not contain video metadata stream info!";
        return -1;
    }

    mVideoMetadataDecodeInfo.streamIndex =
            datasetInfo.video_metadata().stream_index();
    mVideoMetadataDecodeInfo.timestamp.offset = datasetInfo.video_metadata()
                                                        .video_metadata_packet()
                                                        .timestamp()
                                                        .offset();
    mVideoMetadataDecodeInfo.timestamp.type = datasetInfo.video_metadata()
                                                      .video_metadata_packet()
                                                      .timestamp()
                                                      .type();
    mVideoMetadataDecodeInfo.frameNumber.offset =
            datasetInfo.video_metadata()
                    .video_metadata_packet()
                    .frame_number()
                    .offset();
    mVideoMetadataDecodeInfo.frameNumber.type = datasetInfo.video_metadata()
                                                        .video_metadata_packet()
                                                        .frame_number()
                                                        .type();

    return 0;
}

int VideoMetadataProviderImpl::createVideoMetadata(const AVPacket* packet) {
    if (packet->stream_index != mVideoMetadataDecodeInfo.streamIndex) {
        LOG(ERROR) << "Incorrect stream index!";
        return -1;
    }

    VideoMetadata metadata;
    uint8_t* data = packet->data;
    // Set timestamp
    int timestampOffset = mVideoMetadataDecodeInfo.timestamp.offset;
    Type timestampType = mVideoMetadataDecodeInfo.timestamp.type;
    switch (timestampType) {
        case Type::FieldInfo_Type_INT_32: {
            auto timestampPtr =
                    reinterpret_cast<int32_t*>(data + timestampOffset);
            metadata.timestamp = static_cast<uint64_t>(*timestampPtr);
            break;
        }
        case Type::FieldInfo_Type_INT_64: {
            auto timestampPtr =
                    reinterpret_cast<int64_t*>(data + timestampOffset);
            metadata.timestamp = static_cast<uint64_t>(*timestampPtr);
            break;
        }
        case Type::FieldInfo_Type_FLOAT: {
            auto timestampPtr =
                    reinterpret_cast<float*>(data + timestampOffset);
            metadata.timestamp = static_cast<uint64_t>(*timestampPtr);
            break;
        }
        case Type::FieldInfo_Type_DOUBLE: {
            auto timestampPtr =
                    reinterpret_cast<double*>(data + timestampOffset);
            metadata.timestamp = static_cast<uint64_t>(*timestampPtr);
            break;
        }
        default:
            LOG(ERROR) << ": Unrecognized timestamp data type!";
            return -1;
    }

    // Set frame number
    int frameNumberOffset = mVideoMetadataDecodeInfo.frameNumber.offset;
    Type frameNumberType = mVideoMetadataDecodeInfo.frameNumber.type;
    switch (frameNumberType) {
        case Type::FieldInfo_Type_INT_32: {
            auto frameNumberPtr =
                    reinterpret_cast<int32_t*>(data + frameNumberOffset);
            metadata.frameNumber = *frameNumberPtr;
            break;
        }
        default:
            LOG(ERROR) << ": Unrecognized frame number data type!";
            return -1;
    }

    mVideoMetadataQueue->push(metadata);
    return 0;
}

VideoMetadata VideoMetadataProviderImpl::getNextFrameMetadata() {
    CHECK(mVideoMetadataQueue->size() > 0);
    VideoMetadata metadata = std::move(mVideoMetadataQueue->front());
    mVideoMetadataQueue->pop();
    return metadata;
}

VideoMetadata VideoMetadataProviderImpl::peekNextFrameMetadata() {
    CHECK(mVideoMetadataQueue->size() > 0);
    return mVideoMetadataQueue->front();
}

}  // namespace mp4
}  // namespace android
