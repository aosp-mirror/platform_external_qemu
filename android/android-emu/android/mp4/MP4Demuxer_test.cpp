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

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/threads/Thread.h"
#include "android/recording/video/player/VideoPlayer.h"
#include "android/recording/video/player/VideoPlayerWaitInfo.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

#include <gtest/gtest.h>

using namespace android::mp4;
using namespace android::videoplayer;

class MockedVideoPlayer : public VideoPlayer {
public:
    MockedVideoPlayer() = default;
    ~MockedVideoPlayer() = default;
    void start() { mRunning = true; }
    void start(const PlayConfig& playConfig = PlayConfig()) { mRunning = true; }
    void stop() { mRunning = false; }
    bool isRunning() const { return mRunning; }
    void videoRefresh() {}
    void scheduleRefresh(int delayMS) {}

private:
    bool mRunning = false;
    int mPktSerial = 0;
};

class Mp4DemuxerTest : public testing::Test {
protected:
    std::unique_ptr<Mp4Dataset> mDataset;
    std::unique_ptr<Mp4Demuxer> mDemuxer;
    std::unique_ptr<VideoPlayer> mMockedVideoPlayer;
    std::unique_ptr<PacketQueue> mAudioQueue;
    std::unique_ptr<PacketQueue> mVideoQueue;

    virtual void SetUp() override {
        std::string absDataPath = android::base::PathUtils::join(
                android::base::System::get()->getProgramDirectory(), "testdata",
                "video.mp4");

        mMockedVideoPlayer.reset(new MockedVideoPlayer());

        mDataset = Mp4Dataset::create(absDataPath, nullptr);
        mDemuxer = Mp4Demuxer::create(mMockedVideoPlayer.get(), mDataset.get(),
                                      nullptr);

        PacketQueue::init();

        mAudioQueue.reset(new PacketQueue(mMockedVideoPlayer.get()));
        mDemuxer->setAudioPacketQueue(mAudioQueue.get());

        mVideoQueue.reset(new PacketQueue(mMockedVideoPlayer.get()));
        mDemuxer->setVideoPacketQueue(mVideoQueue.get());

        mMockedVideoPlayer->start();
        mAudioQueue->start();
        mVideoQueue->start();
        mDemuxer->demux();
    }

    virtual void TearDown() override { mMockedVideoPlayer->stop(); }
};

TEST_F(Mp4DemuxerTest, demux) {
    AVPacket pkt1, pkt2;
    // Discard the first flush packet
    mVideoQueue->get(&pkt1, true, nullptr);
    // Inspect the first video packet
    EXPECT_FALSE(mVideoQueue->get(&pkt1, true, nullptr) < 0);
    EXPECT_EQ(pkt1.stream_index, 0);
    EXPECT_EQ(pkt1.pts, 0);
    // Inspect the second video packet
    EXPECT_FALSE(mVideoQueue->get(&pkt2, true, nullptr) < 0);
    EXPECT_EQ(pkt2.stream_index, 0);
    EXPECT_EQ(pkt2.pts, 300);
    // Check that audio packet queue only has one (flush) packet
    EXPECT_EQ(mAudioQueue->getNumPackets(), 1);
}
