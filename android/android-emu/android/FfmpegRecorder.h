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
// audio format
//
// example use:
//
//    // Create a ffmpeg_recorder instance
//    FfmpegRecorder *recorder = FfmpegRecorder::create(fbWidth, fbHeight);
//
//    // Initialize the output context
//    recorder->initOutputContext(&recordingInfo);
//
//    // Add audio/video tracks
//    recorder->addVideoTrack(...);
//    recorder->addAudiotrack(...); // Optional
//
//    // Start the recording
//    recorder->start();
//    ...
//    // Encoding audio/video frames
//    recorder->encodeVideoFrame(...);
//    recorder->encodeAudioFrame(...);
//
//    ...
//    // Stop the recording
//    recorder->stop();
//

#pragma once

#include "android/screen-recorder-constants.h"
#include "android/screen-recorder.h"
#include "android/utils/compiler.h"

#include <stdbool.h>
#include <stdint.h>

namespace android {
namespace recording {

// Class to record audio and video from the emulator. This class is thread safe,
// so one can encode audio and video frames on separate threads.
class FfmpegRecorder {
public:
    // Initalizes the output context for the muxer. This call is required for
    // add video/audio contexts and starting the recording.
    virtual bool initOutputContext(const RecordingInfo* info) = 0;

    // Starts the recording and sets the start time of the recording to the time
    // of this call.
    virtual bool start() = 0;

    // Stops the recording.
    virtual bool stop() = 0;

    // Add an audio track from the specified format, and PCM format is assumed.
    // params:
    //   bitRate - the audio recording bit rate. The higher this number is, the
    //             better audio quality and the larger the output file size.
    //   sampleRate - The audio sampling rate (usually 48 kHz for system audio
    //                and 44.1 kHz for mic).
    // returns:
    //   true if the audio track was successfully added, false otherwise.
    virtual bool addAudioTrack(int bitRate, int sampleRate) = 0;

    // Add a video track from the specified format. A video track must be
    // supplied in order to start the recording.
    // params:
    //   width/height - The desired output video resolution. It can be different
    //                  than the framebuffer dimensions, in which case, there
    //                  will be a re-scaling step prior to encoding the video
    //                  frame.
    //   bitRate - The video recording bit rate. The higher this number is, the
    //             better the quality and the larger the output file size.
    //   fps - the frames per second to feed to the encoder.
    //   intraSpacing - the intra-frame (or key frame) spacing. The default is
    //                  12 frames.
    // returns:
    //   true if the video track was successfully added, false otherwise.
    virtual bool addVideoTrack(int width,
                               int height,
                               int bitRate,
                               int fps,
                               int intraSpacing = 12) = 0;

    // Encode and write an audio frame to the recorder.
    // params:
    //   buffer - the byte array for the audio buffer in PCM format.
    //   size - the audio buffer size.
    //   ptUs - the presentation timestamp (in microseconds) of the audio data.
    // returns:
    //   true if successfully encoded frame, false otherwise.
    virtual bool encodeAudioFrame(void* buffer, int size, uint64_t ptUs) = 0;

    // Encode and write a video frame to the recorder.
    // params:
    //   pixels - the byte array for the pixel buffer.
    //   size - the size of the pixel buffer.
    //   ptUs - the presentation timestamp (in microseconds) of the pixel
    //          buffer.
    //   pixFmt - the pixel format of the pixel buffer data.
    // returns:
    //   true if successfully encoded frame, false otherwise.
    virtual bool encodeVideoFrame(const uint8_t* pixels,
                                  int size,
                                  uint64_t ptUs,
                                  RecordPixFmt pixFmt) = 0;

    virtual ~FfmpegRecorder() {}

    // Convert an mp4 or webm video into animated gif file.
    // params:
    //   inFilename - the input video file in webm or mp4 format
    //   outFilename - the output animated gif file
    //   bitRate - bit rate for the gif file, usually smaller number
    //             to reduce the file size.
    // returns:
    //   true if successfully converted to animated gif, false otherwise.
    static bool convertToAnimatedGif(const char* inFilename,
                                     const char* outFilename,
                                     int bitRate);

    // Get the pixel size (in bytes) for pixel format |pixelFormat|.
    // params:
    //   pixelFormat - the pixel format
    // returns:
    //   the size (in bytes) of |pixelFormat|, or -1 if unknown format.
    static int getPixelSize(RecordPixFmt pixelFormat);

    // Creates a FfmpegRecorder instance.
    // Params:
    //   info - a struct containing recording information
    //   fb_width - the framebuffer width
    //   fb_height - the framebuffer height
    static FfmpegRecorder* create(int fbWidth, int fbHeight);

protected:
    FfmpegRecorder() {}
};

}  // namespace recording
}  // namespace android
