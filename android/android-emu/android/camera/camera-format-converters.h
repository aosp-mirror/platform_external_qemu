/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

/*
 * Contains declaration of the API that allows converting frames from one
 * pixel format to another.
 *
 * For the emulator, we really need to convert into two formats: YV12, which is
 * used by the camera framework for video, and RGB32 for preview window.
 */

#include "camera-common.h"

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

/* Checks if conversion between two pixel formats is available.
 * Param:
 *  |from| - Pixel format to convert from.
 *  |to| - Pixel format to convert to.
 * Return:
 *  boolean: 1 if converter is available, or 0 if no conversion exists.
 */
extern int has_converter(uint32_t from, uint32_t to);

/* Calculates the framebuffer size for a given pixel format, width, and height.
 * Param:
 *  |pixel_format| - Pixel format.
 *  |width|, |height| - Framebuffer width and height.
 *  |frame_size| - Out parameter returning the frame size.
 * Return:
 *  True if the framebuffer size could be calculated.
 */
extern bool calculate_framebuffer_size(uint32_t pixel_format,
                                       int width,
                                       int height,
                                       size_t* frame_size);

/* Converts a frame into multiple framebuffers using a slow-but-robust method
 * that supports a larger list of formats.  If convert_frame is unable to
 * perform a conversion, it falls back to this function.
 *
 * Param:
 *  |src_frame| - Frame to convert.
 *  |pixel_format| - Defines pixel format for the converting framebuffer.
 *  |framebuffer_size|, |width|, |height| - Converting framebuffer byte size,
 *                                          width, and height.
 *  |framebuffers| - Array of framebuffers where to convert the frame. Size
 *                   of this array is defined by the 'fbs_num' parameter.
 *                   Note that the caller must make sure that buffers are
 *                   large enough to contain entire frame captured from the
 *                   device.
 *  |fbs_num| - Number of entries in the 'framebuffers' array.
 *  |r_scale|, |g_scale|, |_scale| - White balance scale.
 *  |exp_comp| - Exposure compensation.
 * Return:
 *  0 on success, or non-zero value on failure.
*/
extern int convert_frame_slow(const void* src_frame,
                              uint32_t pixel_format,
                              size_t framebuffer_size,
                              int width,
                              int height,
                              ClientFrameBuffer* framebuffers,
                              int fbs_num,
                              float r_scale,
                              float g_scale,
                              float b_scale,
                              float exp_comp);

/* Converts a frame into multiple framebuffers using the fast path. If the frame
 * cannot be converted with the fast path, this function may return failure.
 *
 * If different resolutions are specified, a resize may be performed.
 *
 * Param:
 *  |src_frame| - Frame to convert.
 *  |pixel_format| - Defines pixel format for the converting framebuffer.
 *  |src_framebuffer_size|, |src_width|, |src_height| - Source framebuffer byte
 *                                                      size,  width, and
 *                                                      height.
 *  |result_frame| - ClientFrame struct containing an array of framebuffers
 *                   where to convert the frame.
 *  |exp_comp| - Exposure compensation.
 * Return:
 *  0 on success, or non-zero value on failure.
 */
extern int convert_frame_fast(const void* src_frame,
                              uint32_t pixel_format,
                              size_t src_framebuffer_size,
                              int src_width,
                              int src_height,
                              ClientFrame* result_frame,
                              float exp_comp,
                              int rotation);

/* Converts a frame into multiple framebuffers.
 *
 * When camera service replies to a framebuffer request from the client, it
 * ussualy sends two framebuffers in the reply: one for video, and another for
 * preview window. Since these two framebuffers have different pixel formats
 * (most of the time), we need to do two conversions for each frame received
 * from the camera. This is the main intention behind this routine: to have a
 * one call that produces as many conversions as needed.
 *
 * Param:
 *  |src_frame| - Frame to convert.
 *  |pixel_format| - Defines pixel format for the converting framebuffer.
 *  |framebuffer_size|, |width|, |height| - Converting framebuffer byte size,
 *                                          width, and height.
 *  |result_frame| - ClientFrame struct containing an array of framebuffers
 *                   where to convert the frame.
 *  |r_scale|, |g_scale|, |_scale| - White balance scale.
 *  |exp_comp| - Exposure compensation.
 *  |direction| - "front" or "back", camera direction.
 *  |orientation| - Rotation of the device, see get_coarse_orientation();
 * Return:
 *  0 on success, or non-zero value on failure.
 */
extern int convert_frame(const void* src_frame,
                         uint32_t pixel_format,
                         size_t framebuffer_size,
                         int width,
                         int height,
                         ClientFrame* result_frame,
                         float r_scale,
                         float g_scale,
                         float b_scale,
                         float exp_comp,
                         const char* direction,
                         int orientation);

ANDROID_END_HEADER
