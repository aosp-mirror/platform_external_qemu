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
#ifndef _ANDROID_SKIN_SCALER_H
#define _ANDROID_SKIN_SCALER_H

#include "android/skin/image.h"
#include "android/skin/surface.h"

typedef struct SkinScaler   SkinScaler;

/* create a new image scaler. by default, it uses a scale of 1.0 */
extern SkinScaler*  skin_scaler_create( void );
extern void         skin_scaler_free(SkinScaler* scaler);

/* change the scale of a given scaler. returns 0 on success, or -1 in case of
 * problem (unsupported scale) */
extern int          skin_scaler_set( SkinScaler*  scaler,
                                     double       scale,
                                     double       xDisp,
                                     double       yDisp );

/* Compute inverse scaled coordinates.
 * On input, |*x| and |*y| contain scaled coordinates in pixels.
 * On output, |*x| and |*y| contain the corresponding unscaled coordinates
 * in pixels.
 */
extern void         skin_scaler_reverse_map(SkinScaler* scaler,
                                            int* x,
                                            int* y);

/* retrieve the position of the scaled source rectangle 'srect' into 'drect'
 * you can use the same pointer for both parameters. */
extern void         skin_scaler_get_scaled_rect( SkinScaler*     scaler,
                                                 const SkinRect* srect,
                                                 SkinRect*       drect );

/* destroy a given SkinScaler instance. */
extern void         skin_scaler_free( SkinScaler*  scaler );

/* perform a scaling operation with the parameters of a given |scaler|.
 * |dst_pix| and |dst_format| describe the destination surface, while
 * |src_pix| describes the source surface.
 * |src_rect| is the input source rectangle that is going to be scaled into
 * the destination.
 * To know which destination pixels were touched, use
 * skin_scaler_get_scaled_rect().
 */
extern void         skin_scaler_scale( SkinScaler* scaler,
                                       const SkinSurfacePixels* dst_pix,
                                       const SkinSurfacePixelFormat* dst_format,
                                       const SkinSurfacePixels* src_pix,
                                       const SkinRect* src_rect);

#endif /* _ANDROID_SKIN_SCALER_H */
