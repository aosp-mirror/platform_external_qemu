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

#include "android/base/Compiler.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/threads/Thread.h"
#include "android/recording/AVScopedPtr.h"
#include "android/recording/GifConverter.h"
#include "android/recording/codecs/audio/VorbisCodec.h"
#include "android/recording/codecs/video/VP9Codec.h"
#include "android/recording/screen-recorder-constants.h"
#include "android/recording/test/DummyAudioProducer.h"
#include "android/recording/test/DummyVideoProducer.h"

#include <gtest/gtest.h>

#include <atomic>
#include <iostream>
#include <string>

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

    EXPECT_TRUE(avformat_open_input(&fmtCtx, file.c_str(), nullptr, nullptr) ==
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

// The death test is failing on PSQ, so need to figure out why. For now, disable
// it.
//TEST(FfmpegRecorder, Creation) {
//    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
//                      "/appdir");
//    TestTempDir* dir = system.getTempRoot();
//
//    // Bad output path
//    auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight,
//                                           "/no/such/directory/unittest.webm",
//                                           kContainerFormat);
//    EXPECT_FALSE(recorder->isValid());
//
//    // Empty filename
//    recorder =
//            FfmpegRecorder::create(kFbWidth, kFbHeight, "", kContainerFormat);
//    EXPECT_FALSE(recorder->isValid());
//
//    // No container format supplied
//    recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, "test.webm", "");
//    EXPECT_FALSE(recorder->isValid());
//
//    // Unknown container format
//    recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, "work.yet",
//                                      "badContainerFormat");
//    EXPECT_FALSE(recorder->isValid());
//
//    // All public API calls on invalid recorder should abort
//    {
//        auto audioProducer = android::recording::createDummyAudioProducer(
//                kAudioSampleRate, kSrcNumSamples, kNumAudioChannels, 1,
//                AudioFormat::AUD_FMT_S16, []() {});
//        CodecParams audioParams;
//        audioParams.sample_rate = kAudioSampleRate;
//        audioParams.bitrate = kAudioBitrate;
//        VorbisCodec audioCodec(
//                std::move(audioParams),
//                toAVSampleFormat(audioProducer->getFormat().audioFormat));
//
//        auto videoProducer = android::recording::createDummyVideoProducer(
//                kFbWidth, kFbHeight, kFPS, 1, VideoFormat::RGBA8888, []() {});
//        // Fill in the codec params for the audio and video
//        CodecParams videoParams;
//        videoParams.width = kFbWidth;
//        videoParams.height = kFbHeight;
//        videoParams.bitrate = kDefaultVideoBitrate;
//        videoParams.fps = kFPS;
//        videoParams.intra_spacing = kIntraSpacing;
//        VP9Codec videoCodec(
//                std::move(videoParams), kFbWidth, kFbHeight,
//                toAVPixelFormat(videoProducer->getFormat().videoFormat));
//
//        ASSERT_DEATH({ recorder->start(); }, "");
//        ASSERT_DEATH({ recorder->stop(); }, "");
//        ASSERT_DEATH(
//                {
//                    recorder->addAudioTrack(std::move(audioProducer),
//                                            &audioCodec);
//                },
//                "");
//        ASSERT_DEATH(
//                {
//                    recorder->addVideoTrack(std::move(videoProducer),
//                                            &videoCodec);
//                },
//                "");
//    }
//
//    // avformat_alloc_output_context2() uses kContainerFormat to determine
//    // container and not deduce from filename extension
//    recorder = FfmpegRecorder::create(kFbWidth, kFbHeight,
//                                      dir->makeSubPath("work.yes"),
//                                      kContainerFormat);
//    EXPECT_TRUE(recorder->isValid());
//
//    // Must have at least a video track or starting will abort
//    ASSERT_DEATH({ recorder->start(); }, "");
//
//    {
//        auto audioProducer = android::recording::createDummyAudioProducer(
//                kAudioSampleRate, kSrcNumSamples, kNumAudioChannels, 1,
//                AudioFormat::AUD_FMT_S16, []() {});
//        CodecParams audioParams;
//        audioParams.sample_rate = kAudioSampleRate;
//        audioParams.bitrate = kAudioBitrate;
//        VorbisCodec audioCodec(
//                std::move(audioParams),
//                toAVSampleFormat(audioProducer->getFormat().audioFormat));
//
//        // Add the audio track
//        EXPECT_TRUE(
//                recorder->addAudioTrack(std::move(audioProducer), &audioCodec));
//    }
//    ASSERT_DEATH({ recorder->start(); }, "");
//
//    {
//        auto videoProducer = android::recording::createDummyVideoProducer(
//                kFbWidth, kFbHeight, kFPS, 1, VideoFormat::RGBA8888, []() {});
//        // Fill in the codec params for the audio and video
//        CodecParams videoParams;
//        videoParams.width = kFbWidth;
//        videoParams.height = kFbHeight;
//        videoParams.bitrate = kDefaultVideoBitrate;
//        videoParams.fps = kFPS;
//        videoParams.intra_spacing = kIntraSpacing;
//        VP9Codec videoCodec(
//                std::move(videoParams), kFbWidth, kFbHeight,
//                toAVPixelFormat(videoProducer->getFormat().videoFormat));
//
//        // Add dummy video track
//        EXPECT_TRUE(
//                recorder->addVideoTrack(std::move(videoProducer), &videoCodec));
//    }
//    // These calls shouldn't abort anymore now that we have a video track.
//    recorder->start();
//    recorder->stop();
//
//    // After stopping, the recorder is in an invalid state
//    {
//        auto audioProducer = android::recording::createDummyAudioProducer(
//                kAudioSampleRate, kSrcNumSamples, kNumAudioChannels, 1,
//                AudioFormat::AUD_FMT_S16, []() {});
//        CodecParams audioParams;
//        audioParams.sample_rate = kAudioSampleRate;
//        audioParams.bitrate = kAudioBitrate;
//        VorbisCodec audioCodec(
//                std::move(audioParams),
//                toAVSampleFormat(audioProducer->getFormat().audioFormat));
//
//        auto videoProducer = android::recording::createDummyVideoProducer(
//                kFbWidth, kFbHeight, kFPS, 1, VideoFormat::RGBA8888, []() {});
//        // Fill in the codec params for the audio and video
//        CodecParams videoParams;
//        videoParams.width = kFbWidth;
//        videoParams.height = kFbHeight;
//        videoParams.bitrate = kDefaultVideoBitrate;
//        videoParams.fps = kFPS;
//        videoParams.intra_spacing = kIntraSpacing;
//        VP9Codec videoCodec(
//                std::move(videoParams), kFbWidth, kFbHeight,
//                toAVPixelFormat(videoProducer->getFormat().videoFormat));
//
//        ASSERT_DEATH({ recorder->start(); }, "");
//        ASSERT_DEATH({ recorder->stop(); }, "");
//        ASSERT_DEATH(
//                {
//                    recorder->addAudioTrack(std::move(audioProducer),
//                                            &audioCodec);
//                },
//                "");
//        ASSERT_DEATH(
//                {
//                    recorder->addVideoTrack(std::move(videoProducer),
//                                            &videoCodec);
//                },
//                "");
//    }
//}

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
