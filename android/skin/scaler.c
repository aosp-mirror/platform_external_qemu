/* Copyright (C) 2008 The Android Open Source Project
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
#include "android/skin/scaler.h"

#include <stdint.h>
#include <math.h>

struct SkinScaler {
    double  scale;
    double  xdisp, ydisp;
    double  invscale;
    int     valid;
};

static SkinScaler  _scaler0;

SkinScaler*
skin_scaler_create( void )
{
    _scaler0.scale    = 1.0;
    _scaler0.xdisp    = 0.0;
    _scaler0.ydisp    = 0.0;
    _scaler0.invscale = 1.0;
    return &_scaler0;
}

/* change the scale of a given scaler. returns 0 on success, or -1 in case of
 * problem (unsupported scale) */
int
skin_scaler_set( SkinScaler*  scaler, double  scale, double xdisp, double ydisp )
{
    /* right now, we only support scales in the 0.5 .. 1.0 range */
    if (scale < 0.1)
        scale = 0.1;
    else if (scale > 6.0)
        scale = 6.0;

    scaler->scale    = scale;
    scaler->xdisp    = xdisp;
    scaler->ydisp    = ydisp;
    scaler->invscale = 1/scale;
    scaler->valid    = 1;

    return 0;
}

void
skin_scaler_free( SkinScaler*  scaler )
{
    (void)scaler;
}

typedef struct {
    SkinRect    rd;         /* destination rectangle */
    int         sx, sy;     /* source start position in 16.16 format */
    int         ix, iy;     /* source increments in 16.16 format */
    int         src_pitch;
    int         src_w;
    int         src_h;
    int         dst_pitch;
    uint8_t*    dst_line;
    uint8_t*    src_line;
    double      scale;
} ScaleOp;


#define  ARGB_SCALE_GENERIC       scale_generic
#define  ARGB_SCALE_05_TO_10      scale_05_to_10
#define  ARGB_SCALE_UP_BILINEAR   scale_up_bilinear
/* #define  ARGB_SCALE_UP_QUICK_4x4  scale_up_quick_4x4 UNUSED */

#include "android/skin/argb.h"


void
skin_scaler_reverse_map(SkinScaler* scaler,
                        int* x,
                        int* y) {
    // Don't use invscale for maximum accuracy.
    *x = (*x - scaler->xdisp) / scaler->scale;
    *y = (*y - scaler->ydisp) / scaler->scale;
}

void
skin_scaler_get_scaled_rect( SkinScaler*     scaler,
                             const SkinRect* srect,
                             SkinRect*       drect )
{
    int sx = srect->pos.x;
    int sy = srect->pos.y;
    int sw = srect->size.w;
    int sh = srect->size.h;
    double scale = scaler->scale;

    if (!scaler->valid) {
        drect[0] = srect[0];
        return;
    }

    drect->pos.x = (int)(sx * scale + scaler->xdisp);
    drect->pos.y = (int)(sy * scale + scaler->ydisp);
    drect->size.w = (int)(ceil((sx + sw) * scale + scaler->xdisp)) - drect->pos.x;
    drect->size.h = (int)(ceil((sy + sh) * scale + scaler->ydisp)) - drect->pos.y;
}

void
skin_scaler_scale( SkinScaler*   scaler,
                   const SkinSurfacePixels* dst_pix,
                   const SkinSurfacePixelFormat* dst_format,
                   const SkinSurfacePixels* src_pix,
                   const SkinRect* src_rect)
{
    ScaleOp   op;

    if ( !scaler->valid ) {
        return;
    }

    {
        op.scale     = scaler->scale;
        op.src_pitch = src_pix->pitch;
        op.src_line  = (uint8_t*)src_pix->pixels;
        op.src_w     = src_pix->w;
        op.src_h     = src_pix->h;
        op.dst_pitch = dst_pix->pitch;
        op.dst_line  = (uint8_t*)dst_pix->pixels;

        /* compute the destination rectangle */
        skin_scaler_get_scaled_rect(scaler, src_rect, &op.rd);

        /* compute the starting source position in 16.16 format
         * and the corresponding increments */
        op.sx = (int)((op.rd.pos.x - scaler->xdisp) * scaler->invscale * 65536);
        op.sy = (int)((op.rd.pos.y - scaler->ydisp) * scaler->invscale * 65536);

        op.ix = (int)( scaler->invscale * 65536 );
        op.iy = op.ix;

        op.dst_line += op.rd.pos.x * 4 + op.rd.pos.y * op.dst_pitch;

        if (op.scale >= 0.5 && op.scale <= 1.0)
            scale_05_to_10( &op );
        else if (op.scale > 1.0)
            scale_up_bilinear( &op );
        else
            scale_generic( &op );
    }

    // The optimized scale functions in argb.h assume the destination is ARGB.
    // If that's not the case, do a channel reorder now.
    if (dst_format->r_shift != 16 ||
        dst_format->g_shift !=  8 ||
        dst_format->b_shift !=  0)
    {
        uint32_t rshift = dst_format->r_shift;
        uint32_t gshift = dst_format->g_shift;
        uint32_t bshift = dst_format->b_shift;
        uint32_t ashift = dst_format->a_shift;
        uint32_t amask  = dst_format->a_mask; // may be 0x00
        int x, y;

        for (y = 0; y < op.rd.size.h; y++)
        {
            uint32_t* line = (uint32_t*)(op.dst_line + y*op.dst_pitch);
            for (x = 0; x < op.rd.size.w; x++) {
                uint32_t r = (line[x] & 0x00ff0000) >> 16;
                uint32_t g = (line[x] & 0x0000ff00) >>  8;
                uint32_t b = (line[x] & 0x000000ff) >>  0;
                uint32_t a = (line[x] & 0xff000000) >> 24;
                line[x] = (r << rshift) | (g << gshift) | (b << bshift) |
                          ((a << ashift) & amask);
            }
        }
    }
}
