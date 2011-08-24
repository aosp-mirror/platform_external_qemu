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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#elif _DARWIN_C_SOURCE
#else
#include <linux/videodev2.h>
#endif
#include "android/camera/camera-format-converters.h"

/* Describes a convertor for one pixel format to another. */
typedef struct FormatConverterEntry {
    /* Pixel format to convert from. */
    uint32_t          from_format;
    /* Pixel format to convert to. */
    uint32_t          to_format;
    /* Address of the conversion routine. */
    FormatConverter   converter;
} FormatConverterEntry;

/* Converts frame from RGB24 (8 bits per color) to NV12 (YUV420)
 * Param:
 *  rgb - RGB frame to convert.
 *  width, height - Width, and height of the RGB frame.
 *  yuv - Buffer to receive the converted frame. Note that this buffer must
 *    be big enough to contain all the converted pixels!
 */
static void
_RGB8_to_YUV420(const uint8_t* rgb,
                int width,
                int height,
                uint8_t* yuv)
{
    const uint8_t* rgb_current = rgb;
    int x, y, yi = 0;
    const int num_pixels = width * height;
    int ui = num_pixels;
    int vi = num_pixels + num_pixels / 4;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            const uint32_t b = rgb_current[0];
            const uint32_t g = rgb_current[1];
            const uint32_t r = rgb_current[2];
            rgb_current += 3;
            yuv[yi++] = (uint8_t)((66*r + 129*g +  25*b + 128) >> 8) + 16;
            if((x % 2 ) == 0 && (y % 2) == 0) {
                yuv[ui++] = (uint8_t)((-38*r - 74*g + 112*b + 128) >> 8 ) + 128;
                yuv[vi++] = (uint8_t)((112*r - 94*g - 18*b + 128) >> 8 ) + 128;
            }
        }
    }
}

/* Converts frame from RGB24 (8 bits per color) to NV21 (YVU420)
 * Param:
 *  rgb - RGB frame to convert.
 *  width, height - Width, and height of the RGB frame.
 *  yuv - Buffer to receive the converted frame. Note that this buffer must
 *    be big enough to contain all the converted pixels!
 */
static void
_RGB8_to_YVU420(const uint8_t* rgb,
                int width,
                int height,
                uint8_t* yuv)
{
    const uint8_t* rgb_current = rgb;
    int x, y, yi = 0;
    const int num_pixels = width * height;
    int vi = num_pixels;
    int ui = num_pixels + num_pixels / 4;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
          const uint32_t b = rgb_current[0];
          const uint32_t g = rgb_current[1];
          const uint32_t r = rgb_current[2];
          rgb_current += 3;
          yuv[yi++] = (uint8_t)((66*r + 129*g +  25*b + 128) >> 8) + 16;
          if((x % 2 ) == 0 && (y % 2) == 0) {
              yuv[ui++] = (uint8_t)((-38*r - 74*g + 112*b + 128) >> 8 ) + 128;
              yuv[vi++] = (uint8_t)((112*r - 94*g - 18*b + 128) >> 8 ) + 128;
          }
        }
    }
}

/* Lists currently implemented converters. */
static const FormatConverterEntry _converters[] = {
    /* RGB24 -> NV12 */
    { V4L2_PIX_FMT_RGB24, V4L2_PIX_FMT_NV12, _RGB8_to_YUV420 },
    /* RGB24 -> YUV420 */
    { V4L2_PIX_FMT_RGB24, V4L2_PIX_FMT_YUV420, _RGB8_to_YUV420 },
    /* RGB24 -> NV21 */
    { V4L2_PIX_FMT_RGB24, V4L2_PIX_FMT_NV21, _RGB8_to_YVU420 },
};

FormatConverter
get_format_converted(uint32_t from, uint32_t to)
{
    const int num_converters = sizeof(_converters) / sizeof(*_converters);
    int n;
    for (n = 0; n < num_converters; n++) {
        if (_converters[n].from_format == from &&
            _converters[n].to_format == to) {
            return _converters[n].converter;
        }
    }

    return NULL;
}
