#include "android/mp4/MP4Demuxer.h"

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/threads/Thread.h"
#include "android/hw-sensors.h"
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
    void stop() { mRunning = false; }
    bool isRunning() const { return mRunning; }
    void videoRefresh() {}
    void scheduleRefresh(int delayMS) {}

private:
    bool mRunning;
    int mPktSerial = 0;
};

class Mp4DemuxerTest : public testing::Test {
protected:
    int mPktSerial = 0;
    VideoPlayerWaitInfo mWaitInfo;
    std::unique_ptr<Mp4Demuxer> mDemuxer;
    std::unique_ptr<VideoPlayer> mMockedVideoPlayer;
    std::unique_ptr<PacketQueue> mAudioQueue;
    std::unique_ptr<PacketQueue> mVideoQueue;

    virtual void SetUp() override {
        std::string absDataPath = android::base::PathUtils::join(
                android::base::System::get()->getProgramDirectory(), "testdata",
                "video.mp4");

        std::unique_ptr<Mp4Dataset> dataset = Mp4Dataset::create(absDataPath);
        mDemuxer = Mp4Demuxer::create(dataset.get(), &mWaitInfo);

        mMockedVideoPlayer.reset(new MockedVideoPlayer());

        PacketQueue::init();

        mAudioQueue.reset(new PacketQueue(mMockedVideoPlayer.get()));
        mDemuxer->setAudioPacketQueue(mAudioQueue.get());

        mVideoQueue.reset(new PacketQueue(mMockedVideoPlayer.get()));
        mDemuxer->setVideoPacketQueue(mVideoQueue.get());

        mMockedVideoPlayer->start();
        mAudioQueue->start();
        mVideoQueue->start();
        mDemuxer->start([] {});
        waitForDemuxFinished();
    }

    virtual void TearDown() override {
        mMockedVideoPlayer->stop();
        mDemuxer->stop();
    }

private:
    // The total number of video packets in the input file. This need to be
    // changed if the input file is swapped.
    static const int MAX_VIDEO_PACKET_COUNT = 47;
    static const int DEMUXER_WAIT_DURATION_MS = 10;

    // A helper function to wait for demuxing to finish
    void waitForDemuxFinished() {
        while (mVideoQueue->getNumPackets() < MAX_VIDEO_PACKET_COUNT) {
            if (mVideoQueue->getNumPackets() == 0) {
                mWaitInfo.lock.lock();
                mWaitInfo.done = true;
                mWaitInfo.cvDone.signalAndUnlock(&mWaitInfo.lock);
            }
            android::base::Thread::sleepMs(DEMUXER_WAIT_DURATION_MS);
        };
    }
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
