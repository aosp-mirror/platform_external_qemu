/* Copyright (C) 2007-2008 The Android Open Source Project
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

#include "android/skin/charmap.h"
#include "android/skin/event.h"
#include "android/skin/image.h"
#include "android/skin/scaler.h"
#include "android/skin/winsys.h"
#include "android/utils/debug.h"
#include "android/utils/setenv.h"
#include "android/utils/system.h"
#include "android/utils/duff.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* when shrinking, we reduce the pixel ratio by this fixed amount */
#define  SHRINK_SCALE  0.6

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
} ADisplay;

static void
display_done( ADisplay*  disp )
{
    disp->data   = NULL;
    skin_image_unref( &disp->onion );
}

static int
display_init( ADisplay*  disp, SkinDisplay*  sdisp, SkinLocation*  loc, SkinRect*  frame )
{
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

        default:
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

    return (disp->data == NULL) ? -1 : 0;
}

static __inline__ uint32_t rgb565_to_rgba32(uint32_t pix,
        uint32_t rshift, uint32_t gshift, uint32_t bshift, uint32_t amask)
{
    uint32_t r8 = ((pix & 0xf800) >>  8) | ((pix & 0xe000) >> 13);
    uint32_t g8 = ((pix & 0x07e0) >>  3) | ((pix & 0x0600) >>  9);
    uint32_t b8 = ((pix & 0x001f) <<  3) | ((pix & 0x001c) >>  2);
    return (r8 << rshift) | (g8 << gshift) | (b8 << bshift) | amask;
}

/* The framebuffer format is R,G,B,X in framebuffer memory, on a
 * little-endian system, this translates to XBGR after a load.
 */
static __inline__ uint32_t xbgr_to_rgba32(uint32_t pix,
        uint32_t rshift, uint32_t gshift, uint32_t bshift, uint32_t amask)
{
    uint32_t r8 = (pix & 0x00ff0000) >> 16;
    uint32_t g8 = (pix & 0x0000ff00) >>  8;
    uint32_t b8 = (pix & 0x000000ff) >>  0;
    return (r8 << rshift) | (g8 << gshift) | (b8 << bshift) | amask;
}

static void
display_set_onion( ADisplay*  disp, SkinImage*  onion, SkinRotation  rotation, int  blend )
{
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

        default:
            orect->pos.x = rect->pos.x;
            orect->pos.y = rect->pos.y + rect->size.h - onion_h;
    }
    orect->size.w = onion_w;
    orect->size.h = onion_h;
}

#define  DOT_MATRIX  0

#if DOT_MATRIX

static void
dotmatrix_dither_argb32( unsigned char*  pixels, int  x, int  y, int  w, int  h, int  pitch )
{
    static const unsigned dotmatrix_argb32[16] = {
        0x003f00, 0x00003f, 0x3f0000, 0x000000,
        0x3f3f3f, 0x000000, 0x3f3f3f, 0x000000,
        0x3f0000, 0x000000, 0x003f00, 0x00003f,
        0x3f3f3f, 0x000000, 0x3f3f3f, 0x000000
    };

    int   yy = y & 3;

    pixels += 4*x + y*pitch;

    for ( ; h > 0; h-- ) {
        unsigned*  line = (unsigned*) pixels;
        int        nn, xx = x & 3;

        for (nn = 0; nn < w; nn++) {
            unsigned  c = line[nn];

            c = c - ((c >> 2) & dotmatrix_argb32[(yy << 2)|xx]);

            xx = (xx + 1) & 3;
            line[nn] = c;
        }

        yy      = (yy + 1) & 3;
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

static void
lcd_brightness_argb32(uint32_t* pixels,
                      SkinRect* r,
                      int pitch,
                      int brightness)
{
    const unsigned  b_min  = LCD_BRIGHTNESS_MIN;
    const unsigned  b_max  = LCD_BRIGHTNESS_MAX;
    const unsigned  b_low  = LCD_BRIGHTNESS_LOW;
    const unsigned  b_high = LCD_BRIGHTNESS_HIGH;

    unsigned        alpha = brightness;
    int             w     = r->size.w;
    int             h     = r->size.h;

    if (alpha <= b_min)
        alpha = b_min;
    else if (alpha > b_max)
        alpha = b_max;

    pixels += 4*r->pos.x + r->pos.y*pitch;

    if (alpha < b_low)
    {
        const unsigned  alpha_min   = (255*LCD_ALPHA_LOW_MIN);
        const unsigned  alpha_range = (255 - alpha_min);

        alpha = alpha_min + ((alpha - b_min)*alpha_range) / (b_low - b_min);

        for ( ; h > 0; h-- ) {
            unsigned*  line = (unsigned*) pixels;
            int        nn   = 0;

            DUFF4(w, {
                unsigned  c  = line[nn];
                unsigned  ag = (c >> 8) & 0x00ff00ff;
                unsigned  rb = (c)      & 0x00ff00ff;

                ag = (ag*alpha)        & 0xff00ff00;
                rb = ((rb*alpha) >> 8) & 0x00ff00ff;

                line[nn] = (unsigned)(ag | rb);
                nn++;
            });
            pixels += pitch;
        }
    }
    else if (alpha > LCD_BRIGHTNESS_HIGH) /* 'superluminous' mode */
    {
        const unsigned  alpha_max   = (255*LCD_ALPHA_HIGH_MAX);
        const unsigned  alpha_range = (255-alpha_max);
        unsigned        ialpha;

        alpha  = ((alpha - b_high)*alpha_range) / (b_max - b_high);
        ialpha = 255-alpha;

        for ( ; h > 0; h-- ) {
            unsigned*  line = (unsigned*) pixels;
            int        nn   = 0;

            DUFF4(w, {
                unsigned  c  = line[nn];
                unsigned  ag = (c >> 8) & 0x00ff00ff;
                unsigned  rb = (c)      & 0x00ff00ff;

                /* interpolate towards bright white, i.e. 0x00ffffff */
                ag = ((ag*ialpha + 0x00ff00ff*alpha)) & 0xff00ff00;
                rb = ((rb*ialpha + 0x00ff00ff*alpha) >> 8) & 0x00ff00ff;

                line[nn] = (unsigned)(ag | rb);
                nn++;
            });
            pixels += pitch;
        }
    }
}


/* this is called when the LCD framebuffer is off */
static void
lcd_off_argb32(uint32_t* pixels, SkinRect* r, int pitch)
{
    int  x = r->pos.x;
    int  y = r->pos.y;
    int  w = r->size.w;
    int  h = r->size.h;

    pixels += 4 * x + y*pitch;
    for ( ; h > 0; h-- ) {
        memset( pixels, 0, w*4 );
        pixels += pitch;
    }
}

static void
display_redraw_rect16(ADisplay* disp,
                      SkinRect* rect,
                      SkinSurfacePixels* dst,
                      SkinSurfacePixelFormat* dst_format)
{
    int           x  = rect->pos.x - disp->rect.pos.x;
    int           y  = rect->pos.y - disp->rect.pos.y;
    int           w  = rect->size.w;
    int           h  = rect->size.h;
    int           disp_w    = disp->rect.size.w;
    int           disp_h    = disp->rect.size.h;
    int           dst_pitch = dst->pitch;
    uint8_t*      dst_line  = (uint8_t*)dst->pixels + rect->pos.x*4 + rect->pos.y*dst_pitch;
    int           src_pitch = disp->datasize.w*2;
    uint8_t*      src_line  = (uint8_t*)disp->data;
    int           yy, xx;
    uint32_t      rshift = dst_format->r_shift;
    uint32_t      gshift = dst_format->g_shift;
    uint32_t      bshift = dst_format->b_shift;
    uint32_t      amask  = dst_format->a_mask; // may be 0x00 for non-alpha format

    switch ( disp->rotation & 3 )
    {
    case SKIN_ROTATION_0:
        src_line += x*2 + y*src_pitch;

        for (yy = h; yy > 0; yy--)
        {
            uint32_t*  dst = (uint32_t*)dst_line;
            uint16_t*  src = (uint16_t*)src_line;

            xx = 0;
            DUFF4(w, {
                dst[xx] = rgb565_to_rgba32(src[xx], rshift, gshift, bshift, amask);
                xx++;
            });
            src_line += src_pitch;
            dst_line += dst_pitch;
        }
        break;

    case SKIN_ROTATION_90:
        src_line += y*2 + (disp_w - x - 1)*src_pitch;

        for (yy = h; yy > 0; yy--)
        {
            uint32_t*  dst = (uint32_t*)dst_line;
            uint8_t*   src = src_line;

            DUFF4(w, {
                dst[0] = rgb565_to_rgba32(((uint16_t*)src)[0], rshift, gshift, bshift, amask);
                src -= src_pitch;
                dst += 1;
            });
            src_line += 2;
            dst_line += dst_pitch;
        }
        break;

    case SKIN_ROTATION_180:
        src_line += (disp_w -1 - x)*2 + (disp_h-1-y)*src_pitch;

        for (yy = h; yy > 0; yy--)
        {
            uint16_t*  src = (uint16_t*)src_line;
            uint32_t*  dst = (uint32_t*)dst_line;

            DUFF4(w, {
                dst[0] = rgb565_to_rgba32(src[0], rshift, gshift, bshift, amask);
                src -= 1;
                dst += 1;
            });
            src_line -= src_pitch;
            dst_line += dst_pitch;
    }
    break;

    default:  /* SKIN_ROTATION_270 */
        src_line += (disp_h-1-y)*2 + x*src_pitch;

        for (yy = h; yy > 0; yy--)
        {
            uint32_t*  dst = (uint32_t*)dst_line;
            uint8_t*   src = src_line;

            DUFF4(w, {
                dst[0] = rgb565_to_rgba32(((uint16_t*)src)[0], rshift, gshift, bshift, amask);
                dst   += 1;
                src   += src_pitch;
            });
            src_line -= 2;
            dst_line += dst_pitch;
        }
    }
}

static void
display_redraw_rect32(ADisplay* disp,
                      SkinRect* rect,
                      SkinSurfacePixels* dst,
                      SkinSurfacePixelFormat* dst_format)
{
    int           x  = rect->pos.x - disp->rect.pos.x;
    int           y  = rect->pos.y - disp->rect.pos.y;
    int           w  = rect->size.w;
    int           h  = rect->size.h;
    int           disp_w    = disp->rect.size.w;
    int           disp_h    = disp->rect.size.h;
    int           dst_pitch = dst->pitch;
    uint8_t*      dst_line  = (uint8_t*)dst->pixels + rect->pos.x*4 + rect->pos.y*dst_pitch;
    int           src_pitch = disp->datasize.w*4;
    uint8_t*      src_line  = (uint8_t*)disp->data;
    int           yy;
    uint32_t      rshift = dst_format->r_shift;
    uint32_t      gshift = dst_format->g_shift;
    uint32_t      bshift = dst_format->b_shift;
    uint32_t      amask  = dst_format->a_mask; // may be 0x00 for non-alpha format

    switch ( disp->rotation & 3 )
    {
    case SKIN_ROTATION_0:
        src_line += x*4 + y*src_pitch;

        for (yy = h; yy > 0; yy--) {
            uint32_t*  src = (uint32_t*)src_line;
            uint32_t*  dst = (uint32_t*)dst_line;

            DUFF4(w, {
                dst[0] = xbgr_to_rgba32(src[0], rshift, gshift, bshift, amask);
                dst++;
                src++;
            });
            src_line += src_pitch;
            dst_line += dst_pitch;
        }
        break;

    case SKIN_ROTATION_90:
        src_line += y*4 + (disp_w - x - 1)*src_pitch;

        for (yy = h; yy > 0; yy--)
        {
            uint32_t*  dst = (uint32_t*)dst_line;
            uint8_t*   src = src_line;

            DUFF4(w, {
                dst[0] = xbgr_to_rgba32(*(uint32_t*)src, rshift, gshift, bshift, amask);
                src -= src_pitch;
                dst += 1;
            });
            src_line += 4;
            dst_line += dst_pitch;
        }
        break;

    case SKIN_ROTATION_180:
        src_line += (disp_w -1 - x)*4 + (disp_h-1-y)*src_pitch;

        for (yy = h; yy > 0; yy--)
        {
            uint32_t*  src = (uint32_t*)src_line;
            uint32_t*  dst = (uint32_t*)dst_line;

            DUFF4(w, {
                dst[0] = xbgr_to_rgba32(src[0], rshift, gshift, bshift, amask);
                src -= 1;
                dst += 1;
            });
            src_line -= src_pitch;
            dst_line += dst_pitch;
    }
    break;

    default:  /* SKIN_ROTATION_270 */
        src_line += (disp_h-1-y)*4 + x*src_pitch;

        for (yy = h; yy > 0; yy--)
        {
            uint32_t*  dst = (uint32_t*)dst_line;
            uint8_t*   src = src_line;

            DUFF4(w, {
                dst[0] = xbgr_to_rgba32(*(uint32_t*)src, rshift, gshift, bshift, amask);
                dst   += 1;
                src   += src_pitch;
            });
            src_line -= 4;
            dst_line += dst_pitch;
        }
    }
}

static void
display_redraw(ADisplay*  disp, SkinRect*  rect, SkinSurface* surface)
{
    SkinRect  r;

    if (skin_rect_intersect( &r, rect, &disp->rect ))
    {
#if 0
        fprintf(stderr, "--- display redraw r.pos(%d,%d) r.size(%d,%d) "
                        "disp.pos(%d,%d) disp.size(%d,%d) datasize(%d,%d) rect.pos(%d,%d) rect.size(%d,%d)\n",
                        r.pos.x - disp->rect.pos.x, r.pos.y - disp->rect.pos.y,
                        r.size.w, r.size.h, disp->rect.pos.x, disp->rect.pos.y,
                        disp->rect.size.w, disp->rect.size.h, disp->datasize.w, disp->datasize.h,
                        rect->pos.x, rect->pos.y, rect->size.w, rect->size.h );
#endif
        SkinSurfacePixelFormat format;
        skin_surface_get_format(surface, &format);

        SkinSurfacePixels pix;
        skin_surface_lock(surface, &pix);

        if (disp->brightness == LCD_BRIGHTNESS_OFF)
        {
            lcd_off_argb32(pix.pixels, &r, pix.pitch);
        }
        else
        {
            if (disp->bits_per_pixel == 32) {
                display_redraw_rect32(disp, &r, &pix, &format);
            } else {
                display_redraw_rect16(disp, &r, &pix, &format);
            }
#if DOT_MATRIX
            dotmatrix_dither_argb32(pix.pixels,
                                    r.pos.x,
                                    r.pos.y,
                                    r.size.w,
                                    r.size.h,
                                    pix.pitch);
#endif
            /* apply lightness */
            lcd_brightness_argb32(pix.pixels,
                                  &r,
                                  pix.pitch,
                                  disp->brightness);
        }
        skin_surface_unlock(surface);

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


typedef struct {
    char      tracking;
    char      inside;
    SkinPos   pos;
    ADisplay*  display;
} FingerState;

static void
finger_state_reset( FingerState*  finger )
{
    finger->tracking = 0;
    finger->inside   = 0;
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
            skin_winsys_set_relative_mouse_mode(true);
            skin_trackball_refresh( state->ball );
            skin_window_redraw( state->window, &state->rect );
        }
    } else {
        if ( state->tracking ) {
            state->tracking = 0;
            skin_winsys_set_relative_mouse_mode(false);
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
        display_done( &layout->displays[nn] );

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
            display_init( disp, part->display, loc, &layout->rect );
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
    ButtonState   button;
    BallState     ball;
    char          enabled;
    char          fullscreen;
    char          no_display;

    char          enable_touch;
    char          enable_trackball;
    char          enable_dpad;
    char          enable_qwerty;

    SkinImage*    onion;
    SkinRotation  onion_rotation;
    int           onion_alpha;

    int           x_pos;
    int           y_pos;

    SkinScaler*   scaler;
    int           shrink;
    double        shrink_scale;
    SkinSurface*  shrink_surface;

    double        effective_scale;
    double        effective_x;
    double        effective_y;
};

static void
add_finger_event(SkinWindow* window,
                 unsigned x,
                 unsigned y,
                 unsigned state)
{
    //fprintf(stderr, "::: finger %d,%d %d\n", x, y, state);

    window->win_funcs->mouse_event(x, y, state);
}

static void
skin_window_find_finger( SkinWindow*  window,
                         int          x,
                         int          y )
{
    FingerState*  finger = &window->finger;

    /* find the display that contains this movement */
    finger->display = NULL;
    finger->inside  = 0;

    if (!window->enable_touch)
        return;

    LAYOUT_LOOP_DISPLAYS(&window->layout,disp)
        if ( skin_rect_contains( &disp->rect, x, y ) ) {
            finger->inside   = 1;
            finger->display  = disp;
            finger->pos.x    = x - disp->origin.x;
            finger->pos.y    = y - disp->origin.y;

            skin_pos_rotate( &finger->pos, &finger->pos, -disp->rotation );
            break;
        }
    LAYOUT_LOOP_END_DISPLAYS
}

static void
skin_window_move_mouse( SkinWindow*  window,
                        int          x,
                        int          y )
{
    FingerState*  finger = &window->finger;
    ButtonState*  button = &window->button;

    if (finger->tracking) {
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

        skin_pos_rotate( &finger->pos, &finger->pos, -disp->rotation );
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
                case kKeyCodeBack:
                case kKeyCodeCall:
                case kKeyCodeEndCall:
                case kKeyCodeTV:
                case kKeyCodeEPG:
                case kKeyCodeDVR:
                case kKeyCodePrevious:
                case kKeyCodeNext:
                case kKeyCodePlay:
                case kKeyCodePause:
                case kKeyCodeStop:
                case kKeyCodeRewind:
                case kKeyCodeFastForward:
                case kKeyCodeBookmarks:
                case kKeyCodeCycleWindows:
                case kKeyCodeChannelUp:
                case kKeyCodeChannelDown:
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
    window->win_funcs->key_event(BTN_MOUSE, down);
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

/* Hide the OpenGL ES framebuffer */
static void
skin_window_hide_opengles( SkinWindow* window )
{
    window->win_funcs->opengles_hide();
    //android_hideOpenglesWindow();
}

/* Show the OpenGL ES framebuffer window */
static void
skin_window_show_opengles( SkinWindow* window )
{
    {
        ADisplay* disp = window->layout.displays;
        SkinRect drect = disp->rect;
        void* winhandle = skin_winsys_get_window_handle();

        skin_scaler_get_scaled_rect(window->scaler, &drect, &drect);

        window->win_funcs->opengles_show(winhandle,
                                         drect.pos.x,
                                         drect.pos.y,
                                         drect.size.w,
                                         drect.size.h,
                                         disp->rotation * -90.);
    }
}

static void
skin_window_redraw_opengles( SkinWindow* window )
{
    window->win_funcs->opengles_redraw();
    //android_redrawOpenglesWindow();
}

static int  skin_window_reset_internal (SkinWindow*, SkinLayout*);

SkinWindow*
skin_window_create(SkinLayout* slayout,
                   int x,
                   int y,
                   double scale,
                   int no_display,
                   const SkinWindowFuncs* win_funcs)
{
    SkinWindow*  window;

    /* If scale is <= 0, we want to check that the window's default size if
     * not larger than the current screen. Otherwise, we need to compute
     * a new scale to ensure it is.
     */
    if (scale <= 0) {
        SkinRect  monitor;
        int       screen_w, screen_h;
        int       win_w = slayout->size.w;
        int       win_h = slayout->size.h;
        double    scale_w, scale_h;

        /* To account for things like menu bars, window decorations etc..
         * We only compute 95% of the real screen size. */
        skin_winsys_get_monitor_rect(&monitor);
        screen_w = monitor.size.w * 0.95;
        screen_h = monitor.size.h * 0.95;

        scale_w = 1.0;
        scale_h = 1.0;

        if (screen_w < win_w && win_w > 1.)
            scale_w = 1.0 * screen_w / win_w;
        if (screen_h < win_h && win_h > 1.)
            scale_h = 1.0 * screen_h / win_h;

        scale = (scale_w <= scale_h) ? scale_w : scale_h;

        VERBOSE_PRINT(init,"autoconfig: -scale %g", scale);
    }

    ANEW0(window);

    window->win_funcs    = win_funcs;
    window->shrink_scale = scale;
    window->shrink       = (scale != 1.0);
    window->scaler       = skin_scaler_create();
    window->no_display   = no_display;

    /* enable everything by default */
    window->enable_touch     = 1;
    window->enable_trackball = 1;
    window->enable_dpad      = 1;
    window->enable_qwerty    = 1;

    window->x_pos = x;
    window->y_pos = y;

    if (skin_window_reset_internal(window, slayout) < 0) {
        skin_window_free(window);
        return NULL;
    }
    skin_winsys_set_window_pos(x, y);

    /* Check that the window is fully visible */
    if (!window->no_display && !skin_winsys_is_window_fully_visible()) {
        SkinRect monitor;
        int win_x, win_y, win_w, win_h;
        int new_x, new_y;

        skin_winsys_get_monitor_rect(&monitor);

        skin_winsys_get_window_pos(&win_x, &win_y);
        win_w = skin_surface_width(window->surface);
        win_h = skin_surface_height(window->surface);

        /* First, we recenter the window */
        new_x = (monitor.size.w - win_w)/2;
        new_y = (monitor.size.h - win_h)/2;

        /* If it is still too large, we ensure the top-border is visible */
        if (new_y < 0)
            new_y = 0;

        /* Done */
        skin_winsys_set_window_pos(new_x, new_y);
        dprint( "emulator window was out of view and was recentered\n" );
    }

    skin_window_show_opengles(window);

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

static void
skin_window_resize( SkinWindow*  window )
{
    if ( !window->no_display )
        skin_window_hide_opengles(window);

    /* now resize window */
    skin_surface_unrefp(&window->surface);
    skin_surface_unrefp(&window->shrink_surface);

    if ( !window->no_display ) {
        int           layout_w = window->layout.rect.size.w;
        int           layout_h = window->layout.rect.size.h;
        int           window_w = layout_w;
        int           window_h = layout_h;
        int           window_x = window->x_pos;
        int           window_y = window->y_pos;
        SkinSurface*  surface;
        double        scale = 1.0;
        int           fullscreen = window->fullscreen;

        if (fullscreen) {
            SkinRect r;
            skin_winsys_get_monitor_rect(&r);
            double  x_scale, y_scale;

            window_x = r.pos.x;
            window_y = r.pos.y;
            window_w = r.size.w;
            window_h = r.size.h;

            x_scale = window_w * 1.0 / layout_w;
            y_scale = window_h * 1.0 / layout_h;

            scale = (x_scale <= y_scale) ? x_scale : y_scale;
        }
        else if (window->shrink) {
            scale = window->shrink_scale;
            window_w = (int) ceil(layout_w*scale);
            window_h = (int) ceil(layout_h*scale);
        }

        surface = skin_surface_create_window(window_x,
                                             window_y,
                                             window_w,
                                             window_h,
                                             fullscreen);
        window->effective_scale = scale;
        window->effective_x     = 0;
        window->effective_y     = 0;

        if (fullscreen) {
            window->effective_x = (window_w - layout_w*scale)*0.5;
            window->effective_y = (window_h - layout_h*scale)*0.5;
        }

        if (scale == 1.0)
        {
            window->surface = surface;
            skin_scaler_set( window->scaler, 1.0, 0, 0 );
        }
        else
        {
            window_w = (int) ceil(window_w / scale );
            window_h = (int) ceil(window_h / scale );

            window->shrink_surface = surface;
            window->surface = skin_surface_create_slow(window_w, window_h);
            if (window->surface == NULL) {
                fprintf(stderr, "### Error: could not create or resize window\n");
                exit(1);
            }
            skin_scaler_set( window->scaler, scale, window->effective_x, window->effective_y );
        }

        skin_window_show_opengles(window);
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

    disp = window->layout.displays;
    if (disp != NULL) {
        if (slayout->onion_image) {
            // Onion was specified in layout file.
            display_set_onion( disp,
                               slayout->onion_image,
                               slayout->onion_rotation,
                               slayout->onion_alpha );
        } else if (window->onion) {
            // Onion was specified via command line.
            display_set_onion( disp,
                               window->onion,
                               window->onion_rotation,
                               window->onion_alpha );
        }
    }

    skin_window_resize(window);

    finger_state_reset( &window->finger );
    button_state_reset( &window->button );
    ball_state_reset( &window->ball, window );

    skin_window_redraw( window, NULL );

    if (slayout->event_type != 0) {
        window->win_funcs->generic_event(
                slayout->event_type,
                slayout->event_code,
                slayout->event_value);
    }

    return 0;
}

int
skin_window_reset ( SkinWindow*  window, SkinLayout*  slayout )
{
    if (!window->fullscreen) {
        skin_winsys_get_window_pos(&window->x_pos, &window->y_pos);
    }
    if (skin_window_reset_internal( window, slayout ) < 0)
        return -1;

    return 0;
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
skin_window_free  ( SkinWindow*  window )
{
    if (window) {
        skin_surface_unrefp(&window->surface);
        skin_surface_unrefp(&window->shrink_surface);

        if (window->onion) {
            skin_image_unref( &window->onion );
            window->onion_rotation = SKIN_ROTATION_0;
        }
        if (window->scaler) {
            skin_scaler_free(window->scaler);
            window->scaler = NULL;
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
        display_set_onion( disp, window->onion, onion_rotation, onion_alpha );
}

static void
skin_window_update_shrink( SkinWindow*  window, SkinRect*  rect )
{
    skin_scaler_scale(window->scaler,
                      window->shrink_surface,
                      window->surface,
                      rect->pos.x,
                      rect->pos.y,
                      rect->size.w,
                      rect->size.h);
}

void
skin_window_set_scale( SkinWindow*  window, double  scale )
{
    window->shrink       = (scale != 1.0);
    window->shrink_scale = scale;

    skin_window_resize( window );
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
            for ( ; disp < end; disp++ )
                display_redraw( disp, rect, window->surface );
        }

        {
            Button*  button = layout->buttons;
            Button*  end    = button + layout->num_buttons;
            for ( ; button < end; button++ )
                button_redraw( button, rect, window->surface );
        }

        if ( window->ball.tracking )
            ball_state_redraw( &window->ball, rect, window->surface );

        if (window->effective_scale != 1.0)
            skin_window_update_shrink( window, rect );
        else
        {
            skin_surface_update(window->surface, rect);
        }
        skin_window_redraw_opengles( window );
    }
}

void
skin_window_toggle_fullscreen( SkinWindow*  window )
{
    if (window && window->surface) {
        if (!window->fullscreen) {
            skin_winsys_get_window_pos(&window->x_pos, &window->y_pos);
        }
        window->fullscreen = !window->fullscreen;
        skin_window_resize( window );
        skin_window_redraw( window, NULL );
    }
}

void
skin_window_get_display( SkinWindow*  window, ADisplayInfo  *info )
{
    ADisplay*  disp = window->layout.displays;

    if (disp != NULL) {
        info->width    = disp->datasize.w;
        info->height   = disp->datasize.h;
        info->rotation = disp->rotation;
        info->data     = disp->data;
    } else {
        info->width    = 0;
        info->height   = 0;
        info->rotation = SKIN_ROTATION_0;
        info->data     = NULL;
    }
}


static void
skin_window_map_to_scale( SkinWindow*  window, int  *x, int  *y )
{
    *x = (*x - window->effective_x) / window->effective_scale;
    *y = (*y - window->effective_y) / window->effective_scale;
}

void
skin_window_process_event(SkinWindow*  window, SkinEvent* ev)
{
    Button*  button;
    int      mx, my;

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
        skin_window_move_mouse( window, mx, my );
        skin_window_find_finger( window, mx, my );
#if 0
        printf("down: x=%d y=%d fx=%d fy=%d fis=%d\n",
               ev->u.mouse.x, ev->u.mouse.y, window->finger.pos.x,
               window->finger.pos.y, window->finger.inside);
#endif
        if (window->finger.inside) {
            window->finger.tracking = 1;
            add_finger_event(window,
                             window->finger.pos.x,
                             window->finger.pos.y,
                             1);
        } else {
            window->button.pressed = NULL;
            button = window->button.hover;
            if(button) {
                button->down += 1;
                skin_window_redraw( window, &button->rect );
                window->button.pressed = button;
                if(button->keycode) {
                    window->win_funcs->key_event(button->keycode, 1);
                }
            }
        }
        break;

    case kEventMouseButtonUp:
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
            skin_window_move_mouse( window, mx, my );
        }
        else if (window->finger.tracking)
        {
            skin_window_move_mouse( window, mx, my );
            window->finger.tracking = 0;
            add_finger_event(window,
                             window->finger.pos.x,
                             window->finger.pos.y,
                             0);
        }
        break;

    case kEventMouseMotion:
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
            skin_window_move_mouse( window, mx, my );
            if ( window->finger.tracking ) {
                add_finger_event(window,
                                 window->finger.pos.x,
                                 window->finger.pos.y,
                                 1);
            }
        }
        break;

    case kEventVideoExpose:
        skin_window_redraw_opengles(window);
        break;

    default:
        ;
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
    ADisplay*  disp = skin_window_display(window);

    if ( !window->surface )
        return;

    if (disp != NULL) {
        SkinRect  r;
        r.pos.x  = x;
        r.pos.y  = y;
        r.size.w = w;
        r.size.h = h;

        skin_rect_rotate( &r, &r, disp->rotation );
        r.pos.x += disp->origin.x;
        r.pos.y += disp->origin.y;

        if (window->effective_scale != 1.0)
            skin_window_redraw( window, &r );
        else
            display_redraw( disp, &r, window->surface );
    }
}
