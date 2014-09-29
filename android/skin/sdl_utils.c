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
#include "android/skin/sdl_utils.h"

SDL_Surface*
sdl_surface_from_argb32( void*  base, int  w, int  h )
{
    return SDL_CreateRGBSurfaceFrom(
                        base, w, h, 32, w*4,
#if HOST_WORDS_BIGENDIAN
                        0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000
#else
                        0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000
#endif
                        );
}
