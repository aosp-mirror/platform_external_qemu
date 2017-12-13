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

#pragma once

#include "android/utils/compiler.h"

#include <stddef.h>

ANDROID_BEGIN_HEADER

extern void *loadpng(const char *fn, unsigned *_width, unsigned *_height);
extern void *readpng(const unsigned char*  base, size_t  size, unsigned *_width,
        unsigned *_height);
extern void savepng(const char* fn, unsigned int nChannels, unsigned int width,
        unsigned int height, void* pixels);
extern void savebmp(const char* fn, unsigned int nChannels, unsigned int width,
        unsigned int height, void* pixels);

ANDROID_END_HEADER
