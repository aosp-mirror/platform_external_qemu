#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/threads/FunctorThread.h"
#include "android/hw-sensors.h"
#include "android/mp4/mp4_dataset.h"
#include "android/mp4/mp4_demuxer.h"
#include "android/recording/video/player/PacketQueue.h"
#include "android/recording/video/player/VideoPlayerWaitInfo.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

using android::videoplayer::PacketQueue;
using android::videoplayer::VideoPlayerWaitInfo;

namespace android {
namespace mp4 {

class Mp4DemuxerImpl : public Mp4Demuxer {
public:
    Mp4DemuxerImpl(Mp4Dataset* dataset,
                VideoPlayerWaitInfo* readingWaitInfo)
        : mDataset(dataset),
          mContinueReadWaitInfo(readingWaitInfo),
          mWorkerThread([this]() { workerThreadFunc(); }) {}
    ~Mp4DemuxerImpl();
    void start(std::function<void()> finishedCallback);
    void stop();

    void setAudioPacketQueue(PacketQueue* audioPacketQueue)
    { mAudioPacketQueue = audioPacketQueue; }

    void setVideoPacketQueue(PacketQueue* videoPacketQueue)
    { mVideoPacketQueue = videoPacketQueue; }

    void setLocationPacketQueue(PacketQueue* locationPacketQueue)
    { mLocationPacketQueue = locationPacketQueue; }

    void setSensorPacketQueue(AndroidSensor sensor, PacketQueue* sensorPacketQueue)
    {
        switch (sensor)
        {
#define  SENSOR_(x,y,z,v,w)  case ANDROID_SENSOR_##x: \
             m##z##PacketQueue = sensorPacketQueue;\
             break;
SENSORS_LIST
#undef SENSOR_
            default:
                LOG(ERROR) << __func__ << ": invalid sensor!";
                break;
        }
    }

private:
    int demux();
    void workerThreadFunc();

private:
    Mp4Dataset* mDataset;
    VideoPlayerWaitInfo* mContinueReadWaitInfo;
    base::FunctorThread mWorkerThread;
    std::function<void()> mFinishedCallback;
    bool mRunning = false;
    PacketQueue* mAudioPacketQueue = nullptr;
    PacketQueue* mVideoPacketQueue = nullptr;
    PacketQueue* mLocationPacketQueue = nullptr;
#define  SENSOR_(x,y,z,v,w)  PacketQueue* m##z##PacketQueue = nullptr;
SENSORS_LIST
#undef SENSOR_
};

std::unique_ptr<Mp4Demuxer> Mp4Demuxer::create(Mp4Dataset* dataset, VideoPlayerWaitInfo* readingWaitInfo) {
    std::unique_ptr<Mp4Demuxer> demuxer;
    demuxer.reset(new Mp4DemuxerImpl(dataset, readingWaitInfo));
    return std::move(demuxer);
}

Mp4DemuxerImpl::~Mp4DemuxerImpl() {
    stop();
}

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)

void Mp4DemuxerImpl::start(std::function<void()> finishedCallback) {
    mFinishedCallback = finishedCallback;
    if(mDataset->init() < 0) {
        LOG(ERROR) << __func__ << ": Failed to initialize mp4 dataset!";
        return;
    }
    mWorkerThread.start();
}

void Mp4DemuxerImpl::stop() {
    mRunning = false;
    mWorkerThread.wait();
}

int Mp4DemuxerImpl::demux() {
    int ret = 0;
    AVPacket packet;
    AVFormatContext* formatCtx = mDataset->getFormatContext();
    const int audioStreamIndex = mDataset->getAudioStreamIndex();
    const int videoStreamIndex = mDataset->getVideoStreamIndex();
    const int dataStreamLocationIndex = mDataset->getLocationDataStreamIndex();
#define  SENSOR_(x,y,z,v,w)  const int dataStream##z##Index = mDataset->getSensorDataStreamIndex(ANDROID_SENSOR_##x);
SENSORS_LIST
#undef SENSOR_

    mRunning = true;
    while (mRunning && (ret = av_read_frame(formatCtx, &packet)) >= 0) {
        if (audioStreamIndex >= 0 && packet.stream_index == audioStreamIndex && mAudioPacketQueue != nullptr) {
            mAudioPacketQueue->put(&packet);
        } else if (videoStreamIndex >= 0 && packet.stream_index == videoStreamIndex && mVideoPacketQueue != nullptr) {
            mVideoPacketQueue->put(&packet);
        } else if (dataStreamLocationIndex >= 0 && packet.stream_index == dataStreamLocationIndex && mLocationPacketQueue != nullptr) {
            mLocationPacketQueue->put(&packet);
        }
#define  SENSOR_(x,y,z,v,w)  else if (dataStream##z##Index >= 0 &&  packet.stream_index == dataStream##z##Index && m##z##PacketQueue != nullptr) {\
                 m##z##PacketQueue->put(&packet);}
SENSORS_LIST
#undef SENSOR_
        else {
            // stream that we don't handle, simply unref and ignore it
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
        if (mLocationPacketQueue != nullptr) {
            curent_queues_total_size += mLocationPacketQueue->getSize();
        }
#define  SENSOR_(x,y,z,v,w) if (m##z##PacketQueue != nullptr) {\
                curent_queues_total_size += m##z##PacketQueue->getSize();}
SENSORS_LIST
#undef SENSOR_

        if (curent_queues_total_size > MAX_QUEUE_SIZE ||
            (formatCtx->streams[videoStreamIndex]->disposition &
             AV_DISPOSITION_ATTACHED_PIC)) {
            // wait 10 ms
            android::base::System::Duration timeoutMs = 10ll;
            VideoPlayerWaitInfo* pwi = mContinueReadWaitInfo;
            pwi->lock.lock();
            const auto deadlineUs =
                    android::base::System::get()->getUnixTimeUs() +
                    timeoutMs * 1000;
            while (android::base::System::get()->getUnixTimeUs() < deadlineUs) {
                pwi->cvDone.timedWait(&pwi->lock, deadlineUs);
            }
            pwi->lock.unlock();
        }
    }

    if (ret == AVERROR_EOF || avio_feof(formatCtx->pb)) {
        if (videoStreamIndex >= 0 && mVideoPacketQueue != nullptr) {
            mVideoPacketQueue->putNullPacket(videoStreamIndex);
        }
        if (audioStreamIndex >= 0 && mAudioPacketQueue != nullptr) {
            mAudioPacketQueue->putNullPacket(audioStreamIndex);
        }
        if (dataStreamLocationIndex >= 0 && mLocationPacketQueue != nullptr) {
            mLocationPacketQueue->putNullPacket(dataStreamLocationIndex);
        }
#define  SENSOR_(x,y,z,v,w) if (dataStream##z##Index >= 0 && m##z##PacketQueue != nullptr) {\
                m##z##PacketQueue->putNullPacket(dataStream##z##Index);}
SENSORS_LIST
#undef SENSOR_
    }

    mFinishedCallback();

    return 0;
}

void Mp4DemuxerImpl::workerThreadFunc() {
    int ret = demux();
    (void)ret;
}

}
}
