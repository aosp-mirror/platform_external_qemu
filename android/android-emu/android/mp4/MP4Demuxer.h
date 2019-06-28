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

#pragma once

#include "android/hw-sensors.h"
#include "android/mp4/MP4Dataset.h"
#include "android/mp4/SensorLocationEventProvider.h"
#include "android/recording/video/player/PacketQueue.h"
#include "android/recording/video/player/VideoPlayer.h"
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
    virtual ~Mp4Demuxer(){};
    static std::unique_ptr<Mp4Demuxer> create(
            ::android::videoplayer::VideoPlayer* player,
            Mp4Dataset* dataset,
            VideoPlayerWaitInfo* readingWaitInfo);
    virtual void demux() = 0;
    virtual void setAudioPacketQueue(PacketQueue* audioPacketQueue) = 0;
    virtual void setVideoPacketQueue(PacketQueue* videoPacketQueue) = 0;
    virtual void setSensorLocationEventProvider(
            SensorLocationEventProvider* eventProvider) = 0;

protected:
    Mp4Demuxer() = default;
};

}  // namespace mp4
}  // namespace android
