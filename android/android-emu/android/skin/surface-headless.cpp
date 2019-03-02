/* Copyright (C) 2015 The Android Open Source Project
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
#ifndef _MSC_VER
#include "android/skin/argb.h"
#endif
#include "android/skin/rect.h"
#include "android/skin/surface.h"
#include "android/skin/winsys.h"
#include "android/skin/qt/emulator-no-qt-no-window.h"
#include "android/utils/setenv.h"

#include <functional>
#include <memory>

#define  DEBUG  1

#if DEBUG
#include "android/utils/debug.h"

#define  D(...)   VERBOSE_PRINT(surface,__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif

static int next_id = 0;

static void skin_surface_free(SkinSurface *s) {
    D("skin_surface_free %d", s->id);
    (void)s;
}

extern int skin_surface_height(SkinSurface *s) {
    D("skin_surface_height %d", s->id);
    return s->h;
}

extern int skin_surface_is_round(SkinSurface *s) {
    return s->isRound;
}

extern void skin_surface_unrefp(SkinSurface* *psurface) {
    SkinSurface *surf = *psurface;
    if (surf) {
        D("skin_surface_unref %d", surf->id);
        skin_surface_free(surf);
        *psurface = NULL;
    }
}

extern int skin_surface_width(SkinSurface *s) {
    D("skin_surface_width %d", s->id);
    return s->w;
}

static SkinSurface* createSkinSurface(std::function<void(SkinSurface*)> fillBitmap) {
    auto s = std::unique_ptr<SkinSurface>(new SkinSurface());
    if (!s) {
        return nullptr;
    }
    s->window = EmulatorNoQtNoWindow::getInstancePtr();
    if (!s->window) {
        return nullptr;
    }
    fillBitmap(s.get());
    if (!s->bitmap) {
        return nullptr;
    }
    s->id = next_id++;
    s->isRound = 0;
    return s.release();
}

extern SkinSurface *skin_surface_create(
    int w, int h, int original_w, int original_h) {

    return createSkinSurface([w, h, original_w, original_h](SkinSurface* s) {
        s->bitmap = new SkinSurfaceBitmap(original_w, original_h);
        s->w = w;
        s->h = h;
        s->isRound = 0;
    });
}

extern SkinSurface* skin_surface_create_from_data(
    const void* data, int size) {
    return createSkinSurface([data, size](SkinSurface* s) {
        s->bitmap = new SkinSurfaceBitmap((const unsigned char*)data, size);
        s->w = s->bitmap->size().width();
        s->h = s->bitmap->size().height();
        s->isRound = 0;
    });
}

extern SkinSurface* skin_surface_create_from_file(const char* path) {
    return createSkinSurface([path](SkinSurface* s) {
        s->bitmap = new SkinSurfaceBitmap(path);
        s->w = s->bitmap->size().width();
        s->h = s->bitmap->size().height();
        s->isRound = 0;
    });
}

extern SkinSurface* skin_surface_create_derived(SkinSurface* source,
                                                SkinRotation rotation,
                                                int blend) {
    return createSkinSurface([source, rotation, blend](SkinSurface* s) {
        s->w = source->w;
        s->h = source->h;
        s->isRound = source->isRound;
        if (rotation == SKIN_ROTATION_90 || rotation == SKIN_ROTATION_270) {
            std::swap(s->w, s->h);
        }
        s->bitmap = new SkinSurfaceBitmap(*source->bitmap, rotation, blend);
    });
}


extern SkinSurface* skin_surface_resize(SkinSurface *surface, int w, int h,
                                        int original_w, int original_h)
{
    if ( surface == NULL ) {
        return skin_surface_create(w, h, original_w, original_h);
    } else if (surface->bitmap->size() == QSize(original_w, original_h)) {
        surface->w = w;
        surface->h = h;
        return surface;
    } else {
        skin_surface_unrefp(&surface);
        return skin_surface_create(w, h, original_w, original_h);
    }
}

extern void skin_surface_create_window(SkinSurface* surface,
                                       int x,
                                       int y,
                                       int w,
                                       int h) {
    D("skin_surface_create_window  %d, %d, %d, %d", x, y, w, h);
    EmulatorNoQtNoWindow *window = EmulatorNoQtNoWindow::getInstance();
    if (window == NULL) return;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
}

extern void skin_surface_update(SkinSurface *surface, SkinRect *rect) {
    (void)surface;
    (void)rect;
}

extern void skin_surface_blit(SkinSurface *dst, SkinPos *pos, SkinSurface *src, SkinRect *rect, SkinBlitOp op) {
    (void)dst;
    (void)pos;
    (void)src;
    (void)rect;
    (void)op;
}

extern void skin_surface_fill(
    SkinSurface *dst, SkinRect *rect, uint32_t argb_premul) {
    D("skin_surface_fill %d: %d, %d, %d, %d: %x",
      dst->id, rect->pos.x, rect->pos.y, rect->size.w, rect->size.h, argb_premul);
}

extern void skin_surface_upload(
    SkinSurface *surface, const SkinRect *rect, const void *pixels, int pitch) {
    D("skin_surface_upload %d: %d,%d,%d,%d",
      surface->id, rect->pos.x, rect->pos.y, rect->size.w, rect->size.h);
    (void)pixels;
    (void)pitch;
}

extern void skin_surface_get_scaled_rect(SkinSurface *surface, const SkinRect *from, SkinRect *to) {
    int fromx = from->pos.x;
    int fromy = from->pos.y;
    int fromw = from->size.w;
    int fromh = from->size.h;
    int w = surface->w;
    int h = surface->h;
    int original_w = surface->bitmap->size().width();
    int original_h = surface->bitmap->size().height();

    to->pos.x = from->pos.x * w / original_w;
    to->pos.y = from->pos.y * h / original_h;

    // The skin surface may not be the same dimensions as the emulator
    // subwindow.  In these cases, the emulator subwindow is usually smaller,
    // which means that if we do simple scaling of the emulator subwindow
    // according to the skin surface's size ratio versus original skin surface
    // size, the part of the skin surface that should coincide with the
    // emulator window will often be bigger than the emulator subwindow by a
    // few pixels (or even partial pixels in the case of high DPI displays),
    // resulting in peeking of skin surface pixels out from underneath the
    // subwindow.  This can look bad if, for example, the skin surface is a
    // Pixel XL Silver.  To address this, we ensure that the subwindow is
    // scaled up a bit more, so that it always covers the part of the skin
    // surface underneath. We don't play with the x/y offset since those are
    // biased to be a bit small too, but they are the left/top offsets, so if
    // they are too small, they cover more of what's underneath, which is
    // actually the kind of bias we like.

    double fudge = original_w == fromw ? 0.0f : 1.0f;

    double surfaceFactorW = ((double)w) / ((double)original_w);
    double surfaceFactorH = ((double)h) / ((double)original_h);

    double wantedFactor = surfaceFactorW > surfaceFactorH ? surfaceFactorW : surfaceFactorH;

    if (fromw <= fromh) {
        to->size.w = (int)(from->size.w * wantedFactor + fudge + 0.5f);
        double originalAspect = ((double)fromh) / ((double)fromw);
        to->size.h = to->size.w * originalAspect + 0.5f;
    } else {
        to->size.h = (int)(from->size.h * wantedFactor + fudge + 0.5f);
        double originalAspect = ((double)fromw) / ((double)fromh);
        to->size.w = to->size.h * originalAspect + 0.5f;
    }

    D("skin_surface_get_scaled_rect %d: %d, %d, %d, %d => %d, %d, %d, %d",
      surface->id, fromx, fromy, fromw, fromh, to->pos.x, to->pos.y, to->size.w, to->size.h);
}

extern void skin_surface_reverse_map(SkinSurface *surface, int *x, int *y) {
    int new_x = *x * surface->bitmap->size().width() / surface->w;
    int new_y = *y * surface->bitmap->size().height() / surface->h;
#if 0
    D("skin_surface_reverse_map %d: %d,%d to %d,%d", surface->id, *x, *y, new_x, new_y);
#endif
    *x = new_x;
    *y = new_y;
}
