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

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// GOLDFISH DMA
//
// Goldfish DMA is an extension to the pipe device
// and is designed to facilitate high-speed RAM->RAM
// transfers from guest to host.
//
// Interface (guest side):
//
// The guest user calls mmap() on a goldfish pipe fd,
// which means that it wants high-speed access to
// host memory. The guest can then write into the
// pointer returned by mmap(), and these writes
// become very quickly visible on the host.
//
// There are two ioctls(): globally lock and lock the
// DMA region.
//
// The pipe user can now both do conventional pipe
// operations and perform DMA transfers on the same fd.
// The mmaped() region can handle very high bandwidth
// transfers, and pipe operations can be used at the same
// time to handle synchronization and command communication.
//
// Interface (host side):
// There is a globally-visible pointer on the host
// (with android_goldfish_dma_get()) that is also visible to
// guest users who mmap() it:
struct GoldfishDmaRegion;
GoldfishDmaRegion* android_goldfish_dma_get(void);
// On startup, android_goldfish_dma_set() initializes the pointer.
// The pointer is obtained during initialization
// of the pipe device.
void android_goldfish_dma_set(GoldfishDmaRegion* x);
//
// Synchronization
// To maximize speed for each use case,
// all synchronization is left up to the users on
// both guest and host.
//
// The guest interface,
// however, does come with a simple interface
// to globally lock the DMA device if desired.
//
// What usually works is associating particular
// pipe transfers with a DMA transfer to achieve
// desired synchronization.

ANDROID_END_HEADER
