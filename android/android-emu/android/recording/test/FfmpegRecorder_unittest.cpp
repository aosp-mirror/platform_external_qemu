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
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/threads/Thread.h"
#include "android/recording/codecs/audio/VorbisCodec.h"
#include "android/recording/codecs/video/VP9Codec.h"
#include "android/recording/screen-recorder-constants.h"
#include "android/recording/test/DummyAudioProducer.h"
#include "android/recording/test/DummyVideoProducer.h"

#include <gtest/gtest.h>

#include <atomic>
#include <iostream>
#include <string>

using android::base::System;
using android::base::TestSystem;
using android::base::TestTempDir;
using android::base::Thread;
using android::recording::AudioFormat;
using android::recording::CodecParams;
using android::recording::VideoFormat;
using android::recording::VorbisCodec;
using android::recording::VP9Codec;
using android::recording::FfmpegRecorder;

static constexpr uint32_t kFbWidth = 1080;
static constexpr uint32_t kFbHeight = 1920;
// Keep the recordings short or the unit-tests will take too long
static constexpr uint8_t kDurationSecs = 5;
static constexpr AudioFormat kInSampleFormat = AudioFormat::AUD_FMT_S16;

// Test recording with parameters and codec configurations that the emulator
// is using.
TEST(FfmpegRecorder, BasicRecording) {
    TestSystem system("/progdir", System::kProgramBitness, "/homedir",
                      "/appdir");
    TestTempDir* dir = system.getTempRoot();
    std::string outputFile = dir->makeSubPath("output.webm");

//    auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, outputFile);
    auto recorder = FfmpegRecorder::create(kFbWidth, kFbHeight, "/usr/local/google/home/joshuaduong/Desktop/unittest.webm");
    EXPECT_TRUE(recorder->isValid());

    // Indicators for when the producers are finished so we can stop the
    // recording.
    std::atomic<bool> audioFinished{false};
    std::atomic<bool> videoFinished{false};

    // Add dummy video track
    // VideoFormat::RGBA8888 = host gpu mode
    // VideoFormat::BGRA8888 = guest gpu mode?
    // VideoFormat::RGB565 = guest gpu mode + no-window?
    auto videoProducer = android::recording::createDummyVideoProducer(
            kFbWidth, kFbHeight, kFPS, kDurationSecs, VideoFormat::RGBA8888,
            [&videoFinished]() {
                videoFinished = true;
            });
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
            kAudioSampleRate, kSrcNumSamples, kNumAudioChannels, kDurationSecs, kInSampleFormat,
            [&audioFinished]() {
                audioFinished = true;
            });
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

    // Wait for kDurationSecs then start polling to see if the producers are
    // done
    Thread::sleepMs(kDurationSecs * 1000);
    while (!audioFinished || !videoFinished) {
        std::cout << "Audio/video producer still not finished. Waiting additional 100 ms.\n";
        Thread::sleepMs(100);
    }

    EXPECT_TRUE(recorder->stop());
    // You should not be able to reuse the recorder.
    EXPECT_FALSE(recorder->isValid());
}
