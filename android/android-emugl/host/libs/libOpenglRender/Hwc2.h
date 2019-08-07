/*
* Copyright (C) 2011-2015 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef _LIBRENDER_HWC2_H
#define _LIBRENDER_HWC2_H

#ifdef _MSC_VER
#include <stdint.h>
#endif

/* Copied from Android source */

// Should be identical to graphics-base-v1.0.h
typedef enum {
    HAL_TRANSFORM_FLIP_H = 1,   // (1 << 0)
    HAL_TRANSFORM_FLIP_V = 2,   // (1 << 1)
    HAL_TRANSFORM_ROT_90 = 4,   // (1 << 2)
    HAL_TRANSFORM_ROT_180 = 3,  // (FLIP_H | FLIP_V)
    HAL_TRANSFORM_ROT_270 = 7,  // ((FLIP_H | FLIP_V) | ROT_90)
} android_transform_t;

// Should be identical to hwcomposer_defs.h
typedef struct hwc_color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} hwc_color_t;
typedef struct hwc_frect {
    float left;
    float top;
    float right;
    float bottom;
} hwc_frect_t;
typedef struct hwc_rect {
    int left;
    int top;
    int right;
    int bottom;
} hwc_rect_t;

typedef enum {
    /* flip source image horizontally */
    HWC_TRANSFORM_FLIP_H = HAL_TRANSFORM_FLIP_H,
    /* flip source image vertically */
    HWC_TRANSFORM_FLIP_V = HAL_TRANSFORM_FLIP_V,
    /* rotate source image 90 degrees clock-wise */
    HWC_TRANSFORM_ROT_90 = HAL_TRANSFORM_ROT_90,
    /* rotate source image 180 degrees */
    HWC_TRANSFORM_ROT_180 = HAL_TRANSFORM_ROT_180,
    /* rotate source image 270 degrees clock-wise */
    HWC_TRANSFORM_ROT_270 = HAL_TRANSFORM_ROT_270,
    /* flip source image horizontally, the rotate 90 degrees clock-wise */
    HWC_TRANSFORM_FLIP_H_ROT_90 = HAL_TRANSFORM_FLIP_H | HAL_TRANSFORM_ROT_90,
    /* flip source image vertically, the rotate 90 degrees clock-wise */
    HWC_TRANSFORM_FLIP_V_ROT_90 = HAL_TRANSFORM_FLIP_V | HAL_TRANSFORM_ROT_90,
} hwc_transform_t;

// Should be identical to hwcomposer2.h
typedef enum {
    HWC2_COMPOSITION_INVALID = 0,
    HWC2_COMPOSITION_CLIENT = 1,
    HWC2_COMPOSITION_DEVICE = 2,
    HWC2_COMPOSITION_SOLID_COLOR = 3,
    HWC2_COMPOSITION_CURSOR = 4,
    HWC2_COMPOSITION_SIDEBAND = 5,
} hwc2_composition_t;
typedef enum {
    HWC2_BLEND_MODE_INVALID = 0,
    HWC2_BLEND_MODE_NONE = 1,
    HWC2_BLEND_MODE_PREMULTIPLIED = 2,
    HWC2_BLEND_MODE_COVERAGE = 3,
} hwc2_blend_mode_t;

// Should be identical to EmuHwc2.h
typedef struct compose_layer {
    uint32_t cbHandle;
    hwc2_composition_t composeMode;
    hwc_rect_t displayFrame;
    hwc_frect_t crop;
    int32_t blendMode;
    float alpha;
    hwc_color_t color;
    hwc_transform_t transform;
} ComposeLayer;
typedef struct compose_device {
    uint32_t version;
    uint32_t targetHandle;
    uint32_t numLayers;
    struct compose_layer layer[0];
} ComposeDevice;
typedef struct compose_device_v2 {
    uint32_t version;
    uint32_t displayId;
    uint32_t targetHandle;
    uint32_t numLayers;
    struct compose_layer layer[0];
} ComposeDevice_v2;

#endif
