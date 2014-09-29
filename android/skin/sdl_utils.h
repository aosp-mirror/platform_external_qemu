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
#ifndef ANDROID_SKIN_SDL_UTILS_H
#define ANDROID_SKIN_SDL_UTILS_H

#include <SDL.h>

/* helper functions */

extern SDL_Surface*    sdl_surface_from_argb32( void*  base, int  w, int  h );

struct SkinSurface;

extern SDL_Surface*    skin_surface_get_sdl(struct SkinSurface* surface);

#endif  // ANDROID_SKIN_SDL_UTILS_H
