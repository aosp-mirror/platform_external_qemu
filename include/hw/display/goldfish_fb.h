/*
 * Goldfish Framebuffer public declarations.
 *
 * Copyright (C) 2014 Alex Bennée <alex.bennee@linaor.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef HW_DISPLAY_GOLDFISH_FB_H
#define HW_DISPLAY_GOLDFISH_FB_H

void goldfish_fb_set_rotation(int rotation);
void goldfish_fb_set_use_host_gpu(int enabled);
void goldfish_fb_set_display_depth(int depth);

#endif /* HW_DISPLAY_GOLDFISH_FB_H */
