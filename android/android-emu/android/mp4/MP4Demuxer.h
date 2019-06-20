#pragma once

#include "android/hw-sensors.h"
#include "android/mp4/MP4Dataset.h"
#include "android/mp4/SensorLocationEventProvider.h"
#include "android/recording/video/player/PacketQueue.h"
#include "android/recording/video/player/VideoPlayerWaitInfo.h"

using android::videoplayer::PacketQueue;
using android::videoplayer::VideoPlayerWaitInfo;

namespace android {
namespace mp4 {

// The MP4 demultiplexer (a.k.a. demuxer) extract packets from an MP4
// input file and dispatch these packets to output corresponding to
// their stream index.
//
// A typical MP4 file has audio/video streams. Some might have other data
// streams.
class Mp4Demuxer {
public:
    virtual ~Mp4Demuxer() {};
    static std::unique_ptr<Mp4Demuxer> create(
            Mp4Dataset* dataset,
            VideoPlayerWaitInfo* readingWaitInfo);
    virtual void start(std::function<void()> finishedCallback) = 0;
    virtual void stop() = 0;
    virtual void setAudioPacketQueue(PacketQueue* audioPacketQueue) = 0;
    virtual void setVideoPacketQueue(PacketQueue* videoPacketQueue) = 0;
    virtual void setSensorLocationEventProvider(
            SensorLocationEventProvider* eventProvider) = 0;

protected:
    Mp4Demuxer() = default;
};

}  // namespace mp4
}  // namespace android
