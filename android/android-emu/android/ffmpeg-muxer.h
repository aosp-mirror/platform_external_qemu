/* Copyright (C) 2006-2016 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

/***
 * ffmpeg-muxer.h
 *
 * a ffmpeg based muxer
 */

#ifndef _FFMPEG_H_
#define _FFMPEG_H_

#ifdef __cplusplus
extern "C" {
#endif

void * start_ffmpeg_recorder(const char *filename, int width, int height, int bitRate, int fps, int anable_audio);
void stop_ffmpeg_recorder(void *recorder);
int ffmpeg_encode_frame(void *recorder, const uint8_t *rgb_pixels, uint32_t size);
int ffmpeg_write_video_data(void *recorder, uint8_t* data, int length, int frameIndex, int is_key_frame, int64_t elapseMS);
int ffmpeg_encode_audio_buffer(void *recorder, uint8_t *buffer, int audioBufferLen);

#ifdef __cplusplus
}
#endif

#endif /* _FFMPEG_H_ */
