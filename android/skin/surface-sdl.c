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
#include "android/skin/surface.h"
#include "android/skin/argb.h"
#include "android/skin/scaler.h"
#include "android/skin/winsys.h"
#include "android/utils/setenv.h"
#include <SDL.h>

#define  DEBUG  1

#if DEBUG
#include "android/utils/debug.h"
#define  D(...)   VERBOSE_PRINT(surface,__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif

struct SkinSurface {
    int refcount;
    SDL_Surface* surface;
    // Only used for scaled window surfaces
    SkinScaler* scaler;
    SDL_Surface* scaled_surface;
};

static void
skin_surface_free( SkinSurface*  s )
{
    if (s->scaled_surface) {
        SDL_FreeSurface(s->scaled_surface);
        s->scaled_surface = NULL;
    }
    if (s->scaler) {
        skin_scaler_free(s->scaler);
        s->scaler = NULL;
    }
    if (s->surface) {
        SDL_FreeSurface(s->surface);
        s->surface = NULL;
    }
    free(s);
}

extern SkinSurface*
skin_surface_ref( SkinSurface*  surface )
{
    if (surface)
        surface->refcount += 1;
    return surface;
}

extern void
skin_surface_unrefp( SkinSurface*  *psurface )
{
    SkinSurface*  surf = *psurface;
    if (surf) {
        if (--surf->refcount <= 0)
            skin_surface_free(surf);
        *psurface = NULL;
    }
}

SDL_Surface*
skin_surface_get_sdl(SkinSurface* s) {
  return s->surface;
}

extern int
skin_surface_width(SkinSurface* s) {
    return s->surface->w;
}

extern int
skin_surface_height(SkinSurface* s) {
    return s->surface->h;
}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#  define  ARGB32_R_MASK  0x00ff0000
#  define  ARGB32_G_MASK  0x0000ff00
#  define  ARGB32_B_MASK  0x000000ff
#  define  ARGB32_A_MASK  0xff000000
#else
#  define  ARGB32_R_MASK  0x00ff0000
#  define  ARGB32_G_MASK  0x0000ff00
#  define  ARGB32_B_MASK  0x000000ff
#  define  ARGB32_A_MASK  0xff000000
#endif

static SDL_Surface*
_sdl_surface_create_rgb( int  width,
                         int  height,
                         int  depth,
                         int  flags )
{
   Uint32   rmask, gmask, bmask, amask;

    if (depth == 8) {
        rmask = gmask = bmask = 0;
        amask = 0xff;
    } else if (depth == 32) {
        rmask = ARGB32_R_MASK;
        gmask = ARGB32_G_MASK;
        bmask = ARGB32_B_MASK;
        amask = ARGB32_A_MASK;
    } else
        return NULL;

    return SDL_CreateRGBSurface(flags, width, height, depth,
                                 rmask, gmask, bmask, amask );
}


static SDL_Surface*
_sdl_surface_create_rgb_from(int   width,
                             int   height,
                             int   pitch,
                             void* pixels,
                             int   depth)
{
   Uint32   rmask, gmask, bmask, amask;

    if (depth == 8) {
        rmask = gmask = bmask = 0;
        amask = 0xff;
    } else if (depth == 32) {
        rmask = ARGB32_R_MASK;
        gmask = ARGB32_G_MASK;
        bmask = ARGB32_B_MASK;
        amask = ARGB32_A_MASK;
    } else
        return NULL;

    return SDL_CreateRGBSurfaceFrom( pixels, width, height, depth, pitch,
                                     rmask, gmask, bmask, amask );
}


static SkinSurface*
_skin_surface_create(SDL_Surface* surface)
{
    SkinSurface* s = malloc(sizeof(*s));
    if (s != NULL) {
        s->refcount = 1;
        s->surface  = surface;
        s->scaler = NULL;
        s->scaled_surface = NULL;
    }
    else {
        SDL_FreeSurface(surface);
        D( "not enough memory to allocate new skin surface !" );
    }
    return  s;
}


SkinSurface*
skin_surface_create_fast( int  w, int  h )
{
    SDL_Surface* surface = _sdl_surface_create_rgb(w, h, 32, SDL_HWSURFACE);
    if (surface == NULL) {
        surface = _sdl_surface_create_rgb( w, h, 32, SDL_SWSURFACE );
        if (surface == NULL) {
            D( "could not create fast %dx%d ARGB32 surface: %s",
               w, h, SDL_GetError() );
            return NULL;
        }
    }
    return _skin_surface_create(surface);
}


SkinSurface*
skin_surface_create_slow( int  w, int  h )
{
    SDL_Surface* surface = _sdl_surface_create_rgb(w, h, 32, SDL_SWSURFACE);
    if (surface == NULL) {
        D( "could not create slow %dx%d ARGB32 surface: %s",
            w, h, SDL_GetError() );
        return NULL;
    }
    return _skin_surface_create(surface);
}


SkinSurface*
skin_surface_create_argb32_from(int       w,
                                int       h,
                                int       pitch,
                                uint32_t* pixels)
{
    SDL_Surface* surface =
            _sdl_surface_create_rgb_from(w, h, pitch, pixels, 32);
    if (surface == NULL) {
        D( "could not create %dx%d slow ARGB32 surface: %s",
            w, h, SDL_GetError() );
        return NULL;
    }
    return _skin_surface_create(surface);
}

extern SkinSurface*
skin_surface_create_window(int x,
                           int y,
                           int w,
                           int h,
                           int original_w,
                           int original_h,
                           int is_fullscreen)
{
    char temp[32];
    sprintf(temp, "%d,%d", x, y);
    setenv("SDL_VIDEO_WINDOW_POS", temp, 1);
    setenv("SDL_VIDEO_WINDOW_FORCE_VISIBLE", "1", 1);

    int flags = SDL_SWSURFACE;
    if (is_fullscreen) {
        flags |= SDL_FULLSCREEN;

        SkinRect r;
        skin_winsys_get_monitor_rect(&r);

        x = r.pos.x;
        y = r.pos.y;
        w = r.size.w;
        h = r.size.h;
    }

    SDL_Surface* surface = SDL_SetVideoMode(w, h, 32, flags);
    if (!surface) {
        fprintf(stderr, "### Error: could not create or resize SDL window: %s\n", SDL_GetError() );
        exit(1);
    }

    SDL_WM_SetPos(x, y);

    double x_scale = w * 1.0 / original_w;
    double y_scale = h * 1.0 / original_h;
    double scale = (x_scale <= y_scale) ? x_scale : y_scale;

    SkinSurface* result = _skin_surface_create(surface);
    if (result && scale != 1.0) {
        result->scaler = skin_scaler_create();

        double effective_x = 0.;
        double effective_y = 0.;

        if (is_fullscreen) {
            effective_x = (w - original_w * scale) * 0.5;
            effective_y = (h - original_h * scale) * 0.5;
        }

        skin_scaler_set(result->scaler,
                        scale,
                        effective_x,
                        effective_y);

        result->scaled_surface = result->surface;

        result->surface = _sdl_surface_create_rgb(original_w,
                                                  original_h,
                                                  32,
                                                  SDL_SWSURFACE);
        if (!result->surface) {
            fprintf(stderr,
                    "### Error: could not create unscaled SDL window: %s\n",
                    SDL_GetError());
            exit(1);
        }
    }
    return result;
}

void
skin_surface_reverse_map(SkinSurface* surface,
                         int* x,
                         int* y) {
    if (surface && surface->scaler) {
        skin_scaler_reverse_map(surface->scaler, x, y);
    }
}

void
skin_surface_get_scaled_rect(SkinSurface* surface,
                             const SkinRect* srect,
                             SkinRect* drect) {
    if (surface && surface->scaler) {
        skin_scaler_get_scaled_rect(surface->scaler, srect, drect);
    } else {
        *drect = *srect;
    }
}

static int _sdl_surface_lock(SDL_Surface* s, SkinSurfacePixels  *pix) {
    if (SDL_LockSurface(s) != 0) {
        D( "could not lock surface %p: %s", s, SDL_GetError() );
        return -1;
    }
    pix->w      = s->w;
    pix->h      = s->h;
    pix->pitch  = s->pitch;
    pix->pixels = s->pixels;
    return 0;
}

extern void _sdl_surface_get_format(SDL_Surface* s,
                                    SkinSurfacePixelFormat* format)
{
    format->r_shift = s->format->Rshift;
    format->g_shift = s->format->Gshift;
    format->b_shift = s->format->Bshift;
    format->a_shift = s->format->Ashift;

    format->r_mask = s->format->Rmask;
    format->g_mask = s->format->Gmask;
    format->b_mask = s->format->Bmask;
    format->a_mask = s->format->Amask;
}

extern void
skin_surface_set_alpha_blending(SkinSurface*  s, int alpha)
{
    if (s && s->surface) {
        SDL_SetAlpha(s->surface, SDL_SRCALPHA, alpha);
    }
}

extern void
skin_surface_update(SkinSurface* s, SkinRect* r) {
    if (!s || !s->surface) {
        return;
    }

    // First, the unscaled case.
    if (!s->scaler) {
        SDL_UpdateRect(s->surface, r->pos.x, r->pos.y, r->size.w, r->size.h);
        return;
    }

    // Now, the scaled case.
    SkinScaler* scaler = s->scaler;
    SDL_Surface* src_surface = s->surface;
    SkinSurfacePixels src_pix;
    _sdl_surface_lock(src_surface, &src_pix);

    SDL_Surface* dst_surface = s->scaled_surface;
    SkinSurfacePixels dst_pix;
    SkinSurfacePixelFormat dst_format;
    _sdl_surface_lock(dst_surface, &dst_pix);
    _sdl_surface_get_format(dst_surface, &dst_format);

    skin_scaler_scale(scaler,
                      &dst_pix,
                      &dst_format,
                      &src_pix,
                      r);

    SDL_UnlockSurface(dst_surface);
    SDL_UnlockSurface(src_surface);

    SkinRect dst_rect;
    skin_scaler_get_scaled_rect(scaler, r, &dst_rect);

    SDL_UpdateRect(dst_surface,
                   dst_rect.pos.x,
                   dst_rect.pos.y,
                   dst_rect.size.w,
                   dst_rect.size.h);
}

void
skin_surface_update_scaled(SkinSurface* dst_surface,
                           SkinScaler* scaler,
                           SkinSurface* src_surface,
                           const SkinRect* src_rect) {
}




static uint32_t
skin_surface_map_argb( SkinSurface*  s, uint32_t  c )
{
    if (s && s->surface) {
        return SDL_MapRGBA( s->surface->format,
                            ((c) >> 16) & 255,
                            ((c) >> 8) & 255,
                            ((c) & 255),
                            ((c) >> 24) & 255 );
    }
    return 0x00000000;
}

extern void
skin_surface_fill(SkinSurface*  dst,
                  SkinRect*     rect,
                  uint32_t      argb_premul)
{
    SDL_Rect rd;

    if (rect) {
        rd.x = rect->pos.x;
        rd.y = rect->pos.y;
        rd.w = rect->size.w;
        rd.h = rect->size.h;
    } else {
        rd.x = 0;
        rd.y = 0;
        rd.w = dst->surface->w;
        rd.h = dst->surface->h;
    }
    uint32_t color = skin_surface_map_argb(dst, argb_premul);

    SDL_FillRect(dst->surface, &rd, color);
}

extern void
skin_surface_blit( SkinSurface*  dst,
                   SkinPos*      dst_pos,
                   SkinSurface*  src,
                   SkinRect*     src_rect,
                   SkinBlitOp    blitop )
{
    SDL_Rect dr, sr;
    dr.x = dst_pos->x;
    dr.y = dst_pos->y;
    dr.w = src_rect->size.w;
    dr.h = src_rect->size.h;

    sr.x = src_rect->pos.x;
    sr.y = src_rect->pos.y;
    sr.w = src_rect->size.w;
    sr.h = src_rect->size.h;

    switch (blitop) {
        case SKIN_BLIT_COPY:
            SDL_SetAlpha(src->surface, 0, 255);
            break;
        case SKIN_BLIT_SRCOVER:
            SDL_SetAlpha(src->surface, SDL_SRCALPHA, 255);
            break;
        default:
            return;
    }
    SDL_BlitSurface(src->surface, &sr, dst->surface, &dr);
}

void
skin_surface_upload(SkinSurface* surface,
                    SkinRect* rect,
                    const void* pixels,
                    int pitch) {
    // Sanity checks
    if (!surface || !surface->surface) {
        return;
    }

    // Crop rectangle.
    int dst_w = skin_surface_width(surface);
    int dst_h = skin_surface_height(surface);
    int dst_pitch = surface->surface->pitch;
    int x = 0, y = 0, w = dst_w, h = dst_h;

    if (rect) {
        x = rect->pos.x;
        y = rect->pos.y;
        w = rect->size.w;
        h = rect->size.h;

        int delta;
        delta = x + w - dst_w;
        if (delta > 0) {
            w -= delta;
        }
        delta = y + h - dst_h;
        if (delta > 0) {
            h -= delta;
        }
        if (x < 0) {
            w += x;
            x = 0;
        }
        if (y < 0) {
            h += y;
            y = 0;
        }
    }

    if (w <= 0 || h <= 0) {
        return;
    }

    SkinSurfacePixels pix;
    _sdl_surface_lock(surface->surface, &pix);

    const uint8_t* src_line = (const uint8_t*)pixels;

    uint8_t* dst_line = (uint8_t*)(
        (char*)pix.pixels + (pix.pitch * y) + (x * 4));

    for (; h > 0; --h) {
        memcpy(dst_line, src_line, 4 * w);
        dst_line += dst_pitch;
        src_line += pitch;
    }

    SDL_UnlockSurface(surface->surface);
    skin_surface_update(surface, rect);
}
