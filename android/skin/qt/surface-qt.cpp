/* Copyright (C) 2014 The Android Open Source Project
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
#include "android/skin/surface.h"
#include "android/skin/argb.h"
#include "android/utils/setenv.h"
#include "android/skin/rect.h"
#include "android/skin/surface.h"
#include "android/skin/winsys.h"
#include "qt-context.h"

#include <QtCore>
#include <QObject>
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QRect>
#include <QWidget>
#include <android/skin/rect.h>
#include <android/skin/surface.h>

#define  DEBUG  1

#if DEBUG
#include "android/utils/debug.h"
#include "../rect.h"

#define  D(...)   VERBOSE_PRINT(surface,__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif

static int count = 0;

SkinSurface *create_surface(int w, int h);

static void
skin_surface_free( SkinSurface*  s )
{
    D("skin_surface_free %d", s->id);
//    if (s->done_func) {
//        s->done_func( s->done_user );
//        s->done_func = NULL;
//    }

    // This probably should be done from Context's thread.
//    if (s->context->bitmap) {
//        delete s->context->bitmap;
//        s->context->bitmap = NULL;
//    }
    free(s);
}

extern SkinSurface*  skin_surface_ref( SkinSurface*  surface ) {
    if (surface) {
        D("skin_surface_ref %d", surface->id);
        surface->refcount += 1;
    }
    return surface;
}

extern void skin_surface_unrefp( SkinSurface*  *psurface ) {
    SkinSurface*  surf = *psurface;
    if (surf) {
        D("skin_surface_unref %d", surf->id);
        if (--surf->refcount <= 0)
            skin_surface_free(surf);
        *psurface = NULL;
    }
}


extern int skin_surface_lock( SkinSurface*  s, SkinSurfacePixels  *pix ) {
    D("skin_surface_lock %d", s->id);
    QemuContext::instance->getBitmapInfo(s, pix);
//    D("skin_surface_lock %d, %d, %d", pix->w, pix->h, pix->pitch);
    return 0;
}

extern void    skin_surface_unlock( SkinSurface*  s ) {
    D("skin_surface_unlock %d", s->id);
}

//extern void  skin_surface_set_done( SkinSurface*  s, SkinSurfaceDoneFunc, void* ) {
//    D("skin_surface_set_done %d", s->id);
//}

extern int skin_surface_width(SkinSurface* s) {
    D("skin_surface_width %d", s->id);
    return s->bitmap->size().width();
}

extern int skin_surface_height(SkinSurface* s) {
    D("skin_surface_height %d", s->id);
    return s->bitmap->size().height();
}

extern SkinSurface*  skin_surface_create_fast( int  w, int  h ) {
    D("skin_surface_create_fast %d, %d", w, h);
    return create_surface(w, h);
}


extern SkinSurface*  skin_surface_create_slow( int  w, int  h ) {
    D("skin_surface_create_slow %d, %d ", w, h);
    return create_surface(w, h);
}

SkinSurface *create_surface(int w, int h) {
    SkinSurface*  s = (SkinSurface*)malloc(sizeof(*s));
    if (s != NULL) {
        QemuContext::instance->createBitmap(s, w, h);
        s->refcount = 1;
        s->id = count++;
//        s->done_func = NULL;
        D("Created surface %d %d,%d", s->id, w, h);
    }
    else {
        D( "not enough memory to allocate new skin surface !" );
    }
    return  s;
}

extern SkinSurface*  skin_surface_create_argb32_from(
                            int                  w,
                            int                  h,
                            int                  pitch,
                            uint32_t*            pixels) {
    D("skin_surface_create_argb32_from %d, %d, pitch %d (%d)", w, h, pitch);
    SkinSurface* s = create_surface(w, h);
    SkinRect rect;
    rect.size.h = h;
    rect.size.w = w;
    rect.pos.x = 0;
    rect.pos.y = 0;
    SkinSurfacePixels p;
    skin_surface_lock(s, &p);
    skin_surface_upload(s, &rect, pixels, pitch);
    skin_surface_unlock(s);

//    SkinSurfacePixels p;
//    skin_surface_lock(s, &p);
//    uint32_t* src = pixels;
//    uint32_t* dst = p.pixels;
//    for (int y = 0; y < h; y++) {
//        for (int x = 0; x < w; x++) {
//            dst[x] = src[x];
//        }
//        dst += p.pitch / sizeof(uint32_t);
//        src += pitch / sizeof(uint32_t);
//    }
//    skin_surface_unlock(s);
    return s;
}

extern SkinSurface* skin_surface_create_window(
                            int x,
                            int y,
                            int w,
                            int h,
                            int original_w,
                            int original_h,
                            int is_fullscreen) {
    D("skin_surface_create_window  %d, %d, %d, %d, %d, %d, fullscreen: %d", x, y, w, h, original_w, original_h, is_fullscreen);
    QemuContext::instance->showWindow(x, y, w, h, is_fullscreen);
    SkinSurface* surface = create_surface(w, h);
    QemuContext::instance->setBackingBitmap(surface->bitmap);
    D("ID of backing bitmap surface is %d", surface->id);
    return surface;
}

extern void    skin_surface_get_format(SkinSurface* s,
                                       SkinSurfacePixelFormat* format) {
    D("skin_surface_get_format %d", s->id);
    format->a_mask = 0xff000000;
    format->r_mask = 0x00ff0000;
    format->g_mask = 0x0000ff00;
    format->b_mask = 0x000000ff;
    format->a_shift = 24;
    format->r_shift = 16;
    format->g_shift = 8;
    format->b_shift = 0;
}

extern void    skin_surface_set_alpha_blending( SkinSurface*  s, int  ) {
    D("skin_surface_set_alpha_blending %d", s->id);
}

extern void    skin_surface_update(SkinSurface* surface, SkinRect* rect) {
    D("skin_surface_update %d: %d,%d,%d,%d", surface->id, rect->pos.x, rect->pos.y, rect->size.w, rect->size.h);
    QRect qrect(rect->pos.x, rect->pos.y, rect->size.w, rect->size.h);
    QemuContext::instance->update(qrect);
}

extern void    skin_surface_blit( SkinSurface*  dst,
                                  SkinPos*      pos,
                                  SkinSurface*  src,
                                  SkinRect*     rect,
                                  SkinBlitOp     op) {
//    D("skin_surface_blit %d (surface %dx%d) (blit rect%d,%d:%dx%d) to %d (surface %dx%d) (pos %d,%d)",
//            src->id, src->bitmap->width(), src->bitmap->height(), rect->pos.x, rect->pos.y, rect->size.w, rect->size.h,
//            dst->id, dst->bitmap->width(), dst->bitmap->height(), pos->x, pos->y);
    QRect qrect(rect->pos.x, rect->pos.y, rect->size.w, rect->size.h);
    QPoint qpos(pos->x, pos->y);
    QPainter::CompositionMode qop;
    switch(op) {
        default:
        case SKIN_BLIT_COPY:
            qop = QPainter::CompositionMode_Source;
            break;
        case SKIN_BLIT_SRCOVER:
            qop = QPainter::CompositionMode_SourceOver;
            break;
    }
    QemuContext::instance->blit(src->bitmap, qrect, dst->bitmap, qpos, qop);
}

extern void    skin_surface_fill(SkinSurface*  dst,
                                 SkinRect*     rect,
                                 uint32_t      argb_premul) {
    D("skin_surface_fill %d: %d, %d, %d, %d: %x", dst->id, rect->pos.x, rect->pos.y, rect->size.w, rect->size.h, argb_premul);
    QRect qrect(rect->pos.x, rect->pos.y, rect->size.w, rect->size.h);
    QColor color(argb_premul);
    QemuContext::instance->fill(dst, qrect, color);
}

extern void    skin_surface_upload(SkinSurface* surface,
        SkinRect* rect,
        const void* pixels,
        int pitch) {
//    D("skin_surface_upload %d: %d,%d %dx%d: %d", surface->id, rect->pos.x, rect->pos.y, rect->size.w, rect->size.h, pitch);

//    uint32_t* src = ((uint32_t*)pixels) + (pitch / sizeof(uint32_t)) * rect->pos.y;
//    uint32_t* dst = ((uint32_t*)surface->bitmap->bits()) + surface->bitmap->width() * rect->pos.y;
////    D("src is %lx(%lx), dst is %lx(%lx)", (long)src, (long)pixels, (long)dst, (long)surface->bitmap->bits());
//    int num = 0;
//    for (int y = rect->pos.y; y < rect->pos.y + rect->size.h; y++) {
//        for (int x = rect->pos.x; x < rect->pos.x + rect->size.w; x++) {
//            *(dst + x) = *(src + x);
//            num++;
//        }
//        src += pitch / sizeof(uint32_t);
//        dst += surface->bitmap->width();
//    }

    uint32_t* src = ((uint32_t*)pixels);
    uint32_t* dst = ((uint32_t*)surface->bitmap->bits()) + surface->bitmap->width() * rect->pos.y;
//    D("src is %lx(%lx), dst is %lx(%lx)", (long)src, (long)pixels, (long)dst, (long)surface->bitmap->bits());
    int num = 0;
    for (int y = rect->pos.y; y < rect->pos.y + rect->size.h; y++) {
        for (int x = rect->pos.x; x < rect->pos.x + rect->size.w; x++) {
            *(dst + x) = *(src + x -rect->pos.x);
            num++;
        }
        src += pitch / sizeof(uint32_t);
        dst += surface->bitmap->width();
    }

//    memcpy(surface->bitmap->bits(), pixels, pitch * surface->bitmap->height());
//    D("copied %d pixels", num);
}

extern void skin_surface_get_scaled_rect(SkinSurface*,
        const SkinRect*,
        SkinRect*) {
    D("skin_surface_get_scaled_rect");
}

extern void skin_surface_reverse_map(SkinSurface*,
        int*,
        int*) {
    D("skin_surface_reverse_map");
}
