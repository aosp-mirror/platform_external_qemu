// Copyright 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once
#include <stddef.h>

#include "android/skin/rect.h"
#include "android/utils/compiler.h"
#include "host-common/misc.h"

ANDROID_BEGIN_HEADER

/* Surface pixels information for surfaces */
typedef struct {
    int w;
    int h;
    int pitch;
    uint32_t* pixels;
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

/* List of composition operators for the blit routine */
typedef enum {
    SKIN_BLIT_COPY = 0,
    SKIN_BLIT_SRCOVER,
} SkinBlitOp;

/* A SkinSurface models a 32-bit ARGB pixel image that can be blitted to or from
 */
typedef struct SkinSurface SkinSurface;

/* Decrement a surface's reference count. Takes the surface's address as
   parameter. It will be set to NULL on exit */
typedef struct QAndroidSurfaceAgent {
    void (*skin_surface_unrefp)(SkinSurface**);

    int (*skin_surface_width)(SkinSurface*);
    int (*skin_surface_height)(SkinSurface*);
    int (*skin_surface_is_round)(SkinSurface*);

    /* Create a surface for the given dimensions or source. The "original"
     * dimensions are the dimensions of the original image, while the regular
     * dimensions are those at which the image is scaled to for display.
     */
    SkinSurface* (*skin_surface_create)(int, int, int, int);
    SkinSurface* (*skin_surface_create_from_data)(const void*, int);
    SkinSurface* (*skin_surface_create_from_file)(const char*);
    SkinSurface* (*skin_surface_create_derived)(SkinSurface*,
                                                SkinRotation,
                                                int);

    /* Check if a surface needs to be resized at all: only when the "original"
     * dimensions change does the surface need to be re-created. Otherwise, it
     * can simply be updated with the appropriate information to avoid extra
     * computation. If given a NULL surface, create a new one.
     */
    SkinSurface* (*skin_surface_resize)(SkinSurface*, int, int, int, int);

    void (*skin_surface_create_window)(SkinSurface*, int, int, int, int);

    void (*skin_surface_reverse_map)(SkinSurface*, int*, int*);

    void (*skin_surface_get_scaled_rect)(SkinSurface*,
                                         const SkinRect*,
                                         SkinRect*);

    void (*skin_surface_update)(SkinSurface*, SkinRect*);

    void (*skin_surface_upload)(SkinSurface*,
                                const SkinRect*,
                                const void*,
                                int);

    /* Blit a surface into another one */
    void (*skin_surface_blit)(SkinSurface*,
                              SkinPos*,
                              SkinSurface*,
                              SkinRect*,
                              SkinBlitOp);

    /* Blit a colored rectangle into a destination surface */
    void (*skin_surface_fill)(SkinSurface*, SkinRect*, uint32_t);

} QAndroidSurfaceAgent;

ANDROID_END_HEADER
