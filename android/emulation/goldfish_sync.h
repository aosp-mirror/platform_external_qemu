/* Copyright (C) 2016 The Android Open Source Project
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

#include <inttypes.h>

extern uint64_t goldfish_sync_create_timeline();
extern uint64_t goldfish_sync_create_fence(uint64_t timeline, uint32_t time);
extern void goldfish_sync_timeline_inc(uint64_t timeline, uint32_t time);
extern void goldfish_sync_timeline_destroy(uint64_t timeline);

