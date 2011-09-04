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

#define  D(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#define  W(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#define  E(...)    VERBOSE_PRINT(camera,__VA_ARGS__)

 /*
 * NOTE: RGB and big/little endian considerations. Wherewer in this code RGB
 * pixels are represented as WORD, or DWORD, the color order inside the
 * WORD / DWORD matches the one that would occur if that WORD / DWORD would have
 * been read from the typecasted framebuffer:
 *
 *      const uint32_t rgb = *reinterpret_cast<const uint32_t*>(framebuffer);
 *
 * So, if this code runs on the little endian CPU, red color in 'rgb' would be
 * masked as 0x000000ff, and blue color would be masked as 0x00ff0000, while if
 * the code runs on a big endian CPU, the red color in 'rgb' would be masked as
 * 0xff000000, and blue color would be masked as 0x0000ff00,
 */

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

/*
 * RGB565 color masks
 */

#ifndef HOST_WORDS_BIGENDIAN
static const uint16_t kRed5     = 0x001f;
static const uint16_t kGreen6   = 0x07e0;
static const uint16_t kBlue5    = 0xf800;
#else   // !HOST_WORDS_BIGENDIAN
static const uint16_t kRed5     = 0xf800;
static const uint16_t kGreen6   = 0x07e0;
static const uint16_t kBlue5    = 0x001f;
#endif  // !HOST_WORDS_BIGENDIAN

/*
 * RGB32 color masks
 */

#ifndef HOST_WORDS_BIGENDIAN
static const uint32_t kRed8     = 0x000000ff;
static const uint32_t kGreen8   = 0x0000ff00;
static const uint32_t kBlue8    = 0x00ff0000;
#else   // !HOST_WORDS_BIGENDIAN
static const uint32_t kRed8     = 0x00ff0000;
static const uint32_t kGreen8   = 0x0000ff00;
static const uint32_t kBlue8    = 0x000000ff;
#endif  // !HOST_WORDS_BIGENDIAN

/*
 * Extracting, and saving color bytes from / to WORD / DWORD RGB.
 */

#ifndef HOST_WORDS_BIGENDIAN
/* Extract red, green, and blue bytes from RGB565 word. */
#define R16(rgb)    (uint8_t)(rgb & kRed5)
#define G16(rgb)    (uint8_t)((rgb & kGreen6) >> 5)
#define B16(rgb)    (uint8_t)((rgb & kBlue5) >> 11)
/* Make 8 bits red, green, and blue, extracted from RGB565 word. */
#define R16_32(rgb) (uint8_t)(((rgb & kRed5) << 3) | ((rgb & kRed5) >> 2))
#define G16_32(rgb) (uint8_t)(((rgb & kGreen6) >> 3) | ((rgb & kGreen6) >> 9))
#define B16_32(rgb) (uint8_t)(((rgb & kBlue5) >> 8) | ((rgb & kBlue5) >> 14))
/* Extract red, green, and blue bytes from RGB32 dword. */
#define R32(rgb)    (uint8_t)(rgb & kRed8)
#define G32(rgb)    (uint8_t)(((rgb & kGreen8) >> 8) & 0xff)
#define B32(rgb)    (uint8_t)(((rgb & kBlue8) >> 16) & 0xff)
/* Build RGB565 word from red, green, and blue bytes. */
#define RGB565(r, g, b) (uint16_t)(((((uint16_t)(b) << 6) | g) << 5) | r)
/* Build RGB32 dword from red, green, and blue bytes. */
#define RGB32(r, g, b) (uint32_t)(((((uint32_t)(b) << 8) | g) << 8) | r)
#else   // !HOST_WORDS_BIGENDIAN
/* Extract red, green, and blue bytes from RGB565 word. */
#define R16(rgb)    (uint8_t)((rgb & kRed5) >> 11)
#define G16(rgb)    (uint8_t)((rgb & kGreen6) >> 5)
#define B16(rgb)    (uint8_t)(rgb & kBlue5)
/* Make 8 bits red, green, and blue, extracted from RGB565 word. */
#define R16_32(rgb) (uint8_t)(((rgb & kRed5) >> 8) | ((rgb & kRed5) >> 14))
#define G16_32(rgb) (uint8_t)(((rgb & kGreen6) >> 3) | ((rgb & kGreen6) >> 9))
#define B16_32(rgb) (uint8_t)(((rgb & kBlue5) << 3) | ((rgb & kBlue5) >> 2))
/* Extract red, green, and blue bytes from RGB32 dword. */
#define R32(rgb)    (uint8_t)((rgb & kRed8) >> 16)
#define G32(rgb)    (uint8_t)((rgb & kGreen8) >> 8)
#define B32(rgb)    (uint8_t)(rgb & kBlue8)
/* Build RGB565 word from red, green, and blue bytes. */
#define RGB565(r, g, b) (uint16_t)(((((uint16_t)(r) << 6) | g) << 5) | b)
/* Build RGB32 dword from red, green, and blue bytes. */
#define RGB32(r, g, b) (uint32_t)(((((uint32_t)(r) << 8) | g) << 8) | b)
#endif  // !HOST_WORDS_BIGENDIAN

/* An union that simplifies breaking 32 bit RGB into separate R, G, and B colors.
 */
typedef union RGB32_t {
    uint32_t    color;
    struct {
#ifndef HOST_WORDS_BIGENDIAN
        uint8_t r; uint8_t g; uint8_t b; uint8_t a;
#else   // !HOST_WORDS_BIGENDIAN
        uint8_t a; uint8_t b; uint8_t g; uint8_t r;
#endif  // HOST_WORDS_BIGENDIAN
    };
} RGB32_t;

/* Clips a value to the unsigned 0-255 range, treating negative values as zero.
 */
static __inline__ int
clamp(int x)
{
    if (x > 255) return 255;
    if (x < 0)   return 0;
    return x;
}

/********************************************************************************
 * Basics of RGB -> YUV conversion
 *******************************************************************************/

/*
 * RGB -> YUV conversion macros
 */
#define RGB2Y(r, g, b) (uint8_t)(((66 * (r) + 129 * (g) +  25 * (b) + 128) >> 8) +  16)
#define RGB2U(r, g, b) (uint8_t)(((-38 * (r) - 74 * (g) + 112 * (b) + 128) >> 8) + 128)
#define RGB2V(r, g, b) (uint8_t)(((112 * (r) - 94 * (g) -  18 * (b) + 128) >> 8) + 128)

/* Converts R8 G8 B8 color to YUV. */
static __inline__ void
R8G8B8ToYUV(uint8_t r, uint8_t g, uint8_t b, uint8_t* y, uint8_t* u, uint8_t* v)
{
    *y = RGB2Y((int)r, (int)g, (int)b);
    *u = RGB2U((int)r, (int)g, (int)b);
    *v = RGB2V((int)r, (int)g, (int)b);
}

/* Converts RGB565 color to YUV. */
static __inline__ void
RGB565ToYUV(uint16_t rgb, uint8_t* y, uint8_t* u, uint8_t* v)
{
    R8G8B8ToYUV(R16_32(rgb), G16_32(rgb), B16_32(rgb), y, u, v);
}

/* Converts RGB32 color to YUV. */
static __inline__ void
RGB32ToYUV(uint32_t rgb, uint8_t* y, uint8_t* u, uint8_t* v)
{
    RGB32_t rgb_c;
    rgb_c.color = rgb;
    R8G8B8ToYUV(rgb_c.r, rgb_c.g, rgb_c.b, y, u, v);
}

/********************************************************************************
 * Basics of YUV -> RGB conversion.
 * Note that due to the fact that guest uses RGB only on preview window, and the
 * RGB format that is used is RGB565, we can limit YUV -> RGB conversions to
 * RGB565 only.
 *******************************************************************************/

/*
 * YUV -> RGB conversion macros
 */

/* "Optimized" macros that take specialy prepared Y, U, and V values:
 *  C = Y - 16
 *  D = U - 128
 *  E = V - 128
 */
#define YUV2RO(C, D, E) clamp((298 * (C) + 409 * (E) + 128) >> 8)
#define YUV2GO(C, D, E) clamp((298 * (C) - 100 * (D) - 208 * (E) + 128) >> 8)
#define YUV2BO(C, D, E) clamp((298 * (C) + 516 * (D) + 128) >> 8)

/*
 *  Main macros that take the original Y, U, and V values
 */
#define YUV2R(y, u, v) clamp((298 * ((y)-16) + 409 * ((v)-128) + 128) >> 8)
#define YUV2G(y, u, v) clamp((298 * ((y)-16) - 100 * ((u)-128) - 208 * ((v)-128) + 128) >> 8)
#define YUV2B(y, u, v) clamp((298 * ((y)-16) + 516 * ((u)-128) + 128) >> 8)


/* Converts YUV color to RGB565. */
static __inline__ uint16_t
YUVToRGB565(int y, int u, int v)
{
    /* Calculate C, D, and E values for the optimized macro. */
    y -= 16; u -= 128; v -= 128;
    const uint16_t r = YUV2RO(y,u,v) >> 3;
    const uint16_t g = YUV2GO(y,u,v) >> 2;
    const uint16_t b = YUV2BO(y,u,v) >> 3;
    return RGB565(r, g, b);
}

/* Converts YUV color to RGB32. */
static __inline__ uint32_t
YUVToRGB32(int y, int u, int v)
{
    /* Calculate C, D, and E values for the optimized macro. */
    y -= 16; u -= 128; v -= 128;
    RGB32_t rgb;
    rgb.r = YUV2RO(y,u,v) & 0xff;
    rgb.g = YUV2GO(y,u,v) & 0xff;
    rgb.b = YUV2BO(y,u,v) & 0xff;
    return rgb.color;
}

/********************************************************************************
 * YUV -> RGB565 converters.
 *******************************************************************************/

/* Common converter for a variety of YUV 4:2:2 formats to RGB565.
 * Meaning of the parameters is pretty much straight forward here, except for the
 * 'next_Y'. In all YUV 4:2:2 formats, every two pixels are encoded in subseqent
 * four bytes, that contain two Ys (one for each pixel), one U, and one V values
 * that are shared between the two pixels. The only difference between formats is
 * how Y,U, and V values are arranged inside the pair. The actual arrangment
 * doesn't make any difference how to advance Us and Vs: subsequent Us and Vs are
 * always four bytes apart. However, with Y things are a bit more messy inside
 * the pair. The only thing that is certain here is that Ys for subsequent pairs
 * are also always four bytes apart. And we have parameter 'next_Y' here that
 * controls the distance between two Ys inside a pixel pair. */
static void _YUY422_to_RGB565(const uint8_t* from_Y,
                              const uint8_t* from_U,
                              const uint8_t* from_V,
                              int next_Y,
                              int width,
                              int height,
                              uint16_t* rgb)
{
    int x, y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x += 2, from_Y += 4, from_U += 4, from_V += 4) {
            const uint8_t u = *from_U, v = *from_V;
            *rgb = YUVToRGB565(*from_Y, u, v); rgb++;
            *rgb = YUVToRGB565(from_Y[next_Y], u, v); rgb++;
        }
    }
}

/* Converts YUYV frame into RGB565 frame. */
static void _YUYV_to_RGB565(const uint8_t* from,
                            int width,
                            int height,
                            uint8_t* to)
{
    _YUY422_to_RGB565(from, from + 1, from + 3, 2, width, height, (uint16_t*)to);
}

/* Converts YVYU frame into RGB565 frame. */
static void _YVYU_to_RGB565(const uint8_t* from,
                            int width,
                            int height,
                            uint8_t* to)
{
    _YUY422_to_RGB565(from, from + 3, from + 1, 2, width, height, (uint16_t*)to);
}

/* Converts UYVY frame into RGB565 frame. */
static void _UYVY_to_RGB565(const uint8_t* from,
                            int width,
                            int height,
                            uint8_t* to)
{
    _YUY422_to_RGB565(from + 1, from, from + 2, 2, width, height, (uint16_t*)to);
}

/* Converts VYUY frame into RGB565 frame. */
static void _VYUY_to_RGB565(const uint8_t* from,
                            int width,
                            int height,
                            uint8_t* to)
{
    _YUY422_to_RGB565(from + 1, from + 2, from, 2, width, height, (uint16_t*)to);
}

/* Converts YYUV frame into RGB565 frame. */
static void _YYUV_to_RGB565(const uint8_t* from,
                            int width,
                            int height,
                            uint8_t* to)
{
    _YUY422_to_RGB565(from, from + 2, from + 3, 1, width, height, (uint16_t*)to);
}

/* Converts YYVU frame into RGB565 frame. */
static void _YYVU_to_RGB565(const uint8_t* from,
                            int width,
                            int height,
                            uint8_t* to)
{
    _YUY422_to_RGB565(from, from + 3, from + 2, 1, width, height, (uint16_t*)to);
}

/********************************************************************************
 * YUV -> RGB32 converters.
 *******************************************************************************/

/* Common converter for a variety of YUV 4:2:2 formats to RGB32.
 * See _YUY422_to_RGB565 comments for explanations. */
static void _YUY422_to_RGB32(const uint8_t* from_Y,
                             const uint8_t* from_U,
                             const uint8_t* from_V,
                             int next_Y,
                             int width,
                             int height,
                             uint32_t* rgb)
{
    int x, y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x += 2, from_Y += 4, from_U += 4, from_V += 4) {
            const uint8_t u = *from_U, v = *from_V;
            *rgb = YUVToRGB32(*from_Y, u, v); rgb++;
            *rgb = YUVToRGB32(from_Y[next_Y], u, v); rgb++;
        }
    }
}

/* Converts YUYV frame into RGB32 frame. */
static void _YUYV_to_RGB32(const uint8_t* from,
                           int width,
                           int height,
                           uint8_t* to)
{
    _YUY422_to_RGB32(from, from + 1, from + 3, 2, width, height, (uint32_t*)to);
}

/* Converts YVYU frame into RGB32 frame. */
static void _YVYU_to_RGB32(const uint8_t* from,
                           int width,
                           int height,
                           uint8_t* to)
{
    _YUY422_to_RGB32(from, from + 3, from + 1, 2, width, height, (uint32_t*)to);
}

/* Converts UYVY frame into RGB32 frame. */
static void _UYVY_to_RGB32(const uint8_t* from,
                           int width,
                           int height,
                           uint8_t* to)
{
    _YUY422_to_RGB32(from + 1, from, from + 2, 2, width, height, (uint32_t*)to);
}

/* Converts VYUY frame into RGB32 frame. */
static void _VYUY_to_RGB32(const uint8_t* from,
                           int width,
                           int height,
                           uint8_t* to)
{
    _YUY422_to_RGB32(from + 1, from + 2, from, 2, width, height, (uint32_t*)to);
}

/* Converts YYUV frame into RGB32 frame. */
static void _YYUV_to_RGB32(const uint8_t* from,
                           int width,
                           int height,
                           uint8_t* to)
{
    _YUY422_to_RGB32(from, from + 2, from + 3, 1, width, height, (uint32_t*)to);
}

/* Converts YYVU frame into RGB32 frame. */
static void _YYVU_to_RGB32(const uint8_t* from,
                           int width,
                           int height,
                           uint8_t* to)
{
    _YUY422_to_RGB32(from, from + 3, from + 2, 1, width, height, (uint32_t*)to);
}

/********************************************************************************
 * YUV -> YV12 converters.
 *******************************************************************************/

/* Common converter for a variety of YUV 4:2:2 formats to YV12.
 * See comments to _YUY422_to_RGB565 for more information. */
static void _YUY422_to_YV12(const uint8_t* from_Y,
                            const uint8_t* from_U,
                            const uint8_t* from_V,
                            int next_Y,
                            int width,
                            int height,
                            uint8_t* yv12)
{
    const int total_pix = width * height;
    uint8_t* to_Y = yv12;
    uint8_t* to_U = yv12 + total_pix;
    uint8_t* to_V = to_U + total_pix / 4;
    uint8_t* c_U = to_U;
    uint8_t* c_V = to_V;
    int x, y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x += 2, to_Y += 2, c_U++, c_V++,
                                       from_Y += 4, from_U += 4, from_V += 4) {
            *to_Y = *from_Y; to_Y[1] = from_Y[next_Y];
            *c_U = *from_U; *c_V = *from_V;
        }
        if (y & 0x1) {
            /* Finished two pixel lines: move to the next U/V line */
            to_U = c_U; to_V = c_V;
        } else {
            /* Reset U/V pointers to the beginning of the line */
            c_U = to_U; c_V = to_V;
        }
    }
}

/* Converts YUYV frame into YV12 frame. */
static void _YUYV_to_YV12(const uint8_t* from,
                          int width,
                          int height,
                          uint8_t* to)
{
    _YUY422_to_YV12(from, from + 1, from + 3, 2, width, height, to);
}

/* Converts YVYU frame into YV12 frame. */
static void _YVYU_to_YV12(const uint8_t* from,
                          int width,
                          int height,
                          uint8_t* to)
{
    _YUY422_to_YV12(from, from + 3, from + 1, 2, width, height, to);
}

/* Converts UYVY frame into YV12 frame. */
static void _UYVY_to_YV12(const uint8_t* from,
                          int width,
                          int height,
                          uint8_t* to)
{
    _YUY422_to_YV12(from + 1, from, from + 2, 2, width, height, to);
}

/* Converts VYUY frame into YV12 frame. */
static void _VYUY_to_YV12(const uint8_t* from,
                          int width,
                          int height,
                          uint8_t* to)
{
    _YUY422_to_YV12(from + 1, from + 2, from, 2, width, height, to);
}

/* Converts YYUV frame into YV12 frame. */
static void _YYUV_to_YV12(const uint8_t* from,
                          int width,
                          int height,
                          uint8_t* to)
{
    _YUY422_to_YV12(from, from + 2, from + 3, 1, width, height, to);
}

/* Converts YYVU frame into YV12 frame. */
static void _YYVU_to_YV12(const uint8_t* from,
                          int width,
                          int height,
                          uint8_t* to)
{
    _YUY422_to_YV12(from, from + 3, from + 2, 1, width, height, to);
}

/********************************************************************************
 * RGB -> YV12 converters.
 *******************************************************************************/

/* Converts RGB565 frame into YV12 frame. */
static void _RGB565_to_YV12(const uint8_t* from,
                            int width,
                            int height,
                            uint8_t* to)
{
    const int total_pix = width * height;
    const uint16_t* rgb = (const uint16_t*)from;
    uint8_t* to_Y = to;
    uint8_t* to_Cb = to + total_pix;
    uint8_t* to_Cr = to_Cb + total_pix / 4;
    uint8_t* Cb = to_Cb;
    uint8_t* Cr = to_Cr;
    int x, y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x += 2, to_Cb++, to_Cr++) {
            RGB565ToYUV(*rgb, to_Y, to_Cb, to_Cr); rgb++; to_Y++;
            RGB565ToYUV(*rgb, to_Y, to_Cb, to_Cr); rgb++; to_Y++;
        }
        if (y & 0x1) {
            to_Cb = Cb; to_Cr = Cr;
        } else {
            Cb = to_Cb; Cr = to_Cr;
        }
    }
}

/* Converts RGB24 frame into YV12 frame. */
static void _RGB24_to_YV12(const uint8_t* from,
                           int width,
                           int height,
                           uint8_t* to)
{
    const int total_pix = width * height;
    /* Bytes per line: each line must be WORD aligned. */
    const int bpl = (width * 3 + 1) & ~1;
    const uint8_t* line_start = from;
    uint8_t* to_Y = to;
    uint8_t* to_Cb = to + total_pix;
    uint8_t* to_Cr = to_Cb + total_pix / 4;
    uint8_t* Cb = to_Cb;
    uint8_t* Cr = to_Cr;
    int x, y;

    for (y = 0; y < height; y++) {
        from = line_start;
        for (x = 0; x < width; x += 2, from += 6, to_Cb++, to_Cr++) {
            R8G8B8ToYUV(from[0], from[1], from[2], to_Y, to_Cb, to_Cr); to_Y++;
            R8G8B8ToYUV(from[3], from[4], from[5], to_Y, to_Cb, to_Cr); to_Y++;
        }
        if (y & 0x1) {
            to_Cb = Cb; to_Cr = Cr;
        } else {
            Cb = to_Cb; Cr = to_Cr;
        }
        /* Advance to next line, keeping proper alignment. */
        line_start += bpl;
    }
}

/* Converts RGB32 frame into YV12 frame. */
static void _RGB32_to_YV12(const uint8_t* from,
                           int width,
                           int height,
                           uint8_t* to)
{
    const int total_pix = width * height;
    uint8_t* to_Y = to;
    uint8_t* to_Cb = to + total_pix;
    uint8_t* to_Cr = to_Cb + total_pix / 4;
    uint8_t* Cb = to_Cb;
    uint8_t* Cr = to_Cr;
    int x, y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x += 2, from += 8, to_Cb++, to_Cr++) {
            R8G8B8ToYUV(from[0], from[1], from[2], to_Y, to_Cb, to_Cr); to_Y++;
            R8G8B8ToYUV(from[4], from[5], from[6], to_Y, to_Cb, to_Cr); to_Y++;
        }
        if (y & 0x1) {
            to_Cb = Cb; to_Cr = Cr;
        } else {
            Cb = to_Cb; Cr = to_Cr;
        }
    }
}

/********************************************************************************
 * BGR -> YV12 converters.
 *******************************************************************************/

/* Converts BGR24 frame into YV12 frame. */
static void _BGR24_to_YV12(const uint8_t* from,
                           int width,
                           int height,
                           uint8_t* to)
{
    const int total_pix = width * height;
    /* Bytes per line: each line must be WORD aligned. */
    const int bpl = (width * 3 + 1) & ~1;
    const uint8_t* line_start = from;
    uint8_t* to_Y = to;
    uint8_t* to_Cb = to + total_pix;
    uint8_t* to_Cr = to_Cb + total_pix / 4;
    uint8_t* Cb = to_Cb;
    uint8_t* Cr = to_Cr;
    int x, y;

    for (y = 0; y < height; y++) {
        from = line_start;
        for (x = 0; x < width; x += 2, from += 6, to_Cb++, to_Cr++) {
            R8G8B8ToYUV(from[2], from[1], from[0], to_Y, to_Cb, to_Cr); to_Y++;
            R8G8B8ToYUV(from[5], from[4], from[3], to_Y, to_Cb, to_Cr); to_Y++;
        }
        if (y & 0x1) {
            to_Cb = Cb; to_Cr = Cr;
        } else {
            Cb = to_Cb; Cr = to_Cr;
        }
        /* Advance to next line, keeping proper alignment. */
        line_start += bpl;
    }
}

/* Converts BGR32 frame into YV12 frame. */
static void _BGR32_to_YV12(const uint8_t* from,
                           int width,
                           int height,
                           uint8_t* to)
{
    const int total_pix = width * height;
    uint8_t* to_Y = to;
    uint8_t* to_Cb = to + total_pix;
    uint8_t* to_Cr = to_Cb + total_pix / 4;
    uint8_t* Cb = to_Cb;
    uint8_t* Cr = to_Cr;
    int x, y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x += 2, from += 8, to_Cb++, to_Cr++) {
            R8G8B8ToYUV(from[2], from[1], from[0], to_Y, to_Cb, to_Cr); to_Y++;
            R8G8B8ToYUV(from[6], from[5], from[4], to_Y, to_Cb, to_Cr); to_Y++;
        }
        if (y & 0x1) {
            to_Cb = Cb; to_Cr = Cr;
        } else {
            Cb = to_Cb; Cr = to_Cr;
        }
    }
}

/********************************************************************************
 * RGB -> RGB565 converters.
 *******************************************************************************/

/* Converts RGB24 frame into RGB565 frame. */
static void _RGB24_to_RGB565(const uint8_t* from,
                             int width,
                             int height,
                             uint8_t* to)
{
    /* Bytes per line: each line must be WORD aligned. */
    const int bpl = (width * 3 + 1) & ~1;
    const uint8_t* line_start = from;
    uint16_t* rgb = (uint16_t*)to;
    int x, y;

    for (y = 0; y < height; y++) {
        from = line_start;
        for (x = 0; x < width; x++, rgb++) {
            const uint16_t r = *from >> 3; from++;
            const uint16_t g = *from >> 2; from++;
            const uint16_t b = *from >> 3; from++;
            *rgb = b | (g << 5) | (r << 11);
        }
        /* Advance to next line, keeping proper alignment. */
        line_start += bpl;
    }
}

/* Converts RGB32 frame into RGB565 frame. */
static void _RGB32_to_RGB565(const uint8_t* from,
                             int width,
                             int height,
                             uint8_t* to)
{
    /* Bytes per line: each line must be WORD aligned. */
    uint16_t* rgb = (uint16_t*)to;
    int x, y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++, rgb++, from++) {
            const uint16_t r = *from >> 3; from++;
            const uint16_t g = *from >> 2; from++;
            const uint16_t b = *from >> 3; from++;
            *rgb = b | (g << 5) | (r << 11);
        }
    }
}

/********************************************************************************
 * BGR -> RGB565 converters.
 *******************************************************************************/

/* Converts BGR24 frame into RGB565 frame. */
static void _BGR24_to_RGB565(const uint8_t* from,
                             int width,
                             int height,
                             uint8_t* to)
{
    /* Bytes per line: each line must be WORD aligned. */
    const int bpl = (width * 3 + 1) & ~1;
    const uint8_t* line_start = from;
    uint16_t* rgb = (uint16_t*)to;
    int x, y;

    for (y = 0; y < height; y++) {
        from = line_start;
        for (x = 0; x < width; x++, rgb++) {
            const uint16_t b = *from >> 3; from++;
            const uint16_t g = *from >> 2; from++;
            const uint16_t r = *from >> 3; from++;
            *rgb = b | (g << 5) | (r << 11);
        }
        /* Advance to next line, keeping proper alignment. */
        line_start += bpl;
    }
}

/* Converts BGR32 frame into RGB565 frame. */
static void _BGR32_to_RGB565(const uint8_t* from,
                           int width,
                           int height,
                           uint8_t* to)
{
    /* Bytes per line: each line must be WORD aligned. */
    uint16_t* rgb = (uint16_t*)to;
    int x, y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++, rgb++, from++) {
            const uint16_t b = *from >> 3; from++;
            const uint16_t g = *from >> 2; from++;
            const uint16_t r = *from >> 3; from++;
            *rgb = b | (g << 5) | (r << 11);
        }
    }
}

/* Describes a converter for one pixel format to another. */
typedef struct FormatConverterEntry {
    /* Pixel format to convert from. */
    uint32_t          from_format;
    /* Pixel format to convert to. */
    uint32_t          to_format;
    /* Address of the conversion routine. */
    FormatConverter   converter;
} FormatConverterEntry;


/* Lists currently implemented converters. */
static const FormatConverterEntry _converters[] = {
    /*
     * YUV 4:2:2 variety -> YV12
     */

    /* YUYV -> YV12 */
    { V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_YVU420, _YUYV_to_YV12 },
    /* UYVY -> YV12 */
    { V4L2_PIX_FMT_UYVY, V4L2_PIX_FMT_YVU420, _UYVY_to_YV12 },
    /* YVYU -> YV12 */
    { V4L2_PIX_FMT_YVYU, V4L2_PIX_FMT_YVU420, _YVYU_to_YV12 },
    /* VYUY -> YV12 */
    { V4L2_PIX_FMT_VYUY, V4L2_PIX_FMT_YVU420, _VYUY_to_YV12 },
    /* YYUV -> YV12 */
    { V4L2_PIX_FMT_YYUV, V4L2_PIX_FMT_YVU420, _YYUV_to_YV12 },
    /* YUY2 -> YV12 This format is the same as YYUV */
    { V4L2_PIX_FMT_YUY2, V4L2_PIX_FMT_YVU420, _YYUV_to_YV12 },
    /* YUNV -> YV12 This format is the same as YYUV */
    { V4L2_PIX_FMT_YUNV, V4L2_PIX_FMT_YVU420, _YYUV_to_YV12 },
    /* V422 -> YV12 This format is the same as YYUV */
    { V4L2_PIX_FMT_V422, V4L2_PIX_FMT_YVU420, _YYUV_to_YV12 },
    /* YYVU -> YV12 */
    { V4L2_PIX_FMT_YYVU, V4L2_PIX_FMT_YVU420, _YYVU_to_YV12 },

    /*
     * YUV 4:2:2 variety -> RGB565
     */

    /* YUYV -> RGB565 */
    { V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_RGB565, _YUYV_to_RGB565 },
    /* UYVY -> RGB565 */
    { V4L2_PIX_FMT_UYVY, V4L2_PIX_FMT_RGB565, _UYVY_to_RGB565 },
    /* YVYU -> RGB565 */
    { V4L2_PIX_FMT_YVYU, V4L2_PIX_FMT_RGB565, _YVYU_to_RGB565 },
    /* VYUY -> RGB565 */
    { V4L2_PIX_FMT_VYUY, V4L2_PIX_FMT_RGB565, _VYUY_to_RGB565 },
    /* YYUV -> RGB565 */
    { V4L2_PIX_FMT_YYUV, V4L2_PIX_FMT_RGB565, _YYUV_to_RGB565 },
    /* YUY2 -> RGB565 This format is the same as YYUV */
    { V4L2_PIX_FMT_YUY2, V4L2_PIX_FMT_RGB565, _YUYV_to_RGB565 },
    /* YUNV -> RGB565 This format is the same as YYUV */
    { V4L2_PIX_FMT_YUNV, V4L2_PIX_FMT_RGB565, _YUYV_to_RGB565 },
    /* V422 -> RGB565 This format is the same as YYUV */
    { V4L2_PIX_FMT_V422, V4L2_PIX_FMT_RGB565, _YUYV_to_RGB565 },
    /* YYVU -> RGB565 This format is the same as YYUV */
    { V4L2_PIX_FMT_YYVU, V4L2_PIX_FMT_RGB565, _YYVU_to_RGB565 },

    /*
     * YUV 4:2:2 variety -> RGB32
     */

    /* YUYV -> RGB565 */
    { V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_RGB32, _YUYV_to_RGB32 },
    /* UYVY -> RGB565 */
    { V4L2_PIX_FMT_UYVY, V4L2_PIX_FMT_RGB32, _UYVY_to_RGB32 },
    /* YVYU -> RGB565 */
    { V4L2_PIX_FMT_YVYU, V4L2_PIX_FMT_RGB32, _YVYU_to_RGB32 },
    /* VYUY -> RGB565 */
    { V4L2_PIX_FMT_VYUY, V4L2_PIX_FMT_RGB32, _VYUY_to_RGB32 },
    /* YYUV -> RGB565 */
    { V4L2_PIX_FMT_YYUV, V4L2_PIX_FMT_RGB32, _YYUV_to_RGB32 },
    /* YUY2 -> RGB565 This format is the same as YYUV */
    { V4L2_PIX_FMT_YUY2, V4L2_PIX_FMT_RGB32, _YUYV_to_RGB32 },
    /* YUNV -> RGB565 This format is the same as YYUV */
    { V4L2_PIX_FMT_YUNV, V4L2_PIX_FMT_RGB32, _YUYV_to_RGB32 },
    /* V422 -> RGB565 This format is the same as YYUV */
    { V4L2_PIX_FMT_V422, V4L2_PIX_FMT_RGB32, _YUYV_to_RGB32 },
    /* YYVU -> RGB565 This format is the same as YYUV */
    { V4L2_PIX_FMT_YYVU, V4L2_PIX_FMT_RGB32, _YYVU_to_RGB32 },

    /*
     * RGB variety -> YV12
     */

    /* RGB565 -> YV12 */
    { V4L2_PIX_FMT_RGB565, V4L2_PIX_FMT_YVU420, _RGB565_to_YV12 },
    /* RGB24 -> YV12 */
    { V4L2_PIX_FMT_RGB24, V4L2_PIX_FMT_YVU420, _RGB24_to_YV12 },
    /* RGB32 -> YV12 */
    { V4L2_PIX_FMT_RGB32, V4L2_PIX_FMT_YVU420, _RGB32_to_YV12 },

    /*
     * BGR variety -> YV12
     */

    /* BGR24 -> YV12 */
    { V4L2_PIX_FMT_BGR24, V4L2_PIX_FMT_YVU420, _BGR24_to_YV12 },
    /* BGR32 -> YV12 */
    { V4L2_PIX_FMT_BGR32, V4L2_PIX_FMT_YVU420, _BGR32_to_YV12 },

    /*
     * RGB variety -> RGB565
     */

    /* RGB24 -> RGB565 */
    { V4L2_PIX_FMT_RGB24, V4L2_PIX_FMT_RGB565, _RGB24_to_RGB565 },
    /* RGB32 -> RGB565 */
    { V4L2_PIX_FMT_RGB32, V4L2_PIX_FMT_RGB565, _RGB32_to_RGB565 },

    /*
     * BGR variety -> RGB565
     */

    /* BGR24 -> RGB565 */
    { V4L2_PIX_FMT_BGR24, V4L2_PIX_FMT_RGB565, _BGR24_to_RGB565 },
    /* BGR32 -> RGB565 */
    { V4L2_PIX_FMT_BGR32, V4L2_PIX_FMT_RGB565, _BGR32_to_RGB565 },
};

/* Gets an address of a routine that provides frame conversion for the
 * given pixel formats.
 * Param:
 *  from - Pixel format to convert from.
 *  to - Pixel format to convert to.
 * Return:
 *  Address of an appropriate conversion routine, or NULL if no conversion
 *  routine exsits for the given pair of pixel formats.
 */
static FormatConverter
_get_format_converter(uint32_t from, uint32_t to)
{
    const int num_converters = sizeof(_converters) / sizeof(*_converters);
    int n;
    for (n = 0; n < num_converters; n++) {
        if (_converters[n].from_format == from &&
            _converters[n].to_format == to) {
            return _converters[n].converter;
        }
    }

    E("%s: No converter found from %.4s to %.4s pixel formats",
      __FUNCTION__, (const char*)&from, (const char*)&to);
    return NULL;
}

/********************************************************************************
 * Public API
 *******************************************************************************/

int
has_converter(uint32_t from, uint32_t to)
{
    return (from == to) ? 1 :
                          (_get_format_converter(from, to) != NULL);
}

int
convert_frame(const void* frame,
              uint32_t pixel_format,
              size_t framebuffer_size,
              int width,
              int height,
              ClientFrameBuffer* framebuffers,
              int fbs_num)
{
    int n;

    for (n = 0; n < fbs_num; n++) {
        if (framebuffers[n].pixel_format == pixel_format) {
            /* Same pixel format. No conversion needed. */
            memcpy(framebuffers[n].framebuffer, frame, framebuffer_size);
        } else {
            /* Get the converter. Note that the client must have ensured the
             * existence of the converter when it was starting the camera. */
            FormatConverter convert =
                _get_format_converter(pixel_format, framebuffers[n].pixel_format);
            if (convert != NULL) {
                convert(frame, width, height, framebuffers[n].framebuffer);
            } else {
                E("%s No converter from %.4s to %.4s for framebuffer # %d ",
                  __FUNCTION__, (const char*)&pixel_format,
                  (const char*)&framebuffers[n].pixel_format, n);
                return -1;
            }
        }
    }

    return 0;
}
