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

#ifndef ANDROID_CAMERA_CAMERA_CAPTURE_H
#define ANDROID_CAMERA_CAMERA_CAPTURE_H

/*
 * Contains declarations for video capturing API that is used by the camera
 * emulator.
 */

#include "camera-common.h"

/* Initializes camera device descriptor, and connects to the camera device.
 * Param:
 *  name - On Linux contains name of the device to be used to capture video.
 *    On Windows contains name to assign to the capturing window. This parameter
 *    can be NULL, in which case '/dev/video0' will be used as device name on
 *    Linux, or 'AndroidEmulatorVC' on Windows.
 *  inp_channel - On Linux defines input channel to use when communicating with
 *    the camera driver. On Windows contains an index (up to 10) of the driver
 *    to use to communicate with the camera device.
 *  pixel_format - Defines pixel format in which the client of the camera API
 *    expects the frames. Note that is this format doesn't match pixel formats
 *    supported by the camera device, the camera API will provide a conversion.
 *    If such conversion is not available, this routine will fail.
 * Return:
 *  Initialized camera device descriptor on success, or NULL on failure.
 */
extern CameraDevice* camera_device_open(const char* name,
                                        int inp_channel,
                                        uint32_t pixel_format);

/* Starts capturing frames from the camera device.
 * Param:
 *  cd - Camera descriptor representing a camera device opened in
 *    camera_device_open routine.
 * Return:
 *  0 on success, or non-zero value on failure.
 */
extern int camera_device_start_capturing(CameraDevice* cd);

/* Stops capturing frames from the camera device.
 * Param:
 *  cd - Camera descriptor representing a camera device opened in
 *    camera_device_open routine.
 * Return:
 *  0 on success, or non-zero value on failure.
 */
extern int camera_device_stop_capturing(CameraDevice* cd);

/* Captures a frame from the camera device.
 * Param:
 *  cd - Camera descriptor representing a camera device opened in
 *    camera_device_open routine.
 *  buffer - Address of the buffer where to read the frame. Note that the buffer
 *    must be large enough to contain the entire frame. Also note that due to
 *    possible format conversion, required buffer size may differ from the
 *    framebuffer size as reported by framebuffer_size int the CameraDevice
 *    structure.
 * Return:
 *  0 on success, or non-zero value on failure.
 */
extern int camera_device_read_frame(CameraDevice* cd, uint8_t* buffer);

/* Closes camera device, opened in camera_device_open routine.
 * Param:
 *  cd - Camera descriptor representing a camera device opened in
 *    camera_device_open routine.
 */
extern void camera_device_close(CameraDevice* cd);

#endif  /* ANDROID_CAMERA_CAMERA_CAPTURE_H */
