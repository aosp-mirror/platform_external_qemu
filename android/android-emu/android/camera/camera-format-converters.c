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

#include "android/camera/camera-format-converters.h"
#include "android/utils/misc.h"

#ifdef __linux__
#include <linux/videodev2.h>
#endif

#include <libyuv.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#define  D_ACTIVE  VERBOSE_CHECK(camera)

/* the T(...) macro is used to dump traffic */
#define T_ACTIVE 0

#if T_ACTIVE
#define T(...) VERBOSE_PRINT(camera, __VA_ARGS__)
#else
#define T(...) ((void)0)
#endif

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
#define kRed8     ((uint32_t)0x000000ffU)
#define kGreen8   ((uint32_t)0x0000ff00U)
#define kBlue8    ((uint32_t)0x00ff0000U)
#else   // !HOST_WORDS_BIGENDIAN
#define kRed8     ((uint32_t)0x00ff0000U)
#define kGreen8   ((uint32_t)0x0000ff00U)
#define kBlue8    ((uint32_t)0x000000ffU)
#endif  // !HOST_WORDS_BIGENDIAN

/*
 * Extracting, and saving color bytes from / to WORD / DWORD RGB.
 */

#ifndef HOST_WORDS_BIGENDIAN
/* Extract red, green, and blue bytes from RGB565 word. */
#define R16(rgb)    (uint8_t)((rgb) & kRed5)
#define G16(rgb)    (uint8_t)(((rgb) & kGreen6) >> 5)
#define B16(rgb)    (uint8_t)(((rgb) & kBlue5) >> 11)
/* Make 8 bits red, green, and blue, extracted from RGB565 word. */
#define R16_32(rgb) (uint8_t)((((rgb) & kRed5) << 3) | (((rgb) & kRed5) >> 2))
#define G16_32(rgb) (uint8_t)((((rgb) & kGreen6) >> 3) | (((rgb) & kGreen6) >> 9))
#define B16_32(rgb) (uint8_t)((((rgb) & kBlue5) >> 8) | (((rgb) & kBlue5) >> 14))
/* Extract red, green, and blue bytes from RGB32 dword. */
#define R32(rgb)    (uint8_t)((rgb) & kRed8)
#define G32(rgb)    (uint8_t)((((rgb) & kGreen8) >> 8) & 0xff)
#define B32(rgb)    (uint8_t)((((rgb) & kBlue8) >> 16) & 0xff)
/* Build RGB565 word from red, green, and blue bytes. */
#define RGB565(r, g, b) (uint16_t)(((((uint16_t)(b) << 6) | (g)) << 5) | (r))
/* Build RGB32 dword from red, green, and blue bytes. */
#define RGB32(r, g, b) (uint32_t)(((((uint32_t)(b) << 8) | (g)) << 8) | (r))
#else   // !HOST_WORDS_BIGENDIAN
/* Extract red, green, and blue bytes from RGB565 word. */
#define R16(rgb)    (uint8_t)(((rgb) & kRed5) >> 11)
#define G16(rgb)    (uint8_t)(((rgb) & kGreen6) >> 5)
#define B16(rgb)    (uint8_t)((rgb) & kBlue5)
/* Make 8 bits red, green, and blue, extracted from RGB565 word. */
#define R16_32(rgb) (uint8_t)((((rgb) & kRed5) >> 8) | (((rgb) & kRed5) >> 14))
#define G16_32(rgb) (uint8_t)((((rgb) & kGreen6) >> 3) | (((rgb) & kGreen6) >> 9))
#define B16_32(rgb) (uint8_t)((((rgb) & kBlue5) << 3) | (((rgb) & kBlue5) >> 2))
/* Extract red, green, and blue bytes from RGB32 dword. */
#define R32(rgb)    (uint8_t)(((rgb) & kRed8) >> 16)
#define G32(rgb)    (uint8_t)(((rgb) & kGreen8) >> 8)
#define B32(rgb)    (uint8_t)((rgb) & kBlue8)
/* Build RGB565 word from red, green, and blue bytes. */
#define RGB565(r, g, b) (uint16_t)(((((uint16_t)(r) << 6) | (g)) << 5) | (b))
/* Build RGB32 dword from red, green, and blue bytes. */
#define RGB32(r, g, b) (uint32_t)(((((uint32_t)(r) << 8) | (g)) << 8) | (b))
#endif  // !HOST_WORDS_BIGENDIAN

/*
 * BAYER bitmasks
 */

/* Bitmask for 8-bits BAYER pixel. */
#define kBayer8     0xff
/* Bitmask for 10-bits BAYER pixel. */
#define kBayer10    0x3ff
/* Bitmask for 12-bits BAYER pixel. */
#define kBayer12    0xfff

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
clamp(int x) {
    return x - (x > 255) * (x - 255) - (x < 0) * x;
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

#if 0
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
#endif

/********************************************************************************
 * Basics of YUV -> RGB conversion.
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


#if 0
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
    rgb.r = YUV2RO(y,u,v);
    rgb.g = YUV2GO(y,u,v);
    rgb.b = YUV2BO(y,u,v);
    return rgb.color;
}
#endif

/* Converts YUV color to separated RGB32 colors. */
static __inline__ void
YUVToRGBPix(int y, int u, int v, uint8_t* r, uint8_t* g, uint8_t* b)
{
    /* Calculate C, D, and E values for the optimized macro. */
    y -= 16; u -= 128; v -= 128;
    *r = (uint8_t)YUV2RO(y,u,v);
    *g = (uint8_t)YUV2GO(y,u,v);
    *b = (uint8_t)YUV2BO(y,u,v);
}

/* Computes a luminance value after taking the exposure compensation.
 * value into account.
 *
 * Param:
 * inputY - The input luminance value.
 * Return:
 * The luminance value after adjusting the exposure compensation.
 */
static __inline__ uint8_t
_change_exposure20(uint8_t inputY, uint32_t exp_comp20)
{
    return clamp((inputY * exp_comp20) >> 20);
}

/* Adjusts an RGB pixel for the given exposure compensation. */
static __inline__ void
_change_exposure_RGB20(uint8_t* r, uint8_t* g, uint8_t* b, uint32_t exp_comp20)
{
    uint8_t y, u, v;
    R8G8B8ToYUV(*r, *g, *b, &y, &u, &v);
    YUVToRGBPix(_change_exposure20(y, exp_comp20), u, v, r, g, b);
}

/* Adjusts an RGB pixel for the given exposure compensation. */
static __inline__ void
_change_exposure_RGB_i20(int* r, int* g, int* b, uint32_t exp_comp20)
{
    uint8_t y, u, v;
    R8G8B8ToYUV(*r, *g, *b, &y, &u, &v);
    y = _change_exposure20(y, exp_comp20);
    *r = YUV2RO(y,u,v);
    *g = YUV2GO(y,u,v);
    *b = YUV2BO(y,u,v);
}

/* Computes the pixel value after adjusting the white balance to the current
 * one. The input the y, u, v channel of the pixel and the adjusted value will
 * be stored in place. The adjustment is done in RGB space.
 */
static __inline__ void
_change_white_balance_YUV(uint8_t* y,
                          uint8_t* u,
                          uint8_t* v,
                          float r_scale,
                          float g_scale,
                          float b_scale)
{
    int r = (float)(YUV2R((int)*y, (int)*u, (int)*v)) / r_scale;
    int g = (float)(YUV2G((int)*y, (int)*u, (int)*v)) / g_scale;
    int b = (float)(YUV2B((int)*y, (int)*u, (int)*v)) / b_scale;

    *y = RGB2Y(r, g, b);
    *u = RGB2U(r, g, b);
    *v = RGB2V(r, g, b);
}

/* Computes the pixel value after adjusting the white balance to the current
 * one. The input the r, and b channels of the pixel and the adjusted value will
 * be stored in place.
 */
static __inline__ void
_change_white_balance_RGB(int* r,
                          int* g,
                          int* b,
                          float r_scale,
                          float g_scale,
                          float b_scale)
{
    *r = (float)*r / r_scale;
    *g = (float)*g / g_scale;
    *b = (float)*b / b_scale;
}

/* Computes the pixel value after adjusting the white balance to the current
 * one. The input the r, and b channels of the pixel and the adjusted value will
 * be stored in place.
 */
static __inline__ void
_change_white_balance_RGB_b(uint8_t* r,
                            uint8_t* g,
                            uint8_t* b,
                            float r_scale,
                            float g_scale,
                            float b_scale)
{
    *r = (float)*r / r_scale;
    *g = (float)*g / g_scale;
    *b = (float)*b / b_scale;
}

/********************************************************************************
 * Generic converters between YUV and RGB formats
 *******************************************************************************/

/*
 * The converters go line by line, convering one frame format to another.
 * It's pretty much straight forward for RGB/BRG, where all colors are
 * grouped next to each other in memory. The only two things that differ one RGB
 * format from another are:
 * - Is it an RGB, or BRG (i.e. color ordering)
 * - Is it 16, 24, or 32 bits format.
 * All these differences are addressed by load_rgb / save_rgb routines, provided
 * for each format in the RGB descriptor to load / save RGB color bytes from / to
 * the buffer. As far as moving from one RGB pixel to the next, there
 * are two question to consider:
 * - How many bytes it takes to encode one RGB pixel (could be 2, 3, or 4)
 * - How many bytes it takes to encode a line (i.e. line alignment issue, which
 *   makes sence only for 24-bit formats, since 16, and 32 bit formats provide
 *   automatic word alignment.)
 * The first question is answered with the 'rgb_inc' field of the RGB descriptor,
 * and the second one is done by aligning rgb pointer up to the nearest 16 bit
 * boundaries at the end of each line.
 * YUV format has much greater complexity for conversion. in YUV color encoding
 * is divided into three separate panes that can be mixed together in any way
 * imaginable. Fortunately, there are still some consistent patterns in different

 * YUV formats that can be abstracted through a descriptor:
 * - At the line-by-line level, colors are always groupped aroud pairs of pixels,
 *   where each pixel in the pair has its own Y value, and each pair of pixels
 *   share thesame U, and V values.
 * - Position of Y, U, and V is the same for each pair, so the distance between
 *   Ys, and U/V for adjacent pairs is the same.
 * - Inside the pair, the distance between two Ys is always the same.

 * Moving between the lines in YUV can also be easily formalized. Essentially,
 * there are three ways how color panes are arranged:
 * 1. All interleaved, where all three Y, U, and V values are encoded together in
 *    one block:
 *               1,2  3,4  5,6      n,n+1
 *              YUVY YUVY YUVY .... YUVY
 *
 *    This type is used to encode YUV 4:2:2 formats.
 *
 * 2. One separate block of memory for Y pane, and one separate block of memory
 *    containing interleaved U, and V panes.
 *
 *      YY | YY | YY | YY
 *      YY | YY | YY | YY
 *      -----------------
 *      UV | UV | UV | UV
 *      -----------------
 *
 *    This type is used to encode 4:2:0 formats.
 *
 * 3. Three separate blocks of memory for each pane.
 *
 *      YY | YY | YY | YY
 *      YY | YY | YY | YY
 *      -----------------
 *      U  | U  | U  | U
 *      V  | V  | V  | V
 *      -----------------
 *
 *    This type is also used to encode 4:2:0 formats.
 *
 * Note that in cases 2, and 3 each pair of U and V is shared among four pixels,
 * grouped together as they are groupped in the framebeffer: divide the frame's
 * rectangle into 2x2 pixels squares, starting from 0,0 corner - and each square
 * represents the group of pixels that share same pair of UV values. So, each
 * line in the U/V panes table is shared between two adjacent lines in Y pane,
 * which provides a pattern on how to move between U/V lines as we move between
 * Y lines.
 *
 * So, all these patterns can be coded in a YUV format descriptor, so there can
 * just one generic way of walking YUV frame.
 *
 * BAYER format.
 * We don't use BAYER inside the guest system, so there is no need to have a
 * converter to the BAYER formats, only from it. The color approximation  used in
 * the BAYER format converters implemented here simply averages corresponded
 * color values found in pixels sorrounding the one for which RGB colors are
 * calculated.
 *
 * Performance considerations:
 * Since converters implemented here are intended to work as part of the camera
 * emulation, making the code super performant is not a priority at all. There
 * will be enough loses in other parts of the emultion to overlook any slight
 * inefficiences in the conversion algorithm as neglectable.
 */

typedef struct RGBDesc RGBDesc;
typedef struct YUVDesc YUVDesc;
typedef struct BayerDesc BayerDesc;

/* Prototype for a routine that loads RGB colors from an RGB/BRG stream.
 * Param:
 *  rgb - Pointer to a pixel inside the stream where to load colors from.
 *  r, g, b - Upon return will contain red, green, and blue colors for the pixel
 *      addressed by 'rgb' pointer.
 * Return:
 *  Pointer to the next pixel in the stream.
 */
typedef const void* (*load_rgb_func)(const void* rgb,
                                     uint8_t* r,
                                     uint8_t* g,
                                     uint8_t* b);

/* Prototype for a routine that saves RGB colors to an RGB/BRG stream.
 * Param:
 *  rgb - Pointer to a pixel inside the stream where to save colors.
 *  r, g, b - Red, green, and blue colors to save to the pixel addressed by
 *      'rgb' pointer.
 * Return:
 *  Pointer to the next pixel in the stream.
 */
typedef void* (*save_rgb_func)(void* rgb, uint8_t r, uint8_t g, uint8_t b);

/* Prototype for a routine that calculates an offset of the first Y, U or V
 * value for the given line in a YUV framebuffer.
 * Param:
 *  desc - Descriptor for the YUV frame for which the offset is being calculated.
 *  line - Zero-based line number for which to calculate the offset.
 *  width, height - Frame dimensions.
 * Return:
 *  Offset of the first Y, U or V value for the given frame line. The offset
 *  returned here is relative to the beginning of the YUV framebuffer.
 */
typedef int (*yuv_offset_func)(const YUVDesc* desc, int line, int width, int height);

/* RGB/BRG format descriptor. */
struct RGBDesc {
    /* Routine that loads RGB colors from a buffer. */
    load_rgb_func   load_rgb;
    /* Routine that saves RGB colors into a buffer. */
    save_rgb_func   save_rgb;
    /* Byte size of an encoded RGB pixel. */
    int             rgb_inc;
};

/* YUV format descriptor. */
struct YUVDesc {
    /* Offset of the first Y value in a fully interleaved YUV framebuffer. */
    int             Y_offset;
    /* Distance between two Y values inside a pair of pixels in a fully
     * interleaved YUV framebuffer. */
    int             Y_inc;
    /* Distance between first Y values of the adjacent pixel pairs in a fully
     * interleaved YUV framebuffer. */
    int             Y_next_pair;
    /* Increment between adjacent U/V values in a YUV framebuffer. */
    int             UV_inc;
    /* Controls location of the first U value in YUV framebuffer. Depending on
     * the actual YUV format can mean three things:
     *  - For fully interleaved YUV formats contains offset of the first U value
     *    in each line.
     *  - For YUV format that use separate, but interleaved UV pane, this field
     *    contains an offset of the first U value in the UV pane.
     *  - For YUV format that use fully separated Y, U, and V panes this field
     *    defines order of U and V panes in the framebuffer:
     *      = 1 - U pane comes first, right after Y pane.
     *      = 0 - U pane follows V pane that startes right after Y pane. */
    int             U_offset;
    /* Controls location of the first V value in YUV framebuffer.
     * See comments to U_offset for more info. */
    int             V_offset;
    /* For aligned YUV, defines the stride alignment in bytes, otherwise this
     * should be 0. */
    int             alignment;
    /* Routine that calculates an offset of the first Y value for the given line
     * in a YUV framebuffer. */
    yuv_offset_func y_offset;
    /* Routine that calculates an offset of the first U value for the given line
     * in a YUV framebuffer. */
    yuv_offset_func u_offset;
    /* Routine that calculates an offset of the first V value for the given line
     * in a YUV framebuffer. */
    yuv_offset_func v_offset;
};

/* Bayer format descriptor. */
struct BayerDesc {
    /* Defines color ordering in the BAYER framebuffer. Can be one of the four:
     *  - "GBRG" for GBGBGB / RGRGRG
     *  - "GRBG" for GRGRGR / BGBGBG
     *  - "RGGB" for RGRGRG / GBGBGB
     *  - "BGGR" for BGBGBG / GRGRGR
     */
    const char* color_order;
    /* Bitmask for valid bits in the pixel:
     *  - 0xff  For a 8-bit BAYER format
     *  - 0x3ff For a 10-bit BAYER format
     *  - 0xfff For a 12-bit BAYER format
     */
    int         mask;
};

/********************************************************************************
 * RGB/BRG load / save routines.
 *******************************************************************************/

/* Loads R, G, and B colors from a ARGB32 framebuffer. */
static const void*
_load_ARGB32(const void* rgb, uint8_t* r, uint8_t* g, uint8_t* b)
{
    const uint8_t* rgb_ptr = (const uint8_t*)rgb;
    *r = rgb_ptr[1]; *g = rgb_ptr[2]; *b = rgb_ptr[3];
    return rgb_ptr + 4;
}

/* Saves R, G, and B colors to a ARGB32 framebuffer. */
static void*
_save_ARGB32(void* rgb, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t* rgb_ptr = (uint8_t*)rgb;
    // For RGB32 we make the surface completely opaque with an alpha of 0xFF
    rgb_ptr[0] = 0xFF; rgb_ptr[1] = r; rgb_ptr[2] = g; rgb_ptr[3] = b;
    return rgb_ptr + 4;
}

/* Loads R, G, and B colors from a RGB32 framebuffer. */
static const void*
_load_RGB32(const void* rgb, uint8_t* r, uint8_t* g, uint8_t* b)
{
    const uint8_t* rgb_ptr = (const uint8_t*)rgb;
    *r = rgb_ptr[0]; *g = rgb_ptr[1]; *b = rgb_ptr[2];
    return rgb_ptr + 4;
}

/* Saves R, G, and B colors to a RGB32 framebuffer. */
static void*
_save_RGB32(void* rgb, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t* rgb_ptr = (uint8_t*)rgb;
    // For RGB32 we make the surface completely opaque with an alpha of 0xFF
    rgb_ptr[0] = r; rgb_ptr[1] = g; rgb_ptr[2] = b; rgb_ptr[3] = 0xFF;
    return rgb_ptr + 4;
}

/* Loads R, G, and B colors from a BRG32 framebuffer. */
static const void*
_load_BRG32(const void* rgb, uint8_t* r, uint8_t* g, uint8_t* b)
{
    const uint8_t* rgb_ptr = (const uint8_t*)rgb;
    *r = rgb_ptr[2]; *g = rgb_ptr[1]; *b = rgb_ptr[0];
    return rgb_ptr + 4;
}

/* Saves R, G, and B colors to a BRG32 framebuffer. */
static void*
_save_BRG32(void* rgb, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t* rgb_ptr = (uint8_t*)rgb;
    // For BRG32 we make the surface completely opaque with an alpha of 0xFF
    rgb_ptr[3] = 0xFF; rgb_ptr[2] = r; rgb_ptr[1] = g; rgb_ptr[0] = b;
    return rgb_ptr + 4;
}

/* Loads R, G, and B colors from a RGB24 framebuffer.
 * Note that it's the caller's responsibility to ensure proper alignment of the
 * returned pointer at the line's break. */
static const void*
_load_RGB24(const void* rgb, uint8_t* r, uint8_t* g, uint8_t* b)
{
    const uint8_t* rgb_ptr = (const uint8_t*)rgb;
    *r = rgb_ptr[0]; *g = rgb_ptr[1]; *b = rgb_ptr[2];
    return rgb_ptr + 3;
}

/* Saves R, G, and B colors to a RGB24 framebuffer.
 * Note that it's the caller's responsibility to ensure proper alignment of the
 * returned pointer at the line's break. */
static void*
_save_RGB24(void* rgb, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t* rgb_ptr = (uint8_t*)rgb;
    rgb_ptr[0] = r; rgb_ptr[1] = g; rgb_ptr[2] = b;
    return rgb_ptr + 3;
}

/* Loads R, G, and B colors from a BRG24 framebuffer.
 * Note that it's the caller's responsibility to ensure proper alignment of the
 * returned pointer at the line's break. */
static const void*
_load_BRG24(const void* rgb, uint8_t* r, uint8_t* g, uint8_t* b)
{
    const uint8_t* rgb_ptr = (const uint8_t*)rgb;
    *r = rgb_ptr[2]; *g = rgb_ptr[1]; *b = rgb_ptr[0];
    return rgb_ptr + 3;
}

/* Saves R, G, and B colors to a BRG24 framebuffer.
 * Note that it's the caller's responsibility to ensure proper alignment of the
 * returned pointer at the line's break. */
static void*
_save_BRG24(void* rgb, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t* rgb_ptr = (uint8_t*)rgb;
    rgb_ptr[2] = r; rgb_ptr[1] = g; rgb_ptr[0] = b;
    return rgb_ptr + 3;
}

/* Loads R, G, and B colors from a RGB565 framebuffer. */
static const void*
_load_RGB16(const void* rgb, uint8_t* r, uint8_t* g, uint8_t* b)
{
    const uint16_t rgb16 = *(const uint16_t*)rgb;
    *r = R16(rgb16); *g = G16(rgb16); *b = B16(rgb16);
    return (const uint8_t*)rgb + 2;
}

/* Saves R, G, and B colors to a RGB565 framebuffer. */
static void*
_save_RGB16(void* rgb, uint8_t r, uint8_t g, uint8_t b)
{
    *(uint16_t*)rgb = RGB565(r & 0x1f, g & 0x3f, b & 0x1f);
    return (uint8_t*)rgb + 2;
}

#if 0
/* Loads R, G, and B colors from a BRG565 framebuffer. */
static const void*
_load_BRG16(const void* rgb, uint8_t* r, uint8_t* g, uint8_t* b)
{
    const uint16_t rgb16 = *(const uint16_t*)rgb;
    *r = B16(rgb16); *g = G16(rgb16); *b = R16(rgb16);
    return (const uint8_t*)rgb + 2;
}

/* Saves R, G, and B colors to a BRG565 framebuffer. */
static void*
_save_BRG16(void* rgb, uint8_t r, uint8_t g, uint8_t b)
{
    *(uint16_t*)rgb = RGB565(b & 0x1f, g & 0x3f, r & 0x1f);
    return (uint8_t*)rgb + 2;
}
#endif

/********************************************************************************
 * YUV's Y/U/V offset calculation routines.
 *******************************************************************************/

/* Y offset in an aligned format such as YV12 */
static int YOffAlignedYUV(const YUVDesc* desc, int line, int width, int height)
{
    int stride = align(width, desc->alignment);
    // As the name implies the Y_next_pair value skips a pair of values. Since
    // this is counting values, not pairs, per line we divide by two.
    return line * stride * (desc->Y_next_pair / 2) + desc->Y_offset;
}

/* Y offset in a packed format with no padding or alignment requirements */
static int YOffPackedYUV(const YUVDesc* desc, int line, int width, int height)
{
    // As the name implies the Y_next_pair value skips a pair of values. Since
    // this is counting values, not pairs, per line we divide by two.
    return line * width * (desc->Y_next_pair / 2) + desc->Y_offset;
}

/* U offset in a fully interleaved YUV 4:2:2 */
static int
_UOffIntrlYUV(const YUVDesc* desc, int line, int width, int height)
{
    /* In interleaved YUV 4:2:2 each pair of pixels is encoded with 4 consecutive
     * bytes (or 2 bytes per pixel). So line size in a fully interleaved YUV 4:2:2
     * is twice its width. */
    return line * width * 2 + desc->U_offset;
}

/* V offset in a fully interleaved YUV 4:2:2 */
static int
_VOffIntrlYUV(const YUVDesc* desc, int line, int width, int height)
{
    /* See _UOffIntrlYUV comments. */
    return line * width * 2 + desc->V_offset;
}

/* U offset in an interleaved UV pane of YUV 4:2:0 */
static int
_UOffIntrlUV(const YUVDesc* desc, int line, int width, int height)
{
    /* UV pane starts right after the Y pane, that occupies 'height * width'
     * bytes. Eacht line in UV pane contains width / 2 'UV' pairs, which makes UV
     * lane to contain as many bytes, as the width is.
     * Each line in the UV pane is shared between two Y lines. So, final formula
     * for the beggining of the UV pane's line for the given line in YUV
     * framebuffer is:
     *
     *    height * width + (line / 2) * width = (height + line / 2) * width
     */
    return (height + line / 2) * width + desc->U_offset;
}

/* V offset in an interleaved UV pane of YUV 4:2:0 */
static int
_VOffIntrlUV(const YUVDesc* desc, int line, int width, int height)
{
    /* See comments in _UOffIntrlUV. */
    return (height + line / 2) * width + desc->V_offset;
}

/* U offset in a 3-pane aligned YUV 4:2:0 */
static int
_UOffSepAlignedYUV(const YUVDesc* desc, int line, int width, int height)
{
    /* U, or V pane starts right after the Y pane, that occupies 'height * width'
     * bytes. Eacht line in each of U and V panes contains width / 2 elements.
     * Also, each line in each of U and V panes is shared between two Y lines.
     * So, final formula for the beggining of a line in the U/V pane is:
     *
     *    <Y pane size> + (line / 2) * width / 2
     *
     * for the pane that follows right after the Y pane, or
     *
     *    <Y pane size> + <Y pane size> / 4 + (line / 2) * width / 2
     *
     * for the second pane.
     */
    const int y_stride = align(width, desc->alignment);
    const int uv_stride = align(y_stride / 2, desc->alignment);
    const int y_pane_size = height * y_stride;
    if (desc->U_offset) {
        /* U pane comes right after the Y pane. */
        return y_pane_size + (line / 2) * uv_stride;
    } else {
        /* U pane follows V pane. */
        return y_pane_size + (height / 2 + line / 2) * uv_stride;
    }
}

/* V offset in a 3-pane aligned YUV 4:2:0 */
static int
_VOffSepAlignedYUV(const YUVDesc* desc, int line, int width, int height)
{
    /* See comment for _UOffSepYUV. */
    const int y_stride = align(width, desc->alignment);
    const int uv_stride = align(y_stride / 2, desc->alignment);
    const int y_pane_size = height * y_stride;
    if (desc->V_offset) {
        /* V pane comes right after the Y pane. */
        return y_pane_size + (line / 2) * uv_stride;
    } else {
        /* V pane follows U pane. */
        return y_pane_size + (height / 2 + line / 2) * uv_stride;
    }
}

/********************************************************************************
 * Bayer routines.
 *******************************************************************************/

/* Gets a color value for the given pixel in a bayer framebuffer.
 * Param:
 *  desc - Bayer framebuffer descriptor.
 *  buf - Beginning of the framebuffer.
 *  x, y - Coordinates of the pixel inside the framebuffer to get the color for.
 *  width - Number of pixel in a line inside the framebuffer.
 * Return:
 *  Given pixel color.
 */
static __inline__ int
_get_bayer_color(const BayerDesc* desc, const void* buf, int x, int y, int width)
{
    if (desc->mask == kBayer8) {
        /* Each pixel is represented with one byte. */
        return *((const uint8_t*)buf + y * width + x);
    } else {
#ifndef HOST_WORDS_BIGENDIAN
        return *((const int16_t*)buf + y * width + x) & desc->mask;
#else
        const uint8_t* pixel = (const uint8_t*)buf + (y * width + x) * 2;
        return (((uint16_t)pixel[1] << 8) | pixel[0]) & desc->mask;
#endif  /* !HOST_WORDS_BIGENDIAN */
    }
}

/* Gets an average value of colors that are horisontally adjacent to a pixel in
 * a bayer framebuffer.
 * Param:
 *  desc - Bayer framebuffer descriptor.
 *  buf - Beginning of the framebuffer.
 *  x, y - Coordinates of the pixel inside the framebuffer that is the center for
 *      the calculation.
 *  width, height - Framebuffer dimensions.
 * Return:
 *  Average color for horisontally adjacent pixels.
 */
static int
_get_bayer_ave_hor(const BayerDesc* desc,
                   const void* buf,
                   int x,
                   int y,
                   int width,
                   int height)
{
    if (x == 0) {
        return _get_bayer_color(desc, buf, x + 1, y, width);
    } else if (x == (width - 1)) {
        return _get_bayer_color(desc, buf, x - 1, y, width);
    } else {
        return (_get_bayer_color(desc, buf, x - 1, y, width) +
                _get_bayer_color(desc, buf, x + 1, y, width)) / 2;
    }
}

/* Gets an average value of colors that are vertically adjacent to a pixel in
 * a bayer framebuffer.
 * Param:
 *  desc - Bayer framebuffer descriptor.
 *  buf - Beginning of the framebuffer.
 *  x, y - Coordinates of the pixel inside the framebuffer that is the center for
 *      the calculation.
 *  width, height - Framebuffer dimensions.
 * Return:
 *  Average color for vertically adjacent pixels.
 */
static int
_get_bayer_ave_vert(const BayerDesc* desc,
                    const void* buf,
                    int x,
                    int y,
                    int width,
                    int height)
{
    if (y == 0) {
        return _get_bayer_color(desc, buf, x, y + 1, width);
    } else if (y == (height - 1)) {
        return _get_bayer_color(desc, buf, x, y - 1, width);
    } else {
        return (_get_bayer_color(desc, buf, x, y - 1, width) +
                _get_bayer_color(desc, buf, x, y + 1, width)) / 2;
    }
}

/* Gets an average value of colors that are horisontally and vertically adjacent
 * to a pixel in a bayer framebuffer.
 * Param:
 *  desc - Bayer framebuffer descriptor.
 *  buf - Beginning of the framebuffer.
 *  x, y - Coordinates of the pixel inside the framebuffer that is the center for
 *      the calculation.
 *  width, height - Framebuffer dimensions.
 * Return:
 *  Average color for horisontally and vertically adjacent pixels.
 */
static int
_get_bayer_ave_cross(const BayerDesc* desc,
                     const void* buf,
                     int x,
                     int y,
                     int width,
                     int height)
{
    if (x > 0 && x < (width - 1) && y > 0 && y < (height - 1)) {
        /* Most of the time the code will take this path. So it makes sence to
         * special case it for performance reasons. */
        return (_get_bayer_color(desc, buf, x - 1, y, width) +
                _get_bayer_color(desc, buf, x + 1, y, width) +
                _get_bayer_color(desc, buf, x, y - 1, width) +
                _get_bayer_color(desc, buf, x, y + 1, width)) / 4;
    } else {
        int sum = 0;
        int num = 0;

        /* Horisontal sum */
        if (x == 0) {
            sum += _get_bayer_color(desc, buf, x + 1, y, width);
            num++;
        } else if (x == (width - 1)) {
            sum += _get_bayer_color(desc, buf, x - 1, y, width);
            num++;
        } else {
            sum += _get_bayer_color(desc, buf, x - 1, y, width) +
                   _get_bayer_color(desc, buf, x + 1, y, width);
            num += 2;
        }

        /* Vertical sum */
        if (y == 0) {
            sum += _get_bayer_color(desc, buf, x, y + 1, width);
            num++;
        } else if (y == (height - 1)) {
            sum += _get_bayer_color(desc, buf, x, y - 1, width);
            num++;
        } else {
            sum += _get_bayer_color(desc, buf, x, y - 1, width) +
                   _get_bayer_color(desc, buf, x, y + 1, width);
            num += 2;
        }

        return sum / num;
    }
}

/* Gets an average value of colors that are diagonally adjacent to a pixel in a
 * bayer framebuffer.
 * Param:
 *  desc - Bayer framebuffer descriptor.
 *  buf - Beginning of the framebuffer.
 *  x, y - Coordinates of the pixel inside the framebuffer that is the center for
 *      the calculation.
 *  width, height - Framebuffer dimensions.
 * Return:
 *  Average color for diagonally adjacent pixels.
 */
static int
_get_bayer_ave_diag(const BayerDesc* desc,
                    const void* buf,
                    int x,
                    int y,
                    int width,
                    int height)
{
    if (x > 0 && x < (width - 1) && y > 0 && y < (height - 1)) {
        /* Most of the time the code will take this path. So it makes sence to
         * special case it for performance reasons. */
        return (_get_bayer_color(desc, buf, x - 1, y - 1, width) +
                _get_bayer_color(desc, buf, x + 1, y - 1, width) +
                _get_bayer_color(desc, buf, x - 1, y + 1, width) +
                _get_bayer_color(desc, buf, x + 1, y + 1, width)) / 4;
    } else {
        int sum = 0;
        int num = 0;
        int xx, yy;
        for (xx = x - 1; xx < (x + 2); xx += 2) {
            for (yy = y - 1; yy < (y + 2); yy += 2) {
                if (xx >= 0 && yy >= 0 && xx < width && yy < height) {
                    sum += _get_bayer_color(desc, buf, xx, yy, width);
                    num++;
                }
            }
        }
        return sum / num;
    }
}

/* Gets pixel color selector for the given pixel in a bayer framebuffer.
 * Param:
 *  desc - Bayer framebuffer descriptor.
 *  x, y - Coordinates of the pixel inside the framebuffer to get the color
 *      selector for.
 * Return:
 *  Pixel color selector:
 *      - 'R' - pixel is red.
 *      - 'G' - pixel is green.
 *      - 'B' - pixel is blue.
 */
static __inline__ char
_get_bayer_color_sel(const BayerDesc* desc, int x, int y)
{
    return desc->color_order[((y & 1) << 1) | (x & 1)];
}

/* Calculates RGB colors for a pixel in a bayer framebuffer.
 * Param:
 *  desc - Bayer framebuffer descriptor.
 *  buf - Beginning of the framebuffer.
 *  x, y - Coordinates of the pixel inside the framebuffer to get the colors for.
 *  width, height - Framebuffer dimensions.
 *  red, green bluu - Upon return will contain RGB colors calculated for the pixel.
 */
static void
_get_bayerRGB(const BayerDesc* desc,
              const void* buf,
              int x,
              int y,
              int width,
              int height,
              int* red,
              int* green,
              int* blue)
{
    const char pixel_color = _get_bayer_color_sel(desc, x, y);

    if (pixel_color == 'G') {
        /* This is a green pixel. */
        const char next_pixel_color = _get_bayer_color_sel(desc, x + 1, y);
        *green = _get_bayer_color(desc, buf, x, y, width);
        if (next_pixel_color == 'R') {
            *red = _get_bayer_ave_hor(desc, buf, x, y, width, height);
            *blue = _get_bayer_ave_vert(desc, buf, x, y, width, height);
        } else {
            *red = _get_bayer_ave_vert(desc, buf, x, y, width, height);
            *blue = _get_bayer_ave_hor(desc, buf, x, y, width, height);
        }
    } else if (pixel_color == 'R') {
        /* This is a red pixel. */
        *red = _get_bayer_color(desc, buf, x, y, width);
        *green = _get_bayer_ave_cross(desc, buf, x, y, width, height);
        *blue = _get_bayer_ave_diag(desc, buf, x, y, width, height);
    } else {
        /* This is a blue pixel. */
        *blue = _get_bayer_color(desc, buf, x, y, width);
        *green = _get_bayer_ave_cross(desc, buf, x, y, width, height);
        *red = _get_bayer_ave_diag(desc, buf, x, y, width, height);
    }
}

/********************************************************************************
 * Generic YUV/RGB/BAYER converters
 *******************************************************************************/

/* Generic converter from an RGB/BRG format to a YUV format. */
static void
RGBToYUV(const RGBDesc* rgb_fmt,
         const YUVDesc* yuv_fmt,
         const void* rgb,
         void* yuv,
         int width,
         int height,
         float r_scale,
         float g_scale,
         float b_scale,
         float exp_comp)
{
    const uint32_t exp_comp20 = exp_comp * (1u << 20);
    int y, x;
    const int Y_Inc = yuv_fmt->Y_inc;
    const int UV_inc = yuv_fmt->UV_inc;
    const int Y_next_pair = yuv_fmt->Y_next_pair;
    for (y = 0; y < height; y++) {
        uint8_t* pY =
            (uint8_t*)yuv + yuv_fmt->y_offset(yuv_fmt, y, width, height);
        uint8_t* pU =
            (uint8_t*)yuv + yuv_fmt->u_offset(yuv_fmt, y, width, height);
        uint8_t* pV =
            (uint8_t*)yuv + yuv_fmt->v_offset(yuv_fmt, y, width, height);
        for (x = 0; x < width; x += 2,
                               pY += Y_next_pair, pU += UV_inc, pV += UV_inc) {
            uint8_t r, g, b;
            rgb = rgb_fmt->load_rgb(rgb, &r, &g, &b);
            _change_white_balance_RGB_b(&r, &g, &b, r_scale, g_scale, b_scale);
            R8G8B8ToYUV(r, g, b, pY, pU, pV);
            *pY = _change_exposure20(*pY, exp_comp20);
            rgb = rgb_fmt->load_rgb(rgb, &r, &g, &b);
            _change_white_balance_RGB_b(&r, &g, &b, r_scale, g_scale, b_scale);
            pY[Y_Inc] =
                    _change_exposure20(RGB2Y((int)r, (int)g, (int)b), exp_comp20);
        }
        /* Aling rgb_ptr to 16 bit */
        if (((uintptr_t)rgb & 1) != 0) rgb = (const uint8_t*)rgb + 1;
    }
}

/* Generic converter from one RGB/BRG format to another RGB/BRG format. */
static void
RGBToRGB(const RGBDesc* src_rgb_fmt,
         const RGBDesc* dst_rgb_fmt,
         const void* src_rgb,
         void* dst_rgb,
         int width,
         int height,
         float r_scale,
         float g_scale,
         float b_scale,
         float exp_comp)
{
    const uint32_t exp_comp20 = exp_comp * (1u << 20);
    int x, y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            uint8_t r, g, b;
            src_rgb = src_rgb_fmt->load_rgb(src_rgb, &r, &g, &b);
            _change_white_balance_RGB_b(&r, &g, &b, r_scale, g_scale, b_scale);
            _change_exposure_RGB20(&r, &g, &b, exp_comp20);
            dst_rgb = dst_rgb_fmt->save_rgb(dst_rgb, r, g, b);
        }
        /* Aling rgb pinters to 16 bit */
        if (((uintptr_t)src_rgb & 1) != 0) src_rgb = (uint8_t*)src_rgb + 1;
        if (((uintptr_t)dst_rgb & 1) != 0) dst_rgb = (uint8_t*)dst_rgb + 1;
    }
}

/* Generic converter from a YUV format to an RGB/BRG format. */
static void
YUVToRGB(const YUVDesc* yuv_fmt,
         const RGBDesc* rgb_fmt,
         const void* yuv,
         void* rgb,
         int width,
         int height,
         float r_scale,
         float g_scale,
         float b_scale,
         float exp_comp)
{
    const uint32_t exp_comp20 = exp_comp * (1u << 20);
    int y, x;
    const int Y_Inc = yuv_fmt->Y_inc;
    const int UV_inc = yuv_fmt->UV_inc;
    const int Y_next_pair = yuv_fmt->Y_next_pair;
    for (y = 0; y < height; y++) {
        const uint8_t* pY =
            (const uint8_t*)yuv + yuv_fmt->y_offset(yuv_fmt, y, width, height);
        const uint8_t* pU =
            (const uint8_t*)yuv + yuv_fmt->u_offset(yuv_fmt, y, width, height);
        const uint8_t* pV =
            (const uint8_t*)yuv + yuv_fmt->v_offset(yuv_fmt, y, width, height);
        for (x = 0; x < width; x += 2,
                               pY += Y_next_pair, pU += UV_inc, pV += UV_inc) {
            uint8_t r, g, b;
            const uint8_t U = *pU;
            const uint8_t V = *pV;
            YUVToRGBPix(*pY, U, V, &r, &g, &b);
            _change_white_balance_RGB_b(&r, &g, &b, r_scale, g_scale, b_scale);
            _change_exposure_RGB20(&r, &g, &b, exp_comp20);
            rgb = rgb_fmt->save_rgb(rgb, r, g, b);
            YUVToRGBPix(pY[Y_Inc], U, V, &r, &g, &b);
            _change_white_balance_RGB_b(&r, &g, &b, r_scale, g_scale, b_scale);
            _change_exposure_RGB20(&r, &g, &b, exp_comp20);
            rgb = rgb_fmt->save_rgb(rgb, r, g, b);
        }
        /* Aling rgb_ptr to 16 bit */
        if (((uintptr_t)rgb & 1) != 0) rgb = (uint8_t*)rgb + 1;
    }
}

/* Generic converter from one YUV format to another YUV format. */
static void
YUVToYUV(const YUVDesc* src_fmt,
         const YUVDesc* dst_fmt,
         const void* src,
         void* dst,
         int width,
         int height,
         float r_scale,
         float g_scale,
         float b_scale,
         float exp_comp)
{
    const uint32_t exp_comp20 = exp_comp * (1u << 20);
    int y, x;
    const int Y_Inc_src = src_fmt->Y_inc;
    const int UV_inc_src = src_fmt->UV_inc;
    const int Y_next_pair_src = src_fmt->Y_next_pair;
    const int Y_Inc_dst = dst_fmt->Y_inc;
    const int UV_inc_dst = dst_fmt->UV_inc;
    const int Y_next_pair_dst = dst_fmt->Y_next_pair;
    for (y = 0; y < height; y++) {
        const uint8_t* pYsrc =
            (const uint8_t*)src + src_fmt->y_offset(src_fmt, y, width, height);
        const uint8_t* pUsrc =
            (const uint8_t*)src + src_fmt->u_offset(src_fmt, y, width, height);
        const uint8_t* pVsrc =
            (const uint8_t*)src + src_fmt->v_offset(src_fmt, y, width, height);
        uint8_t* pYdst =
            (uint8_t*)dst + dst_fmt->y_offset(dst_fmt, y, width, height);
        uint8_t* pUdst =
            (uint8_t*)dst + dst_fmt->u_offset(dst_fmt, y, width, height);
        uint8_t* pVdst =
            (uint8_t*)dst + dst_fmt->v_offset(dst_fmt, y, width, height);
        for (x = 0; x < width; x += 2, pYsrc += Y_next_pair_src,
                                       pUsrc += UV_inc_src,
                                       pVsrc += UV_inc_src,
                                       pYdst += Y_next_pair_dst,
                                       pUdst += UV_inc_dst,
                                       pVdst += UV_inc_dst) {
            *pYdst = *pYsrc; *pUdst = *pUsrc; *pVdst = *pVsrc;
            _change_white_balance_YUV(pYdst, pUdst, pVdst, r_scale, g_scale, b_scale);
            *pYdst = _change_exposure20(*pYdst, exp_comp20);
            pYdst[Y_Inc_dst] = _change_exposure20(pYsrc[Y_Inc_src], exp_comp20);
        }
    }
}

/* Generic converter from a BAYER format to an RGB/BRG format. */
static void
BAYERToRGB(const BayerDesc* bayer_fmt,
           const RGBDesc* rgb_fmt,
           const void* bayer,
           void* rgb,
           int width,
           int height,
           float r_scale,
           float g_scale,
           float b_scale,
           float exp_comp)
{
    const uint32_t exp_comp20 = exp_comp * (1u << 20);
    int y, x;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            int r, g, b;
            _get_bayerRGB(bayer_fmt, bayer, x, y, width, height, &r, &g, &b);
            if (bayer_fmt->mask == kBayer10) {
                r >>= 2; g >>= 2; b >>= 2;
            } else if (bayer_fmt->mask == kBayer12) {
                r >>= 4; g >>= 4; b >>= 4;
            }
            _change_white_balance_RGB(&r, &g, &b, r_scale, g_scale, b_scale);
            _change_exposure_RGB_i20(&r, &g, &b, exp_comp20);
            rgb = rgb_fmt->save_rgb(rgb, r, g, b);
        }
        /* Aling rgb_ptr to 16 bit */
        if (((uintptr_t)rgb & 1) != 0) rgb = (uint8_t*)rgb + 1;
    }
}

/* Generic converter from a BAYER format to a YUV format. */
static void
BAYERToYUV(const BayerDesc* bayer_fmt,
           const YUVDesc* yuv_fmt,
           const void* bayer,
           void* yuv,
           int width,
           int height,
           float r_scale,
           float g_scale,
           float b_scale,
           float exp_comp)
{
    const uint32_t exp_comp20 = exp_comp * (1u << 20);
    int y, x;
    const int Y_Inc = yuv_fmt->Y_inc;
    const int UV_inc = yuv_fmt->UV_inc;
    const int Y_next_pair = yuv_fmt->Y_next_pair;
    for (y = 0; y < height; y++) {
        uint8_t* pY =
            (uint8_t*)yuv + yuv_fmt->y_offset(yuv_fmt, y, width, height);
        uint8_t* pU =
            (uint8_t*)yuv + yuv_fmt->u_offset(yuv_fmt, y, width, height);
        uint8_t* pV =
            (uint8_t*)yuv + yuv_fmt->v_offset(yuv_fmt, y, width, height);
        for (x = 0; x < width; x += 2,
                               pY += Y_next_pair, pU += UV_inc, pV += UV_inc) {
            int r, g, b;
            _get_bayerRGB(bayer_fmt, bayer, x, y, width, height, &r, &g, &b);
            _change_white_balance_RGB(&r, &g, &b, r_scale, g_scale, b_scale);
            _change_exposure_RGB_i20(&r, &g, &b, exp_comp20);
            R8G8B8ToYUV(r, g, b, pY, pU, pV);
            _get_bayerRGB(bayer_fmt, bayer, x + 1, y, width, height, &r, &g, &b);
            _change_white_balance_RGB(&r, &g, &b, r_scale, g_scale, b_scale);
            _change_exposure_RGB_i20(&r, &g, &b, exp_comp20);
            pY[Y_Inc] = RGB2Y(r, g, b);
        }
    }
}

/********************************************************************************
 * RGB format descriptors.
 */

/* Describes RGB32 format. */
static const RGBDesc _ARGB32 =
{
    .load_rgb   = _load_ARGB32,
    .save_rgb   = _save_ARGB32,
    .rgb_inc    = 4
};

/* Describes RGB32 format. */
static const RGBDesc _RGB32 =
{
    .load_rgb   = _load_RGB32,
    .save_rgb   = _save_RGB32,
    .rgb_inc    = 4
};

/* Describes BRG32 format. */
static const RGBDesc _BRG32 =
{
    .load_rgb   = _load_BRG32,
    .save_rgb   = _save_BRG32,
    .rgb_inc    = 4
};

/* Describes RGB24 format. */
static const RGBDesc _RGB24 =
{
    .load_rgb   = _load_RGB24,
    .save_rgb   = _save_RGB24,
    .rgb_inc    = 3
};

/* Describes BRG24 format. */
static const RGBDesc _BRG24 =
{
    .load_rgb   = _load_BRG24,
    .save_rgb   = _save_BRG24,
    .rgb_inc    = 3
};

/* Describes RGB16 format. */
static const RGBDesc _RGB16 =
{
    .load_rgb   = _load_RGB16,
    .save_rgb   = _save_RGB16,
    .rgb_inc    = 2
};

#if 0
/* Describes BRG16 format. */
static const RGBDesc _BRG16 =
{
    .load_rgb   = _load_BRG16,
    .save_rgb   = _save_BRG16,
    .rgb_inc    = 2
};
#endif

/********************************************************************************
 * YUV 4:2:2 format descriptors.
 */

/* YUYV  (also YUY2, YUNV, V422): 4:2:2, YUV are interleaved. */
static const YUVDesc _YUYV =
{
    .Y_offset       = 0,
    .Y_inc          = 2,
    .Y_next_pair    = 4,
    .UV_inc         = 4,
    .U_offset       = 1,
    .V_offset       = 3,
    .y_offset       = &YOffPackedYUV,
    .u_offset       = &_UOffIntrlYUV,
    .v_offset       = &_VOffIntrlYUV
};

/* UYVY: 4:2:2, YUV are interleaved. */
static const YUVDesc _UYVY =
{
    .Y_offset       = 1,
    .Y_inc          = 2,
    .Y_next_pair    = 4,
    .UV_inc         = 4,
    .U_offset       = 0,
    .V_offset       = 2,
    .y_offset       = &YOffPackedYUV,
    .u_offset       = &_UOffIntrlYUV,
    .v_offset       = &_VOffIntrlYUV
};

/* YVYU: 4:2:2, YUV are interleaved. */
static const YUVDesc _YVYU =
{
    .Y_offset       = 0,
    .Y_inc          = 2,
    .Y_next_pair    = 4,
    .UV_inc         = 4,
    .U_offset       = 3,
    .V_offset       = 1,
    .y_offset       = &YOffPackedYUV,
    .u_offset       = &_UOffIntrlYUV,
    .v_offset       = &_VOffIntrlYUV
};

/* VYUY: 4:2:2, YUV are interleaved. */
static const YUVDesc _VYUY =
{
    .Y_offset       = 1,
    .Y_inc          = 2,
    .Y_next_pair    = 4,
    .UV_inc         = 4,
    .U_offset       = 2,
    .V_offset       = 0,
    .y_offset       = &YOffPackedYUV,
    .u_offset       = &_UOffIntrlYUV,
    .v_offset       = &_VOffIntrlYUV
};

/* YYUV: 4:2:2, YUV are interleaved. */
static const YUVDesc _YYUV =
{
    .Y_offset       = 0,
    .Y_inc          = 1,
    .Y_next_pair    = 4,
    .UV_inc         = 4,
    .U_offset       = 2,
    .V_offset       = 3,
    .y_offset       = &YOffPackedYUV,
    .u_offset       = &_UOffIntrlYUV,
    .v_offset       = &_VOffIntrlYUV
};

/* YYVU: 4:2:2, YUV are interleaved. */
static const YUVDesc _YYVU =
{
    .Y_offset       = 0,
    .Y_inc          = 1,
    .Y_next_pair    = 4,
    .UV_inc         = 4,
    .U_offset       = 3,
    .V_offset       = 2,
    .y_offset       = &YOffPackedYUV,
    .u_offset       = &_UOffIntrlYUV,
    .v_offset       = &_VOffIntrlYUV
};

/********************************************************************************
 * YUV 4:2:0 descriptors.
 */

/* YV12: 4:2:0, YUV are fully separated, U pane follows V pane */
static const YUVDesc _YV12 =
{
    .Y_offset       = 0,
    .Y_inc          = 1,
    .Y_next_pair    = 2,
    .UV_inc         = 1,
    .U_offset       = 0,
    .V_offset       = 1,
    .alignment      = 16,
    .y_offset       = &YOffAlignedYUV,
    .u_offset       = &_UOffSepAlignedYUV,
    .v_offset       = &_VOffSepAlignedYUV
};

/* YU12: 4:2:0, YUV are fully separated, V pane follows U pane */
static const YUVDesc _YU12 =
{
    .Y_offset       = 0,
    .Y_inc          = 1,
    .Y_next_pair    = 2,
    .UV_inc         = 1,
    .U_offset       = 1,
    .V_offset       = 0,
    .alignment      = 16,
    .y_offset       = &YOffAlignedYUV,
    .u_offset       = &_UOffSepAlignedYUV,
    .v_offset       = &_VOffSepAlignedYUV
};

/* NV12: 4:2:0, UV are interleaved, V follows U in UV pane */
static const YUVDesc _NV12 =
{
    .Y_offset       = 0,
    .Y_inc          = 1,
    .Y_next_pair    = 2,
    .UV_inc         = 2,
    .U_offset       = 0,
    .V_offset       = 1,
    .y_offset       = &YOffPackedYUV,
    .u_offset       = &_UOffIntrlUV,
    .v_offset       = &_VOffIntrlUV
};

/* NV21: 4:2:0, UV are interleaved, U follows V in UV pane */
static const YUVDesc _NV21 =
{
    .Y_offset       = 0,
    .Y_inc          = 1,
    .Y_next_pair    = 2,
    .UV_inc         = 2,
    .U_offset       = 1,
    .V_offset       = 0,
    .y_offset       = &YOffPackedYUV,
    .u_offset       = &_UOffIntrlUV,
    .v_offset       = &_VOffIntrlUV
};

/********************************************************************************
 * RGB bayer format descriptors.
 */

/* Descriptor for a 8-bit GBGB / RGRG format. */
static const BayerDesc _GB8 =
{
    .mask           = kBayer8,
    .color_order    = "GBRG"
};

/* Descriptor for a 8-bit GRGR / BGBG format. */
static const BayerDesc _GR8 =
{
    .mask           = kBayer8,
    .color_order    = "GRBG"
};

/* Descriptor for a 8-bit BGBG / GRGR format. */
static const BayerDesc _BG8 =
{
    .mask           = kBayer8,
    .color_order    = "BGGR"
};

/* Descriptor for a 8-bit RGRG / GBGB format. */
static const BayerDesc _RG8 =
{
    .mask           = kBayer8,
    .color_order    = "RGGB"
};

/* Descriptor for a 10-bit GBGB / RGRG format. */
static const BayerDesc _GB10 =
{
    .mask           = kBayer10,
    .color_order    = "GBRG"
};

/* Descriptor for a 10-bit GRGR / BGBG format. */
static const BayerDesc _GR10 =
{
    .mask           = kBayer10,
    .color_order    = "GRBG"
};

/* Descriptor for a 10-bit BGBG / GRGR format. */
static const BayerDesc _BG10 =
{
    .mask           = kBayer10,
    .color_order    = "BGGR"
};

/* Descriptor for a 10-bit RGRG / GBGB format. */
static const BayerDesc _RG10 =
{
    .mask           = kBayer10,
    .color_order    = "RGGB"
};

/* Descriptor for a 12-bit GBGB / RGRG format. */
static const BayerDesc _GB12 =
{
    .mask           = kBayer12,
    .color_order    = "GBRG"
};

/* Descriptor for a 12-bit GRGR / BGBG format. */
static const BayerDesc _GR12 =
{
    .mask           = kBayer12,
    .color_order    = "GRBG"
};

/* Descriptor for a 12-bit BGBG / GRGR format. */
static const BayerDesc _BG12 =
{
    .mask           = kBayer12,
    .color_order    = "BGGR"
};

/* Descriptor for a 12-bit RGRG / GBGB format. */
static const BayerDesc _RG12 =
{
    .mask           = kBayer12,
    .color_order    = "RGGB"
};


/********************************************************************************
 * List of descriptors for supported formats.
 *******************************************************************************/

/* Enumerates pixel formats supported by converters. */
typedef enum PIXFormatSel {
    /* Pixel format is RGB/BGR */
    PIX_FMT_RGB,
    /* Pixel format is YUV */
    PIX_FMT_YUV,
    /* Pixel format is BAYER */
    PIX_FMT_BAYER
} PIXFormatSel;

/* Formats entry in the list of descriptors for supported formats. */
typedef struct PIXFormat {
    /* "FOURCC" (V4L2_PIX_FMT_XXX) format type. */
    uint32_t        fourcc_type;
    /* RGB/YUV/BAYER format selector */
    PIXFormatSel    format_sel;
    /* Bits per pixel */
    size_t bits_per_pixel;
    union {
        /* References RGB format descriptor for that format. */
        const RGBDesc*      rgb_desc;
        /* References YUV format descriptor for that format. */
        const YUVDesc*      yuv_desc;
        /* References BAYER format descriptor for that format. */
        const BayerDesc*    bayer_desc;
    } desc;
} PIXFormat;

/* Array of supported pixel format descriptors, if changed consider updating
 * CameraFormatConverters_unittest.cpp. */
static const PIXFormat _PIXFormats[] = {
        /* RGB/BRG formats. */
        {V4L2_PIX_FMT_ARGB32, PIX_FMT_RGB, 32, .desc.rgb_desc = &_ARGB32},
        {V4L2_PIX_FMT_RGB32, PIX_FMT_RGB, 32, .desc.rgb_desc = &_RGB32},
        {V4L2_PIX_FMT_BGR32, PIX_FMT_RGB, 32, .desc.rgb_desc = &_BRG32},
        {V4L2_PIX_FMT_RGB565, PIX_FMT_RGB, 16, .desc.rgb_desc = &_RGB16},
        {V4L2_PIX_FMT_RGB24, PIX_FMT_RGB, 24, .desc.rgb_desc = &_RGB24},
        {V4L2_PIX_FMT_BGR24, PIX_FMT_RGB, 24, .desc.rgb_desc = &_BRG24},

        /* YUV 4:2:0 formats. */
        {V4L2_PIX_FMT_YVU420, PIX_FMT_YUV, 12, .desc.yuv_desc = &_YV12},
        {V4L2_PIX_FMT_YUV420, PIX_FMT_YUV, 12, .desc.yuv_desc = &_YU12},
        {V4L2_PIX_FMT_NV12, PIX_FMT_YUV, 12, .desc.yuv_desc = &_NV12},
        {V4L2_PIX_FMT_NV21, PIX_FMT_YUV, 12, .desc.yuv_desc = &_NV21},

        /* YUV 4:2:2 formats. */
        {V4L2_PIX_FMT_YUYV, PIX_FMT_YUV, 16, .desc.yuv_desc = &_YUYV},
        {V4L2_PIX_FMT_YYUV, PIX_FMT_YUV, 16, .desc.yuv_desc = &_YYUV},
        // TODO: V4L2_PIX_FMT_YVYU is curious, it only exists in this codebase.
        {V4L2_PIX_FMT_YVYU, PIX_FMT_YUV, 16, .desc.yuv_desc = &_YVYU},
        {V4L2_PIX_FMT_UYVY, PIX_FMT_YUV, 16, .desc.yuv_desc = &_UYVY},
        {V4L2_PIX_FMT_VYUY, PIX_FMT_YUV, 16, .desc.yuv_desc = &_VYUY},
        // TODO: V4L2_PIX_FMT_YYVU is curious, it only exists in this codebase.
        {V4L2_PIX_FMT_YYVU, PIX_FMT_YUV, 16, .desc.yuv_desc = &_YYVU},
        // Aliases for V4L2_PIX_FMT_YUYV.
        {V4L2_PIX_FMT_YUY2, PIX_FMT_YUV, 16, .desc.yuv_desc = &_YUYV},
        {V4L2_PIX_FMT_YUNV, PIX_FMT_YUV, 16, .desc.yuv_desc = &_YUYV},
        {V4L2_PIX_FMT_V422, PIX_FMT_YUV, 16, .desc.yuv_desc = &_YUYV},

        /* BAYER formats. */
        {V4L2_PIX_FMT_SBGGR8, PIX_FMT_BAYER, 8, .desc.bayer_desc = &_BG8},
        {V4L2_PIX_FMT_SGBRG8, PIX_FMT_BAYER, 8, .desc.bayer_desc = &_GB8},
        {V4L2_PIX_FMT_SGRBG8, PIX_FMT_BAYER, 8, .desc.bayer_desc = &_GR8},
        {V4L2_PIX_FMT_SRGGB8, PIX_FMT_BAYER, 8, .desc.bayer_desc = &_RG8},
        // 10-bit and 12-bit bayer formats are unpacked to 16-bits.
        {V4L2_PIX_FMT_SBGGR10, PIX_FMT_BAYER, 16, .desc.bayer_desc = &_BG10},
        {V4L2_PIX_FMT_SGBRG10, PIX_FMT_BAYER, 16, .desc.bayer_desc = &_GB10},
        {V4L2_PIX_FMT_SGRBG10, PIX_FMT_BAYER, 16, .desc.bayer_desc = &_GR10},
        {V4L2_PIX_FMT_SRGGB10, PIX_FMT_BAYER, 16, .desc.bayer_desc = &_RG10},
        {V4L2_PIX_FMT_SBGGR12, PIX_FMT_BAYER, 16, .desc.bayer_desc = &_BG12},
        {V4L2_PIX_FMT_SGBRG12, PIX_FMT_BAYER, 16, .desc.bayer_desc = &_GB12},
        {V4L2_PIX_FMT_SGRBG12, PIX_FMT_BAYER, 16, .desc.bayer_desc = &_GR12},
        {V4L2_PIX_FMT_SRGGB12, PIX_FMT_BAYER, 16, .desc.bayer_desc = &_RG12},
};
static const int _PIXFormats_num = sizeof(_PIXFormats) / sizeof(*_PIXFormats);

/* Get an entry in the array of supported pixel format descriptors.
 * Param:
 *  |pixel_format| - "fourcc" pixel format to lookup an entry for.
 * Return:
 *  Pointer to the found entry, or NULL if no entry exists for the given pixel
 *  format.
 */
static const PIXFormat* get_pixel_format_descriptor(uint32_t pixel_format) {
    int f;
    for (f = 0; f < _PIXFormats_num; f++) {
        if (_PIXFormats[f].fourcc_type == pixel_format) {
            return &_PIXFormats[f];
        }
    }
    W("%s: Pixel format %.4s is unknown",
      __FUNCTION__, (const char*)&pixel_format);
    return NULL;
}

/* Get whether the provided PIXFormat is convertible by libyuv.
 * Param:
 *  |desc| - Pixel format descriptor, must be non-null.
 * Return:
 *  True if the pixel format is convertible by libyuv.
 */
static bool valid_libyuv_pixformat(const PIXFormat* desc) {
    // Bayer formats are not supported, and RGB656 has the opposite byte order,
    // so libyuv cannot be used to convert it.
    // libyuv does not support YYUV, YVYU, VYUY and YYVU format conversion
    return (desc->format_sel != PIX_FMT_BAYER &&
            desc->fourcc_type != V4L2_PIX_FMT_RGB565 &&
            desc->fourcc_type != V4L2_PIX_FMT_YYUV &&
            desc->fourcc_type != V4L2_PIX_FMT_YVYU &&
            desc->fourcc_type != V4L2_PIX_FMT_VYUY &&
            desc->fourcc_type != V4L2_PIX_FMT_YYVU);
}

/* Given a source format and |result_frame| structure, determine if the libyuv
 * fast-path should be used to convert to the destination formats. To use the
 * fast-path, all formats should be valid libyuv formats.
 *
 * Param:
 *  |src_desc| - Source pixel format descriptor, must be non-null.
 *  |result_frame| - ClientFrame structure defining the destination formats.
 * Return:
 *  True if libyuv is supported for the given conversion.
 */
static bool libyuv_supported(const PIXFormat* src_desc,
                             ClientFrame* result_frame) {
    // Use libyuv if all of the formats are valid.
    bool valid_format = valid_libyuv_pixformat(src_desc);

    int i;
    for (i = 0; i < result_frame->framebuffers_count; ++i) {
        const PIXFormat* desc = get_pixel_format_descriptor(
                result_frame->framebuffers[i].pixel_format);
        valid_format =
                valid_format && desc != NULL && valid_libyuv_pixformat(desc);
    }

    return valid_format;
}

/* Convert a V4L2 pixel format into an equivalent libyuv format.
 * Param:
 *  |pixel_format| - V4L2 pixel format.
 * Return:
 *  Returns the equivalent Libyuv fourcc for the input pixel format.
 */
static uint32_t pixel_format_to_libyuv(uint32_t pixel_format) {
    // Libyuv's fourcc is mostly compatible with V4L2 pixel formats, with some
    // exceptions which need to be explicitly mapped.  Convert those here.

    switch (pixel_format) {
        case V4L2_PIX_FMT_RGB32:
            return FOURCC_ABGR;
        case V4L2_PIX_FMT_ARGB32:
            return FOURCC_BGRA;
        case V4L2_PIX_FMT_BGR32:
            return FOURCC_ARGB;
        // Aliases for V4L2_PIX_FMT_YUYV.
        case V4L2_PIX_FMT_YUY2:
        case V4L2_PIX_FMT_YUNV:
        case V4L2_PIX_FMT_V422:
            return V4L2_PIX_FMT_YUYV;
    }

    return pixel_format;
}

/********************************************************************************
 * Public API
 *******************************************************************************/

int has_converter(uint32_t from, uint32_t to) {
    if (from == to) {
        /* Same format: converter exists. */
        return 1;
    }
    return get_pixel_format_descriptor(from) != NULL &&
           get_pixel_format_descriptor(to) != NULL;
}

bool calculate_framebuffer_size(uint32_t pixel_format,
                                int width,
                                int height,
                                size_t* frame_size) {
    const PIXFormat* desc = get_pixel_format_descriptor(pixel_format);
    if (desc == NULL) {
        E("%s: Source pixel format %.4s is unknown", __FUNCTION__,
          (const char*)&pixel_format);
        return false;
    }

    if (width <= 0 || height <= 0) {
        E("%s: width and height must be positive and non-zero: %d x %d",
          __FUNCTION__, width, height);
        return false;
    }

    if (desc->format_sel == PIX_FMT_YUV && desc->desc.yuv_desc->alignment) {
        // For aligned YUV, the size must include padding for aligned rows.
        const int alignment = desc->desc.yuv_desc->alignment;
        const int y_stride = align(width, alignment);
        const int u_or_v_stride = align(y_stride / 2, alignment);

        *frame_size =
                y_stride * height + 2 * u_or_v_stride * ((height + 1) / 2);
    } else {
        *frame_size = align(width, 2) * height * desc->bits_per_pixel / 8;
    }

    return true;
}

int convert_frame_slow(const void* src_frame,
                       uint32_t pixel_format,
                       size_t framebuffer_size,
                       int width,
                       int height,
                       ClientFrameBuffer* framebuffers,
                       int fbs_num,
                       float r_scale,
                       float g_scale,
                       float b_scale,
                       float exp_comp)
{
    int n;
    const PIXFormat* src_desc = get_pixel_format_descriptor(pixel_format);
    if (src_desc == NULL) {
        E("%s: Source pixel format %.4s is unknown",
          __FUNCTION__, (const char*)&pixel_format);
        return -1;
    }

    for (n = 0; n < fbs_num; n++) {
        /* Note that we need to apply white balance, exposure compensation, etc.
         * when we transfer the captured frame to the user framebuffer. So, even
         * if source and destination formats are the same, we will have to go
         * thrugh the converters to apply these things. */
        const PIXFormat* dst_desc =
                get_pixel_format_descriptor(framebuffers[n].pixel_format);
        if (dst_desc == NULL) {
            E("%s: Destination pixel format %.4s is unknown",
              __FUNCTION__, (const char*)&framebuffers[n].pixel_format);
            return -1;
        }
        switch (src_desc->format_sel) {
            case PIX_FMT_RGB:
                if (dst_desc->format_sel == PIX_FMT_RGB) {
                    RGBToRGB(src_desc->desc.rgb_desc, dst_desc->desc.rgb_desc,
                             src_frame, framebuffers[n].framebuffer, width,
                             height, r_scale, g_scale, b_scale, exp_comp);
                } else if (dst_desc->format_sel == PIX_FMT_YUV) {
                    RGBToYUV(src_desc->desc.rgb_desc, dst_desc->desc.yuv_desc,
                             src_frame, framebuffers[n].framebuffer, width,
                             height, r_scale, g_scale, b_scale, exp_comp);
                } else {
                    E("%s: Unexpected destination pixel format %d",
                      __FUNCTION__, dst_desc->format_sel);
                    return -1;
                }
                break;
            case PIX_FMT_YUV:
                if (dst_desc->format_sel == PIX_FMT_RGB) {
                    YUVToRGB(src_desc->desc.yuv_desc, dst_desc->desc.rgb_desc,
                             src_frame, framebuffers[n].framebuffer, width,
                             height, r_scale, g_scale, b_scale, exp_comp);
                } else if (dst_desc->format_sel == PIX_FMT_YUV) {
                    YUVToYUV(src_desc->desc.yuv_desc, dst_desc->desc.yuv_desc,
                             src_frame, framebuffers[n].framebuffer, width,
                             height, r_scale, g_scale, b_scale, exp_comp);
                } else {
                    E("%s: Unexpected destination pixel format %d",
                      __FUNCTION__, dst_desc->format_sel);
                    return -1;
                }
                break;
            case PIX_FMT_BAYER:
                if (dst_desc->format_sel == PIX_FMT_RGB) {
                    BAYERToRGB(src_desc->desc.bayer_desc,
                               dst_desc->desc.rgb_desc, src_frame,
                               framebuffers[n].framebuffer, width, height,
                               r_scale, g_scale, b_scale, exp_comp);
                } else if (dst_desc->format_sel == PIX_FMT_YUV) {
                    BAYERToYUV(src_desc->desc.bayer_desc,
                               dst_desc->desc.yuv_desc, src_frame,
                               framebuffers[n].framebuffer, width, height,
                               r_scale, g_scale, b_scale, exp_comp);
                } else {
                    E("%s: Unexpected destination pixel format %d",
                      __FUNCTION__, dst_desc->format_sel);
                    return -1;
                }
                break;
            default:
                E("%s: Unexpected source pixel format %d",
                  __FUNCTION__, dst_desc->format_sel);
                return -1;
        }
    }

    return 0;
}

typedef struct YUVInfo {
    size_t y_stride;
    size_t u_or_v_stride;
    size_t y_size;
    size_t u_or_v_size;
} YUVInfo;

YUVInfo get_yuv_info(int width, int height) {
    YUVInfo result;
    result.y_stride = align(width, 16);
    result.u_or_v_stride = align(result.y_stride / 2, 16);
    result.y_size = result.y_stride * height;
    result.u_or_v_size = result.u_or_v_stride * ((height + 1) / 2);
    return result;
}

bool resize_staging(ClientFrame* result_frame, size_t required) {
    if (*result_frame->staging_framebuffer_size < required) {
        D("%s: Resizing staging framebuffer from %d to %d", __FUNCTION__,
          *result_frame->staging_framebuffer_size, required);

        *result_frame->staging_framebuffer =
                (uint8_t*)realloc(*result_frame->staging_framebuffer, required);

        if (*result_frame->staging_framebuffer) {
            // Zero out the new memory.
            memset(*result_frame->staging_framebuffer +
                           *result_frame->staging_framebuffer_size,
                   0, required - *result_frame->staging_framebuffer_size);

            *result_frame->staging_framebuffer_size = required;
        } else {
            W("%s: Failed to resize camera staging framebuffer", __FUNCTION__);
            *result_frame->staging_framebuffer_size = 0;
            return false;
        }
    }

    return true;
}

int convert_to_i420(const void* src_frame,
                    uint32_t pixel_format,
                    size_t framebuffer_size,
                    int src_width,
                    int src_height,
                    int dst_width,
                    int dst_height,
                    int rotation,
                    YUVInfo src_info,
                    YUVInfo dst_info,
                    ClientFrame* result_frame) {
    const size_t staging_size = dst_info.y_size + 2 * dst_info.u_or_v_size;

    uint8_t* y_staging = *result_frame->staging_framebuffer;
    uint8_t* u_staging = y_staging + dst_info.y_size;
    uint8_t* v_staging = u_staging + dst_info.u_or_v_size;

    if (pixel_format == V4L2_PIX_FMT_YUV420 && rotation == 0) {
        memcpy(y_staging, src_frame, staging_size);
    } else if (pixel_format == V4L2_PIX_FMT_YVU420 && rotation == 0) {
        // YVU420 is 16-byte aligned, but there is no way to specify alignment
        // with ConvertToI420; manually convert it to YUV420 (swapping U and V)
        // with libyuv's I420Copy.
        const uint8_t* src_y = src_frame;
        const uint8_t* src_v = src_y + src_info.y_size;
        const uint8_t* src_u = src_v + src_info.u_or_v_size;

        int result = I420Copy(src_y,                   // src_y
                              src_info.y_stride,       // src_stride_y
                              src_u,                   // src_u
                              src_info.u_or_v_stride,  // src_stride_u
                              src_v,                   // src_v
                              src_info.u_or_v_stride,  // src_stride_v
                              y_staging,               // dst_y
                              src_info.y_stride,       // dst_stride_y
                              u_staging,               // dst_u
                              src_info.u_or_v_stride,  // dst_stride_u
                              v_staging,               // dst_v
                              src_info.u_or_v_stride,  // dst_stride_v
                              src_width,               // width
                              src_height);             // height
        if (result != 0) {
            W("%s: I420Copy failed with format %.4s, error %d", __FUNCTION__,
                (const char*)(&pixel_format), result);
            return -1;
        }
    } else {
        const uint32_t src_format = pixel_format_to_libyuv(pixel_format);
        int crop_width;
        int crop_height;
        if (rotation == 90 || rotation == 270) {
            crop_width = dst_height;
            crop_height = dst_width;
        } else {
            crop_width = dst_width;
            crop_height = dst_height;
        }

        int result = ConvertToI420(src_frame,               // src_frame
                                   framebuffer_size,        // src_size
                                   y_staging,               // dst_y
                                   dst_info.y_stride,       // dst_stride_y
                                   u_staging,               // dst_u
                                   dst_info.u_or_v_stride,  // dst_stride_u
                                   v_staging,               // dst_v
                                   dst_info.u_or_v_stride,  // dst_stride_v
                                   0,                       // crop_x
                                   0,                       // crop_y
                                   src_width,               // src_width
                                   src_height,              // src_height
                                   crop_width,              // crop_width
                                   crop_height,             // crop_height
                                   rotation,                // rotation
                                   src_format);             // format
        if (result != 0) {
            W("%s: Could not convert from %.4s to I420, error %d", __FUNCTION__,
                (const char*)(&pixel_format), result);
            return -1;
        }
    }

    return 0;
}

int convert_frame_fast(const void* src_frame,
                       uint32_t pixel_format,
                       size_t framebuffer_size,
                       int src_width,
                       int src_height,
                       ClientFrame* result_frame,
                       float exp_comp,
                       int rotation) {
    const uint32_t exp_comp20 = exp_comp * (1u << 20);
    int n;
    for (n = 0; n < result_frame->framebuffers_count; ++n) {
        YUVInfo src_info = get_yuv_info(src_width, src_height);
        int result_width = result_frame->framebuffers[n].width;
        int result_height = result_frame->framebuffers[n].height;
        int rotated_width = 0;
        int rotated_height = 0;
        if (rotation == 1 || rotation == 3) {
            // Require rotation and cropping.
            // We want to crop as little as possible.
            if (src_height * result_height > src_width * result_width) {
                rotated_height = src_width;
                rotated_width = rotated_height * result_width / result_height;
            } else {
                rotated_width = src_height;
                rotated_height = rotated_width * result_height / result_width;
            }
        } else {
            if (src_width * result_height > src_height * result_width) {
                // Source aspect ratio is larger than result aspect ratio,
                // thus crop the width.
                rotated_height = src_height;
                rotated_width = rotated_height * result_width / result_height;
            } else {
                // Otherwise, crop the height.
                rotated_width = src_width;
                rotated_height = rotated_width * result_height / result_width;
            }
        }
        YUVInfo rotated_info = get_yuv_info(rotated_width, rotated_height);
        size_t rotated_size =
                rotated_info.y_size + 2 * rotated_info.u_or_v_size;
        YUVInfo result_info = get_yuv_info(result_width, result_height);
        const size_t result_size =
            result_info.y_size + 2 * result_info.u_or_v_size;
        const bool has_resize = (rotated_width != result_width ||
                                 rotated_height != result_height);
        const size_t required_staging_size =
                rotated_size + (has_resize ? result_size : 0);
        if (!resize_staging(result_frame, required_staging_size)) {
            D("%s: Failed to resize the camera staging buffer", __FUNCTION__);
            return -1;
        }
        // Convert to I420, the intermediate format required for libyuv.
        int result = convert_to_i420(src_frame, pixel_format, framebuffer_size,
                                     src_width, src_height, rotated_width,
                                     rotated_height, rotation * 90, src_info,
                                     rotated_info, result_frame);
        if (result != 0) {
            W("%s: Failed to convert the camera frame", __FUNCTION__);
            return result;
        }
        src_info = rotated_info;
        uint8_t* src_y = *result_frame->staging_framebuffer;
        uint8_t* src_u = src_y + src_info.y_size;
        uint8_t* src_v = src_u + src_info.u_or_v_size;

        // If there is a resize, resize to the destination size.
        if (has_resize) {
            uint8_t* dest_y = *result_frame->staging_framebuffer + rotated_size;
            uint8_t* dest_u = dest_y + result_info.y_size;
            uint8_t* dest_v = dest_u + result_info.u_or_v_size;

            result = I420Scale(src_y,                      // src_y
                               src_info.y_stride,          // src_stride_y
                               src_u,                      // src_u
                               src_info.u_or_v_stride,     // src_stride_u
                               src_v,                      // src_v
                               src_info.u_or_v_stride,     // src_stride_v
                               rotated_width,              // rotated_width
                               rotated_height,             // rotated_height
                               dest_y,                     // dst_y
                               result_info.y_stride,       // dst_stride_y
                               dest_u,                     // dst_u
                               result_info.u_or_v_stride,  // dst_stride_u
                               dest_v,                     // dst_v
                               result_info.u_or_v_stride,  // dst_stride_v
                               result_width,               // dst_width
                               result_height,              // dst_v
                               kFilterNone);               // filtering
            if (result != 0) {
                W("%s: Failed to resize camera frame.", __FUNCTION__);
                return -1;
            }

            // dest becomes the new src for the rest of the conversion.
            src_y = dest_y;
            src_u = dest_u;
            src_v = dest_v;
            src_info = result_info;
        }
        size_t src_size = result_size;

        // Apply exposure compensation.
        if (exp_comp != 1.0f) {
            int row;
            int x;
            for (row = 0; row < result_height; ++row) {
                uint8_t* y_row = src_y + row * src_info.y_stride;
                for (x = 0; x < result_width; ++x) {
                    y_row[x] = _change_exposure20(y_row[x], exp_comp20);
                }
            }
        }

        // Convert to the target framebuffer formats.
        void* dest = result_frame->framebuffers[n].framebuffer;
        const uint32_t dest_format = pixel_format_to_libyuv(
                result_frame->framebuffers[n].pixel_format);

        if (dest_format == V4L2_PIX_FMT_YUV420) {
            memcpy(dest, src_y, src_size);
        } else if (dest_format == V4L2_PIX_FMT_YVU420) {
            // YVU420 is 16-byte aligned, but there is no way to specify u and v
            // with ConvertFromI420; manually convert it to YUV420 (swapping U
            // and V) with libyuv's I420Copy.
            uint8_t* dest_y = dest;
            uint8_t* dest_v = dest_y + src_info.y_size;
            uint8_t* dest_u = dest_v + src_info.u_or_v_size;

            result = I420Copy(src_y,                   // src_y
                              src_info.y_stride,       // src_stride_y
                              src_u,                   // src_u
                              src_info.u_or_v_stride,  // src_stride_u
                              src_v,                   // src_v
                              src_info.u_or_v_stride,  // src_stride_v
                              dest_y,                  // dst_y
                              src_info.y_stride,       // dst_stride_y
                              dest_u,                  // dst_u
                              src_info.u_or_v_stride,  // dst_stride_u
                              dest_v,                  // dst_v
                              src_info.u_or_v_stride,  // dst_stride_v
                              result_width,            // width
                              result_height);          // height
        } else {
            result = ConvertFromI420(src_y,                   // y
                                     src_info.y_stride,       // y_stride
                                     src_u,                   // u
                                     src_info.u_or_v_stride,  // u_stride
                                     src_v,                   // v
                                     src_info.u_or_v_stride,  // v_stride
                                     dest,                    // dst_sample
                                     0,              // dst_sample_stride
                                     result_width,   // width
                                     result_height,  // height
                                     dest_format);   // format
        }

        if (result != 0) {
            // Failed to convert with libyuv, fallback to original method for
            // this one frame.
            W("%s: Could not convert frame with fast path to %.4s %dx%d",
                __FUNCTION__, (const char*)(&dest_format), result_width,
                result_height);
            return -1;
        }
    }
    return 0;
}

int convert_frame(const void* src_frame,
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
                  int orientation) {
    const PIXFormat* src_desc = get_pixel_format_descriptor(pixel_format);
    if (src_desc == NULL) {
        E("%s: Source pixel format %.4s is unknown", __FUNCTION__,
          (const char*)&pixel_format);
        return -1;
    }

    int rotation = 0 == strcmp("back", direction) ? (7 - orientation) % 4
                                                  : (3 + orientation) % 4;

    // Enable a fast-path with libyuv if the white-balance no-ops.
    if (r_scale == 1.0f && g_scale == 1.0f && b_scale == 1.0f &&
        libyuv_supported(src_desc, result_frame)) {
        if (convert_frame_fast(src_frame, pixel_format, framebuffer_size, width,
                               height, result_frame, exp_comp, rotation) == 0) {
            return 0;
        }
    }

    // Fast path preconditions were not met or conversion failed, fall back to
    // the slow path.
    // Note that this does not rotate the frames properly.
    // BUG: 260913366
    int res = convert_frame_slow(src_frame, pixel_format, framebuffer_size,
                                 width, height, result_frame->framebuffers,
                                 result_frame->framebuffers_count, r_scale,
                                 g_scale, b_scale, exp_comp);
    return res;
}
