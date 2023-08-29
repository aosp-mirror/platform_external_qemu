/* Copyright (C) 2010 The Android Open Source Project
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
#include <glib.h>
#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

#include "qemu/osdep.h"
#include "qemu/typedefs.h"
#include "android/framebuffer.h"

void android_display_init_no_window(QFrameBuffer* qfbuff);
bool android_display_init(DisplayState* ds, QFrameBuffer* qfbuff);

ANDROID_END_HEADER
