/*
 * Copyright (C) 2017 The Android Open Source Project
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
 * Contains declarations for virtual scene camera API that is used by the camera
 * emulator.
 */

#include "android/camera/camera-common.h"
#include "android/utils/compiler.h"

#include <stdint.h>

ANDROID_BEGIN_HEADER

// Parse command line options for the virtual scene.
void camera_virtualscene_parse_cmdline();

// Get the preferred pixel format for the virtual scene camera, which should be
// used for maximum performance. If the preferred format is not supported by the
// guest camera_virtualscene_read_frame will perform colorspace conversion.
//
// Returns a V4L2_PIX_FMT_* pixel format.
uint32_t camera_virtualscene_preferred_format();

// Initializes camera device descriptor, and connects to the camera device.
//
// Returns null on failure. On success, the CameraDevice must be closed with
// camera_virtualscene_close.
//
// |name| - The name of the device to open. Unused for virtualscene.
// |inp_channel| - Input channel for the webcam. Unused for virtualscene.
//
// Returns initialized camera device descriptor on success, or NULL on failure.
CameraDevice* camera_virtualscene_open(const char* name, int inp_channel);

// Starts capturing frames from the CameraDevice.
//
// This may be called on any thread, but calling this function will reset
// EGL rendering state, so callers must either not perform other rendering on
// the same thread or be resilient to having the EGL state reset.
//
// |cd| - Camera descriptor representing a camera device, must have been opened
//        with the camera_virtualscene_open API.
// |pixel_format| - Pixel format for the captured frames, preferably the format
//                  returned from camera_virtualscene_preferred_format.
// |width|, |height| - Frame dimensions for the captured video frame. Must match
//                     dimensions supported by the camera for the pixel format
//                     defined by the |pixel_format| parameter.
//
// Returns 0 on success, or non-zero on failure.
int camera_virtualscene_start_capturing(CameraDevice* cd,
                                        uint32_t pixel_format,
                                        int frame_width,
                                        int frame_height);

// Stops capturing frames from the camera device.
//
// This may be called on any thread, but calling this function will reset
// EGL rendering state, so callers must either not perform other rendering on
// the same thread or be resilient to having the EGL state reset.
//
// |cd| - Camera descriptor representing a camera device, must have been opened
//        with the camera_virtualscene_open API.
//
// Returns 0 on success, or non-zero on failure.
int camera_virtualscene_stop_capturing(CameraDevice* cd);

// Captures a frame from the camera device.
//
// This may be called on any thread, but calling this function will reset
// EGL rendering state, so callers must either not perform other rendering on
// the same thread or be resilient to having the EGL state reset.
//
// |cd| - Camera descriptor representing a camera device, must have been opened
//        with the camera_virtualscene_open API.
// |result_frame| - ClientFrame struct containing an array of framebuffers
//                  where to convert the frame.
// |r_scale|, |g_scale|, |b_scale| - White balance scale.
// |exp_comp| - Exposure compensation.
//
// Returns 0 on success, or non-zero on failure.
int camera_virtualscene_read_frame(CameraDevice* cd,
                                   ClientFrame* result_frame,
                                   float r_scale,
                                   float g_scale,
                                   float b_scale,
                                   float exp_comp);

// Closes the camera device, which was opened from the camera_virtualscene_open
// API.
//
// This may be called on any thread, but calling this function will reset
// EGL rendering state, so callers must either not perform other rendering on
// the same thread or be resilient to having the EGL state reset.
//
// |cd| - Camera descriptor representing a camera device, must have been opened
//        with the camera_virtualscene_open API.
void camera_virtualscene_close(CameraDevice* cd);

ANDROID_END_HEADER
