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
//    auto recorder = FfmpegRecorder::create(fbWidth, fbHeight, filename,
//    containerFormat);
//
//    // Add audio/video tracks
//    recorder->addVideoTrack(std::move(videoProducer), myVideoCodec);
//    // audio is optional
//    recorder->addAudioTrack(std::move(audioProducer), myAudioCodec);
//
//    // Start the recording
//    recorder->start();
//
//    // Stop the recording
//    recorder->stop();
//
// See android/recording/screen-recorder.cpp for an example.

#pragma once

#include "android/base/StringView.h"   // for StringView
#include <stdint.h>                    // for uint16_t
#include <memory>                      // for unique_ptr

namespace android {
namespace recording {
class Producer;
template <class T> class Codec;
}  // namespace recording
}  // namespace android

extern "C" {
struct SwrContext;
struct SwsContext;
}

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
    // or the recording has been stopped. Once the recorder has become invalid,
    // it is no longer usable.
    virtual bool isValid() = 0;

    // Starts the recording. Must, at the minimum, have a video track added
    // before starting.
    virtual bool start() = 0;

    // Stops the recording. Returns true iff recording has started and there was
    // at least one video frame encoded, otherwise returns false. Note: if a
    // recording has started and then stop() is called, the recorder will be set
    // to an invalid state.
    virtual bool stop() = 0;

    // Add an audio track.
    // params:
    //   producer - The audio producer. The recorder will take ownership of the
    //              producer.
    //   codec - The codec helper used to create the audio codec and
    //                 resampling contexts.
    // returns:
    //   true if the audio track was successfully added, false otherwise.
    virtual bool addAudioTrack(std::unique_ptr<Producer> producer,
                               const Codec<SwrContext>* codec) = 0;

    // Add a video track. A video track must be
    // supplied in order to start the recording.
    // params:
    //   producer - The video producer. The recorder will take ownership of the
    //              producer.
    //   codec - The codec helper used to create the video codec and
    //                 rescaling contexts.
    // returns:
    //   true if the video track was successfully added, false otherwise.
    virtual bool addVideoTrack(std::unique_ptr<Producer> producer,
                               const Codec<SwsContext*>* codec) = 0;

    virtual ~FfmpegRecorder() {}

    // Creates a FfmpegRecorder instance.
    // Params:
    //   fb_width - the framebuffer width (must be > 0)
    //   fb_height - the framebuffer height (must be > 0)
    //   filename - the output filename
    //   containerFormat - the output container format. This will be used to
    //   determine which container to use for the output format, and not the
    //   filename.
    //
    // returns:
    //   null if unable to create the recorder.
    static std::unique_ptr<FfmpegRecorder> create(
            uint16_t fbWidth,
            uint16_t fbHeight,
            android::base::StringView filename,
            android::base::StringView containerFormat);

protected:
    FfmpegRecorder() = default;
};

}  // namespace recording
}  // namespace android
