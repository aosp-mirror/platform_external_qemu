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

#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/threads/FunctorThread.h"
#include "android/hw-sensors.h"
#include "android/mp4/MP4Dataset.h"
#include "android/mp4/SensorLocationEventProvider.h"
#include "android/recording/video/player/PacketQueue.h"
#include "android/recording/video/player/VideoPlayerWaitInfo.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

using android::videoplayer::PacketQueue;
using android::videoplayer::VideoPlayerWaitInfo;

namespace android {
namespace mp4 {

static constexpr const size_t kMaxQueueSize = 15 * 1024 * 1024;  // 15MiB

class Mp4DemuxerImpl : public Mp4Demuxer {
public:
    Mp4DemuxerImpl(Mp4Dataset* dataset, VideoPlayerWaitInfo* readingWaitInfo)
        : mDataset(dataset),
          mContinueReadWaitInfo(readingWaitInfo),
          mWorkerThread([this]() { workerThreadFunc(); }) {}
    virtual ~Mp4DemuxerImpl();
    void start(std::function<void()> finishedCallback);
    void stop();
    void wait();

    void setAudioPacketQueue(PacketQueue* audioPacketQueue) {
        mAudioPacketQueue = audioPacketQueue;
    }

    void setVideoPacketQueue(PacketQueue* videoPacketQueue) {
        mVideoPacketQueue = videoPacketQueue;
    }

    void setSensorLocationEventProvider(
            SensorLocationEventProvider* eventProvider) {
        mEventProvider = eventProvider;
    }

private:
    int demux();
    void workerThreadFunc();

    bool isDataStreamIndex(int index);

private:
    Mp4Dataset* mDataset;
    VideoPlayerWaitInfo* mContinueReadWaitInfo;
    base::FunctorThread mWorkerThread;
    std::function<void()> mFinishedCallback;
    bool mRunning = false;
    PacketQueue* mAudioPacketQueue = nullptr;
    PacketQueue* mVideoPacketQueue = nullptr;
    SensorLocationEventProvider* mEventProvider = nullptr;
};

std::unique_ptr<Mp4Demuxer> Mp4Demuxer::create(
        Mp4Dataset* dataset,
        VideoPlayerWaitInfo* readingWaitInfo) {
    std::unique_ptr<Mp4Demuxer> demuxer;
    demuxer.reset(new Mp4DemuxerImpl(dataset, readingWaitInfo));
    return std::move(demuxer);
}

Mp4DemuxerImpl::~Mp4DemuxerImpl() {
    stop();
}

void Mp4DemuxerImpl::start(std::function<void()> finishedCallback) {
    mFinishedCallback = finishedCallback;
    mWorkerThread.start();
}

void Mp4DemuxerImpl::stop() {
    mRunning = false;
    mWorkerThread.wait();
}

void Mp4DemuxerImpl::wait() {
    mWorkerThread.wait();
}

int Mp4DemuxerImpl::demux() {
    int ret = 0;
    AVPacket packet;
    AVFormatContext* formatCtx = mDataset->getFormatContext();
    const int audioStreamIndex = mDataset->getAudioStreamIndex();
    const int videoStreamIndex = mDataset->getVideoStreamIndex();

    mRunning = true;
    while (mRunning && (ret = av_read_frame(formatCtx, &packet)) >= 0) {
        if (mAudioPacketQueue != nullptr && audioStreamIndex >= 0 &&
            packet.stream_index == audioStreamIndex) {
            mAudioPacketQueue->put(&packet);
        } else if (mVideoPacketQueue != nullptr && videoStreamIndex >= 0 &&
                   packet.stream_index == videoStreamIndex) {
            mVideoPacketQueue->put(&packet);
        } else {
            // Create an event from this packet if it's from one of the known
            // data stream that carries event info
            if (mEventProvider != nullptr &&
                isDataStreamIndex(packet.stream_index)) {
                mEventProvider->createEvent(&packet);
            }

            // At this point, the packet is no longer useful.
            // It either has been consumed by mEventProvider, or isn't from a
            // stream we care about. Wipe the packet here.
            av_packet_unref(&packet);
        }

        // if the queues are full, no need to read more
        int curent_queues_total_size = 0;
        if (mAudioPacketQueue != nullptr) {
            curent_queues_total_size += mAudioPacketQueue->getSize();
        }
        if (mVideoPacketQueue != nullptr) {
            curent_queues_total_size += mVideoPacketQueue->getSize();
        }

        if ((curent_queues_total_size > kMaxQueueSize ||
            (formatCtx->streams[videoStreamIndex]->disposition &
             AV_DISPOSITION_ATTACHED_PIC)) &&
             mContinueReadWaitInfo != nullptr) {
            // wait 10 ms
            android::base::System::Duration timeoutMs = 10ll;
            mContinueReadWaitInfo->lock.lock();
            const auto deadlineUs =
                    android::base::System::get()->getUnixTimeUs() +
                    timeoutMs * 1000;
            while (android::base::System::get()->getUnixTimeUs() < deadlineUs) {
                mContinueReadWaitInfo->cvDone.timedWait(&mContinueReadWaitInfo->lock, deadlineUs);
            }
            mContinueReadWaitInfo->lock.unlock();
        }
    }

    if (ret == AVERROR_EOF || avio_feof(formatCtx->pb)) {
        if (videoStreamIndex >= 0 && mVideoPacketQueue != nullptr) {
            mVideoPacketQueue->putNullPacket(videoStreamIndex);
        }
        if (audioStreamIndex >= 0 && mAudioPacketQueue != nullptr) {
            mAudioPacketQueue->putNullPacket(audioStreamIndex);
        }
    }

    mFinishedCallback();

    return 0;
}

void Mp4DemuxerImpl::workerThreadFunc() {
    int ret = demux();
    (void)ret;
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
