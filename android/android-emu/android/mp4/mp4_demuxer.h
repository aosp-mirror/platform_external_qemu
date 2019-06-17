#pragma once

#include "android/hw-sensors.h"
#include "android/mp4/mp4_dataset.h"
#include "android/recording/video/player/PacketQueue.h"
#include "android/recording/video/player/VideoPlayerWaitInfo.h"

using android::videoplayer::PacketQueue;
using android::videoplayer::VideoPlayerWaitInfo;

namespace android {
namespace mp4 {

class Mp4Demuxer {
public:
    ~Mp4Demuxer() = default;
    static std::unique_ptr<Mp4Demuxer> create(Mp4Dataset* dataset, VideoPlayerWaitInfo* readingWaitInfo);
    virtual void start(std::function<void()> finishedCallback);
    virtual void stop();
    virtual void setAudioPacketQueue(PacketQueue* audioPacketQueue);
    virtual void setVideoPacketQueue(PacketQueue* videoPacketQueue);
    virtual void setSensorPacketQueue(AndroidSensor sensor, PacketQueue* sensorPacketQueue);
    virtual void setLocationPacketQueue(PacketQueue* locationPacketQueue);
protected:
    Mp4Demuxer() = default;
};

}
}
