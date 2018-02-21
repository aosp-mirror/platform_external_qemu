// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

///
// FfmpegRecorder.h
//
// a ffmpeg based muxer, webm container format, VP9 video format and VORBIS
// audio format. This class is a one-time use object. You must create a new
// recorder for each new recording.
//
// example use:
//
//    // Create a ffmpeg_recorder instance
//    FfmpegRecorder *recorder = FfmpegRecorder::create(fbWidth, fbHeight,
//    filename);
//
//    // Add audio/video tracks
//    recorder->addVideoTrack(...);
//    recorder->addAudiotrack(...); // Optional
//
//    // Attach audio/video producers to the recorder
//    recorder->attachVideoProducer(new VideoProducer(...));
//    recorder->attachAudioProducer(new AudioProducer(...));
//
//    // Start the recording
//    recorder->start();
//
//    // Stop the recording
//    recorder->stop();
//

#pragma once

#include "android/base/StringView.h"
#include "android/recording/Frame.h"
#include "android/recording/Producer.h"
#include "android/recording/screen-recorder-constants.h"
#include "android/recording/screen-recorder.h"
#include "android/utils/compiler.h"

#include <stdbool.h>
#include <stdint.h>

namespace android {
namespace recording {

// Class to record audio and video from the emulator. This class is thread safe,
// so one can encode audio and video frames on separate threads.
class FfmpegRecorder {
public:
    // Returns whether the recorder is valid and can be used. The recorder is
    // valid if, in the constructor, the output context was created successfully
    // and the recording has either not started or is in progress. If the
    // recorder is invalid, then either the output context failed to initialize
    // or the recording has been stopped.
    virtual bool isValid() = 0;

    // Starts the recording.
    virtual bool start() = 0;

    // Stops the recording.
    virtual bool stop() = 0;

    // Add an audio track from the specified format, and PCM format is assumed.
    // params:
    //   bitRate - the audio recording bit rate (bps). The higher this number
    //   is, the
    //             better audio quality and the larger the output file size.
    //   sampleRate - The audio sampling rate (usually 48 kHz for system audio
    //                and 44.1 kHz for mic).
    // returns:
    //   true if the audio track was successfully added, false otherwise.
    virtual bool addAudioTrack(uint32_t bitRate, uint32_t sampleRate) = 0;

    // Add a video track from the specified format. A video track must be
    // supplied in order to start the recording.
    // params:
    //   width/height - The desired output video resolution. It can be different
    //                  than the framebuffer dimensions, in which case, there
    //                  will be a re-scaling step prior to encoding the video
    //                  frame.
    //   bitRate - The video recording bit rate (bps). The higher this number
    //   is, the
    //             better the quality and the larger the output file size.
    //   fps - the frames per second to feed to the encoder.
    //   intraSpacing - the intra-frame (or key frame) spacing. The default is
    //                  12 frames.
    // returns:
    //   true if the video track was successfully added, false otherwise.
    virtual bool addVideoTrack(uint16_t width,
                               uint16_t height,
                               uint32_t bitRate,
                               uint32_t fps,
                               int intraSpacing = 12) = 0;

    // Give the recorder an audio producer. The recorder will take ownership of
    // the producer.
    virtual void attachAudioProducer(Producer* producer) = 0;
    // Give the recorder a video producer. The recorder will take ownership of
    // the producer.
    virtual void attachVideoProducer(Producer* producer) = 0;

    virtual ~FfmpegRecorder() {}

    // Creates a FfmpegRecorder instance.
    // Params:
    //   info - a struct containing recording information
    //   fb_width - the framebuffer width
    //   fb_height - the framebuffer height
    //   filename - the output filename
    //
    // returns:
    //   null if unable to create the recorder.
    static FfmpegRecorder* create(uint16_t fbWidth,
                                  uint16_t fbHeight,
                                  android::base::StringView filename);

protected:
    FfmpegRecorder() {}
};

}  // namespace recording
}  // namespace android
