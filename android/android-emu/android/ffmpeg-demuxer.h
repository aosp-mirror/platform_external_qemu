// Copyright 2016 The Android Open Source Project
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
// ffmpeg-muxer.h
//
// a ffmpeg based muxer, mp4 container format, H264 video format and AAC audio
// format
//
// example use:
//
//    ffmpeg_decoder *decoder = ffmpeg_create_decoder("~/test.mp4");
//    ffmpeg_add_video_track(decoder, 1280, 720, 512*1024*1024, 30);
//    ffmpeg_add_audio_track(decoder, 64*1024, 48000);
//    ffmpeg_encode_video_frame(decoder, rgb, size);
//    ffmpeg_encode_audio_frame(decoder, audio, audio_buf_size);
//    ...
//    ffmpeg_delete_decoder(decoder);
//

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>
#include <stdint.h>

ANDROID_BEGIN_HEADER

typedef void (*FrameDecodedCallback)(void);

typedef struct ffmpeg_decoder ffmpeg_decoder;

// Create an instance of the ffmpeg decoder (mp4 container format)
// params:
//   path - the path to save the generated video file which should have .mp4
//   extension
// return:
//   opaque pointer to the decoder struct, which must be freed by calling
//   ffmpeg_delete_decoder()
//   NULL if failed
//
// this method is thread safe
ffmpeg_decoder* ffmpeg_create_decoder(bool wantAudio, const char *path, void (*cb)(void));

//
// Save the output file and delete the decoder, this method must be called.
// params:
//  decoder: the decoder pointer returned from ffmpeg_create_decoder()
//
// this method is thread safe
void ffmpeg_free_decoder(ffmpeg_decoder *decoder);

void ffmpeg_decode_next_frame(ffmpeg_decoder *decoder);

ANDROID_END_HEADER
