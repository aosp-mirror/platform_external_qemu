// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/recording/FfmpegRecorder.h"

#include <gtest/gtest.h>                                  // for AssertionRe...
#include <atomic>                                         // for atomic
#include <functional>                                     // for __base
#include <iostream>                                       // for operator<<
#include <string>                                         // for string, ope...
#include <utility>                                        // for move

#include "android/base/StringView.h"                      // for StringView
#include "android/base/memory/ScopedPtr.h"                // for FuncDelete
#include "android/base/system/System.h"                   // for System, Sys...
#include "android/base/testing/TestSystem.h"              // for TestSystem
#include "android/base/testing/TestTempDir.h"             // for TestTempDir
#include "android/base/threads/Thread.h"                  // for Thread
#include "android/recording/AVScopedPtr.h"                // for makeAVScope...
#include "android/recording/Frame.h"                      // for toAVPixelFo...
#include "android/recording/GifConverter.h"               // for GifConverter
#include "android/recording/Producer.h"                   // for Producer
#include "android/recording/codecs/Codec.h"               // for CodecParams
#include "android/recording/codecs/audio/VorbisCodec.h"   // for VorbisCodec
#include "android/recording/codecs/video/VP9Codec.h"      // for VP9Codec
#include "android/recording/screen-recorder-constants.h"  // for kContainerF...
#include "android/recording/test/DummyAudioProducer.h"    // for createDummy...
#include "android/recording/test/DummyVideoProducer.h"    // for createDummy...

extern "C" {
#include <libavcodec/avcodec.h>                           // for AVCodecContext
#include <libavformat/avformat.h>                         // for AVFormatCon...
#include <libavutil/avutil.h>                             // for AVMEDIA_TYP...
}

using android::base::c_str;
using android::base::StringView;
using android::base::System;
using android::base::TestSystem;
using android::base::TestTempDir;
using android::base::Thread;
using namespace android::recording;

static constexpr uint32_t kFbWidth = 1080;
static constexpr uint32_t kFbHeight = 1920;
// Keep the recordings short or the unit-tests will take too long
static constexpr uint8_t kDurationSecs = 3;

static void checkMediaFile(StringView file,
                           uint8_t expDuration,
                           uint32_t expWidth,
                           uint32_t expHeight,
                           AVCodecID expVideoId,
                           AVCodecID expAudioId) {
    AVFormatContext* fmtCtx = nullptr;

    EXPECT_TRUE(avformat_open_input(&fmtCtx, c_str(file), nullptr, nullptr) ==
                0);
    AVScopedPtr<AVFormatContext> pFmtCtx = makeAVScopedPtr(fmtCtx);

    // Code from av_dump_format to calulate the duration
    int64_t duration = pFmtCtx->duration +
                       (pFmtCtx->duration <= INT64_MAX - 5000 ? 5000 : 0);
    duration /= AV_TIME_BASE;
    EXPECT_GE(duration, expDuration);

    for (int i = 0; i < pFmtCtx->nb_streams; ++i) {
        auto codec_type = pFmtCtx->streams[i]->codec->codec_type;
        if (codec_type == AVMEDIA_TYPE_VIDEO) {
            EXPECT_EQ(pFmtCtx->streams[i]->codec->codec_id, expVideoId);
            EXPECT_EQ(pFmtCtx->streams[i]->codec->width, expWidth);
            EXPECT_EQ(pFmtCtx->streams[i]->codec->height, expHeight);
        } else if (codec_type == AVMEDIA_TYPE_AUDIO) {
            EXPECT_EQ(pFmtCtx->streams[i]->codec->codec_id, expAudioId);
        }
    }
}

// Test recording with parameters and codec configurations that the emulator
// is using. Returns output file on success.
static std::string setupRecordingTest(VideoFormat videoFmt,
                                      AudioFormat audioFmt,
                                      uint32_t outputWidth,
                                      uint32_t outputHeight,
                                      StringView outputFile) {
    auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, outputFile,
                                           kContainerFormat);
    EXPECT_TRUE(recorder->isValid());

    // Indicators for when the producers are finished so we can stop the
    // recording.
    std::atomic<bool> audioFinished{false};
    std::atomic<bool> videoFinished{false};

    // Add dummy video track
    auto videoProducer = android::recording::createDummyVideoProducer(
            kFbWidth, kFbHeight, kFPS, kDurationSecs, videoFmt,
            [&videoFinished]() { videoFinished = true; });
    // Fill in the codec params for the audio and video
    CodecParams videoParams;
    videoParams.width = outputWidth;
    videoParams.height = outputHeight;
    videoParams.bitrate = kDefaultVideoBitrate;
    videoParams.fps = kFPS;
    videoParams.intra_spacing = kIntraSpacing;
    VP9Codec videoCodec(
            std::move(videoParams), kFbWidth, kFbHeight,
            toAVPixelFormat(videoProducer->getFormat().videoFormat));
    EXPECT_TRUE(recorder->addVideoTrack(std::move(videoProducer),
                                       &videoCodec));

    // Add the audio track
    auto audioProducer = android::recording::createDummyAudioProducer(
            kAudioSampleRate, kSrcNumSamples, kNumAudioChannels, kDurationSecs,
            audioFmt, [&audioFinished]() { audioFinished = true; });
    CodecParams audioParams;
    audioParams.sample_rate = kAudioSampleRate;
    audioParams.bitrate = kAudioBitrate;
    VorbisCodec audioCodec(
            std::move(audioParams),
            toAVSampleFormat(audioProducer->getFormat().audioFormat));
    EXPECT_TRUE(recorder->addAudioTrack(std::move(audioProducer),
                                       &audioCodec));

    // Start the recording
    EXPECT_TRUE(recorder->start());

    // Wait for a couple of seconds then start polling to see if the producers
    // are done
    Thread::sleepMs(2 * 1000);
    while (!audioFinished || !videoFinished) {
        std::cout << "Audio/video producer still not finished. Waiting "
                     "additional 1s.\n";
        Thread::sleepMs(1000);
    }

    EXPECT_TRUE(recorder->stop());
    // You should not be able to reuse the recorder.
    EXPECT_FALSE(recorder->isValid());

    checkMediaFile(outputFile, kDurationSecs, outputWidth, outputHeight,
                   videoCodec.getCodecId(), audioCodec.getCodecId());

    return outputFile;
}

TEST(FfmpegRecorder, RecorderCreation) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();

    // Bad width
    EXPECT_DEATH(
            {
                auto recorder = FfmpegRecorder::create(
                        0, kFbHeight, dir->makeSubPath("work.yes"),
                        kContainerFormat);
            },
            "");

    // Bad height
    EXPECT_DEATH(
            {
                auto recorder = FfmpegRecorder::create(
                        kFbWidth, 0, dir->makeSubPath("work.yes"),
                        kContainerFormat);
            },
            "");

    // Bad output path
    auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight,
                                           "/no/such/directory/unittest.webm",
                                           kContainerFormat);
    EXPECT_FALSE(recorder->isValid());

    // Bad container format
    recorder = FfmpegRecorder::create(kFbWidth, kFbHeight,
                                      dir->makeSubPath("work.yes"),
                                      "NotAValidFormat");
    EXPECT_FALSE(recorder->isValid());

    // All valid parameters
    recorder = FfmpegRecorder::create(kFbWidth, kFbHeight,
                                      dir->makeSubPath("work.yes"),
                                      kContainerFormat);
    EXPECT_TRUE(recorder->isValid());
}

TEST(FfmpegRecorder, UsingInvalidRecorder) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");

    // All public API calls should fail an assertion with an invalid recorder
    // (except for isValid()).
    {
        // Invalid recorder
        auto recorder = FfmpegRecorder::create(
                kFbWidth, kFbHeight, "/no/such/directory/unittest.webm",
                kContainerFormat);
        EXPECT_FALSE(recorder->isValid());
        EXPECT_DEATH({ recorder->start(); }, "");
    }

    {
        // Invalid recorder
        auto recorder = FfmpegRecorder::create(
                kFbWidth, kFbHeight, "/no/such/directory/unittest.webm",
                kContainerFormat);
        EXPECT_FALSE(recorder->isValid());
        EXPECT_DEATH({ recorder->stop(); }, "");
    }

    {
        // Invalid recorder
        auto recorder = FfmpegRecorder::create(
                kFbWidth, kFbHeight, "/no/such/directory/unittest.webm",
                kContainerFormat);
        EXPECT_FALSE(recorder->isValid());
        // Valid audio producer and audio codec
        auto audioProducer = android::recording::createDummyAudioProducer(
                kAudioSampleRate, kSrcNumSamples, kNumAudioChannels, 1,
                AudioFormat::AUD_FMT_S16, []() {});
        CodecParams audioParams;
        audioParams.sample_rate = kAudioSampleRate;
        audioParams.bitrate = kAudioBitrate;
        VorbisCodec audioCodec(
                std::move(audioParams),
                toAVSampleFormat(audioProducer->getFormat().audioFormat));
        EXPECT_DEATH(
                {
                    recorder->addAudioTrack(std::move(audioProducer),
                                            &audioCodec);
                },
                "");
    }

    {
        // Invalid recorder
        auto recorder = FfmpegRecorder::create(
                kFbWidth, kFbHeight, "/no/such/directory/unittest.webm",
                kContainerFormat);
        EXPECT_FALSE(recorder->isValid());
        // Valid video producer and video codec
        auto videoProducer = android::recording::createDummyVideoProducer(
                kFbWidth, kFbHeight, kFPS, 1, VideoFormat::RGBA8888, []() {});
        // Fill in the codec params for the audio and video
        CodecParams videoParams;
        videoParams.width = kFbWidth;
        videoParams.height = kFbHeight;
        videoParams.bitrate = kDefaultVideoBitrate;
        videoParams.fps = kFPS;
        videoParams.intra_spacing = kIntraSpacing;
        VP9Codec videoCodec(
                std::move(videoParams), kFbWidth, kFbHeight,
                toAVPixelFormat(videoProducer->getFormat().videoFormat));
        EXPECT_DEATH(
                {
                    recorder->addVideoTrack(std::move(videoProducer),
                                            &videoCodec);
                },
                "");
    }
}

TEST(FfmpegRecorder, AddAudioTrack) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    std::string outputFile = dir->makeSubPath("unittest.webm");

    {
        // Valid recorder
        auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, outputFile,
                                               kContainerFormat);
        EXPECT_TRUE(recorder->isValid());
        // Valid audio producer and null audio codec
        auto audioProducer = android::recording::createDummyAudioProducer(
                kAudioSampleRate, kSrcNumSamples, kNumAudioChannels, 1,
                AudioFormat::AUD_FMT_S16, []() {});
        EXPECT_DEATH(
                { recorder->addAudioTrack(std::move(audioProducer), nullptr); },
                "");
    }

    {
        // Valid recorder
        auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, outputFile,
                                               kContainerFormat);
        EXPECT_TRUE(recorder->isValid());
        // null audio producer and valid audio codec
        CodecParams audioParams;
        audioParams.sample_rate = kAudioSampleRate;
        audioParams.bitrate = kAudioBitrate;
        VorbisCodec audioCodec(std::move(audioParams),
                               toAVSampleFormat(AudioFormat::AUD_FMT_S16));
        EXPECT_DEATH({ recorder->addAudioTrack(nullptr, &audioCodec); }, "");
    }

    {
        // Valid recorder
        auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, outputFile,
                                               kContainerFormat);
        EXPECT_TRUE(recorder->isValid());
        // Valid audio producer and audio codec
        auto audioProducer = android::recording::createDummyAudioProducer(
                kAudioSampleRate, kSrcNumSamples, kNumAudioChannels, 1,
                AudioFormat::AUD_FMT_S16, []() {});
        CodecParams audioParams;
        audioParams.sample_rate = kAudioSampleRate;
        audioParams.bitrate = kAudioBitrate;
        VorbisCodec audioCodec(
                std::move(audioParams),
                toAVSampleFormat(audioProducer->getFormat().audioFormat));
        EXPECT_TRUE(
                recorder->addAudioTrack(std::move(audioProducer), &audioCodec));
    }
}

TEST(FfmpegRecorder, AddVideoTrack) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    std::string outputFile = dir->makeSubPath("unittest.webm");

    {
        // Valid recorder
        auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, outputFile,
                                               kContainerFormat);
        EXPECT_TRUE(recorder->isValid());

        // Valid video producer and null video codec
        auto videoProducer = android::recording::createDummyVideoProducer(
                kFbWidth, kFbHeight, kFPS, kDurationSecs, VideoFormat::RGBA8888,
                []() {});
        EXPECT_DEATH(
                { recorder->addVideoTrack(std::move(videoProducer), nullptr); },
                "");
    }

    {
        // Valid recorder
        auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, outputFile,
                                               kContainerFormat);
        EXPECT_TRUE(recorder->isValid());
        // null video producer and valid video codec
        CodecParams videoParams;
        videoParams.width = kFbWidth;
        videoParams.height = kFbHeight;
        videoParams.bitrate = kDefaultVideoBitrate;
        videoParams.fps = kFPS;
        videoParams.intra_spacing = kIntraSpacing;
        VP9Codec videoCodec(std::move(videoParams), kFbWidth, kFbHeight,
                            toAVPixelFormat(VideoFormat::RGBA8888));
        EXPECT_DEATH({ recorder->addVideoTrack(nullptr, &videoCodec); }, "");
    }

    {
        // Valid recorder
        auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, outputFile,
                                               kContainerFormat);
        EXPECT_TRUE(recorder->isValid());
        // Valid video producer and video codec
        auto videoProducer = android::recording::createDummyVideoProducer(
                kFbWidth, kFbHeight, kFPS, kDurationSecs, VideoFormat::RGBA8888,
                []() {});
        CodecParams videoParams;
        videoParams.width = kFbWidth;
        videoParams.height = kFbHeight;
        videoParams.bitrate = kDefaultVideoBitrate;
        videoParams.fps = kFPS;
        videoParams.intra_spacing = kIntraSpacing;
        VP9Codec videoCodec(
                std::move(videoParams), kFbWidth, kFbHeight,
                toAVPixelFormat(videoProducer->getFormat().videoFormat));
        EXPECT_TRUE(
                recorder->addVideoTrack(std::move(videoProducer), &videoCodec));
    }
}

TEST(FfmpegRecorder, Start) {
    // start() will only succeed if the recording hasn't been started yet and a
    // video track has been added.
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    std::string outputFile = dir->makeSubPath("unittest.webm");

    {
        // Valid recorder
        auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, outputFile,
                                               kContainerFormat);
        EXPECT_TRUE(recorder->isValid());

        EXPECT_DEATH({ recorder->start(); }, "");
    }

    {
        // Valid recorder
        auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, outputFile,
                                               kContainerFormat);
        EXPECT_TRUE(recorder->isValid());

        // Valid audio producer and audio codec
        auto audioProducer = android::recording::createDummyAudioProducer(
                kAudioSampleRate, kSrcNumSamples, kNumAudioChannels, 1,
                AudioFormat::AUD_FMT_S16, []() {});
        CodecParams audioParams;
        audioParams.sample_rate = kAudioSampleRate;
        audioParams.bitrate = kAudioBitrate;
        VorbisCodec audioCodec(
                std::move(audioParams),
                toAVSampleFormat(audioProducer->getFormat().audioFormat));
        EXPECT_TRUE(
                recorder->addAudioTrack(std::move(audioProducer), &audioCodec));
        EXPECT_DEATH({ recorder->start(); }, "");
    }

    {
        // Valid recorder
        auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, outputFile,
                                               kContainerFormat);
        EXPECT_TRUE(recorder->isValid());
        // Valid video producer and video codec
        auto videoProducer = android::recording::createDummyVideoProducer(
                kFbWidth, kFbHeight, kFPS, kDurationSecs, VideoFormat::RGBA8888,
                []() {});
        CodecParams videoParams;
        videoParams.width = kFbWidth;
        videoParams.height = kFbHeight;
        videoParams.bitrate = kDefaultVideoBitrate;
        videoParams.fps = kFPS;
        videoParams.intra_spacing = kIntraSpacing;
        VP9Codec videoCodec(
                std::move(videoParams), kFbWidth, kFbHeight,
                toAVPixelFormat(videoProducer->getFormat().videoFormat));
        EXPECT_TRUE(
                recorder->addVideoTrack(std::move(videoProducer), &videoCodec));
        EXPECT_TRUE(recorder->start());
    }

    {
        // Valid recorder
        auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, outputFile,
                                               kContainerFormat);
        EXPECT_TRUE(recorder->isValid());
        // Valid video producer and video codec
        auto videoProducer = android::recording::createDummyVideoProducer(
                kFbWidth, kFbHeight, kFPS, kDurationSecs, VideoFormat::RGBA8888,
                []() {});
        CodecParams videoParams;
        videoParams.width = kFbWidth;
        videoParams.height = kFbHeight;
        videoParams.bitrate = kDefaultVideoBitrate;
        videoParams.fps = kFPS;
        videoParams.intra_spacing = kIntraSpacing;
        VP9Codec videoCodec(
                std::move(videoParams), kFbWidth, kFbHeight,
                toAVPixelFormat(videoProducer->getFormat().videoFormat));
        EXPECT_TRUE(
                recorder->addVideoTrack(std::move(videoProducer), &videoCodec));

        // Valid audio producer and audio codec
        auto audioProducer = android::recording::createDummyAudioProducer(
                kAudioSampleRate, kSrcNumSamples, kNumAudioChannels, 1,
                AudioFormat::AUD_FMT_S16, []() {});
        CodecParams audioParams;
        audioParams.sample_rate = kAudioSampleRate;
        audioParams.bitrate = kAudioBitrate;
        VorbisCodec audioCodec(
                std::move(audioParams),
                toAVSampleFormat(audioProducer->getFormat().audioFormat));
        EXPECT_TRUE(
                recorder->addAudioTrack(std::move(audioProducer), &audioCodec));

        EXPECT_TRUE(recorder->start());
    }
}

// Class that doesn't send any video frames
class NoVideoFrameProducer : public android::recording::Producer {
public:
    NoVideoFrameProducer() = default;
    intptr_t main() final { return 0; }
    virtual void stop() override {}
};  // NoVideoFrameProducer

TEST(FfmpegRecorder, Stop) {
    // stop() will only succeed if the recording has been started.
    // If no video frames were encoded, then stop() will return false.
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    std::string outputFile = dir->makeSubPath("unittest.webm");

    // 1) Recording with only video track added, but no video frames encoded
    {
        // Valid recorder
        auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, outputFile,
                                               kContainerFormat);
        EXPECT_TRUE(recorder->isValid());
        EXPECT_FALSE(recorder->stop());
        // Valid video producer and video codec (no audio track)
        std::unique_ptr<Producer> videoProducer(new NoVideoFrameProducer());
        CodecParams videoParams;
        videoParams.width = kFbWidth;
        videoParams.height = kFbHeight;
        videoParams.bitrate = kDefaultVideoBitrate;
        videoParams.fps = kFPS;
        videoParams.intra_spacing = kIntraSpacing;
        VP9Codec videoCodec(
                std::move(videoParams), kFbWidth, kFbHeight,
                toAVPixelFormat(videoProducer->getFormat().videoFormat));
        EXPECT_TRUE(
                recorder->addVideoTrack(std::move(videoProducer), &videoCodec));
        EXPECT_FALSE(recorder->stop());
        EXPECT_TRUE(recorder->start());
        // NoVideoFrameProducer doesn't generate any video frames
        EXPECT_FALSE(recorder->stop());
        EXPECT_FALSE(recorder->isValid());
    }

    // 2) Recording with audio/video track added, but no video frames encoded
    {
        // Valid recorder
        auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, outputFile,
                                               kContainerFormat);
        EXPECT_TRUE(recorder->isValid());
        EXPECT_FALSE(recorder->stop());
        // Valid video producer and video codec
        std::unique_ptr<Producer> videoProducer(new NoVideoFrameProducer());
        CodecParams videoParams;
        videoParams.width = kFbWidth;
        videoParams.height = kFbHeight;
        videoParams.bitrate = kDefaultVideoBitrate;
        videoParams.fps = kFPS;
        videoParams.intra_spacing = kIntraSpacing;
        VP9Codec videoCodec(
                std::move(videoParams), kFbWidth, kFbHeight,
                toAVPixelFormat(videoProducer->getFormat().videoFormat));
        EXPECT_TRUE(
                recorder->addVideoTrack(std::move(videoProducer), &videoCodec));
        EXPECT_FALSE(recorder->stop());

        // Valid audio producer and audio codec
        auto audioProducer = android::recording::createDummyAudioProducer(
                kAudioSampleRate, kSrcNumSamples, kNumAudioChannels, 1,
                AudioFormat::AUD_FMT_S16, []() {});
        CodecParams audioParams;
        audioParams.sample_rate = kAudioSampleRate;
        audioParams.bitrate = kAudioBitrate;
        VorbisCodec audioCodec(
                std::move(audioParams),
                toAVSampleFormat(audioProducer->getFormat().audioFormat));
        EXPECT_TRUE(
                recorder->addAudioTrack(std::move(audioProducer), &audioCodec));
        EXPECT_FALSE(recorder->stop());

        EXPECT_TRUE(recorder->start());
        // NoVideoFrameProducer doesn't generate any video frames
        EXPECT_FALSE(recorder->stop());
        EXPECT_FALSE(recorder->isValid());
    }

    // 3) Recording with only video track added
    {
        // Valid recorder
        auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, outputFile,
                                               kContainerFormat);
        EXPECT_TRUE(recorder->isValid());
        EXPECT_FALSE(recorder->stop());
        // Valid video producer and video codec (no audio track)
        auto videoProducer = android::recording::createDummyVideoProducer(
                kFbWidth, kFbHeight, kFPS, kDurationSecs, VideoFormat::RGBA8888,
                []() {});
        CodecParams videoParams;
        videoParams.width = kFbWidth;
        videoParams.height = kFbHeight;
        videoParams.bitrate = kDefaultVideoBitrate;
        videoParams.fps = kFPS;
        videoParams.intra_spacing = kIntraSpacing;
        VP9Codec videoCodec(
                std::move(videoParams), kFbWidth, kFbHeight,
                toAVPixelFormat(videoProducer->getFormat().videoFormat));
        EXPECT_TRUE(
                recorder->addVideoTrack(std::move(videoProducer), &videoCodec));
        EXPECT_FALSE(recorder->stop());
        EXPECT_TRUE(recorder->start());
        // 500 ms should be a long enough time to get at least one frame
        Thread::sleepMs(500);
        EXPECT_TRUE(recorder->stop());
        EXPECT_FALSE(recorder->isValid());
    }

    // 4) Recording with audio/video track added
    {
        // Valid recorder
        auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, outputFile,
                                               kContainerFormat);
        EXPECT_TRUE(recorder->isValid());
        EXPECT_FALSE(recorder->stop());
        // Valid video producer and video codec (no audio track)
        auto videoProducer = android::recording::createDummyVideoProducer(
                kFbWidth, kFbHeight, kFPS, kDurationSecs, VideoFormat::RGBA8888,
                []() {});
        CodecParams videoParams;
        videoParams.width = kFbWidth;
        videoParams.height = kFbHeight;
        videoParams.bitrate = kDefaultVideoBitrate;
        videoParams.fps = kFPS;
        videoParams.intra_spacing = kIntraSpacing;
        VP9Codec videoCodec(
                std::move(videoParams), kFbWidth, kFbHeight,
                toAVPixelFormat(videoProducer->getFormat().videoFormat));
        EXPECT_TRUE(
                recorder->addVideoTrack(std::move(videoProducer), &videoCodec));
        EXPECT_FALSE(recorder->stop());

        // Valid audio producer and audio codec
        auto audioProducer = android::recording::createDummyAudioProducer(
                kAudioSampleRate, kSrcNumSamples, kNumAudioChannels, 1,
                AudioFormat::AUD_FMT_S16, []() {});
        CodecParams audioParams;
        audioParams.sample_rate = kAudioSampleRate;
        audioParams.bitrate = kAudioBitrate;
        VorbisCodec audioCodec(
                std::move(audioParams),
                toAVSampleFormat(audioProducer->getFormat().audioFormat));
        EXPECT_TRUE(
                recorder->addAudioTrack(std::move(audioProducer), &audioCodec));
        EXPECT_FALSE(recorder->stop());
        EXPECT_TRUE(recorder->start());
        // 500 ms should be a long enough time to get at least one frame
        Thread::sleepMs(500);
        EXPECT_TRUE(recorder->stop());
        EXPECT_FALSE(recorder->isValid());
    }
}

TEST(FfmpegRecorder, RecordRGBA8888AndAudFmtS16) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    std::string outputFile = dir->makeSubPath("unittest_rgba8888_auds16.webm");

    setupRecordingTest(VideoFormat::RGBA8888, AudioFormat::AUD_FMT_S16,
                       kFbWidth, kFbHeight, outputFile);
}

TEST(FfmpegRecorder, RecordBGRA8888AndAudFmtS16) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    std::string outputFile = dir->makeSubPath("unittest_bgra8888_auds16.webm");

    setupRecordingTest(VideoFormat::BGRA8888, AudioFormat::AUD_FMT_S16,
                       kFbWidth, kFbHeight, outputFile);
}

TEST(FfmpegRecorder, RecordRGB565AndAudFmtS16) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    std::string outputFile = dir->makeSubPath("unittest_rgb565_auds16.webm");

    setupRecordingTest(VideoFormat::RGB565, AudioFormat::AUD_FMT_S16, kFbWidth,
                       kFbHeight, outputFile);
}

TEST(FfmpegRecorder, RecordRGBA8888AndAudFmtU8) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    std::string outputFile = dir->makeSubPath("unittest_rgba8888_audu8.webm");

    setupRecordingTest(VideoFormat::RGBA8888, AudioFormat::AUD_FMT_U8, kFbWidth,
                       kFbHeight, outputFile);
}

TEST(FfmpegRecorder, RecordBGRA8888AndAudFmtU8) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    std::string outputFile = dir->makeSubPath("unittest_bgra8888_audu8.webm");

    setupRecordingTest(VideoFormat::BGRA8888, AudioFormat::AUD_FMT_U8, kFbWidth,
                       kFbHeight, outputFile);
}

TEST(FfmpegRecorder, RecordRGB565AndAudFmtU8) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    std::string outputFile = dir->makeSubPath("unittest_rgb565_audu8.webm");

    setupRecordingTest(VideoFormat::RGB565, AudioFormat::AUD_FMT_U8, kFbWidth,
                       kFbHeight, outputFile);
}

TEST(FfmpegRecorder, scaledRecording) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    std::string outputFile =
            dir->makeSubPath("unittest_rgba8888_auds16_480_800.webm");

    setupRecordingTest(VideoFormat::RGBA8888, AudioFormat::AUD_FMT_S16, 480,
                       800, outputFile);
}

TEST(GifConverter, ConvertWebmToGif) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    std::string outputFile = dir->makeSubPath("unittest_rgba8888_auds16.webm");

    auto file =
            setupRecordingTest(VideoFormat::RGBA8888, AudioFormat::AUD_FMT_S16,
                               kFbWidth, kFbHeight, outputFile);
    auto outputGif = file.substr(0, file.size() - 4) + "gif";

    EXPECT_FALSE(GifConverter::toAnimatedGif("/does/not/exist/test.webm",
                                             outputGif, 64 * 1024));

    EXPECT_FALSE(GifConverter::toAnimatedGif(file, "/does/not/exist/test.gif",
                                             64 * 1024));

    EXPECT_TRUE(GifConverter::toAnimatedGif(file, outputGif, 64 * 1024));
}
