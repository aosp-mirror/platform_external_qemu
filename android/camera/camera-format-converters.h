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

#ifndef ANDROID_CAMERA_CAMERA_FORMAT_CONVERTERS_H
#define ANDROID_CAMERA_CAMERA_FORMAT_CONVERTERS_H

/*
 * Contains declaration of the API that allows converting frames from one
 * pixel format to another.
 */

#include "camera-common.h"

/* Prototype of a routine that converts frame from one pixel format to another.
 * Param:
 *  from - Frame to convert.
 *  width, height - Width, and height of the frame to convert.
 *  to - Buffer to receive the converted frame. Note that this buffer must
 *    be big enough to contain all the converted pixels!
 */
typedef void (*FormatConverter)(const uint8_t* from,
                                int width,
                                int height,
                                uint8_t* to);


/* Gets an address of a routine that provides frame conversion for the
 * given pixel format.
 * Param:
 *  from - Pixel format to convert from.
 *  to - Pixel format to convert to.
 * Return:
 *  Address of an appropriate conversion routine, or NULL if no conversion
 *  routine exsits for the given pair of pixel formats.
 */
extern FormatConverter get_format_converted(uint32_t from, uint32_t to);

#endif  /* ANDROID_CAMERA_CAMERA_FORMAT_CONVERTERS_H */
