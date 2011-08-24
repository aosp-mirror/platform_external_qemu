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

#ifndef ANDROID_CAMERA_CAMERA_COMMON_H_
#define ANDROID_CAMERA_CAMERA_COMMON_H_

/*
 * Contains declarations of platform-independent the stuff that is used in
 * camera emulation.
 */

#ifdef _WIN32
/* Include declarations that are missing in Windows SDK headers. */
#include "android/camera/camera-win.h"
#endif  /* _WIN32 */

/* Describes a connected camera device.
 * This is a pratform-independent camera device descriptor that is used in
 * the camera API. This descriptor also contains some essential camera
 * properties, so the client of this API can use them to properly prepare to
 * handle frame capturing.
 */
typedef struct CameraDevice {
    /* Opaque pointer used by the camera capturing API. */
    void*       opaque;
    /* Frame's width in number of pixels. */
    int         width;
    /* Frame's height in number of pixels. */
    int         height;
    /* Number of bytes encoding each pixel. */
    uint32_t    bpp;
    /* Number of bytes encoding each line. */
    uint32_t    bpl;
    /* Pixel format of the frame captured from the camera device. */
    uint32_t    pixel_format;
    /* Total size in bytes of the framebuffer. */
    size_t      framebuffer_size;
} CameraDevice;

#endif  /* ANDROID_CAMERA_CAMERA_COMMON_H_ */
