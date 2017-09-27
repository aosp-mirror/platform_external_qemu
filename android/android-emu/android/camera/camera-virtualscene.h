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
 * Contains declarations for video capturing API that is used by the camera
 * emulator.
 */

#include "camera-common.h"

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

/* Virtual Scene Camera API follows */

uint32_t camera_virtualscene_preferred_format();

/* Initializes camera device descriptor, and connects to the camera device.
 * Param:
 *  name - The name of the device to open. Unused for virtualscene.
 *  inp_channel - Input channel to use when communicating with webcam.
 *    Unused for virtualscene.
 * Return:
 *  Initialized camera device descriptor on success, or NULL on failure.
 */
CameraDevice* camera_virtualscene_open(const char* name, int inp_channel);

/* Starts capturing frames from the camera device.
 * Param:
 *  cd - Camera descriptor representing a camera device opened in
 *    camera_virtualscene_open routine.
 *  pixel_format - Defines pixel format for the captured frames. Must be one of
 *      the formats, supported by the camera device.
 *  width, height - Frame dimensions for the captured video frame. Must match
 *      dimensions supported by the camera for the pixel format defined by the
 *      'pixel_format' parameter.
 * Return:
 *  0 on success, or non-zero value on failure.
 */
int camera_virtualscene_start_capturing(CameraDevice* cd,
                                        uint32_t pixel_format,
                                        int frame_width,
                                        int frame_height);

/* Stops capturing frames from the camera device.
 * Param:
 *  cd - Camera descriptor representing a camera device opened in
 *    camera_virtualscene_open routine.
 * Return:
 *  0 on success, or non-zero value on failure.
 */
int camera_virtualscene_stop_capturing(CameraDevice* cd);

/* Captures a frame from the camera device.
 * Param:
 *  cd - Camera descriptor representing a camera device opened in
 *    camera_virtualscene_open routine.
 *  framebuffers - Array of framebuffers where to read the frame. Size of this
 *      array is defined by the 'fbs_num' parameter. Note that the caller must
 *      make sure that buffers are large enough to contain entire frame captured
 *      from the device.
 *  fbs_num - Number of entries in the 'framebuffers' array.
 *  r_scale, g_scale, b_scale - White balance scale.
 *  exp_comp - Exposure compensation.
 * Return:
 *  0 on success, or non-zero value on failure. There is a special vaule 1
 *  returned from this routine which indicates that frames were not available in
 *  the device. This value is returned on Linux implementation when frame ioctl
 *  has returned EAGAIN error. The client should respond to this value by
 *  repeating the read, rather than reporting an error.
 */
int camera_virtualscene_read_frame(CameraDevice* cd,
                                   ClientFrameBuffer* framebuffers,
                                   int fbs_num,
                                   float r_scale,
                                   float g_scale,
                                   float b_scale,
                                   float exp_comp);

/* Closes camera device, opened in camera_virtualscene_open routine.
 * Param:
 *  cd - Camera descriptor representing a camera device opened in
 *    camera_virtualscene_open routine.
 */
void camera_virtualscene_close(CameraDevice* cd);

ANDROID_END_HEADER
