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
    int                  refcount;
    uint32_t*            pixels;
    SDL_Surface*         surface;
    SkinSurfaceDoneFunc  done_func;
    void*                done_user;
};

static void
skin_surface_free( SkinSurface*  s )
{
    if (s->done_func) {
        s->done_func( s->done_user );
        s->done_func = NULL;
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

void
skin_surface_set_done( SkinSurface*  s, SkinSurfaceDoneFunc  done_func, void*  done_user )
{
    s->done_func = done_func;
    s->done_user = done_user;
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
_sdl_surface_create_rgb_from( int   width,
                              int   height,
                              int   pitch,
                              void* pixels,
                              int   depth )
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
_skin_surface_create( SDL_Surface*  surface,
                      void*         pixels )
{
    SkinSurface*  s = malloc(sizeof(*s));
    if (s != NULL) {
        s->refcount = 1;
        s->pixels   = pixels;
        s->surface  = surface;
        s->done_func = NULL;
        s->done_user = NULL;
    }
    else {
        SDL_FreeSurface(surface);
        free(pixels);
        D( "not enough memory to allocate new skin surface !" );
    }
    return  s;
}


SkinSurface*
skin_surface_create_fast( int  w, int  h )
{
    SDL_Surface*  surface;

    surface = _sdl_surface_create_rgb( w, h, 32, SDL_HWSURFACE );
    if (surface == NULL) {
        surface = _sdl_surface_create_rgb( w, h, 32, SDL_SWSURFACE );
        if (surface == NULL) {
            D( "could not create fast %dx%d ARGB32 surface: %s",
               w, h, SDL_GetError() );
            return NULL;
        }
    }
    return _skin_surface_create( surface, NULL );
}


SkinSurface*
skin_surface_create_slow( int  w, int  h )
{
    SDL_Surface*  surface;

    surface = _sdl_surface_create_rgb( w, h, 32, SDL_SWSURFACE );
    if (surface == NULL) {
        D( "could not create slow %dx%d ARGB32 surface: %s",
            w, h, SDL_GetError() );
        return NULL;
    }
    return _skin_surface_create( surface, NULL );
}


SkinSurface*
skin_surface_create_argb32_from(
                        int                  w,
                        int                  h,
                        int                  pitch,
                        uint32_t*            pixels,
                        int                  do_copy )
{
    SDL_Surface*  surface;
    uint32_t*     pixcopy = NULL;

    if (do_copy) {
        size_t  size = h*pitch;
        pixcopy = malloc( size );
        if (pixcopy == NULL && size > 0) {
            D( "not enough memory to create %dx%d ARGB32 surface",
               w, h );
            return NULL;
        }
        memcpy( pixcopy, pixels, size );
    }

    surface = _sdl_surface_create_rgb_from( w, h, pitch,
                                            pixcopy ? pixcopy : pixels,
                                            32 );
    if (surface == NULL) {
        D( "could not create %dx%d slow ARGB32 surface: %s",
            w, h, SDL_GetError() );
        return NULL;
    }
    return _skin_surface_create( surface, pixcopy );
}

extern SkinSurface*
skin_surface_create_window(int x,
                           int y,
                           int w,
                           int h,
                           int is_fullscreen)
{
    char temp[32];
    sprintf(temp, "%d,%d", x, y);
    setenv("SDL_VIDEO_WINDOW_POS", temp, 1);
    setenv("SDL_VIDEO_WINDOW_FORCE_VISIBLE", "1", 1);

    int flags = SDL_SWSURFACE;
    if (is_fullscreen) {
        flags |= SDL_FULLSCREEN;
    }

    SDL_Surface* surface = SDL_SetVideoMode(w, h, 32, flags);
    if (!surface) {
        fprintf(stderr, "### Error: could not create or resize SDL window: %s\n", SDL_GetError() );
        exit(1);
    }

    SDL_WM_SetPos(x, y);

    return _skin_surface_create(surface, NULL);
}

extern int
skin_surface_lock( SkinSurface*  s, SkinSurfacePixels  *pix )
{
    if (!s || !s->surface) {
        D( "error: trying to lock stale surface %p", s );
        return -1;
    }
    if ( SDL_LockSurface( s->surface ) != 0 ) {
        D( "could not lock surface %p: %s", s, SDL_GetError() );
        return -1;
    }
    pix->w      = s->surface->w;
    pix->h      = s->surface->h;
    pix->pitch  = s->surface->pitch;
    pix->pixels = s->surface->pixels;
    return 0;
}

/* unlock a slow surface that was previously locked */
extern void
skin_surface_unlock( SkinSurface*  s )
{
    if (s && s->surface)
        SDL_UnlockSurface( s->surface );
}


extern void
skin_surface_get_format(SkinSurface* s,
                        SkinSurfacePixelFormat* format)
{
    format->r_shift = s->surface->format->Rshift;
    format->g_shift = s->surface->format->Gshift;
    format->b_shift = s->surface->format->Bshift;
    format->a_shift = s->surface->format->Ashift;

    format->r_mask = s->surface->format->Rmask;
    format->g_mask = s->surface->format->Gmask;
    format->b_mask = s->surface->format->Bmask;
    format->a_mask = s->surface->format->Amask;
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
    if (s && s->surface) {
        SDL_UpdateRect(s->surface, r->pos.x, r->pos.y, r->size.w, r->size.h);
    }
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
