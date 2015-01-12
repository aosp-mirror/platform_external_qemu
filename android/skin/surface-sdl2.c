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
#include "android/skin/scaler.h"
#include "android/utils/setenv.h"
#include <SDL.h>

#include <math.h>
#include <stdarg.h>
#include <stdio.h>

// Set to 1 to enable debugging.
#define  DEBUG  1

#if DEBUG
#include "android/utils/debug.h"
#define  D(...)   VERBOSE_PRINT(surface,__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif

// Helper macro to check for SDL errors after calls that return a negative
// value on failure.
#define SDL_CHECK(cmd) do { \
    int ret = (cmd); \
    if (DEBUG && VERBOSE_CHECK(surface) && ret < 0) { \
        fprintf(stderr, "ERROR:%s:%d: %s: %s\n", __FILE__, __LINE__, \
                #cmd, SDL_GetError()); \
    } } while (0)

static void panic(const char* format, ...) {
    fprintf(stderr, "ERROR: ");
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

// The data associated with a single window.
typedef struct {
    SDL_Renderer* renderer;
    double scale;
    double scale_x_offset;
    double scale_y_offset;
} WindowData;

// Technical note:
//
// Everytime the window is resized, the content of textures is discarded.
// Most SkinSurfaces corresponds to bitmaps that need to survive resizing,
// so keep them as in-memory SDL_Surface objects, with a corresponding
// SDL_Texture object to speed up blitting.
//
// The following object is used to model a global list of SkinSurface
// objects, and provide a way to iterate over them, in order to regenerate
// all textures after a resize.
//
// Also keeps the global SDL_Renderer instance, since only one window is
// supported at the moment. In a multi-window version of the UI, each
// window will have its own renderer.

#define MAX_SURFACES  256

typedef struct {
    int num_surfaces;
    SkinSurface* surfaces[MAX_SURFACES];
    WindowData window;
} Globals;

static Globals s_globals = {
    .num_surfaces = 0,
    .surfaces = { 0, },
    .window = {
        .renderer = NULL,
        .scale = 1.0,
        .scale_x_offset = 0.,
        .scale_y_offset = 0.
    },
};

// Record a new global surface |s|.
static void globals_add_surface(SkinSurface* s) {
    Globals* g = &s_globals;
    if (s && g->num_surfaces < MAX_SURFACES) {
        g->surfaces[g->num_surfaces++] = s;
    }
}

// Remove a surface |s| from the global list.
static void globals_remove_surface(SkinSurface* s) {
    Globals* g = &s_globals;
    int nn;
    for (nn = 0; nn < g->num_surfaces; ++nn) {
        if (g->surfaces[nn] == s) {
            g->num_surfaces -= 1;
            g->surfaces[nn] = g->surfaces[g->num_surfaces];
            g->surfaces[g->num_surfaces] = NULL;
        }
    }
}

// Iterate over all surfaces in the global list, calling |handle_func| for
// each one of them. |handle_opaque| is passed to each invokation as the
// |opaque| parameter.
//
// IMPORTANT: Clients should *NOT* try to add/remove surfaces during
//            iteration.
typedef void (*SkinSurfaceHandlerFunction)(SkinSurface* surface, void* opaque);

static void globals_foreach_surface(SkinSurfaceHandlerFunction handle_func,
                                    void* handle_opaque) {
    Globals* g = &s_globals;
    int nn;
    for (nn = 0; nn < g->num_surfaces; ++nn) {
        handle_func(g->surfaces[nn], handle_opaque);
    }
}

static SDL_Renderer* globals_get_renderer(void) {
    return s_globals.window.renderer;
}

static void globals_set_renderer(SDL_Renderer* renderer) {
    s_globals.window.renderer = renderer;
}

static void globals_set_window_scale(double scale,
                                     double scale_x_offset,
                                     double scale_y_offset) {
    WindowData* data = &s_globals.window;
    data->scale = scale;
    data->scale_x_offset = scale_x_offset;
    data->scale_y_offset = scale_y_offset;
}

static const WindowData* globals_get_window_data(void) {
    return &s_globals.window;
}

// Defined in winsys-sdl2.c
extern SDL_Window* skin_winsys_get_window(void);
extern void skin_winsys_set_window(SDL_Window* window);

// Used to model a SkinSurface instance.
// |refcount| is a simple reference count.
// |w| and |h| are the surface's width and height in pixels.
// |surface| is an in-memory SDL_Surface that is used to keep the surface's
// content, even after window resizes.
// |texture| is an SDL_Texture in GPU memory, its content gets erased when
// the window is resized.
struct SkinSurface {
    int refcount;
    int w;
    int h;
    SDL_Surface* surface;
    SDL_Texture* texture;
};

// Destroy a given SkinSurface instance |s|.
static void skin_surface_free(SkinSurface*  s) {
    if (!s) {
        return;
    }
    globals_remove_surface(s);

    if (s->surface) {
        SDL_FreeSurface(s->surface);
        s->surface = NULL;
    }
    if (s->texture) {
        SDL_DestroyTexture(s->texture);
        s->texture = NULL;
    }
    free(s);
}

SkinSurface* skin_surface_ref(SkinSurface* surface) {
    if (surface) {
        surface->refcount += 1;
    }
    return surface;
}

void skin_surface_unrefp(SkinSurface** psurface) {
    SkinSurface* surf = *psurface;
    if (surf) {
        if (--surf->refcount <= 0)
            skin_surface_free(surf);
        *psurface = NULL;
    }
}

int skin_surface_width(SkinSurface* s) {
    return s->w;
}

int skin_surface_height(SkinSurface* s) {
    return s->h;
}

// Create a new SkinSurface instance. The |surface| parameter is optional
// and can be NULL, for the SkinSurface corresponding to the final window.
// The |texture| parameter is also optional.

static SkinSurface* _skin_surface_create(SDL_Surface* surface,
                                         SDL_Texture* texture,
                                         int w,
                                         int h) {
    SkinSurface*  s = malloc(sizeof(*s));
    if (!s) {
        if (surface) {
            SDL_FreeSurface(surface);
        }
        if (texture) {
            SDL_DestroyTexture(texture);
        }
        D( "not enough memory to allocate new skin surface !" );
        return NULL;
    }
    s->refcount = 1;
    s->surface = surface;
    s->texture = texture;
    s->w = w;
    s->h = h;

    globals_add_surface(s);
    return  s;
}

#define  ARGB32_R_MASK  0x00ff0000
#define  ARGB32_G_MASK  0x0000ff00
#define  ARGB32_B_MASK  0x000000ff
#define  ARGB32_A_MASK  0xff000000


static SDL_Surface* _sdl_surface_create(int width, int height) {
    SDL_Surface* surface = SDL_CreateRGBSurface(
            0,
            width,
            height,
            32,
            ARGB32_R_MASK,
            ARGB32_G_MASK,
            ARGB32_B_MASK,
            ARGB32_A_MASK);
    if (!surface) {
        D("Could not create %dx%d surface: %s", width, height, SDL_GetError());
    }
    return surface;
}

SkinSurface* skin_surface_create_fast(int  w, int  h) {
    SDL_Surface* surface = _sdl_surface_create(w, h);
    if (!surface) {
        return NULL;
    }
    return _skin_surface_create(surface, NULL, w, h);
}


SkinSurface* skin_surface_create_slow(int  w, int  h) {
    SDL_Surface* surface = _sdl_surface_create(w, h);
    if (!surface) {
        return NULL;
    }
    return _skin_surface_create(surface, NULL, w, h);
}


SkinSurface* skin_surface_create_argb32_from(int w,
                                             int h,
                                             int pitch,
                                             uint32_t* pixels) {
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
            pixels,
            w,
            h,
            32,
            pitch,
            ARGB32_R_MASK,
            ARGB32_G_MASK,
            ARGB32_B_MASK,
            ARGB32_A_MASK);
    if (!surface) {
        D("Could not create ARGB32 %dx%d surface: %s", w, h, SDL_GetError());
        return NULL;
    }
    return _skin_surface_create(surface, NULL, w, h);
}

static void _skin_surface_destroy_texture(SkinSurface* s, void* opaque) {
    if (s->texture) {
        SDL_DestroyTexture(s->texture);
        s->texture = NULL;
    }
}

// This function is used to repopulate the content of SkinSurface textures
// after a window resize event, from the corresponding SDL_Surface pixels.
// This must be called after _skin_surface_destroy_texture().
static void _skin_surface_reset_texture(SkinSurface* s, void* opaque) {
    if (!s->surface || s->texture) {
        // The window SkinSurface doesn't have a surface and has a texture
        // we don't want to regenerate.
        return;
    }
    SDL_Renderer* renderer = opaque;
    s->texture = SDL_CreateTextureFromSurface(renderer, s->surface);
    if (!s->texture) {
        D("%s: Could not create texture from surface: %s",
            __FUNCTION__,
            SDL_GetError());
    }
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
    static SkinSurface* result = NULL;

    SDL_Window* window = skin_winsys_get_window();
    SDL_Renderer* renderer = globals_get_renderer();

    D("%s: x=%d y=%d w=%d h=%d original_w=%d original_h=%d is_fullscreen=%d",
      __FUNCTION__, x, y, w, h, original_w, original_h, is_fullscreen);

    // Textures do not survive window resize events, so
    globals_foreach_surface(&_skin_surface_destroy_texture, NULL);

    if (!window) {
        // NOTE: Don't use SDL_WINDOW_ALLOW_HIGHDPI here. On OS X, this will
        //       make mouse event coordinates twice smaller than needed
        //       when running on a high-dpi machine (e.g. recent MacBook Pro),
        //       making the UI unusable. Note that this doesn't happen on a
        //       'low-dpi' device such as a MacPro connected to a 30" monitor.
        int window_flags = SDL_WINDOW_OPENGL;
        if (is_fullscreen) {
            window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        }
        window = SDL_CreateWindow("Android emulator",
                                  x, y, w, h,
                                  window_flags);
        if (!window) {
            panic("Could not create SDL2 window: %s\n", SDL_GetError());
        }
        skin_winsys_set_window(window);
    } else {
        if (is_fullscreen) {
            SDL_CHECK(SDL_SetWindowFullscreen(window,
                                              SDL_WINDOW_FULLSCREEN_DESKTOP));
#if DEBUG
#endif
        } else {
            SDL_CHECK(SDL_SetWindowFullscreen(window, 0));
            SDL_SetWindowPosition(window, x, y);
            SDL_SetWindowSize(window, w, h);
        }
    }

    // Generate renderer
    if (!renderer) {
        SDL_CHECK(SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear"));
        renderer = SDL_CreateRenderer(window,
                                    -1,
                                    SDL_RENDERER_ACCELERATED|
                                    SDL_RENDERER_PRESENTVSYNC|
                                    SDL_RENDERER_TARGETTEXTURE);
        if  (!renderer) {
            panic("Could not create renderer: %s\n", SDL_GetError());
        }
        globals_set_renderer(renderer);
    }

    SDL_CHECK(SDL_RenderSetLogicalSize(renderer, original_w, original_h));

    if (DEBUG && VERBOSE_CHECK(surface)) {
        SDL_RendererInfo info;
        SDL_CHECK(SDL_GetRendererInfo(renderer, &info));
        printf("renderer.name = %s\n", info.name);
        printf("renderer.flags = 0x%x\n", info.flags);
        printf("renderer.num_texture_formats = %d\n", info.num_texture_formats);
        int nn;
        for (nn = 0; nn < info.num_texture_formats; ++nn) {
            printf("   0x%x (%s)\n", info.texture_formats[nn],
                   SDL_GetPixelFormatName(info.texture_formats[nn]));
        }
        printf("renderer.max_texture_width = %d\n", info.max_texture_width);
        printf("renderer.max_texture_height = %d\n", info.max_texture_height);
    }

    // Compute scaling parameters.
    {
        int window_x, window_y, window_w, window_h;

        SDL_GetWindowSize(window, &window_w, &window_h);
        SDL_GetWindowPosition(window, &window_x, &window_y);

        D("Window pos=(%d,%d) size=(%d,%d)", window_x, window_y, window_w, window_h);

        double x_scale = window_w * 1.0 / original_w;
        double y_scale = window_h * 1.0 / original_h;
        double scale = (x_scale <= y_scale) ? x_scale : y_scale;

        double effective_x = 0.;
        double effective_y = 0.;

        if (is_fullscreen) {
            effective_x = (window_w - original_w * scale) * 0.5;
            effective_y = (window_h - original_h * scale) * 0.5;
        }

        globals_set_window_scale(scale, effective_x, effective_y);
    }

    SDL_Texture* texture =
            SDL_CreateTexture(renderer,
                              SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_TARGET,
                              original_w,
                              original_h);
    if (!texture) {
        panic("Could not create window texture: %s", SDL_GetError());
    }

    SDL_CHECK(SDL_SetRenderTarget(renderer, texture));
    SDL_CHECK(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255));
    SDL_CHECK(SDL_RenderClear(renderer));

    SDL_CHECK(SDL_SetRenderTarget(renderer, NULL));
    SDL_CHECK(SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255));
    SDL_CHECK(SDL_RenderClear(renderer));
    SDL_CHECK(SDL_RenderCopy(renderer, texture, NULL, NULL));
    SDL_RenderPresent(renderer);

    if (!result) {
        result = _skin_surface_create(NULL, texture, original_w, original_h);
    } else {
        result->texture = texture;
        result->w = original_w;
        result->h = original_h;
    }
    skin_surface_ref(result);

    // Ensure all textures are regenerated properly.
    globals_foreach_surface(&_skin_surface_reset_texture, renderer);

    return result;
}

void skin_surface_reverse_map(SkinSurface* surface,
                              int* x,
                              int* y) {
    // Nothing to do here, SDL2 handles the reverse mapping of input
    // coordinates automatically!!
    (void)surface;
    (void)x;
    (void)y;
}

void skin_surface_get_scaled_rect(SkinSurface* surface,
                                  const SkinRect* srect,
                                  SkinRect* drect) {
    const WindowData* data = globals_get_window_data();
    if (data->scale == 1.0) {
        *drect = *srect;
    } else {
        int sx = srect->pos.x;
        int sy = srect->pos.y;
        int sw = srect->size.w;
        int sh = srect->size.h;
        double scale = data->scale;

        drect->pos.x = (int)(sx * scale + data->scale_x_offset);
        drect->pos.y = (int)(sy * scale + data->scale_y_offset);
        drect->size.w = (int)(ceil((sx + sw) * scale + data->scale_x_offset)) - drect->pos.x;
        drect->size.h = (int)(ceil((sy + sh) * scale + data->scale_y_offset)) - drect->pos.y;
    }
}


void skin_surface_update(SkinSurface* s, SkinRect* r) {
    if (!s || !s->texture) {
        return;
    }

    // With SDL2, always update the whole texture to the window, and
    // ignore the rectangle.
    (void)r;

    SDL_Renderer* renderer = globals_get_renderer();
    SDL_CHECK(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255));
    SDL_CHECK(SDL_RenderClear(renderer));
    SDL_CHECK(SDL_RenderCopy(renderer, s->texture, NULL, NULL));
    SDL_RenderPresent(renderer);
}

void skin_surface_fill(SkinSurface* dst,
                       SkinRect* rect,
                       uint32_t argb_premul)
{
    SDL_Rect r = {
        .x = 0,
        .y = 0,
        .w = dst->w,
        .h = dst->h,
    };
    if (rect) {
        r.x = rect->pos.x;
        r.y = rect->pos.y;
        r.w = rect->size.w;
        r.h = rect->size.h;
    }

    if (dst->surface) {
        SDL_CHECK(SDL_FillRect(dst->surface, &r, argb_premul));
    }

    if (dst->texture) {
        SDL_Renderer* renderer = globals_get_renderer();

        if (SDL_SetRenderTarget(renderer, dst->texture) < 0) {
            D("error: trying to fill texture: %s", SDL_GetError());
            return;
        }

        SDL_CHECK(SDL_SetRenderDrawColor(renderer,
                                         (argb_premul >> 16) & 255,
                                         (argb_premul >> 8) & 255,
                                         (argb_premul) & 255,
                                         (argb_premul >> 24) & 255));

        SDL_CHECK(SDL_RenderFillRect(renderer, &r));
        SDL_CHECK(SDL_SetRenderTarget(renderer, NULL));
    }
}

void skin_surface_blit(SkinSurface* dst,
                       SkinPos* dst_pos,
                       SkinSurface* src,
                       SkinRect* src_rect,
                       SkinBlitOp blitop) {
    SDL_Renderer* renderer = globals_get_renderer();

    if (SDL_SetRenderTarget(renderer, dst->texture) < 0) {
        D("error: trying to blit to texture: %s", SDL_GetError());
        return;
    }

    SDL_Rect srect = {
        .x = src_rect->pos.x,
        .y = src_rect->pos.y,
        .w = src_rect->size.w,
        .h = src_rect->size.h,
    };

    SDL_Rect drect = {
        .x = dst_pos->x,
        .y = dst_pos->y,
        .w = src_rect->size.w,
        .h = src_rect->size.h,
    };

    switch (blitop) {
        case SKIN_BLIT_COPY:
            SDL_CHECK(SDL_SetTextureBlendMode(src->texture,
                                              SDL_BLENDMODE_NONE));
            break;
        case SKIN_BLIT_SRCOVER:
            SDL_CHECK(SDL_SetTextureBlendMode(src->texture,
                                              SDL_BLENDMODE_BLEND));
            break;
        default:
            return;
    }
    SDL_CHECK(SDL_SetTextureAlphaMod(src->texture, 255));
    SDL_CHECK(SDL_RenderCopy(renderer, src->texture, &srect, &drect));

    SDL_CHECK(SDL_SetRenderTarget(renderer, NULL));
}

void skin_surface_upload(SkinSurface* surface,
                         SkinRect* rect,
                         const void* pixels,
                         int pitch) {
    // Sanity checks
    if (!surface || !surface->texture) {
        return;
    }

    SDL_Rect r = {
        .x = 0,
        .y = 0,
        .w = surface->w,
        .h = surface->h,
    };
    if (rect) {
        r.x = rect->pos.x;
        r.y = rect->pos.y;
        r.w = rect->size.w;
        r.h = rect->size.h;
    }

    SDL_CHECK(SDL_UpdateTexture(surface->texture, &r, pixels, pitch));
}
