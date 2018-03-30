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
static constexpr uint8_t kDurationSecs = 5;

static void checkMediaFile(StringView file,
                           uint8_t expDuration,
                           AVCodecID expVideoId,
                           AVCodecID expAudioId) {
    AVFormatContext* fmtCtx = nullptr;

    EXPECT_TRUE(avformat_open_input(&fmtCtx, file.c_str(), nullptr, nullptr) ==
                0);
    AVScopedPtr<AVFormatContext> pFmtCtx = makeAVScopedPtr(fmtCtx);

    EXPECT_GE(pFmtCtx->duration, expDuration);

    for (int i = 0; i < pFmtCtx->nb_streams; ++i) {
        auto codec_type = pFmtCtx->streams[i]->codec->codec_type;
        if (codec_type == AVMEDIA_TYPE_VIDEO) {
            EXPECT_EQ(pFmtCtx->streams[i]->codec->codec_id, expVideoId);
        } else if (codec_type == AVMEDIA_TYPE_AUDIO) {
            EXPECT_EQ(pFmtCtx->streams[i]->codec->codec_id, expAudioId);
        }
    }
}

// Test recording with parameters and codec configurations that the emulator
// is using.
static void setupRecordingTest(VideoFormat videoFmt,
                               AudioFormat audioFmt,
                               StringView filename) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    std::string outputFile = dir->makeSubPath(filename);

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
    // TODO: testing scaling too
    videoParams.width = kFbWidth;
    videoParams.height = kFbHeight;
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

    checkMediaFile(outputFile, kDurationSecs, videoCodec.getCodecId(),
                   audioCodec.getCodecId());
}

TEST(FfmpegRecorder, Creation) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();

    auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight,
                                           "/no/such/directory/unittest.webm",
                                           kContainerFormat);
    EXPECT_FALSE(recorder->isValid());

    recorder =
            FfmpegRecorder::create(kFbWidth, kFbHeight, "", kContainerFormat);
    EXPECT_FALSE(recorder->isValid());

    recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, "test.webm", "");
    EXPECT_FALSE(recorder->isValid());

    recorder = FfmpegRecorder::create(kFbWidth, kFbHeight,
                                      dir->makeSubPath("work.yes"),
                                      kContainerFormat);
    EXPECT_TRUE(recorder->isValid());

    recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, "work.yet",
                                      "badContainerFormat");
    EXPECT_FALSE(recorder->isValid());
}

TEST(FfmpegRecorder, RecordRGBA8888AndAudFmtS16) {
    setupRecordingTest(VideoFormat::RGBA8888, AudioFormat::AUD_FMT_S16,
                       "unittest_rgba8888_auds16.webm");
}

TEST(FfmpegRecorder, RecordBGRA8888AndAudFmtS16) {
    setupRecordingTest(VideoFormat::BGRA8888, AudioFormat::AUD_FMT_S16,
                       "unittest_bgra8888_auds16.webm");
}

TEST(FfmpegRecorder, RecordRGB565AndAudFmtS16) {
    setupRecordingTest(VideoFormat::RGB565, AudioFormat::AUD_FMT_S16,
                       "unittest_rgb565_auds16.webm");
}

TEST(FfmpegRecorder, RecordRGBA8888AndAudFmtU8) {
    setupRecordingTest(VideoFormat::RGBA8888, AudioFormat::AUD_FMT_U8,
                       "unittest_rgba8888_audu8.webm");
}

TEST(FfmpegRecorder, RecordBGRA8888AndAudFmtU8) {
    setupRecordingTest(VideoFormat::BGRA8888, AudioFormat::AUD_FMT_U8,
                       "unittest_bgra8888_audu8.webm");
}

TEST(FfmpegRecorder, RecordRGB565AndAudFmtU8) {
    setupRecordingTest(VideoFormat::RGB565, AudioFormat::AUD_FMT_U8,
                       "unittest_rgb565_audu8.webm");
}
