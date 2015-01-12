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
#ifndef _ANDROID_SKIN_SURFACE_H
#define _ANDROID_SKIN_SURFACE_H

#include "android/skin/region.h"
#include <stdint.h>

/* a SkinSurface models a 32-bit ARGB pixel image that can be blitted to or from
 */
typedef struct SkinSurface  SkinSurface;

struct SkinScaler;

/* increment surface's reference count */
extern SkinSurface*  skin_surface_ref( SkinSurface*  surface );

/* decrement a surface's reference count. takes the surface's address as parameter.
   it will be set to NULL on exit */
extern void          skin_surface_unrefp( SkinSurface*  *psurface );

extern int skin_surface_width(SkinSurface* s);
extern int skin_surface_height(SkinSurface* s);

/* there are two kinds of surfaces:

   - fast surfaces, whose content can be placed in video memory or
     RLE-compressed for faster blitting and blending. the pixel format
     used internally might be something very different from ARGB32.

   - slow surfaces, whose content (pixel buffer) can be accessed and modified
     with _lock()/_unlock() but may be blitted far slower since they reside in
     system memory.
*/

/* create a 'fast' surface that contains a copy of an input ARGB32 pixmap */
extern SkinSurface*  skin_surface_create_fast( int  w, int  h );

/* create an empty 'slow' surface containing an ARGB32 pixmap */
extern SkinSurface*  skin_surface_create_slow( int  w, int  h );

/* create a 'slow' surface from a given pixel buffer. if 'do_copy' is TRUE, then
 * the content of 'pixels' is copied into a heap-allocated buffer. otherwise
 * the data will be used directly.
 */
extern SkinSurface*  skin_surface_create_argb32_from(
                            int                  w,
                            int                  h,
                            int                  pitch,
                            uint32_t*            pixels);

extern SkinSurface* skin_surface_create_window(
                            int x,
                            int y,
                            int w,
                            int h,
                            int original_w,
                            int original_h,
                            int is_fullscreen);

extern void skin_surface_reverse_map(SkinSurface* surface,
                                     int* x,
                                     int* y);

extern void skin_surface_get_scaled_rect(SkinSurface* surface,
                                         const SkinRect* srect,
                                         SkinRect* drect);

/* surface pixels information for slow surfaces */
typedef struct {
    int         w;
    int         h;
    int         pitch;
    uint32_t*   pixels;
} SkinSurfacePixels;

typedef struct {
    uint32_t r_shift;
    uint32_t r_mask;
    uint32_t g_shift;
    uint32_t g_mask;
    uint32_t b_shift;
    uint32_t b_mask;
    uint32_t a_shift;
    uint32_t a_mask;
} SkinSurfacePixelFormat;

extern void    skin_surface_get_format(SkinSurface* s,
                                       SkinSurfacePixelFormat* format);

extern void    skin_surface_set_alpha_blending( SkinSurface*  s, int alpha );

extern void    skin_surface_update(SkinSurface* surface, SkinRect* rect);

extern void    skin_surface_update_scaled(SkinSurface* dst_surface,
                                          struct SkinScaler* scaler,
                                          SkinSurface* src_surface,
                                          const SkinRect* src_rect);

extern void    skin_surface_upload(SkinSurface* surface,
                                   SkinRect* rect,
                                   const void* pixels,
                                   int pitch);

/* list of composition operators for the blit routine */
typedef enum {
    SKIN_BLIT_COPY = 0,
    SKIN_BLIT_SRCOVER,
} SkinBlitOp;


/* blit a surface into another one */
extern void    skin_surface_blit( SkinSurface*  dst,
                                  SkinPos*      dst_pos,
                                  SkinSurface*  src,
                                  SkinRect*     src_rect,
                                  SkinBlitOp    blitop );

/* blit a colored rectangle into a destination surface */
extern void    skin_surface_fill(SkinSurface*  dst,
                                 SkinRect*     rect,
                                 uint32_t      argb_premul);

#endif /* _ANDROID_SKIN_SURFACE_H */
