/* Copyright (C) 2007-2015 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "android/skin/window.h"

#include <math.h>                               // for ceil
#include <stdint.h>                             // for uint32_t, uint8_t
#include <stdio.h>                              // for NULL, fprintf, stderr
#include <stdlib.h>                             // for calloc, free, malloc

#include "android/crashreport/crash-handler.h"  // for crashhandler_die_format
#include "android/emulation/control/multi_display_agent.h"
#include "android/emulator-window.h"
#include "android/featurecontrol/feature_control.h"
#include "android/globals.h"
#include "android/hw-sensors.h"
#include "android/multitouch-screen.h"          // for multitouch_create_but...
#include "android/skin/event.h"                 // for SkinEvent, (anonymous...
#include "android/skin/image.h"                 // for skin_image_unref, ski...
#include "android/skin/keycode.h"               // for skin_keycode_rotate
#include "android/skin/linux_keycodes.h"        // for LINUX_BTN_MOUSE
#include "android/skin/surface.h"               // for skin_surface_blit
#include "android/skin/winsys.h"                // for skin_winsys_set_windo...
#include "android/utils/debug.h"                // for derror, dprint, VERBO...
#include "android/utils/duff.h"                 // for DUFF4
#include "android/utils/intmap.h"               // for AIntMap
#include "android/utils/system.h"               // for AFREE, AARRAY_NEW0

/* default id of free slot in multitouch_fingers_ids vector*/
#define FINGER_SLOT_FREE -1

/* when shrinking, we reduce the pixel ratio by this fixed amount */
#define  SHRINK_SCALE  0.6

/* maxinum number of additional displays that can be added*/
#define MAX_DISPLAY_NUM 10
/* maximum value of LCD brighness */
#define  LCD_BRIGHTNESS_MIN      0
#define  LCD_BRIGHTNESS_DEFAULT  128
#define  LCD_BRIGHTNESS_MAX      255

typedef struct Background {
    SkinImage*   image;
    SkinRect     rect;
    SkinPos      origin;
} Background;

static void
background_done( Background*  back )
{
    skin_image_unref( &back->image );
}

static void
background_init(Background* back,
                SkinBackground* sback,
                SkinLocation* loc,
                SkinRect* frame)
{
    SkinRect  r;

    back->image = skin_image_rotate( sback->image, loc->rotation );
    skin_rect_rotate( &r, &sback->rect, loc->rotation );
    r.pos.x += loc->anchor.x;
    r.pos.y += loc->anchor.y;

    back->origin = r.pos;
    skin_rect_intersect( &back->rect, &r, frame );
}

static void
background_redraw(Background* back, SkinRect* rect, SkinSurface* surface)
{
    SkinRect  r;

    if (skin_rect_intersect( &r, rect, &back->rect ) )
    {
        SkinRect src_rect;
        src_rect.pos.x = r.pos.x - back->origin.x;
        src_rect.pos.y = r.pos.y - back->origin.y;
        src_rect.size = r.size;

        skin_surface_blit(surface,
                          &r.pos,
                          skin_image_surface(back->image),
                          &src_rect,
                          SKIN_BLIT_SRCOVER);
    }
}

typedef struct SubDisplay {
    struct SubDisplay* next;
    SkinRect  rect;
    int id;
} SubDisplay;

typedef struct ADisplay {
    SkinRect       rect;
    SkinPos        origin;
    SkinRotation   rotation;
    SkinSize       datasize;  /* framebuffer size */
    void*          data;      /* framebuffer pixels */
    int            bits_per_pixel;  /* framebuffer depth */
    SkinImage*     onion;       /* onion image */
    SkinRect       onion_rect;  /* onion rect, if any */
    int            brightness;
    void*          gpu_frame;   /* GL_RGBA, datasize.w * datasize.h * 4 bytes */
    SkinSurface*   surface;     /* displayed surface after rotation + onion */
    uint32_t id;
} ADisplay;

static void adisplay_done(ADisplay* disp) {
    if (disp->gpu_frame) {
        free(disp->gpu_frame);
        disp->gpu_frame = NULL;
    }
    skin_surface_unrefp(&disp->surface);
    disp->data = NULL;
    skin_image_unref(&disp->onion);
}

static int adisplay_init(ADisplay* disp,
                         SkinDisplay* sdisp,
                         SkinLocation* loc,
                         SkinRect* frame) {
    skin_rect_rotate( &disp->rect, &sdisp->rect, loc->rotation );
    disp->rect.pos.x += loc->anchor.x;
    disp->rect.pos.y += loc->anchor.y;

    disp->rotation = (loc->rotation + sdisp->rotation) & 3;
    switch (disp->rotation) {
        case SKIN_ROTATION_0:
            disp->origin = disp->rect.pos;
            break;

        case SKIN_ROTATION_90:
            disp->origin.x = disp->rect.pos.x + disp->rect.size.w;
            disp->origin.y = disp->rect.pos.y;
            break;

        case SKIN_ROTATION_180:
            disp->origin.x = disp->rect.pos.x + disp->rect.size.w;
            disp->origin.y = disp->rect.pos.y + disp->rect.size.h;
            break;

        case SKIN_ROTATION_270:
            disp->origin.x = disp->rect.pos.x;
            disp->origin.y = disp->rect.pos.y + disp->rect.size.h;
            break;
    }
    skin_size_rotate( &disp->datasize, &sdisp->rect.size, sdisp->rotation );
    skin_rect_intersect( &disp->rect, &disp->rect, frame );
#if 0
    fprintf(stderr, "... display_init  rect.pos(%d,%d) rect.size(%d,%d) datasize(%d,%d)\n",
                    disp->rect.pos.x, disp->rect.pos.y,
                    disp->rect.size.w, disp->rect.size.h,
                    disp->datasize.w, disp->datasize.h);
#endif
    disp->id = 0;
    disp->data = NULL;
    disp->bits_per_pixel = 0;
    if (sdisp->framebuffer_funcs) {
        disp->data =
                sdisp->framebuffer_funcs->get_pixels(sdisp->framebuffer);
        disp->bits_per_pixel =
                sdisp->framebuffer_funcs->get_depth(sdisp->framebuffer);
    }
    disp->onion  = NULL;

    disp->brightness = LCD_BRIGHTNESS_DEFAULT;

    disp->gpu_frame = NULL;

    disp->surface = skin_surface_create(disp->rect.size.w,
                                        disp->rect.size.h,
                                        disp->rect.size.w,
                                        disp->rect.size.h);
    return (disp->data == NULL) ? -1 : 0;
}

static __inline__ uint32_t rgb565_to_argb32(uint32_t pix) {
#if 1
    uint32_t r8 = ((pix & 0xf800) >>  8) | ((pix & 0xe000) >> 13);
    uint32_t g8 = ((pix & 0x07e0) >>  3) | ((pix & 0x0600) >>  9);
    uint32_t b8 = ((pix & 0x001f) <<  3) | ((pix & 0x001c) >>  2);
    return (r8 << 16) | (g8 << 8) | (b8 << 0) | 0xff000000U;
#else
    uint32_t r8 = ((pix & 0xf800) << 8) | ((pix & 0xe000) >> 5);
    uint32_t g8 = ((pix & 0x07e0) << 5) | ((pix & 0x0600) >> 1);
    uint32_t b8 = ((pix & 0x001f) <<  3) | ((pix & 0x001c) >>  2);
    return r8 | g8 | b8 | 0xff000000U;
#endif
}

static void adisplay_set_onion(ADisplay* disp,
                               SkinImage* onion,
                               SkinRotation rotation,
                               int  blend) {
    int        onion_w, onion_h;
    SkinRect*  rect  = &disp->rect;
    SkinRect*  orect = &disp->onion_rect;

    rotation = (rotation + disp->rotation) & 3;

    skin_image_unref( &disp->onion );
    disp->onion = skin_image_clone_full( onion, rotation, blend );

    onion_w = skin_image_w(disp->onion);
    onion_h = skin_image_h(disp->onion);

    switch (rotation) {
        case SKIN_ROTATION_0:
            orect->pos = rect->pos;
            break;

        case SKIN_ROTATION_90:
            orect->pos.x = rect->pos.x + rect->size.w - onion_w;
            orect->pos.y = rect->pos.y;
            break;

        case SKIN_ROTATION_180:
            orect->pos.x = rect->pos.x + rect->size.w - onion_w;
            orect->pos.y = rect->pos.y + rect->size.h - onion_h;
            break;

        case SKIN_ROTATION_270:
            orect->pos.x = rect->pos.x;
            orect->pos.y = rect->pos.y + rect->size.h - onion_h;
    }
    orect->size.w = onion_w;
    orect->size.h = onion_h;
}

// Set to 1 to enable experimental dot-matrix display mode.
#define  DOT_MATRIX  0

#if DOT_MATRIX

static void dotmatrix_dither_argb32(unsigned char* pixels,
                                    int x,
                                    int y,
                                    int w,
                                    int h,
                                    int pitch) {
    static const unsigned dotmatrix_argb32[16] = {
        0x003f00, 0x00003f, 0x3f0000, 0x000000,
        0x3f3f3f, 0x000000, 0x3f3f3f, 0x000000,
        0x3f0000, 0x000000, 0x003f00, 0x00003f,
        0x3f3f3f, 0x000000, 0x3f3f3f, 0x000000
    };

    int yy = y & 3;

    pixels += (4 * x) + (y * pitch);

    for (; h > 0; h--) {
        unsigned* line = (unsigned*) pixels;
        int nn, xx = x & 3;

        for (nn = 0; nn < w; nn++) {
            unsigned  c = line[nn];

            c = c - ((c >> 2) & dotmatrix_argb32[(yy << 2)|xx]);

            xx = (xx + 1) & 3;
            line[nn] = c;
        }
        yy = (yy + 1) & 3;
        pixels += pitch;
    }
}

#endif /* DOT_MATRIX */

/* technical note about the lightness emulation
 *
 * we try to emulate something that looks like the Dream's
 * non-linear LCD lightness, without going too dark or bright.
 *
 * the default lightness is around 105 (about 40%) and we prefer
 * to keep full RGB colors at that setting, to not alleviate
 * developers who will not understand why the emulator's colors
 * look slightly too dark.
 *
 * we also want to implement a 'bright' mode by de-saturating
 * colors towards bright white.
 *
 * All of this leads to the implementation below that looks like
 * the following:
 *
 * if (level == MIN)
 *     screen is off
 *
 * if (level > MIN && level < LOW)
 *     interpolate towards black, with
 *     MINALPHA = 0.2
 *     alpha = MINALPHA + (1-MINALPHA)*(level-MIN)/(LOW-MIN)
 *
 * if (level >= LOW && level <= HIGH)
 *     keep full RGB colors
 *
 * if (level > HIGH)
 *     interpolate towards bright white, with
 *     MAXALPHA = 0.6
 *     alpha = MAXALPHA*(level-HIGH)/(MAX-HIGH)
 *
 * we probably want some sort of power law instead of interpolating
 * linearly, but frankly, this is sufficient for most uses.
 */

#define  LCD_BRIGHTNESS_LOW   80
#define  LCD_BRIGHTNESS_HIGH  180

#define  LCD_ALPHA_LOW_MIN      0.2
#define  LCD_ALPHA_HIGH_MAX     0.6

/* treat as special value to turn screen off */
#define  LCD_BRIGHTNESS_OFF   LCD_BRIGHTNESS_MIN

static void lcd_brightness_argb32(uint32_t* pixels,
                                  int w,
                                  int h,
                                  int pitch,
                                  int brightness)
{
    const unsigned  b_min  = LCD_BRIGHTNESS_MIN;
    const unsigned  b_max  = LCD_BRIGHTNESS_MAX;
    const unsigned  b_low  = LCD_BRIGHTNESS_LOW;
    const unsigned  b_high = LCD_BRIGHTNESS_HIGH;

    unsigned        alpha = brightness;

    if (alpha <= b_min)
        alpha = b_min;
    else if (alpha > b_max)
        alpha = b_max;

    if (alpha < b_low)
    {
        const unsigned  alpha_min   = (255 * LCD_ALPHA_LOW_MIN);
        const unsigned  alpha_range = (255 - alpha_min);

        alpha = alpha_min + ((alpha - b_min) * alpha_range) / (b_low - b_min);

        for (; h > 0; h--) {
            unsigned* line = (unsigned*) pixels;
            int nn = 0;

            DUFF4(w, {
                unsigned c = line[nn];
                unsigned ag = (c >> 8) & 0x00ff00ff;
                unsigned rb = (c)      & 0x00ff00ff;

                ag = (ag * alpha)        & 0xff00ff00;
                rb = ((rb * alpha) >> 8) & 0x00ff00ff;

                line[nn] = (unsigned)(ag | rb);
                nn++;
            });
            pixels += (pitch / sizeof(uint32_t));
        }
    }
    else if (alpha > LCD_BRIGHTNESS_HIGH) /* 'superluminous' mode */
    {
        const unsigned  alpha_max   = (255 * LCD_ALPHA_HIGH_MAX);
        const unsigned  alpha_range = (255 - alpha_max);
        unsigned ialpha;

        alpha  = ((alpha - b_high) * alpha_range) / (b_max - b_high);
        ialpha = 255 - alpha;

        for ( ; h > 0; h-- ) {
            unsigned* line = (unsigned*) pixels;
            int nn = 0;

            DUFF4(w, {
                unsigned c = line[nn];
                unsigned ag = (c >> 8) & 0x00ff00ff;
                unsigned rb = (c)      & 0x00ff00ff;

                /* interpolate towards bright white, i.e. 0x00ffffff */
                ag = ((ag*ialpha + 0x00ff00ff*alpha)) & 0xff00ff00;
                rb = ((rb*ialpha + 0x00ff00ff*alpha) >> 8) & 0x00ff00ff;

                line[nn] = (unsigned)(ag | rb);
                nn++;
            });
            pixels += (pitch / sizeof(uint32_t));
        }
    }
}

static void adisplay_update_surface_pixels_16(ADisplay* disp,
                                              SkinRect* dst_rect,
                                              uint8_t* dst_pixels,
                                              int dst_pitch) {
    int           x  = dst_rect->pos.x;
    int           y  = dst_rect->pos.y;
    int           w  = dst_rect->size.w;
    int           h  = dst_rect->size.h;
    int           src_w    = disp->datasize.w;
    int           src_h    = disp->datasize.h;
    int           src_pitch = src_w * 2;
    uint8_t*      src_line  = (uint8_t*)disp->data;
    uint8_t*      dst_line  = dst_pixels;
    int           yy, xx;

    switch ( disp->rotation & 3 )
    {
    case SKIN_ROTATION_0:
        src_line += (x * 2) + (y * src_pitch);

        for (yy = h; yy > 0; yy--) {
            uint32_t* dst = (uint32_t*)dst_line;
            uint16_t* src = (uint16_t*)src_line;

            xx = 0;
            DUFF4(w, {
                dst[xx] = rgb565_to_argb32(src[xx]);
                xx++;
            });
            src_line += src_pitch;
            dst_line += dst_pitch;
        }
        break;

    case SKIN_ROTATION_90:
        src_line += (y * 2) + ((src_h - x - 1) * src_pitch);

        for (yy = h; yy > 0; yy--) {
            uint32_t* dst = (uint32_t*)dst_line;
            uint8_t* src = src_line;

            DUFF4(w, {
                dst[0] = rgb565_to_argb32(((uint16_t*)src)[0]);
                src -= src_pitch;
                dst += 1;
            });
            src_line += 2;
            dst_line += dst_pitch;
        }
        break;

    case SKIN_ROTATION_180:
        src_line += ((src_w - 1 - x) * 2) + ((src_h - 1 - y) * src_pitch);

        for (yy = h; yy > 0; yy--) {
            uint16_t* src = (uint16_t*)src_line;
            uint32_t* dst = (uint32_t*)dst_line;

            DUFF4(w, {
                dst[0] = rgb565_to_argb32(src[0]);
                src -= 1;
                dst += 1;
            });
            src_line -= src_pitch;
            dst_line += dst_pitch;
    }
    break;

    default:  /* SKIN_ROTATION_270 */
        src_line += ((src_w - 1 - y) * 2) + (x * src_pitch);

        for (yy = h; yy > 0; yy--) {
            uint32_t* dst = (uint32_t*)dst_line;
            uint8_t* src = src_line;

            DUFF4(w, {
                dst[0] = rgb565_to_argb32(((uint16_t*)src)[0]);
                dst += 1;
                src += src_pitch;
            });
            src_line -= 2;
            dst_line += dst_pitch;
        }
    }
}

static void adisplay_update_surface_pixels_32(ADisplay* disp,
                                              SkinRect* rect,
                                              uint8_t* dst_pixels,
                                              int dst_pitch,
                                              const void* src_pixels) {
    int           x  = rect->pos.x;
    int           y  = rect->pos.y;
    int           w  = rect->size.w;
    int           h  = rect->size.h;
    int           src_w     = disp->datasize.w;
    int           src_h     = disp->datasize.h;
    uint8_t*      dst_line  = dst_pixels;
    int           src_pitch = disp->datasize.w * 4;
    const uint8_t* src_line  = (const uint8_t*)src_pixels;
    int           yy;

    switch ( disp->rotation & 3 )
    {
    case SKIN_ROTATION_0:
        src_line += (x * 4) + (y * src_pitch);

        for (yy = h; yy > 0; yy--) {
            uint32_t*  src = (uint32_t*)src_line;
            uint32_t*  dst = (uint32_t*)dst_line;

            DUFF4(w, {
                dst[0] = src[0];
                dst++;
                src++;
            });
            src_line += src_pitch;
            dst_line += dst_pitch;
        }
        break;

    case SKIN_ROTATION_90:
        src_line += (y * 4) + ((src_h - x - 1) * src_pitch);

        for (yy = h; yy > 0; yy--) {
            uint32_t* dst = (uint32_t*)dst_line;
            const uint8_t*  src = src_line;

            DUFF4(w, {
                dst[0] = *(uint32_t*)src;
                src -= src_pitch;
                dst += 1;
            });
            src_line += 4;
            dst_line += dst_pitch;
        }
        break;

    case SKIN_ROTATION_180:
        src_line += ((src_w - 1 - x) * 4) + ((src_h - 1 - y) * src_pitch);

        for (yy = h; yy > 0; yy--) {
            const uint32_t*  src = (const uint32_t*)src_line;
            uint32_t*  dst = (uint32_t*)dst_line;

            DUFF4(w, {
                dst[0] = src[0];
                src -= 1;
                dst += 1;
            });
            src_line -= src_pitch;
            dst_line += dst_pitch;
    }
    break;

    default:  /* SKIN_ROTATION_270 */
        src_line += ((src_w - 1 - y) * 4) + (x * src_pitch);

        for (yy = h; yy > 0; yy--)
        {
            uint32_t*  dst = (uint32_t*)dst_line;
            const uint8_t*   src = src_line;

            DUFF4(w, {
                dst[0] = *(uint32_t*)src;
                dst   += 1;
                src   += src_pitch;
            });
            src_line -= 4;
            dst_line += dst_pitch;
        }
    }
}

struct local_pixels_buffer_t {
    uint8_t* pixels;
    int size;
};

static struct local_pixels_buffer_t local_pixels_buffer = {.pixels = NULL, .size = 0};

static uint8_t* local_calloc(int size) {
    if (local_pixels_buffer.pixels == NULL) {
        local_pixels_buffer.pixels = calloc(1, size);
        local_pixels_buffer.size = size;
    } else if (local_pixels_buffer.size < size) {
        local_pixels_buffer.size = size > 2 * local_pixels_buffer.size ?
                                      size : 2 * local_pixels_buffer.size;
        free(local_pixels_buffer.pixels);
        local_pixels_buffer.pixels = calloc(1, local_pixels_buffer.size);
    }
    return local_pixels_buffer.pixels;
}

// Update the content of the display surface from the framebuffer content.
// |disp| is the target ADisplay instance.
// |rect| is the rectangle to update, in coordinates relative to the
// display surface, i.e. (0,0) is always the top-left corner.
static void adisplay_update_surface(ADisplay* disp,
                                    SkinRect* rect) {
    int x = rect->pos.x;
    int y = rect->pos.y;
    int w = rect->size.w;
    int h = rect->size.h;

    // Clip update rectangle for sanity.
    int delta = (x + w) - disp->rect.size.w;
    if (delta > 0) {
        w -= delta;
    }
    if (x < 0) {
        w += x;
        x = 0;
    }
    delta = (y + h) - disp->rect.size.h;
    if (delta > 0) {
        h -= delta;
    }
    if (y < 0) {
        h += delta;
        y = 0;
    }
    if (w <= 0 || h <= 0) {
        return;  // nothing to do.
    }

    // Allocate a temporary buffer to get the potentially rotated / converted
    // content.
    int sz = disp->datasize.h > disp->datasize.w ? disp->datasize.h : disp->datasize.w;
    int dst_pitch = 4 * sz;
    const int size = sz * dst_pitch;
    uint8_t* dst_pixels = local_calloc(size);
    if (dst_pixels == NULL) {
        derror("ERROR: %s:%d cannot allocate memory of %d bytes.\n",
               __func__, __LINE__, size);
        crashhandler_die_format(
                    "Display surface memory allocation failed "
                    "(requested %d bytes)",
                    size);
    }

    SkinRect dst_r = {
        .pos.x = x,
        .pos.y = y,
        .size.w = w,
        .size.h = h };

    if (disp->gpu_frame) {
        // Content comes from the emulated GPU.
        adisplay_update_surface_pixels_32(
                disp, &dst_r, dst_pixels, dst_pitch, disp->gpu_frame);
    } else {
        // Content comes from the emulated framebuffer.
        if (disp->bits_per_pixel == 32) {
            adisplay_update_surface_pixels_32(
                    disp, &dst_r, dst_pixels, dst_pitch, disp->data);
        } else {
            adisplay_update_surface_pixels_16(
                    disp, &dst_r, dst_pixels, dst_pitch);
        }
    }

    // Apply brightness modulation.
    lcd_brightness_argb32((uint32_t*)dst_pixels,
                          disp->datasize.w,
                          disp->datasize.h,
                          dst_pitch,
                          disp->brightness);

    // Update the display surface content
    skin_surface_upload(disp->surface, &dst_r, dst_pixels, dst_pitch);

    // Done
}

static void adisplay_redraw(ADisplay* disp,
                            SkinRect* rect,
                            SkinSurface* surface,
                            bool using_emugl_subwindow) {
    SkinRect  r;

    if (!skin_rect_intersect(&r, rect, &disp->rect)) {
        return;
    }

#if 0
    fprintf(stderr, "--- display redraw r.pos(%d,%d) r.size(%d,%d) "
            "disp.pos(%d,%d) disp.size(%d,%d) datasize(%d,%d) rect.pos(%d,%d) rect.size(%d,%d)\n",
            r.pos.x, r.pos.y,
            r.size.w, r.size.h, disp->rect.pos.x, disp->rect.pos.y,
            disp->rect.size.w, disp->rect.size.h, disp->datasize.w, disp->datasize.h,
            rect->pos.x, rect->pos.y, rect->size.w, rect->size.h );
#endif

    // Update the content of the display surface.
    SkinRect src_r = r;
    src_r.pos.x -= disp->rect.pos.x;
    src_r.pos.y -= disp->rect.pos.y;
    if (!using_emugl_subwindow) {
        adisplay_update_surface(disp, &src_r);
    }

    if (disp->brightness == LCD_BRIGHTNESS_OFF) {
        // Fill window surface with solid black.
        skin_surface_fill(surface, &r, 0xff000000);
    } else {
        skin_surface_blit(surface,
                          &r.pos,
                          disp->surface,
                          &src_r,
                          SKIN_BLIT_COPY);
    }

    /* Apply onion skin */
    if (disp->onion != NULL) {
        SkinRect  r2;

        if ( skin_rect_intersect( &r2, &r, &disp->onion_rect ) ) {
            SkinRect src_rect;
            src_rect.pos.x = r2.pos.x - disp->onion_rect.pos.x;
            src_rect.pos.y = r2.pos.y - disp->onion_rect.pos.y;
            src_rect.size = r2.size;

            skin_surface_blit(surface,
                                &r2.pos,
                                skin_image_surface(disp->onion),
                                &src_rect,
                                SKIN_BLIT_SRCOVER);
        }
    }

    skin_surface_update(surface, &r);
}


typedef struct Button {
    SkinImage*       image;
    SkinRect         rect;
    SkinPos          origin;
    Background*      background;
    unsigned         keycode;
    int              down;
} Button;

static void
button_done( Button*  button )
{
    skin_image_unref( &button->image );
    button->background = NULL;
}

static void
button_init( Button*  button, SkinButton*  sbutton, SkinLocation*  loc, Background*  back, SkinRect*  frame, SkinLayout*  slayout )
{
    SkinRect  r;

    button->image      = skin_image_rotate( sbutton->image, loc->rotation );
    button->background = back;
    button->keycode    = sbutton->keycode;
    button->down       = 0;

    if (slayout->has_dpad_rotation) {
        /* Dpad keys must be rotated if the skin provides a 'dpad-rotation' field.
         * this is used as a counter-measure to the fact that the framework always assumes
         * that the physical D-Pad has been rotated when in landscape mode.
         */
        button->keycode = skin_keycode_rotate( button->keycode, -slayout->dpad_rotation );
    }

    skin_rect_rotate( &r, &sbutton->rect, loc->rotation );
    r.pos.x += loc->anchor.x;
    r.pos.y += loc->anchor.y;
    button->origin = r.pos;
    skin_rect_intersect( &button->rect, &r, frame );
}

static void
button_redraw(Button* button, SkinRect* rect, SkinSurface* surface)
{
    SkinRect  r;

    if (skin_rect_intersect( &r, rect, &button->rect ))
    {
        if ( button->down && button->image != SKIN_IMAGE_NONE )
        {
            SkinRect src_rect;
            src_rect.pos.x = r.pos.x - button->origin.x;
            src_rect.pos.y = r.pos.y - button->origin.y;
            src_rect.size = r.size;

            if (button->image != SKIN_IMAGE_NONE) {
                skin_surface_blit(surface,
                                  &r.pos,
                                  skin_image_surface(button->image),
                                  &src_rect,
                                  SKIN_BLIT_SRCOVER);
                if (button->down > 1) {
                    skin_surface_blit(surface,
                                      &r.pos,
                                      skin_image_surface(button->image),
                                      &src_rect,
                                      SKIN_BLIT_SRCOVER);
                }
            }
        }
    }
}


typedef enum {
    CORNER_TOP_LEFT,
    CORNER_TOP_RIGHT,
    CORNER_BOTTOM_RIGHT,
    CORNER_BOTTOM_LEFT
} WhichCorner;

typedef struct {
    char        tracking;
    char        inside;
    char        at_corner;
    WhichCorner which_corner;
    SkinPos     pos;
    ADisplay*   display;
} FingerState;

static void
finger_state_reset( FingerState*  finger )
{
    finger->tracking  = 0;
    finger->inside    = 0;
    finger->at_corner = 0;
    finger->pos.x = 0;
    finger->pos.y = 0;
}

typedef struct {
    char tracking;
    uint32_t button_pressed;
} MouseState;

static void mouse_state_reset(MouseState* mouse) {
    mouse->tracking = 0;
    mouse->button_pressed = 0u;
}

typedef struct {
    Button*   pressed;
    Button*   hover;
} ButtonState;

static void
button_state_reset( ButtonState*  button )
{
    button->pressed = NULL;
    button->hover   = NULL;
}

typedef struct {
    char            tracking;
    SkinTrackBall*  ball;
    SkinRect        rect;
    SkinWindow*     window;
} BallState;

static void
ball_state_reset( BallState*  state, SkinWindow*  window )
{
    state->tracking = 0;
    state->ball     = NULL;

    state->rect.pos.x  = 0;
    state->rect.pos.y  = 0;
    state->rect.size.w = 0;
    state->rect.size.h = 0;
    state->window      = window;
}

static void
ball_state_redraw(BallState* state, SkinRect* rect, SkinSurface* surface)
{
    SkinRect  r;

    if (skin_rect_intersect( &r, rect, &state->rect ))
        skin_trackball_draw(state->ball, 0, 0, surface);
}

static void
ball_state_show( BallState*  state, int  enable )
{
    if (enable) {
        if ( !state->tracking ) {
            state->tracking = 1;
            skin_trackball_refresh( state->ball );
            skin_window_redraw( state->window, &state->rect );
        }
    } else {
        if ( state->tracking ) {
            state->tracking = 0;
            skin_window_redraw( state->window, &state->rect );
        }
    }
}


static void
ball_state_set( BallState*  state, SkinTrackBall*  ball )
{
    ball_state_show( state, 0 );

    state->ball = ball;
    if (ball != NULL) {
        skin_trackball_rect(ball, &state->rect);
    }
}

typedef struct Layout {
    int          num_buttons;
    int          num_backgrounds;
    int          num_displays;
    unsigned     color;
    Button*      buttons;
    Background*  backgrounds;
    ADisplay*    displays;
    //Used for additional displays that can be added/removed on demand
    AIntMap* multi_displays;
    SkinRect     rect;
    SkinLayout*  slayout;
} Layout;

#define  LAYOUT_LOOP_BUTTONS(layout,button)                          \
    do {                                                             \
        Button*  __button = (layout)->buttons;                       \
        Button*  __button_end = __button + (layout)->num_buttons;    \
        for ( ; __button < __button_end; __button ++ ) {             \
            Button*  button = __button;

#define  LAYOUT_LOOP_END_BUTTONS \
        }                        \
    } while (0);

#define  LAYOUT_LOOP_DISPLAYS(layout,display)                          \
    do {                                                               \
        ADisplay*  __display = (layout)->displays;                     \
        ADisplay*  __display_end = __display + (layout)->num_displays; \
        for ( ; __display < __display_end; __display ++ ) {            \
            ADisplay*  display = __display;

#define  LAYOUT_LOOP_END_DISPLAYS \
        }                         \
    } while (0);

static void
layout_done( Layout*  layout )
{
    int  nn;

    for (nn = 0; nn < layout->num_buttons; nn++)
        button_done( &layout->buttons[nn] );

    for (nn = 0; nn < layout->num_backgrounds; nn++)
        background_done( &layout->backgrounds[nn] );

    for (nn = 0; nn < layout->num_displays; nn++)
        adisplay_done( &layout->displays[nn] );

    if (layout->multi_displays) {
        AIntMapIterator  iter[1];
        aintMapIterator_init(iter, layout->multi_displays);
        while (aintMapIterator_next(iter)) {
            adisplay_done(iter->value);
        }
        aintMapIterator_done(iter);
        aintMap_free(layout->multi_displays);
        layout->multi_displays = NULL;
    }

    AFREE( layout->buttons );
    layout->buttons = NULL;

    AFREE( layout->backgrounds );
    layout->backgrounds = NULL;

    AFREE( layout->displays );
    layout->displays = NULL;
    layout->num_buttons     = 0;
    layout->num_backgrounds = 0;
    layout->num_displays    = 0;
}

static int
layout_init( Layout*  layout, SkinLayout*  slayout )
{
    int       n_buttons, n_backgrounds, n_displays;

    /* first, count the number of elements of each kind */
    n_buttons     = 0;
    n_backgrounds = 0;
    n_displays    = 0;

    layout->color   = slayout->color;
    layout->slayout = slayout;

    SKIN_LAYOUT_LOOP_LOCS(slayout,loc)
        SkinPart*    part = loc->part;

        if ( part->background->valid )
            n_backgrounds += 1;
        if ( part->display->valid )
            n_displays += 1;

        SKIN_PART_LOOP_BUTTONS(part, sbutton)
            n_buttons += 1;
            (void)sbutton;
        SKIN_PART_LOOP_END
    SKIN_LAYOUT_LOOP_END

    layout->num_buttons     = n_buttons;
    layout->num_backgrounds = n_backgrounds;
    layout->num_displays    = n_displays;

    /* now allocate arrays, then populate them */
    AARRAY_NEW0(layout->buttons,     n_buttons);
    AARRAY_NEW0(layout->backgrounds, n_backgrounds);
    AARRAY_NEW0(layout->displays,    n_displays);

    layout->multi_displays = aintMap_new();
    if (layout->buttons == NULL && n_buttons > 0) goto Fail;
    if (layout->backgrounds == NULL && n_backgrounds > 0) goto Fail;
    if (layout->displays == NULL && n_displays > 0) goto Fail;

    n_buttons     = 0;
    n_backgrounds = 0;
    n_displays    = 0;

    layout->rect.pos.x = 0;
    layout->rect.pos.y = 0;
    layout->rect.size  = slayout->size;

    SKIN_LAYOUT_LOOP_LOCS(slayout,loc)
        SkinPart*    part = loc->part;
        Background*  back = NULL;

        if ( part->background->valid ) {
            back = layout->backgrounds + n_backgrounds;
            background_init( back, part->background, loc, &layout->rect );
            n_backgrounds += 1;
        }
        if ( part->display->valid ) {
            ADisplay*  disp = layout->displays + n_displays;
            adisplay_init( disp, part->display, loc, &layout->rect );
            n_displays += 1;
        }

        SKIN_PART_LOOP_BUTTONS(part, sbutton)
            Button*  button = layout->buttons + n_buttons;
            button_init( button, sbutton, loc, back, &layout->rect, slayout );
            n_buttons += 1;
        SKIN_PART_LOOP_END
    SKIN_LAYOUT_LOOP_END

    return 0;

Fail:
    layout_done(layout);
    return -1;
}

struct SkinWindow {
    const SkinWindowFuncs* win_funcs;
    SkinSurface*  surface;
    Layout        layout;
    SkinPos       pos;
    FingerState   finger;
    FingerState   secondary_finger;
    FingerState   pen;
    FingerState   multitouch_fingers[MTS_POINTERS_NUM];
    int           multitouch_fingers_ids[MTS_POINTERS_NUM ];
    //Map from display ID to FingerState.
    //Used for additional displays that can be added/removed on demand
    AIntMap*      multi_display_fingers;
    MouseState    mouse;
    ButtonState   button;
    BallState     ball;
    bool          use_emugl_subwindow;

    char          enable_touch;
    char          enable_trackball;
    char          enable_dpad;
    char          enable_qwerty;

    SkinImage*    onion;
    SkinRotation  onion_rotation;
    int           onion_alpha;

    int           x_pos;
    int           y_pos;
    double        scale;
    double        dpr; // Needed for HiDPI

    // For dragging and resizing the window
    int           drag_x_start;
    int           drag_y_start;
    int           window_x_start;
    int           window_y_start;

    // Zoom-related parameters
    double        zoom;
    SkinRect      subwindow;
    SkinPos       subwindow_original;
    SkinSize      framebuffer;
    SkinSize      container;
    int           scroll_h; // Needed for OSX
};

const FingerState empty_touch_point_state = {0};

static int getIndexOfMultitouchPoint(int idToSearchFor,
                                     const SkinWindow* const data) {
    for (unsigned i = 0; i < MTS_POINTERS_NUM ; ++i) {
        if (data->multitouch_fingers_ids[i] == idToSearchFor) {
            return i;
        }
    }
    return -1;
}

static int removeMultitouchFinger(int idOfTouchPointToRemove,
                                   SkinWindow* const data) {
    for (unsigned i = 0; i < MTS_POINTERS_NUM ; ++i) {
        if (data->multitouch_fingers_ids[i] == idOfTouchPointToRemove) {
            data->multitouch_fingers[i] = empty_touch_point_state;
            data->multitouch_fingers_ids[i] = FINGER_SLOT_FREE;
            return 0;
        }
    }
    return -1;
}

static int addMultitouchFinger(int idOfTouchPoint, SkinWindow* const data) {
    for (unsigned i = 0; i < MTS_POINTERS_NUM ; ++i) {
        if (data->multitouch_fingers_ids[i] == FINGER_SLOT_FREE) {
            data->multitouch_fingers_ids[i] = idOfTouchPoint;
            data->multitouch_fingers[i] = empty_touch_point_state;
            return i;
        }
    }
    return -1;
}

static void add_mouse_event(SkinWindow* window, int32_t rel_x, int32_t rel_y) {
    uint32_t id = 0;
    // TODO(liyl): handle multi-display and display rotation.
    window->win_funcs->mouse_event(rel_x, rel_y, window->mouse.button_pressed,
                                   id);
}

static void add_mouse_wheel_event(SkinWindow* window,
                                  int32_t x_delta,
                                  int32_t y_delta) {
    uint32_t id = 0;
    // TODO(liyl): handle multi-display and display rotation.
    window->win_funcs->mouse_wheel_event(x_delta, y_delta, id);
}

static  void
generate_touch_event(FingerState* finger,
                                 int* x,
                                 int* y,
                                 int* device_id,
                                 uint32_t display_id) {
    unsigned posX = *x;
    unsigned posY = *y;
    if (finger->display) {
        if (android_foldable_is_folded()) {
            int fx, fy, fw, fh;
            android_foldable_get_folded_area(&fx, &fy, &fw, &fh);
            switch (finger->display->rotation) {
                case SKIN_ROTATION_0:
                    posX = *x + fx;
                    posY = *y + fy;
                    break;
                case SKIN_ROTATION_180:
                    posX = *x + fx + fw - android_hw->hw_lcd_width;
                    posY = *y + fy + fh - android_hw->hw_lcd_height;
                    break;
                case SKIN_ROTATION_90:
                    posX = *x + fx;
                    posY = *y + fy + fh - android_hw->hw_lcd_height;
                    break;
                case SKIN_ROTATION_270:
                    posX = *x + fx + fw - android_hw->hw_lcd_width;
                    posY = *y + fy;
                    break;
            }
        } else {
            const QAndroidMultiDisplayAgent* const multiDisplayAgent =
                    emulator_window_get()->uiEmuAgent->multiDisplay;
            if (!multiDisplayAgent->isMultiDisplayWindow()) {
                multiDisplayAgent->translateCoordination(&posX, &posY, &display_id);
            }
        }
    }
    *x = posX;
    *y = posY;
    *device_id = display_id;
}

static void
add_finger_event(SkinWindow* window,
                 FingerState* finger,
                 unsigned x,
                 unsigned y,
                 unsigned state,
                 uint32_t displayId)
{
    unsigned posX = x;
    unsigned posY = y;

    if (finger->display) {
        if (android_foldable_is_folded()) {
            int fx, fy, fw, fh;
            android_foldable_get_folded_area(&fx, &fy, &fw, &fh);
            switch (finger->display->rotation) {
            case SKIN_ROTATION_0:
                posX = x + fx;
                posY = y + fy;
                break;
            case SKIN_ROTATION_180:
                posX = x + fx + fw - android_hw->hw_lcd_width;
                posY = y + fy + fh - android_hw->hw_lcd_height;
                break;
            case SKIN_ROTATION_90:
                posX = x + fx;
                posY = y + fy + fh - android_hw->hw_lcd_height;
                break;
            case SKIN_ROTATION_270:
                posX = x + fx + fw - android_hw->hw_lcd_width;
                posY = y + fy;
            break;
            }
        } else {
            const QAndroidMultiDisplayAgent* const multiDisplayAgent =
                emulator_window_get()->uiEmuAgent->multiDisplay;
            if (!multiDisplayAgent->isMultiDisplayWindow()) {
                multiDisplayAgent->translateCoordination(&posX, &posY, &displayId);
            }
        }
    }
    window->win_funcs->mouse_event(posX, posY, state, displayId);
}

static void
add_pen_event(SkinWindow* window,
              FingerState* pen,
              unsigned x,
              unsigned y,
              const SkinEvent* ev,
              unsigned state)
{
    unsigned posX = x;
    unsigned posY = y;
    uint32_t id = 0;

    if (pen->display) {
        if (android_foldable_is_folded()) {
            int fx, fy, fw, fh;
            android_foldable_get_folded_area(&fx, &fy, &fw, &fh);
            switch (pen->display->rotation) {
            case SKIN_ROTATION_0:
                posX = x + fx;
                posY = y + fy;
                break;
            case SKIN_ROTATION_180:
                posX = x + fx + fw - android_hw->hw_lcd_width;
                posY = y + fy + fh - android_hw->hw_lcd_height;
                break;
            case SKIN_ROTATION_90:
                posX = x + fx;
                posY = y + fy + fh - android_hw->hw_lcd_height;
                break;
            case SKIN_ROTATION_270:
                posX = x + fx + fw - android_hw->hw_lcd_width;
                posY = y + fy;
                break;
            }
        } else {
            const QAndroidMultiDisplayAgent* const multiDisplayAgent =
                emulator_window_get()->uiEmuAgent->multiDisplay;
            multiDisplayAgent->translateCoordination(&posX, &posY, &id);
        }
    }

    window->win_funcs->pen_event(posX, posY, ev, state, id);
}

static void
skin_window_find_finger( SkinWindow*  window,
                         FingerState* finger,
                         int          x,
                         int          y,
                         int displayId)
{
    ADisplay* multi_display = NULL;
    /* find the display that contains this movement */
    finger->display = NULL;
    finger->inside = 0;
    finger->at_corner = 0;

    if (!window->enable_touch)
        return;

    // First check multi display is available.
    multi_display = aintMap_get(window->layout.multi_displays, displayId);
    if (multi_display) {
        finger->inside = skin_rect_contains( &multi_display->rect, x, y );
        if (finger->inside) {
            finger->display = multi_display;
            finger->pos.x   = x - multi_display->origin.x;
            finger->pos.y   = y - multi_display->origin.y;
            skin_pos_rotate( &finger->pos, &finger->pos, multi_display->rotation );
            skin_winsys_set_window_cursor_normal();
        }
        return;
    }

    bool roundDevice = skin_surface_is_round(window->surface);
LAYOUT_LOOP_DISPLAYS(&window->layout,disp)
    if (roundDevice) {
        // The device's center is at (x + w/2, y + w/2),
        // (assuming w==h). To be on the screen, the touch
        // must be within w/2 of the center. Or d^2 < w^2/4.
        int xDist = disp->rect.pos.x + disp->rect.size.w/2 - x;
        int yDist = disp->rect.pos.y + disp->rect.size.w/2 - y;
        int distSquared = xDist*xDist + yDist*yDist;
        // On circular devices with a chin, the bottom of the screen is cut off,
        // so check to make sure that the touch is about the chin.
        if (y >= disp->rect.pos.y + disp->rect.size.h) {
            finger->inside = false;
        } else {
            finger->inside = distSquared <= (disp->rect.size.w * disp->rect.size.w)/4;
        }
    } else {
        // Rectangular device
        finger->inside = skin_rect_contains( &disp->rect, x, y );
    }
    if (finger->inside) {
        finger->display = disp;
        finger->pos.x   = x - disp->origin.x;
        finger->pos.y   = y - disp->origin.y;
        skin_pos_rotate( &finger->pos, &finger->pos, disp->rotation );
        skin_winsys_set_window_cursor_normal();
        break;
    }

    int roundAdjustment = 0;
    if (roundDevice) {
        // Round image
        // For rectangular images, the touch targets for resizing
        // include anything outsize the display's bounding rectangle.
        // This doesn't work for round devices because the frame
        // itself is inside that rectangle. Adjusting by 0.2 gives
        // an intuitive "corner" target.
        roundAdjustment = 0.2 * disp->rect.size.w;
    }
    int leftEdge   = disp->rect.pos.x                     + roundAdjustment;
    int rightEdge  = disp->rect.pos.x + disp->rect.size.w - roundAdjustment;
    int topEdge    = disp->rect.pos.y                     + roundAdjustment;
    int bottomEdge = disp->rect.pos.y + disp->rect.size.h - roundAdjustment;
    if (   (x < leftEdge || x >= rightEdge)
        && (y < topEdge  || y >= bottomEdge)
        && !skin_winsys_window_has_frame()                                     )
    {
        finger->at_corner = 1;
        // Which corner are we at?
        if (x < leftEdge) {
            finger->which_corner = (y < topEdge) ? CORNER_TOP_LEFT : CORNER_BOTTOM_LEFT;
        } else {
            finger->which_corner = (y < topEdge) ? CORNER_TOP_RIGHT : CORNER_BOTTOM_RIGHT;
        }
        skin_winsys_set_window_cursor_resize(finger->which_corner == CORNER_BOTTOM_LEFT ||
                                             finger->which_corner == CORNER_TOP_RIGHT     );
        break;
    } else {
        skin_winsys_set_window_cursor_normal();
        break;
    }
LAYOUT_LOOP_END_DISPLAYS
}

static void
skin_window_move_mouse( SkinWindow*  window,
                        FingerState* finger,
                        int          x,
                        int          y )
{
    ButtonState*  button = &window->button;

    if (finger->tracking && finger->display) {
        ADisplay*  disp   = finger->display;
        char       inside = 1;
        int        dx     = x - disp->rect.pos.x;
        int        dy     = y - disp->rect.pos.y;

        if (dx < 0) {
            dx = 0;
            inside = 0;
        }
        else if (dx >= disp->rect.size.w) {
            dx = disp->rect.size.w - 1;
            inside = 0;
        }
        if (dy < 0) {
            dy = 0;
            inside = 0;
        } else if (dy >= disp->rect.size.h) {
            dy = disp->rect.size.h-1;
            inside = 0;
        }
        finger->inside = inside;
        finger->pos.x  = dx + (disp->rect.pos.x - disp->origin.x);
        finger->pos.y  = dy + (disp->rect.pos.y - disp->origin.y);

        skin_pos_rotate( &finger->pos, &finger->pos, disp->rotation );
    }

    {
        Button*  hover = button->hover;

        if (hover) {
            if ( skin_rect_contains( &hover->rect, x, y ) )
                return;

            hover->down = 0;
            skin_window_redraw( window, &hover->rect );
            button->hover = NULL;
        }

        hover = NULL;
        LAYOUT_LOOP_BUTTONS( &window->layout, butt )
            if ( skin_rect_contains( &butt->rect, x, y ) ) {
                hover = butt;
                break;
            }
        LAYOUT_LOOP_END_BUTTONS

        /* filter DPAD and QWERTY buttons right here */
        if (hover != NULL) {
            switch (hover->keycode) {
                /* these correspond to the DPad */
                case kKeyCodeDpadUp:
                case kKeyCodeDpadDown:
                case kKeyCodeDpadLeft:
                case kKeyCodeDpadRight:
                case kKeyCodeDpadCenter:
                    if (!window->enable_dpad)
                        hover = NULL;
                    break;

                /* these correspond to non-qwerty buttons */
                case kKeyCodeSoftLeft:
                case kKeyCodeSoftRight:
                case kKeyCodeVolumeUp:
                case kKeyCodeVolumeDown:
                case kKeyCodePower:
                case kKeyCodeHome:
                case kKeyCodeHomePage:
                case kKeyCodeBack:
                case kKeyCodeCall:
                case kKeyCodeEndCall:
                case kKeyCodeTV:
                case kKeyCodeEPG:
                case kKeyCodeDVR:
                case kKeyCodePrevious:
                case kKeyCodeNext:
                case kKeyCodePlay:
                case kKeyCodePlaypause:
                case kKeyCodePause:
                case kKeyCodeStop:
                case kKeyCodeRewind:
                case kKeyCodeFastForward:
                case kKeyCodeBookmarks:
                case kKeyCodeCycleWindows:
                case kKeyCodeChannelUp:
                case kKeyCodeChannelDown:
                case kKeyCodeAppSwitch:
                case kKeyCodeStemPrimary:
                case kKeyCodeStem1:
                case kKeyCodeStem2:
                case kKeyCodeStem3:
                case kKeyCodeSleep:
                    break;

                /* all the rest is assumed to be qwerty */
                default:
                    if (!window->enable_qwerty)
                        hover = NULL;
            }
        }

        if (hover != NULL) {
            hover->down = 1;
            skin_window_redraw( window, &hover->rect );
            button->hover = hover;
        }
    }
}

static void
skin_window_trackball_press( SkinWindow*  window, int  down )
{
    window->win_funcs->key_event(LINUX_BTN_MOUSE, down);
}

static void
skin_window_trackball_move( SkinWindow*  window, int  xrel, int  yrel )
{
    BallState*  state = &window->ball;

    if ( skin_trackball_move( state->ball, xrel, yrel ) ) {
        skin_trackball_refresh( state->ball );
        skin_window_redraw( window, &state->rect );
    }
}

void
skin_window_set_trackball( SkinWindow*  window, SkinTrackBall*  ball )
{
    BallState*  state = &window->ball;

    ball_state_set( state, ball );
}

void
skin_window_show_trackball( SkinWindow*  window, int  enable )
{
    BallState*  state = &window->ball;

    if (state->ball != NULL && window->enable_trackball) {
        ball_state_show(state, enable);
    }
}

typedef struct {
    SkinWindow* window;
    int wx;
    int wy;
    int ww;
    int wh;
    int fbw;
    int fbh;
    float rot;
    float dpr;
    bool deleteExisting;
} gles_show_data;

static void skin_window_run_opengles_show(void* p) {
    gles_show_data* data = p;
    data->window->win_funcs->opengles_show(skin_winsys_get_window_handle(),
                                           data->wx,
                                           data->wy,
                                           data->ww,
                                           data->wh,
                                           data->fbw,
                                           data->fbh,
                                           data->dpr,
                                           data->rot,
                                           data->deleteExisting);
    AFREE(data);
}

static void
skin_window_setup_opengles_subwindow( SkinWindow* window, gles_show_data* data)
{
    data->window = window;
    data->wx = window->subwindow.pos.x;
    data->wy = window->subwindow.pos.y;
    data->ww = window->subwindow.size.w;
    data->wh = window->subwindow.size.h;

    data->fbw = window->framebuffer.w;
    data->fbh = window->framebuffer.h;

    data->dpr = 1.0;
    // Only sets the pixel ratio to adjust the framebuffer size if HiDPI scaling
    // is enabled.
    if (!android_cmdLineOptions->no_hidpi_scaling) {
        data->dpr = window->dpr;
    }

#if 0
    printf("%s pos %d %d, size %d %d, fb %d %d\n", __func__,
           data->wx, data->wy, data->ww, data->wh,
           data->fbw, data->fbh);
#endif
#if defined(__APPLE__)
    // The native GL subwindow for OSX (using Cocoa) uses cartesian (y-up) coordinates. We
    // have code to transform window (y-down) coordinates into cartesian coordinates at that
    // level, but Qt seems to do this transformation on its own. This means the transformation
    // is done *twice* with Qt + OSX, resulting in the incorrect y value. The following "undoes"
    // the transformation by doing it a third time. Additionally, because the scroll bar now
    // affects the relative coordinate system inside the window, it must be taken into account
    // as well. At the end of this function, data->wy will equal the bottom coordinate of the
    // subwindow (in units from the bottom of the overall window) if this is the Qt OSX emulator.
    data->wy = window->container.h - (data->wy + data->wh);
    data->wy += window->scroll_h;
#endif  // __APPLE__
}

/* Show the OpenGL ES framebuffer window */
static void
skin_window_show_opengles(SkinWindow* window, bool deleteExisting)
{
    ADisplay* disp = window->layout.displays;
    gles_show_data* data = NULL;
    ANEW0(data);
    skin_window_setup_opengles_subwindow(window, data);
    data->rot = disp->rotation * 90.;
    data->deleteExisting = deleteExisting;
    skin_winsys_run_ui_update(&skin_window_run_opengles_show, data, false);
}

static void skin_window_redraw_opengles(SkinWindow* window) {
    window->win_funcs->opengles_redraw();
}

static int  skin_window_reset_internal (SkinWindow*, SkinLayout*);

typedef struct {
    SkinWindow* window;
    SkinRect monitor;
    int win_w;
    int win_h;
} EnsureFullyVisibleData;

static void skin_window_ensure_fully_visible(void* ptr) {
    EnsureFullyVisibleData* data = ptr;
    if (!skin_winsys_is_window_fully_visible()) {
        int new_x, new_y;

        if (VERBOSE_CHECK(init)) {
            int win_x, win_y, win_w, win_h;
            skin_winsys_get_window_pos(&win_x, &win_y);
            win_w = skin_surface_width(data->window->surface);
            win_h = skin_surface_height(data->window->surface);

            dprint("Window was not fully visible: "
                    "monitor=[%d,%d,%d,%d] window=[%d,%d,%d,%d]",
                    data->monitor.pos.x,
                    data->monitor.pos.y,
                    data->monitor.size.w,
                    data->monitor.size.h,
                    win_x,
                    win_y,
                    win_w,
                    win_h);
        }

        /* First, we recenter the window */
        new_x = (data->monitor.size.w - data->win_w)/2;
        new_y = (data->monitor.size.h - data->win_h)/2;

        /* If it is still too large, we ensure the top-border is visible */
        if (new_y < 0)
            new_y = 0;

        /* If it is somehow off screen horizontally, put it back */
        if (new_x < 0)
            new_x = 0;

        /* Don't try to put it back if the emulator is only partially too
           far to the right, because that invites bouncing. */
        if (new_x >= data->monitor.size.w)
            new_x -= data->win_w;

        VERBOSE_PRINT(init, "Window repositioned to [%d,%d]", new_x, new_y);

        /* Done */
        skin_winsys_set_window_pos(new_x, new_y);
        dprint( "emulator window was out of view and was recentered" );
    }
    AFREE(data);
}

static void
skin_window_set_device_pixel_ratio( SkinWindow* window )
{
    double dpr = 1.0;
#ifdef __APPLE__
    // Only macOS devices with Retina display needs to get the device pixel
    // ratio to zoom the opengles display.
    skin_winsys_get_device_pixel_ratio(&dpr);
#endif
    window->dpr = dpr;
}

SkinWindow* skin_window_create(SkinLayout* slayout,
                               int x,
                               int y,
                               bool enable_scale,
                               bool use_emugl_subwindow,
                               const SkinWindowFuncs* win_funcs) {
    SkinWindow*  window;

    ANEW0(window);

    window->win_funcs    = win_funcs;
    window->use_emugl_subwindow = use_emugl_subwindow;
    window->surface = NULL;

    /* enable everything by default */
    window->enable_touch     = 1;
    window->enable_trackball = 1;
    window->enable_dpad      = 1;
    window->enable_qwerty    = 1;

    window->x_pos = x;
    window->y_pos = y;
    window->scroll_h = 0;

    window->drag_x_start = 0;
    window->drag_y_start = 0;

    skin_window_set_device_pixel_ratio(window);

    SkinRect  monitor;
    int       win_w = slayout->size.w;
    int       win_h = slayout->size.h;
    double    scale_w = 1.0;
    double    scale_h = 1.0;

    skin_winsys_get_monitor_rect(&monitor);

    // Make the default scale about 80% of the screen size.
    if (enable_scale && monitor.size.w < win_w && win_w > 1.)
        scale_w = 0.80 * monitor.size.w / win_w;
    if (enable_scale && monitor.size.h < win_h && win_h > 1.)
        scale_h = 0.80 * monitor.size.h / win_h;

    window->scale = (scale_w <= scale_h) ? scale_w : scale_h;

    if (skin_window_reset_internal(window, slayout) < 0) {
        skin_window_free(window);
        return NULL;
    }
    skin_winsys_set_window_pos(x, y);

    /* Ensure that the window is fully visible */
    if (enable_scale) {
        EnsureFullyVisibleData* data;
        ANEW0(data);
        data->window = window;
        data->monitor = monitor;
        data->win_w = win_w;
        data->win_h = win_h;
        skin_winsys_run_ui_update(skin_window_ensure_fully_visible,
                                  data, false);
    }

    skin_window_show_opengles(window, false);

    return window;
}

void
skin_window_enable_touch( SkinWindow*  window, int  enabled )
{
    window->enable_touch = !!enabled;
}

void
skin_window_enable_trackball( SkinWindow*  window, int  enabled )
{
    window->enable_trackball = !!enabled;
}

void
skin_window_enable_dpad( SkinWindow*  window, int  enabled )
{
    window->enable_dpad = !!enabled;
}

void
skin_window_enable_qwerty( SkinWindow*  window, int  enabled )
{
    window->enable_qwerty = !!enabled;
}

void
skin_window_set_title( SkinWindow*  window, const char*  title )
{
    if (window && title) {
        skin_winsys_set_window_title(title);
    }
}

void
skin_window_position_changed( SkinWindow* window, int x, int y )
{
    window->x_pos = x;
    window->y_pos = y;
}

int
skin_window_recompute_subwindow_rect( SkinWindow* window, SkinRect* subwindow )
{
    // The full subwindow must be intersected with the container to compute the actual subwindow
    SkinRect result;

    SkinRect container;
    container.pos.x = 0;
    container.pos.y = 0;
    container.size.w = window->container.w;
    container.size.h = window->container.h;

    // If the emulator window is so small that the native subwindow wouldn't even appear,
    // force the subwindow to at least have positive size, else the native window managers
    // may crash. For example, on Linux, X11 crashes when told to create a window with 0 size.
    if (!skin_rect_intersect(&result, subwindow, &container)) {
        result.size.w = 1;
        result.size.h = 1;
    }

    if (skin_rect_equals(&window->subwindow, &result)) {
        return 0;
    }

    window->subwindow.pos.x = result.pos.x;
    window->subwindow.pos.y = result.pos.y;
    window->subwindow.size.w = result.size.w;
    window->subwindow.size.h = result.size.h;

    skin_winsys_set_device_geometry(&window->subwindow);

    return 1;
}

void
skin_window_scroll_updated( SkinWindow* window, int dx, int xmax, int dy, int ymax )
{
    // Pretend the subwindow has moved by the appropriate amount
    SkinRect subwindow;
    subwindow.pos.x = window->subwindow_original.x - dx;
    subwindow.pos.y = window->subwindow_original.y - dy;
    subwindow.size.w = window->framebuffer.w;
    subwindow.size.h = window->framebuffer.h;

    skin_window_recompute_subwindow_rect(window, &subwindow);
    skin_window_show_opengles(window, false);

    // Compute the margins around the sub-window, then transform the current scroll values
    // to take into account these margins.
    int left_buf = window->subwindow_original.x;
    int right_buf = skin_surface_width(window->surface)
                        - (window->framebuffer.w + left_buf);
    float px;
    if (left_buf + right_buf >= xmax) {
        // The subwindow fits entirely in the container, so there are no intermediate translations
        px = dx < left_buf ? 0 : 1;
    } else {
        px = (float) (dx - left_buf) / (float) (xmax - (left_buf + right_buf));
    }

    int top_buf = window->subwindow_original.y;
    int bottom_buf = skin_surface_height(window->surface)
                         - (window->framebuffer.h + top_buf);
    float py;
    if (top_buf + bottom_buf >= ymax) {
        // The subwindow fits entirely in the container, so there are no intermediate translations
        py = dy < top_buf ? 1 : 0;
    } else {
        // Use (ymax - dy) instead of (dy) since OGL coordinates are Y-up
        py = (float) ((ymax - dy) - bottom_buf) / (float) (ymax - (bottom_buf + top_buf));
    }

    // Clamp the values to [0,1]. (dx,dy) = (0,0) indicates to align the bottom left corner of the
    // framebuffer with the bottom left corner of the subwindow. (1,1) indicates to align the
    // top right corner of the framebuffer with the top right corner of the subwindow. All
    // intermediate values are an interpolation between these two states.
    px = px > 1 ? 1 : (px < 0 ? 0 : px);
    py = py > 1 ? 1 : (py < 0 ? 0 : py);

    window->win_funcs->opengles_setTranslation(px, py);
}

static void
skin_window_resize( SkinWindow*  window, int resize_container )
{
    int           layout_w = window->layout.rect.size.w;
    int           layout_h = window->layout.rect.size.h;
    int           window_w = layout_w;
    int           window_h = layout_h;
    int           window_x = window->x_pos;
    int           window_y = window->y_pos;
    double        scale = window->scale;
    double        dpr = window->dpr;

    // Pre-record the container dimensions
    if (resize_container) {
        window->container.w = (int) ceil(layout_w * scale);
        window->container.h = (int) ceil(layout_h * scale);
    }

    if (window->zoom != 1.0) {
        scale *= window->zoom;
    }

    if (scale != 1.0) {
        window_w = (int) ceil(layout_w * scale);
        window_h = (int) ceil(layout_h * scale);
    }

    // If HiDPI scaling is disabled on the device, window->container and
    // (window_w, window_h) should be the actual pixel size of container and
    // the window; the logical pixel size used for Qt should be divided by the
    // display's pixel ratio.
    if (android_cmdLineOptions->no_hidpi_scaling) {
        window->container.w = (int) ceil(window->container.w / dpr);
        window->container.h = (int) ceil(window->container.h / dpr);
        window_w = (int) ceil(window_w / dpr);
        window_h = (int) ceil(window_h / dpr);
    }

    // Attempt to resize the window surface. If it doesn't exist, a new one will be
    // allocated. If it does exist, but its original dimensions do not match the new
    // ones, it will be de-allocated and a new one will be returned.
    window->surface = skin_surface_resize(window->surface,
                                          window_w, window_h,
                                          layout_w, layout_h);

    // Force redraw the background and skin to avoid drawing empty frames. This reduces flicker
    // on resize and rotate.
    Layout* layout = &window->layout;
    skin_surface_fill(window->surface,
                      &layout->rect,
                      layout->color);

    Background*  back = layout->backgrounds;
    Background*  end  = back + layout->num_backgrounds;
    for ( ; back < end; back++ )
        background_redraw( back, &layout->rect, window->surface );

    skin_surface_create_window(window->surface, window_x, window_y,
                               window_w, window_h);

    // Calculate the framebuffer and window sizes and locations
    ADisplay* disp = window->layout.displays;
    SkinRect drect = disp->rect;
    skin_surface_get_scaled_rect(window->surface, &drect, &drect);

    // Store original values to use for scrolling later
    window->subwindow_original.x = drect.pos.x;
    window->subwindow_original.y = drect.pos.y;

    window->framebuffer.w = drect.size.w;
    window->framebuffer.h = drect.size.h;

    // The OpenGL framebuffer subwindow uses actual pixel size to initialize.
    // so size of "window->framebuffer" should be the Skin size (logical pixel)
    // multiplies display's pixel ratio.
    if (android_cmdLineOptions->no_hidpi_scaling) {
        window->framebuffer.w = window->framebuffer.w * dpr;
        window->framebuffer.h = window->framebuffer.h * dpr;
    }

    if (skin_window_recompute_subwindow_rect(window, &drect)) {
        skin_window_show_opengles(window, false);
    }
}

static int
skin_window_reset_internal ( SkinWindow*  window, SkinLayout*  slayout )
{
    Layout         layout;

    ADisplay*      disp;

    if ( layout_init( &layout, slayout ) < 0 )
        return -1;

    layout_done( &window->layout );

    window->layout = layout;

    // Reset viewport parameters
    window->zoom = 1.0;
    window->scroll_h = 0;
    window->framebuffer.w = 0;
    window->framebuffer.h = 0;

    // Resetting the subwindow parameters ensures that a proper resizing of
    // the native subwindow actually occurs. Without this, it is possible for
    // a rotation to not reach the subwindow (for example, in the case of a
    // square watch skin, which might have two layouts with *exactly* the same
    // subwindow).
    window->subwindow.pos.x = -1;
    window->subwindow.pos.y = -1;
    window->subwindow.size.w = 0;
    window->subwindow.size.h = 0;

    disp = window->layout.displays;

    if (disp != NULL) {
        if (slayout->onion_image) {
            // Onion was specified in layout file.
            adisplay_set_onion(disp,
                               slayout->onion_image,
                               slayout->onion_rotation,
                               slayout->onion_alpha );
        } else if (window->onion) {
            // Onion was specified via command line.
            adisplay_set_onion(disp,
                               window->onion,
                               window->onion_rotation,
                               window->onion_alpha );
        }
    }

    skin_window_resize(window, 1);

    finger_state_reset( &window->finger );
    finger_state_reset( &window->secondary_finger );
    finger_state_reset( &window->pen );
    button_state_reset( &window->button );
    ball_state_reset( &window->ball, window );
    mouse_state_reset( &window->mouse );

    skin_window_redraw( window, NULL );

    if ( layout.displays ) {
        // TODO(grigoryj): debug output for investigating the rotation bug.
        if (VERBOSE_CHECK(rotation)) {
            dinfo("Setting device orientation %d", layout.displays->rotation);
        }
        window->win_funcs->set_device_orientation(layout.displays->rotation);
    }

    for(unsigned i=0;i<MTS_POINTERS_NUM;++i)
    {
         window->multitouch_fingers_ids[i] = FINGER_SLOT_FREE;
         window->multitouch_fingers[i] = empty_touch_point_state;
    }
    if (window->multi_display_fingers) {
        aintMap_free(window->multi_display_fingers);
    }
    window->multi_display_fingers = aintMap_new();
    return 0;
}

int
skin_window_reset ( SkinWindow*  window, SkinLayout*  slayout )
{
    skin_winsys_get_window_pos(&window->x_pos, &window->y_pos);
    if (skin_window_reset_internal( window, slayout ) < 0)
        return -1;

    return 0;
}

void
skin_window_zoomed_window_resized( SkinWindow* window, int dx, int dy, int w, int h, int scroll_h )
{
    // Pretend the subwindow has moved by the appropriate amount
    SkinRect subwindow;
    subwindow.pos.x = window->subwindow_original.x - dx;
    subwindow.pos.y = window->subwindow_original.y - dy;
    subwindow.size.w = window->framebuffer.w;
    subwindow.size.h = window->framebuffer.h;

    window->container.w = w;
    window->container.h = h;
    window->scroll_h = scroll_h;

    if (skin_window_recompute_subwindow_rect(window, &subwindow)) {
        skin_window_show_opengles(window, false);
    }
}

void
skin_window_set_lcd_brightness( SkinWindow*  window, int  brightness )
{
    ADisplay*  disp = window->layout.displays;

    if (disp != NULL) {
        disp->brightness = brightness;
        skin_window_redraw( window, NULL );
    }
}

void
skin_window_free( SkinWindow*  window )
{
    if (window) {
        skin_surface_unrefp(&window->surface);

        if (window->onion) {
            skin_image_unref(&window->onion);
            window->onion_rotation = SKIN_ROTATION_0;
        }
        layout_done( &window->layout );
        AFREE(window);
    }
}

void
skin_window_set_onion( SkinWindow*   window,
                       SkinImage*    onion,
                       SkinRotation  onion_rotation,
                       int           onion_alpha )
{
    ADisplay*  disp;
    SkinImage*  old = window->onion;

    window->onion          = skin_image_ref(onion);
    window->onion_rotation = onion_rotation;
    window->onion_alpha    = onion_alpha;

    skin_image_unref( &old );

    disp = window->layout.displays;

    if (disp != NULL)
        adisplay_set_onion(disp, window->onion, onion_rotation, onion_alpha);
}

void
skin_window_set_layout_region(SkinWindow* window, int xOffset, int yOffset, int width, int height)
{
    int displayIdx;
    for (displayIdx = 0; displayIdx < window->layout.num_displays; displayIdx++) {
        window->layout.displays[displayIdx].rect.pos.x = xOffset;
        window->layout.displays[displayIdx].rect.pos.y = yOffset;
    }

    window->layout.rect.size.w = width;
    window->layout.rect.size.h = height;
}

void
skin_window_set_display_region_and_update(SkinWindow* window, int xOffset,
                                          int yOffset, int width, int height)
{

    int displayIdx;
    for (displayIdx = 0; displayIdx < window->layout.num_displays; displayIdx++) {
        ADisplay* disp = &(window->layout.displays[displayIdx]);
        disp->rect.pos.x = xOffset;
        disp->rect.pos.y = yOffset;
        disp->rect.size.w = width;
        disp->rect.size.h = height;
        switch(disp->rotation) {
          case SKIN_ROTATION_0:
              disp->origin = disp->rect.pos;
              break;
          case SKIN_ROTATION_90:
              disp->origin.x = disp->rect.pos.x + disp->rect.size.w;
              disp->origin.y = disp->rect.pos.y;
              break;
          case SKIN_ROTATION_180:
              disp->origin.x = disp->rect.pos.x + disp->rect.size.w;
              disp->origin.y = disp->rect.pos.y + disp->rect.size.h;
              break;
          case SKIN_ROTATION_270:
              disp->origin.x = disp->rect.pos.x;
              disp->origin.y = disp->rect.pos.y + disp->rect.size.h;
              break;
        }
    }

    window->layout.rect.size.w = width;
    window->layout.rect.size.h = height;
}

void
skin_window_set_scale(SkinWindow* window, double scale)
{
    window->scale = scale;

    // The scroll bars *will* be gone if this function is called, so make sure
    // they are not taken into account when resizing the window.
    window->scroll_h = 0;
    window->zoom = 1.0;      // Scaling the window should reset all "viewport" parameters

    skin_window_resize( window, 1 );
    skin_window_redraw( window, NULL );
}

void
skin_window_set_zoom(SkinWindow* window, double zoom, int dw, int dh, int scroll_h)
{
    // Pre-record the container dimensions
    window->container.w = dw;
    window->container.h = dh;
    window->scroll_h = scroll_h;
    window->zoom = zoom;

    skin_window_resize( window, 0 );
    skin_window_redraw( window, NULL );
}

void
skin_window_redraw( SkinWindow*  window, SkinRect*  rect )
{
    if (window != NULL && window->surface != NULL) {
        Layout*  layout = &window->layout;

        if (rect == NULL)
            rect = &layout->rect;

        {
            SkinRect  r;

            if ( skin_rect_intersect( &r, rect, &layout->rect ) ) {
                skin_surface_fill(window->surface,
                                  &r,
                                  layout->color);
            }
        }

        {
            Background*  back = layout->backgrounds;
            Background*  end  = back + layout->num_backgrounds;
            for ( ; back < end; back++ )
                background_redraw( back, rect, window->surface );
        }

        {
            ADisplay*  disp = layout->displays;
            ADisplay*  end  = disp + layout->num_displays;
            for (; disp < end; disp++) {
                adisplay_redraw(disp, rect, window->surface,
                                window->use_emugl_subwindow);
            }
        }

        {
            Button*  button = layout->buttons;
            Button*  end    = button + layout->num_buttons;
            for ( ; button < end; button++ )
                button_redraw( button, rect, window->surface );
        }

        if ( window->ball.tracking )
            ball_state_redraw( &window->ball, rect, window->surface );

        skin_surface_update(window->surface, rect);
        skin_window_redraw_opengles(window);
    }
}

static void
skin_window_map_to_scale( SkinWindow*  window, int  *x, int  *y )
{
    skin_surface_reverse_map(window->surface, x, y);
}


void
skin_window_process_touch_event(SkinWindow*  window, SkinEvent* ev){
    Button*  button;
    int      mx, my;
    int button_state = 0;
    int deviceId = 0;
    int displayId = ev->u.multi_touch_point.display_id;
    FingerState* touchFinger = NULL;
    if (ev->type == kEventTouchBegin || ev->type == kEventTouchEnd ||
        ev->type == kEventTouchUpdate) {
        button_state = multitouch_create_buttons_state(
                ev->type != kEventTouchEnd, ev->u.multi_touch_point.skip_sync,
                false);
        int index = 0;
        if (ev->type == kEventTouchBegin) {
            index = addMultitouchFinger(ev->u.multi_touch_point.id,window);
        } else{
            index = getIndexOfMultitouchPoint(
                    ev->u.multi_touch_point.id,
                    window);
        }
        if(index == -1){
            return;
        }
        touchFinger = &window->multitouch_fingers[index];
    } else {
        return;
    }
    switch (ev->type) {
       case kEventTouchBegin:
       {
            if (window->ball.tracking) {
                skin_window_trackball_press(window, 1);
                break;
            }
            mx = ev->u.multi_touch_point.x;
            my = ev->u.multi_touch_point.y;
            skin_window_map_to_scale(window, &mx, &my);
            skin_window_move_mouse(window, touchFinger, mx, my);
            skin_window_find_finger(window, touchFinger, mx, my, displayId);
            if (touchFinger->inside) {
                touchFinger->tracking = 1;
                generate_touch_event(touchFinger, &touchFinger->pos.x,
                                     &touchFinger->pos.y, &deviceId, displayId);
                ev->u.multi_touch_point.x = touchFinger->pos.x;
                ev->u.multi_touch_point.y = touchFinger->pos.y;
                window->win_funcs->touch_events(ev, deviceId);
            } else if (!multitouch_should_skip_sync(button_state)) {
                window->button.pressed = NULL;
                button = window->button.hover;
                if (button) {
                    button->down += 1;
                    skin_window_redraw(window, &button->rect);
                    window->button.pressed = button;
                    if (button->keycode) {
                        window->win_funcs->key_event(button->keycode, 1);
                    }
                } else if (!skin_winsys_window_has_frame()) {
                    window->drag_x_start = ev->u.multi_touch_point.x_global;
                    window->drag_y_start = ev->u.multi_touch_point.y_global;
                    skin_winsys_get_frame_pos(&window->window_x_start,
                                              &window->window_y_start);
                    if (touchFinger->at_corner) {
                        skin_winsys_set_window_overlay_for_resize(
                                touchFinger->which_corner);
                    }
                }
            }
            break;
       }
       case kEventTouchEnd:
       {
            if (window->drag_x_start != 0 && window->drag_y_start != 0) {
                if (!touchFinger->at_corner) {
                    if (skin_winsys_is_window_off_screen()) {
                        skin_winsys_set_window_pos(0, 0);
                    }
                    window->drag_x_start = 0;
                    window->drag_y_start = 0;
                    return;
                }
                touchFinger->at_corner = 0;
                skin_winsys_clear_window_overlay();
                int window_pos_x, window_pos_y;
                int window_width, window_height;
                skin_winsys_get_frame_pos(&window_pos_x, &window_pos_y);
                skin_winsys_get_window_size(&window_width, &window_height);
                int delta_x =
                        ev->u.multi_touch_point.x_global - window->drag_x_start;
                int delta_y =
                        ev->u.multi_touch_point.y_global - window->drag_y_start;
                window->drag_x_start = 0;
                window->drag_y_start = 0;
                switch (touchFinger->which_corner) {
                    case CORNER_BOTTOM_RIGHT:
                        window_width += delta_x;
                        window_height += delta_y;
                        break;
                    case CORNER_TOP_RIGHT:
                        window_pos_y += delta_y;
                        window_width += delta_x;
                        window_height -= delta_y;
                        break;
                    case CORNER_BOTTOM_LEFT:
                        window_pos_x += delta_x;
                        window_width -= delta_x;
                        window_height += delta_y;
                        break;
                    case CORNER_TOP_LEFT:
                        window_pos_x += delta_x;
                        window_pos_y += delta_y;
                        window_width -= delta_x;
                        window_height -= delta_y;
                        break;
                }
                skin_winsys_set_window_pos(window_pos_x, window_pos_y);
                skin_winsys_set_window_size(window_width, window_height);
                skin_winsys_set_window_cursor_normal();
                return;
            }
            window->drag_x_start = 0;
            window->drag_y_start = 0;
            if (window->ball.tracking) {
                skin_window_trackball_press(window, 0);
                break;
            }
            button = window->button.pressed;
            mx = ev->u.multi_touch_point.x;
            my = ev->u.multi_touch_point.y;
            skin_window_map_to_scale(window, &mx, &my);
            if (button) {
                button->down = 0;
                skin_window_redraw(window, &button->rect);
                if (button->keycode) {
                    window->win_funcs->key_event(button->keycode, 0);
                }
                window->button.pressed = NULL;
                window->button.hover = NULL;
                skin_window_move_mouse(window, touchFinger, mx, my);
            } else if(touchFinger->tracking){
                deviceId = 0;
                touchFinger->tracking = 0;
                skin_window_move_mouse(window, touchFinger, mx, my);
                generate_touch_event(touchFinger, &touchFinger->pos.x,
                                     &touchFinger->pos.y, &deviceId, displayId);
                ev->u.multi_touch_point.x = touchFinger->pos.x;
                ev->u.multi_touch_point.y = touchFinger->pos.y;
                removeMultitouchFinger(ev->u.multi_touch_point.id,window);
                window->win_funcs->touch_events(ev, deviceId);
            }
            break;
        }
       case kEventTouchUpdate:
       {
            mx = ev->u.multi_touch_point.x;
            my = ev->u.multi_touch_point.y;
            skin_window_map_to_scale(window, &mx, &my);
            skin_window_move_mouse(window, touchFinger, mx, my);
            if (window->drag_x_start == 0 && window->drag_y_start == 0) {
                skin_window_find_finger(window, touchFinger, mx, my, displayId);
            } else {
                if (touchFinger->at_corner) {
                    skin_winsys_paint_overlay_for_resize(mx, my);
                    break;
                }
                int posX = window->window_x_start +
                           ev->u.multi_touch_point.x_global -
                           window->drag_x_start;
                int posY = window->window_y_start +
                           ev->u.multi_touch_point.y_global -
                           window->drag_y_start;
                skin_winsys_set_window_pos(posX, posY);
                break;
            }
            mx = ev->u.multi_touch_point.x;
            my = ev->u.multi_touch_point.y;
            skin_window_map_to_scale(window, &mx, &my);
            if (!window->button.pressed) {
                skin_window_move_mouse(window, touchFinger, mx, my);
                if (touchFinger->tracking) {
                    generate_touch_event(touchFinger, &touchFinger->pos.x,
                                         &touchFinger->pos.y, &deviceId, displayId);
                    ev->u.multi_touch_point.x = touchFinger->pos.x;
                    ev->u.multi_touch_point.y = touchFinger->pos.y;
                    window->win_funcs->touch_events(ev, deviceId);
                }
            }
            break;
       }
       default:
            derror("%s: Invalid enum value passed to function. Only accepts: kEventTouchBegin,kEventTouchEnd,kEventTouchUpdate", __FUNCTION__);
            break;
    }
}


void
skin_window_process_event(SkinWindow*  window, SkinEvent* ev)
{
    Button*  button;
    int      mx, my;
    uint32_t displayId = ev->u.mouse.display_id;
    // This button state set will still be interpreted correctly for
    // single-touch, which only uses the first bit.
    int button_state = multitouch_create_buttons_state(
            ev->type != kEventMouseButtonUp, ev->u.mouse.skip_sync,
            ev->u.mouse.button == kMouseButtonSecondaryTouch);

    FingerState* finger = NULL;
    if (displayId == 0) {
        if (ev->u.mouse.button == kMouseButtonSecondaryTouch) {
            finger = &window->secondary_finger;
        } else {
            finger = &window->finger;
        }
    } else {
        finger = aintMap_get(window->multi_display_fingers, displayId);
        if (!finger) {
            derror("ERROR: display with id %u is not found.\n", displayId);
            return;
        }
    }

    MouseState* mouse = &window->mouse;

    if (!window->surface)
        return;

    switch (ev->type) {
    case kEventMouseButtonDown:
        if ( window->ball.tracking ) {
            skin_window_trackball_press( window, 1 );
            break;
        }

        mx = ev->u.mouse.x;
        my = ev->u.mouse.y;
        skin_window_map_to_scale( window, &mx, &my );
        skin_window_move_mouse( window, finger, mx, my );
        skin_window_find_finger( window, finger, mx, my, displayId);
#if 0
        printf("down: x=%d y=%d fx=%d fy=%d fis=%d\n",
               ev->u.mouse.x, ev->u.mouse.y, window->finger.pos.x,
               window->finger.pos.y, window->finger.inside);
#endif
        if (feature_is_enabled(kFeature_VirtioMouse)) {
            if (mouse->tracking) {
                // update button state
                mouse->button_pressed |= 1 << (ev->u.mouse.button);

                int32_t rel_x = ev->u.mouse.xrel;
                int32_t rel_y = ev->u.mouse.yrel;
                skin_window_map_to_scale(window, &rel_x, &rel_y);

                add_mouse_event(window, rel_x, rel_y);
            }
        } else if (finger->inside) {
            // The click is inside the touch screen
            finger->tracking = 1;
            add_finger_event(window,
                             finger,
                             finger->pos.x,
                             finger->pos.y,
                             button_state,
                             displayId);
        } else if (!multitouch_should_skip_sync(button_state) &&
                   !multitouch_is_second_finger(button_state)) {
            // This is a single click outside the touch screen
            window->button.pressed = NULL;
            button = window->button.hover;
            if(button) {
                // The click is on a hard button
                button->down += 1;
                skin_window_redraw( window, &button->rect );
                window->button.pressed = button;
                if(button->keycode) {
                    window->win_funcs->key_event(button->keycode, 1);
                }
            } else if (!skin_winsys_window_has_frame()) {
                // Drag or resize the device window
                window->drag_x_start = ev->u.mouse.x_global;
                window->drag_y_start = ev->u.mouse.y_global;
                skin_winsys_get_frame_pos(&window->window_x_start, &window->window_y_start);
                if (finger->at_corner) {
                    skin_winsys_set_window_overlay_for_resize(finger->which_corner);
                }
            }
        }
        break;

    case kEventMouseButtonUp:
        if (window->drag_x_start != 0  &&  window->drag_y_start != 0) {
            if (!finger->at_corner) {
                // Finished moving the window. If updates got backed up,
                // the motion could have been erratic. Ensure the window
                // is visible.
                if (skin_winsys_is_window_off_screen()) {
                    skin_winsys_set_window_pos(0, 0);
                }
                window->drag_x_start = 0;
                window->drag_y_start = 0;
                return;
            }
            // Finished resizing
            finger->at_corner = 0;
            skin_winsys_clear_window_overlay();
            int window_pos_x, window_pos_y;
            int window_width, window_height;
            skin_winsys_get_frame_pos(&window_pos_x, &window_pos_y);
            skin_winsys_get_window_size(&window_width, &window_height);

            int delta_x = ev->u.mouse.x_global - window->drag_x_start;
            int delta_y = ev->u.mouse.y_global - window->drag_y_start;

            window->drag_x_start = 0;
            window->drag_y_start = 0;

            switch (finger->which_corner) {
                case CORNER_BOTTOM_RIGHT:
                    // Just resize the window, don't move it.
                    window_width  += delta_x;
                    window_height += delta_y;
                    break;
                case CORNER_TOP_RIGHT:
                    // Resize. Move the window's Y.
                    window_pos_y  += delta_y;
                    window_width  += delta_x;
                    window_height -= delta_y;
                    break;
                case CORNER_BOTTOM_LEFT:
                    // Resize. Move the window's X.
                    window_pos_x  += delta_x;
                    window_width  -= delta_x;
                    window_height += delta_y;
                    break;
                case CORNER_TOP_LEFT:
                    // Resize. Move the window's top and left.
                    window_pos_x  += delta_x;
                    window_pos_y  += delta_y;
                    window_width  -= delta_x;
                    window_height -= delta_y;
                    break;
            }
            skin_winsys_set_window_pos(window_pos_x, window_pos_y);
            skin_winsys_set_window_size(window_width, window_height);
            skin_winsys_set_window_cursor_normal();
            return;
        }
        window->drag_x_start = 0;
        window->drag_y_start = 0;

        if ( window->ball.tracking ) {
            skin_window_trackball_press( window, 0 );
            break;
        }
        button = window->button.pressed;
        mx = ev->u.mouse.x;
        my = ev->u.mouse.y;
        skin_window_map_to_scale( window, &mx, &my );
        if (button)
        {
            button->down = 0;
            skin_window_redraw( window, &button->rect );
            if(button->keycode) {
                window->win_funcs->key_event(button->keycode, 0);
            }
            window->button.pressed = NULL;
            window->button.hover   = NULL;
            skin_window_move_mouse( window, finger, mx, my );
        } else if (feature_is_enabled(kFeature_VirtioMouse)) {
            skin_window_move_mouse(window, finger, mx, my);
            if (mouse->tracking) {
                // update button state
                mouse->button_pressed &= ~(1 << (ev->u.mouse.button));

                int32_t rel_x = ev->u.mouse.xrel;
                int32_t rel_y = ev->u.mouse.yrel;
                skin_window_map_to_scale(window, &rel_x, &rel_y);

                add_mouse_event(window, rel_x, rel_y);
            }
        } else if (finger->tracking) {
            skin_window_move_mouse( window, finger, mx, my );
            finger->tracking = 0;
            add_finger_event(window,
                             finger,
                             finger->pos.x,
                             finger->pos.y,
                             button_state, displayId);
        }
        break;

    case kEventMouseMotion:
        mx = ev->u.mouse.x;
        my = ev->u.mouse.y;
        skin_window_map_to_scale( window, &mx, &my );
        skin_window_move_mouse( window, finger, mx, my );

        if (window->drag_x_start == 0  &&  window->drag_y_start == 0) {
            // Button is not pressed. Check the position and set the cursor.
            skin_window_find_finger( window, finger, mx, my, displayId);
        } else {
            if (finger->at_corner) {
                // Resize: just update the overlay until the mouse button is released
                skin_winsys_paint_overlay_for_resize(mx, my);
                break;
            }
            // The user is dragging the window
            int posX = window->window_x_start + ev->u.mouse.x_global - window->drag_x_start;
            int posY = window->window_y_start + ev->u.mouse.y_global - window->drag_y_start;
            skin_winsys_set_window_pos(posX, posY);
            break;
        }
        if ( window->ball.tracking ) {
            skin_window_trackball_move(window,
                                       ev->u.mouse.xrel,
                                       ev->u.mouse.yrel);
            break;
        }
        mx = ev->u.mouse.x;
        my = ev->u.mouse.y;
        skin_window_map_to_scale( window, &mx, &my );
        if ( !window->button.pressed )
        {
            skin_window_move_mouse( window, finger, mx, my );
            if (feature_is_enabled(kFeature_VirtioMouse)) {
                if (mouse->tracking) {
                    int32_t rel_x = ev->u.mouse.xrel;
                    int32_t rel_y = ev->u.mouse.yrel;
                    skin_window_map_to_scale(window, &rel_x, &rel_y);

                    add_mouse_event(window, rel_x, rel_y);
                }
            } else if (finger->tracking) {
                add_finger_event(window,
                                 finger,
                                 finger->pos.x,
                                 finger->pos.y,
                                 button_state,
                                 displayId);
            }
        }
        break;

    case kEventMouseWheel:
        if (feature_is_enabled(kFeature_VirtioMouse) && mouse->tracking) {
            add_mouse_wheel_event(window, ev->u.wheel.x_delta,
                                  ev->u.wheel.y_delta);
        }
        break;

    case kEventMouseStartTracking:
        window->mouse.tracking = 1;
        break;

    case kEventMouseStopTracking:
        window->mouse.tracking = 0;
        break;

    case kEventRotaryInput:
        window->win_funcs->rotary_input_event(ev->u.rotary_input.delta);
        break;

    case kEventScreenChanged:
        // Re-setup the guest window and the OpenGL ES subwindow with a
        // potentially different window size / framebuffer size.
        //
        // For example, if HiDPI is enabled, the guest window size keeps the
        // same, while OpenGL ES subwindow will be initialized with a
        // framebuffer of 2x size. Otherwise, if HiDPI is disabled, the
        // framebuffer still gets the same size, but window size will be halved.
        skin_window_set_device_pixel_ratio(window);
        skin_window_resize(window, 1);
        skin_window_show_opengles(window, true);
        break;

    case kEventWindowChanged:
        // The window changed (such as a resize or rotation)
        skin_window_show_opengles(window, false);
        break;

    default:
        ;
    }
}

// Processing of pen events
void
skin_window_process_pen_event(SkinWindow* window, SkinEvent* ev)
{
    Button* button;
    int     mx, my;
    uint32_t displayId = 0;
    int button_state = multitouch_create_buttons_state(
                ev->type != kEventPenRelease, ev->u.pen.skip_sync, false);
    FingerState* pen = &window->pen;

    if (!window->surface)
        return;

    switch (ev->type) {
    case kEventPenPress:
        if ( window->ball.tracking ) {
            skin_window_trackball_press( window, 1 );
            break;
        }

        mx = ev->u.pen.x;
        my = ev->u.pen.y;
        skin_window_map_to_scale( window, &mx, &my );
        skin_window_move_mouse( window, pen, mx, my );
        skin_window_find_finger( window, pen, mx, my, displayId);

        if (pen->inside) {
            // The click is inside the touch screen
            pen->tracking = 1;
            add_pen_event(window, pen, pen->pos.x, pen->pos.y, ev, button_state);
        } else if (!multitouch_should_skip_sync(button_state)) {
            // This is a single click outside the touch screen
            window->button.pressed = NULL;
            button = window->button.hover;
            if(button) {
                // The click is on a hard button
                button->down += 1;
                skin_window_redraw( window, &button->rect );
                window->button.pressed = button;
                if(button->keycode) {
                    window->win_funcs->key_event(button->keycode, 1);
                }
            } else if (!skin_winsys_window_has_frame()) {
                // Drag or resize the device window
                window->drag_x_start = ev->u.pen.x_global;
                window->drag_y_start = ev->u.pen.y_global;
                skin_winsys_get_frame_pos(&window->window_x_start, &window->window_y_start);
                if (pen->at_corner) {
                    skin_winsys_set_window_overlay_for_resize(pen->which_corner);
                }
            }
        }
        break;

    case kEventPenRelease:
        if (window->drag_x_start != 0  &&  window->drag_y_start != 0) {
            if (!pen->at_corner) {
                // Finished moving the window. If updates got backed up,
                // the motion could have been erratic. Ensure the window
                // is visible.
                if (skin_winsys_is_window_off_screen()) {
                    skin_winsys_set_window_pos(0, 0);
                }
                window->drag_x_start = 0;
                window->drag_y_start = 0;
                return;
            }
            // Finished resizing
            pen->at_corner = 0;
            skin_winsys_clear_window_overlay();
            int window_pos_x, window_pos_y;
            int window_width, window_height;
            skin_winsys_get_frame_pos(&window_pos_x, &window_pos_y);
            skin_winsys_get_window_size(&window_width, &window_height);

            int delta_x = ev->u.pen.x_global - window->drag_x_start;
            int delta_y = ev->u.pen.y_global - window->drag_y_start;

            window->drag_x_start = 0;
            window->drag_y_start = 0;

            switch (pen->which_corner) {
                case CORNER_BOTTOM_RIGHT:
                    // Just resize the window, don't move it.
                    window_width  += delta_x;
                    window_height += delta_y;
                    break;
                case CORNER_TOP_RIGHT:
                    // Resize. Move the window's Y.
                    window_pos_y  += delta_y;
                    window_width  += delta_x;
                    window_height -= delta_y;
                    break;
                case CORNER_BOTTOM_LEFT:
                    // Resize. Move the window's X.
                    window_pos_x  += delta_x;
                    window_width  -= delta_x;
                    window_height += delta_y;
                    break;
                case CORNER_TOP_LEFT:
                    // Resize. Move the window's top and left.
                    window_pos_x  += delta_x;
                    window_pos_y  += delta_y;
                    window_width  -= delta_x;
                    window_height -= delta_y;
                    break;
            }
            skin_winsys_set_window_pos(window_pos_x, window_pos_y);
            skin_winsys_set_window_size(window_width, window_height);
            skin_winsys_set_window_cursor_normal();
            return;
        }
        window->drag_x_start = 0;
        window->drag_y_start = 0;

        if ( window->ball.tracking ) {
            skin_window_trackball_press( window, 0 );
            break;
        }
        button = window->button.pressed;
        mx = ev->u.pen.x;
        my = ev->u.pen.y;
        skin_window_map_to_scale( window, &mx, &my );
        if (button)
        {
            button->down = 0;
            skin_window_redraw( window, &button->rect );
            if(button->keycode) {
                window->win_funcs->key_event(button->keycode, 0);
            }
            window->button.pressed = NULL;
            window->button.hover   = NULL;
            skin_window_move_mouse( window, pen, mx, my );
        } else if (pen->tracking) {
            skin_window_move_mouse( window, pen, mx, my );
            pen->tracking = 0;
            add_pen_event(window, pen, pen->pos.x, pen->pos.y, ev, button_state);
        }
        break;

    case kEventPenMove:
        mx = ev->u.pen.x;
        my = ev->u.pen.y;
        skin_window_map_to_scale( window, &mx, &my );
        skin_window_move_mouse( window, pen, mx, my );

        if (window->drag_x_start == 0  &&  window->drag_y_start == 0) {
            // Button is not pressed. Check the position and set the cursor.
            skin_window_find_finger( window, pen, mx, my, displayId);
        } else {
            if (pen->at_corner) {
                // Resize: just update the overlay until the mouse button is released
                skin_winsys_paint_overlay_for_resize(mx, my);
                break;
            }
            // The user is dragging the window
            int posX = window->window_x_start + ev->u.pen.x_global - window->drag_x_start;
            int posY = window->window_y_start + ev->u.pen.y_global - window->drag_y_start;
            skin_winsys_set_window_pos(posX, posY);
            break;
        }

        mx = ev->u.pen.x;
        my = ev->u.pen.y;
        skin_window_map_to_scale( window, &mx, &my );
        if ( !window->button.pressed )
        {
            skin_window_move_mouse( window, pen, mx, my );
            if (pen->tracking) {
                add_pen_event(window, pen, pen->pos.x, pen->pos.y, ev, button_state);
            }
        }
        break;

    default:
        break;
    }
}

static ADisplay*
skin_window_display( SkinWindow*  window )
{
    return window->layout.displays;
}

void
skin_window_update_display( SkinWindow*  window, int  x, int  y, int  w, int  h )
{
    if ( !window->surface )
        return;

    ADisplay* disp = skin_window_display(window);

    if (disp != NULL) {
        SkinRect  r;
        r.pos.x  = x;
        r.pos.y  = y;
        r.size.w = w;
        r.size.h = h;

        skin_rect_rotate( &r, &r, disp->rotation );
        r.pos.x += disp->origin.x;
        r.pos.y += disp->origin.y;

        adisplay_redraw(disp, &r, window->surface, window->use_emugl_subwindow);
    }
}


void skin_window_update_gpu_frame(SkinWindow* window,
                                  int w,
                                  int h,
                                  const void* pixels) {
    if (!window) {
        return;
    }

    ADisplay* disp = skin_window_display(window);
    if (!disp || disp->datasize.w != w || disp->datasize.h != h) {
        derror("%s: bad values!", __FUNCTION__);
        return;
    }

    if (!disp->gpu_frame) {
        const int size = 4 * w * h;
        disp->gpu_frame = calloc(1, size);
        if (!disp->gpu_frame) {
            derror("ERROR: %s:%d cannot allocate memory of %d bytes.\n",
                   __func__, __LINE__, size);
            crashhandler_die_format(
                    "GPU frame memory allocation failed (requested %d bytes)",
                    size);
        }
    }
    // Convert from GL_RGBA to 32-bit ARGB.
    {
        const uint8_t* src = pixels;
        uint32_t* dst = (uint32_t*)disp->gpu_frame;
        uint32_t* dst_end = dst + w * h;
        for (; dst < dst_end; src += 4, dst += 1) {
            dst[0] = ((uint32_t)src[3] << 24) |
                     ((uint32_t)src[0] << 16) |
                     ((uint32_t)src[1] << 8) |
                      (uint32_t)src[2];
        }
    }

    skin_window_update_display(window, 0, 0, w, h);
}

void skin_window_update_rotation(SkinWindow* window, SkinRotation rotation) {
    if (!window)
        return;

    skin_winsys_update_rotation(rotation);
}

int skin_window_add_display(SkinWindow* window, uint32_t id, uint32_t w, uint32_t h) {
    FingerState* fs = NULL;
    ANEW0(fs);
    finger_state_reset(fs);
    aintMap_set(window->multi_display_fingers, id, fs);
    ADisplay* disp = NULL;
    ANEW0(disp);
    disp->id = id;
    disp->rect.size.w = w;
    disp->rect.size.h = h;
    disp->rect.pos.x = 0;
    disp->rect.pos.y = 0;
    disp->rotation = SKIN_ROTATION_0;
    disp->origin = disp->rect.pos;
    disp->onion  = NULL;
    disp->brightness = LCD_BRIGHTNESS_DEFAULT;
    disp->gpu_frame = NULL;
    disp->surface = skin_surface_create(disp->rect.size.w,
                                        disp->rect.size.h,
                                        disp->rect.size.w,
                                        disp->rect.size.h);
    aintMap_set(window->layout.multi_displays, id, disp);
    return 0;
}

int skin_window_remove_display(SkinWindow* window, uint32_t id) {
    ADisplay* disp = NULL;
    FingerState* fs = NULL;
    disp = aintMap_del(window->layout.multi_displays, id);
    if (disp) {
        AFREE(disp);
    }
    fs = aintMap_del(window->multi_display_fingers, id);
    if (fs) {
        AFREE(fs);
    }
    return 0;
}
