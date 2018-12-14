// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either expresso or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

typedef enum {
    HAL_PIXEL_FORMAT_RGBA_8888 = 1,
    HAL_PIXEL_FORMAT_RGBX_8888 = 2,
    HAL_PIXEL_FORMAT_RGB_888 = 3,
    HAL_PIXEL_FORMAT_RGB_565 = 4,
    HAL_PIXEL_FORMAT_BGRA_8888 = 5,
    HAL_PIXEL_FORMAT_RGBA_1010102 = 43, // 0x2B
    HAL_PIXEL_FORMAT_RGBA_FP16 = 22, // 0x16
    HAL_PIXEL_FORMAT_YV12 = 842094169, // 0x32315659
    HAL_PIXEL_FORMAT_Y8 = 538982489, // 0x20203859
    HAL_PIXEL_FORMAT_Y16 = 540422489, // 0x20363159
    HAL_PIXEL_FORMAT_RAW16 = 32, // 0x20
    HAL_PIXEL_FORMAT_RAW10 = 37, // 0x25
    HAL_PIXEL_FORMAT_RAW12 = 38, // 0x26
    HAL_PIXEL_FORMAT_RAW_OPAQUE = 36, // 0x24
    HAL_PIXEL_FORMAT_BLOB = 33, // 0x21
    HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED = 34, // 0x22
    HAL_PIXEL_FORMAT_YCBCR_420_888 = 35, // 0x23
    HAL_PIXEL_FORMAT_YCBCR_422_888 = 39, // 0x27
    HAL_PIXEL_FORMAT_YCBCR_444_888 = 40, // 0x28
    HAL_PIXEL_FORMAT_FLEX_RGB_888 = 41, // 0x29
    HAL_PIXEL_FORMAT_FLEX_RGBA_8888 = 42, // 0x2A
    HAL_PIXEL_FORMAT_YCBCR_422_SP = 16, // 0x10
    HAL_PIXEL_FORMAT_YCRCB_420_SP = 17, // 0x11
    HAL_PIXEL_FORMAT_YCBCR_422_I = 20, // 0x14
    HAL_PIXEL_FORMAT_JPEG = 256, // 0x100
} android_pixel_format_t;

enum {
    /* buffer is never read in software */
    GRALLOC_USAGE_SW_READ_NEVER         = 0x00000000U,
    /* buffer is rarely read in software */
    GRALLOC_USAGE_SW_READ_RARELY        = 0x00000002U,
    /* buffer is often read in software */
    GRALLOC_USAGE_SW_READ_OFTEN         = 0x00000003U,
    /* mask for the software read values */
    GRALLOC_USAGE_SW_READ_MASK          = 0x0000000FU,

    /* buffer is never written in software */
    GRALLOC_USAGE_SW_WRITE_NEVER        = 0x00000000U,
    /* buffer is rarely written in software */
    GRALLOC_USAGE_SW_WRITE_RARELY       = 0x00000020U,
    /* buffer is often written in software */
    GRALLOC_USAGE_SW_WRITE_OFTEN        = 0x00000030U,
    /* mask for the software write values */
    GRALLOC_USAGE_SW_WRITE_MASK         = 0x000000F0U,

    /* buffer will be used as an OpenGL ES texture */
    GRALLOC_USAGE_HW_TEXTURE            = 0x00000100U,
    /* buffer will be used as an OpenGL ES render target */
    GRALLOC_USAGE_HW_RENDER             = 0x00000200U,
    /* buffer will be used by the 2D hardware blitter */
    GRALLOC_USAGE_HW_2D                 = 0x00000400U,
    /* buffer will be used by the HWComposer HAL module */
    GRALLOC_USAGE_HW_COMPOSER           = 0x00000800U,
    /* buffer will be used with the framebuffer device */
    GRALLOC_USAGE_HW_FB                 = 0x00001000U,

    /* buffer should be displayed full-screen on an external display when
     * possible */
    GRALLOC_USAGE_EXTERNAL_DISP         = 0x00002000U,

    /* Must have a hardware-protected path to external display sink for
     * this buffer.  If a hardware-protected path is not available, then
     * either don't composite only this buffer (preferred) to the
     * external sink, or (less desirable) do not route the entire
     * composition to the external sink.  */
    GRALLOC_USAGE_PROTECTED             = 0x00004000U,

    /* buffer may be used as a cursor */
    GRALLOC_USAGE_CURSOR                = 0x00008000U,

    /* buffer will be used with the HW video encoder */
    GRALLOC_USAGE_HW_VIDEO_ENCODER      = 0x00010000U,
    /* buffer will be written by the HW camera pipeline */
    GRALLOC_USAGE_HW_CAMERA_WRITE       = 0x00020000U,
    /* buffer will be read by the HW camera pipeline */
    GRALLOC_USAGE_HW_CAMERA_READ        = 0x00040000U,
    /* buffer will be used as part of zero-shutter-lag queue */
    GRALLOC_USAGE_HW_CAMERA_ZSL         = 0x00060000U,
    /* mask for the camera access values */
    GRALLOC_USAGE_HW_CAMERA_MASK        = 0x00060000U,
    /* mask for the software usage bit-mask */
    GRALLOC_USAGE_HW_MASK               = 0x00071F00U,

    /* buffer will be used as a RenderScript Allocation */
    GRALLOC_USAGE_RENDERSCRIPT          = 0x00100000U,

    /* Set by the consumer to indicate to the producer that they may attach a
     * buffer that they did not detach from the BufferQueue. Will be filtered
     * out by GRALLOC_USAGE_ALLOC_MASK, so gralloc modules will not need to
     * handle this flag. */
    GRALLOC_USAGE_FOREIGN_BUFFERS       = 0x00200000U,

    /* Mask of all flags which could be passed to a gralloc module for buffer
     * allocation. Any flags not in this mask do not need to be handled by
     * gralloc modules. */
    GRALLOC_USAGE_ALLOC_MASK            = ~(GRALLOC_USAGE_FOREIGN_BUFFERS),

    /* implementation-specific private usage flags */
    GRALLOC_USAGE_PRIVATE_0             = 0x10000000U,
    GRALLOC_USAGE_PRIVATE_1             = 0x20000000U,
    GRALLOC_USAGE_PRIVATE_2             = 0x40000000U,
    GRALLOC_USAGE_PRIVATE_3             = 0x80000000U,
    GRALLOC_USAGE_PRIVATE_MASK          = 0xF0000000U,
};

typedef enum {
    GRALLOC1_CONSUMER_USAGE_NONE = 0,
    GRALLOC1_CONSUMER_USAGE_CPU_READ_NEVER = 0,
    /* 1ULL << 0 */
    GRALLOC1_CONSUMER_USAGE_CPU_READ = 1ULL << 1,
    GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN = 1ULL << 2 |
            GRALLOC1_CONSUMER_USAGE_CPU_READ,
    /* 1ULL << 3 */
    /* 1ULL << 4 */
    /* 1ULL << 5 */
    /* 1ULL << 6 */
    /* 1ULL << 7 */
    GRALLOC1_CONSUMER_USAGE_GPU_TEXTURE = 1ULL << 8,
    /* 1ULL << 9 */
    /* 1ULL << 10 */
    GRALLOC1_CONSUMER_USAGE_HWCOMPOSER = 1ULL << 11,
    GRALLOC1_CONSUMER_USAGE_CLIENT_TARGET = 1ULL << 12,
    /* 1ULL << 13 */
    /* 1ULL << 14 */
    GRALLOC1_CONSUMER_USAGE_CURSOR = 1ULL << 15,
    GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER = 1ULL << 16,
    /* 1ULL << 17 */
    GRALLOC1_CONSUMER_USAGE_CAMERA = 1ULL << 18,
    /* 1ULL << 19 */
    GRALLOC1_CONSUMER_USAGE_RENDERSCRIPT = 1ULL << 20,

    /* Indicates that the consumer may attach buffers to their end of the
     * BufferQueue, which means that the producer may never have seen a given
     * dequeued buffer before. May be ignored by the gralloc device. */
    GRALLOC1_CONSUMER_USAGE_FOREIGN_BUFFERS = 1ULL << 21,

    /* 1ULL << 22 */
    GRALLOC1_CONSUMER_USAGE_GPU_DATA_BUFFER = 1ULL << 23,
    /* 1ULL << 24 */
    /* 1ULL << 25 */
    /* 1ULL << 26 */
    /* 1ULL << 27 */

    /* Bits reserved for implementation-specific usage flags */
    GRALLOC1_CONSUMER_USAGE_PRIVATE_0 = 1ULL << 28,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_1 = 1ULL << 29,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_2 = 1ULL << 30,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_3 = 1ULL << 31,

    /* 1ULL << 32 */
    /* 1ULL << 33 */
    /* 1ULL << 34 */
    /* 1ULL << 35 */
    /* 1ULL << 36 */
    /* 1ULL << 37 */
    /* 1ULL << 38 */
    /* 1ULL << 39 */
    /* 1ULL << 40 */
    /* 1ULL << 41 */
    /* 1ULL << 42 */
    /* 1ULL << 43 */
    /* 1ULL << 44 */
    /* 1ULL << 45 */
    /* 1ULL << 46 */
    /* 1ULL << 47 */

    /* Bits reserved for implementation-specific usage flags */
    GRALLOC1_CONSUMER_USAGE_PRIVATE_19 = 1ULL << 48,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_18 = 1ULL << 49,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_17 = 1ULL << 50,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_16 = 1ULL << 51,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_15 = 1ULL << 52,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_14 = 1ULL << 53,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_13 = 1ULL << 54,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_12 = 1ULL << 55,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_11 = 1ULL << 56,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_10 = 1ULL << 57,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_9 = 1ULL << 58,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_8 = 1ULL << 59,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_7 = 1ULL << 60,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_6 = 1ULL << 61,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_5 = 1ULL << 62,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_4 = 1ULL << 63,
} gralloc1_consumer_usage_t;

typedef enum {
    GRALLOC1_PRODUCER_USAGE_NONE = 0,
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE_NEVER = 0,
    /* 1ULL << 0 */
    GRALLOC1_PRODUCER_USAGE_CPU_READ = 1ULL << 1,
    GRALLOC1_PRODUCER_USAGE_CPU_READ_OFTEN = 1ULL << 2 |
            GRALLOC1_PRODUCER_USAGE_CPU_READ,
    /* 1ULL << 3 */
    /* 1ULL << 4 */
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE = 1ULL << 5,
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN = 1ULL << 6 |
            GRALLOC1_PRODUCER_USAGE_CPU_WRITE,
    /* 1ULL << 7 */
    /* 1ULL << 8 */
    GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET = 1ULL << 9,
    /* 1ULL << 10 */
    /* 1ULL << 11 */
    /* 1ULL << 12 */
    /* 1ULL << 13 */

    /* The consumer must have a hardware-protected path to an external display
     * sink for this buffer. If a hardware-protected path is not available, then
     * do not attempt to display this buffer. */
    GRALLOC1_PRODUCER_USAGE_PROTECTED = 1ULL << 14,

    /* 1ULL << 15 */
    /* 1ULL << 16 */
    GRALLOC1_PRODUCER_USAGE_CAMERA = 1ULL << 17,
    /* 1ULL << 18 */
    /* 1ULL << 19 */
    /* 1ULL << 20 */
    /* 1ULL << 21 */
    GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER = 1ULL << 22,
    GRALLOC1_PRODUCER_USAGE_SENSOR_DIRECT_DATA = 1ULL << 23,
    /* 1ULL << 24 */
    /* 1ULL << 25 */
    /* 1ULL << 26 */
    /* 1ULL << 27 */

    /* Bits reserved for implementation-specific usage flags */
    GRALLOC1_PRODUCER_USAGE_PRIVATE_0 = 1ULL << 28,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_1 = 1ULL << 29,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_2 = 1ULL << 30,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_3 = 1ULL << 31,

    /* 1ULL << 32 */
    /* 1ULL << 33 */
    /* 1ULL << 34 */
    /* 1ULL << 35 */
    /* 1ULL << 36 */
    /* 1ULL << 37 */
    /* 1ULL << 38 */
    /* 1ULL << 39 */
    /* 1ULL << 40 */
    /* 1ULL << 41 */
    /* 1ULL << 42 */
    /* 1ULL << 43 */
    /* 1ULL << 44 */
    /* 1ULL << 45 */
    /* 1ULL << 46 */
    /* 1ULL << 47 */

    /* Bits reserved for implementation-specific usage flags */
    GRALLOC1_PRODUCER_USAGE_PRIVATE_19 = 1ULL << 48,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_18 = 1ULL << 49,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_17 = 1ULL << 50,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_16 = 1ULL << 51,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_15 = 1ULL << 52,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_14 = 1ULL << 53,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_13 = 1ULL << 54,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_12 = 1ULL << 55,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_11 = 1ULL << 56,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_10 = 1ULL << 57,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_9 = 1ULL << 58,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_8 = 1ULL << 59,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_7 = 1ULL << 60,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_6 = 1ULL << 61,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_5 = 1ULL << 62,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_4 = 1ULL << 63,
} gralloc1_producer_usage_t;