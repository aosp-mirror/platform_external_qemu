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

#include "android/base/files/PathUtils.h"                        // for Path...
#include "android/base/system/System.h"                          // for System
#include "android/mp4/MP4Dataset.h"                              // for Mp4D...
#include "offworld.pb.h"                  // for Data...
#include "android/recording/video/player/PacketQueue.h"          // for Pack...
#include "android/recording/video/player/VideoPlayer.h"          // for Play...
#include "android/recording/video/player/VideoPlayerWaitInfo.h"  // for vide...

extern "C" {
#include "libavcodec/avcodec.h"                                  // for AVPa...
}

#include <gtest/gtest.h>                                         // for Message
#include <string>                                                // for string

using namespace android::mp4;
using namespace android::videoplayer;

class MockedVideoPlayer : public VideoPlayer {
public:
    MockedVideoPlayer() = default;
    ~MockedVideoPlayer() = default;
    void start() { mRunning = true; }
    void start(const PlayConfig& playConfig = PlayConfig()) { mRunning = true; }
    void stop() { mRunning = false; }
    void pause() {}
    void pauseAt(double timestamp) {}
    void seek(double timestamp) {}
    bool isRunning() const { return mRunning; }
    bool isLooping() const { return 0; }
    void videoRefresh() {}
    void scheduleRefresh(int delayMS) {}
    void loadVideoFileWithData(const ::offworld::DatasetInfo& datasetInfo) {}
    void setErrorStatusAndRecordErrorMessage(std::string errorDetails){};
    bool getErrorStatus(){ return true; };
    std::string getErrorMessage(){ return ""; };

private:
    bool mRunning = false;
    int mPktSerial = 0;
    bool isError = false;
    std::string errorMessage = "";
};

class Mp4DemuxerTest : public testing::Test {
protected:
    std::unique_ptr<PacketQueue> mAudioQueue;
    std::unique_ptr<PacketQueue> mVideoQueue;
    std::unique_ptr<MockedVideoPlayer> mMockedVideoPlayer;
    std::unique_ptr<Mp4Dataset> mDataset;
    std::unique_ptr<Mp4Demuxer> mDemuxer;

    virtual void SetUp() override {
        std::string absDataPath = android::base::PathUtils::join(
                android::base::System::get()->getProgramDirectory(), "testdata",
                "video.mp4");

        mMockedVideoPlayer.reset(new MockedVideoPlayer());

        mDataset = Mp4Dataset::create(
                absDataPath, ::offworld::DatasetInfo::default_instance());
        mDemuxer = Mp4Demuxer::create(mMockedVideoPlayer.get(), mDataset.get(),
                                      nullptr);

        PacketQueue::init();

        mAudioQueue.reset(new PacketQueue(mMockedVideoPlayer.get()));
        mDemuxer->setAudioPacketQueue(mAudioQueue.get());

        mVideoQueue.reset(new PacketQueue(mMockedVideoPlayer.get()));
        mDemuxer->setVideoPacketQueue(mVideoQueue.get());

        mMockedVideoPlayer->start(PlayConfig());
        mAudioQueue->start();
        mVideoQueue->start();
    }

    virtual void TearDown() override {
        mMockedVideoPlayer->stop();
    }
};

TEST_F(Mp4DemuxerTest, demuxNextPacket) {
    AVPacket audioPkt;
    AVPacket videoPkt;

    // Check that the first and only packet in each queue is flush packet
    EXPECT_EQ(mAudioQueue->getNumPackets(), 1);
    EXPECT_EQ(mVideoQueue->getNumPackets(), 1);
    mVideoQueue->get(&videoPkt, true, nullptr);
    EXPECT_EQ(videoPkt.data, PacketQueue::sFlushPkt.data);
    mAudioQueue->get(&audioPkt, true, nullptr);
    EXPECT_EQ(audioPkt.data, PacketQueue::sFlushPkt.data);

    // Inspect the first video packet
    mDemuxer->demuxNextPacket();
    EXPECT_EQ(mVideoQueue->getNumPackets(), 1);
    EXPECT_FALSE(mVideoQueue->get(&videoPkt, true, nullptr) < 0);
    EXPECT_EQ(mVideoQueue->getNumPackets(), 0);
    EXPECT_EQ(videoPkt.stream_index, 0);
    EXPECT_EQ(videoPkt.pts, 0);

    // Inspect the second video packet
    mDemuxer->demuxNextPacket();
    EXPECT_EQ(mVideoQueue->getNumPackets(), 1);
    EXPECT_FALSE(mVideoQueue->get(&videoPkt, true, nullptr) < 0);
    EXPECT_EQ(mVideoQueue->getNumPackets(), 0);
    EXPECT_EQ(videoPkt.stream_index, 0);
    EXPECT_EQ(videoPkt.pts, 300);
}

TEST_F(Mp4DemuxerTest, seek) {
    AVPacket pkt;

    // Discard the first flush packet
    mVideoQueue->get(&pkt, true, nullptr);

    mDemuxer->demuxNextPacket();
    mDemuxer->demuxNextPacket();
    EXPECT_EQ(mVideoQueue->getNumPackets(), 2);
    mDemuxer->seek(0.5);

    // Seek should clear the packet queue and put a flush packet
    EXPECT_EQ(mVideoQueue->getNumPackets(), 1);
    mVideoQueue->get(&pkt, true, nullptr);
    EXPECT_EQ(pkt.data, PacketQueue::sFlushPkt.data);
}
