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

#include "android/mp4/MP4Demuxer.h"

#include <stddef.h>                                      // for size_t
#include <stdint.h>                                      // for int64_t
#include <utility>                                       // for move

#include "android/base/Log.h"                            // for LOG, LogMessage
#include "android/base/synchronization/Lock.h"           // for Lock, AutoLock
#include "android/hw-sensors.h"                          // for AndroidSensor
#include "android/mp4/MP4Dataset.h"                      // for Mp4Dataset
#include "android/mp4/SensorLocationEventProvider.h"     // for SensorLocati...
#include "android/mp4/VideoMetadataProvider.h"           // for VideoMetadat...
#include "android/recording/video/player/PacketQueue.h"  // for PacketQueue
#include "android/recording/video/player/VideoPlayer.h"  // for VideoPlayer

extern "C" {
#include "libavcodec/avcodec.h"                          // for AVPacket
#include <libavformat/avformat.h>                        // for av_seek_frame
#include <libavformat/avio.h>                            // for avio_feof
#include <libavutil/avutil.h>                            // for AV_TIME_BASE
#include <libavutil/error.h>                             // for AVERROR_EOF
}

namespace android {
namespace videoplayer {
struct VideoPlayerWaitInfo;
}  // namespace videoplayer
}  // namespace android


using android::videoplayer::PacketQueue;
using android::videoplayer::VideoPlayerWaitInfo;

namespace android {
namespace mp4 {

static constexpr const size_t kMaxQueueSize = 15 * 1024 * 1024;  // 15MiB

class Mp4DemuxerImpl : public Mp4Demuxer {
public:
    Mp4DemuxerImpl(::android::videoplayer::VideoPlayer* player,
                   Mp4Dataset* dataset,
                   VideoPlayerWaitInfo* readingWaitInfo)
        : mPlayer(player),
          mDataset(dataset),
          mContinueReadWaitInfo(readingWaitInfo) {
        mDemuxEnded = false;
    }

    virtual ~Mp4DemuxerImpl() { }

    virtual DemuxResult demuxNextPacket();
    virtual void seek(double timestamp);

    virtual void setAudioPacketQueue(PacketQueue* audioPacketQueue) {
        mAudioPacketQueue = audioPacketQueue;
    }

    virtual void setVideoPacketQueue(PacketQueue* videoPacketQueue) {
        mVideoPacketQueue = videoPacketQueue;
    }

    virtual void setSensorLocationEventProvider(
            std::shared_ptr<SensorLocationEventProvider> eventProvider) {
        mEventProvider = eventProvider.get();
    }

    virtual void setVideoMetadataProvider(
            std::shared_ptr<VideoMetadataProvider> metadataProvider) {
        mVideoMetadataProvider = metadataProvider.get();
    }

private:
    bool isDataStreamIndex(int index);

private:
    const ::android::videoplayer::VideoPlayer* const mPlayer = nullptr;
    Mp4Dataset* mDataset;
    VideoPlayerWaitInfo* mContinueReadWaitInfo;
    PacketQueue* mAudioPacketQueue = nullptr;
    PacketQueue* mVideoPacketQueue = nullptr;
    SensorLocationEventProvider* mEventProvider = nullptr;
    VideoMetadataProvider* mVideoMetadataProvider = nullptr;
    bool mDemuxEnded = true;

    // lock to protect demuxer functions from being called at the same time
    android::base::Lock mLock;
};

std::unique_ptr<Mp4Demuxer> Mp4Demuxer::create(
        ::android::videoplayer::VideoPlayer* player,
        Mp4Dataset* dataset,
        VideoPlayerWaitInfo* readingWaitInfo) {
    std::unique_ptr<Mp4Demuxer> demuxer;
    demuxer.reset(new Mp4DemuxerImpl(player, dataset, readingWaitInfo));
    return std::move(demuxer);
}

DemuxResult Mp4DemuxerImpl::demuxNextPacket() {
    base::AutoLock lock(mLock);
    AVPacket packet;
    AVFormatContext* formatCtx = mDataset->getFormatContext();
    const int audioStreamIndex = mDataset->getAudioStreamIndex();
    const int videoStreamIndex = mDataset->getVideoStreamIndex();
    const int videoMetadataIndex = mDataset->getVideoMetadataStreamIndex();

    int ret = av_read_frame(formatCtx, &packet);

    if (ret >= 0) {
        if (mAudioPacketQueue != nullptr && audioStreamIndex >= 0 &&
            packet.stream_index == audioStreamIndex) {
            mAudioPacketQueue->put(&packet);
        } else if (mVideoPacketQueue != nullptr && videoStreamIndex >= 0 &&
                   packet.stream_index == videoStreamIndex) {
            mVideoPacketQueue->put(&packet);
        } else if (mVideoMetadataProvider != nullptr &&
                   videoMetadataIndex >= 0 &&
                   packet.stream_index == videoMetadataIndex) {
            mVideoMetadataProvider->createVideoMetadata(&packet);
        } else {
            // Create an event from this packet if it's from one of the known
            // data stream that carries event info
            if (mEventProvider &&
                isDataStreamIndex(packet.stream_index)) {
                mEventProvider->createEvent(&packet);
            }

            // At this point, the packet is no longer useful.
            // It either has been consumed by mEventProvider, or isn't from a
            // stream we care about. Wipe the packet here.
            av_packet_unref(&packet);
        }
        return DemuxResult::OK;
    }

    if (ret == AVERROR_EOF || avio_feof(formatCtx->pb)) {
        auto stream = formatCtx->streams[videoStreamIndex];
        // seek to start of the streams in looping mode
        if (mPlayer->isLooping()) {
            av_seek_frame(formatCtx, -1, 0, 0);
            return DemuxResult::OK;
        }
        // If not looping, put a null packet to each acket queue to mark the end
        // of current video file.
        if (!mDemuxEnded) {
            if (videoStreamIndex >= 0 && mVideoPacketQueue != nullptr) {
                mVideoPacketQueue->putNullPacket(videoStreamIndex);
            }
            if (audioStreamIndex >= 0 && mAudioPacketQueue != nullptr) {
                mAudioPacketQueue->putNullPacket(audioStreamIndex);
            }
            mDemuxEnded = true;
        }
        return DemuxResult::AV_EOF;
    } else {  // other errors
        return DemuxResult::UNKNOWN;
    }
}

void Mp4DemuxerImpl::seek(double timestamp) {
    base::AutoLock lock(mLock);
    AVFormatContext* formatCtx = mDataset->getFormatContext();
    const int audioStreamIndex = mDataset->getAudioStreamIndex();
    const int videoStreamIndex = mDataset->getVideoStreamIndex();

    int64_t convertedTimestamp = timestamp * (double) (AV_TIME_BASE);
    int ret = av_seek_frame(formatCtx, -1, convertedTimestamp, AVSEEK_FLAG_ANY);

    if (ret < 0) {
        LOG(ERROR) << "av_seek_frame returned error";
        return;
    }
    if (mAudioPacketQueue != nullptr && audioStreamIndex >= 0) {
        mAudioPacketQueue->flush();
    }
    if (mVideoPacketQueue != nullptr && videoStreamIndex >= 0) {
        mVideoPacketQueue->flush();
    }
}

bool Mp4DemuxerImpl::isDataStreamIndex(int index) {
    bool ret = false;
    if (index < 0)
        return ret;

    if (index == mDataset->getLocationDataStreamIndex()) {
        ret = true;
    }

    for (int i = 0; i < MAX_SENSORS; i++) {
        if (index == mDataset->getSensorDataStreamIndex((AndroidSensor)i)) {
            ret = true;
        }
    }

    return ret;
}

}  // namespace mp4
}  // namespace android
