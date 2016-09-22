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

static const uint32_t kDmaBufSizeMB = 32;
// GOLDFISH DMA
//
// Goldfish DMA is an extension to the pipe device
// and is designed to facilitate high-speed RAM->RAM
// transfers from guest to host.
//
// See docs/GOLDFISH-VIRTUAL-HARDWARE.txt and
//     docs/ANDROID-QEMU-PIPE.txt
//
// for more details.
//
// The central part of this extension is a large,
// physically contiguous region of memory allocated
// on startup of the pipe device and mapped
// to host memory. Pipe users in the guest can
// map this memory and writes to it will be visible
// on the host very quickly.
//
// Interface (guest side):
//
// The guest user calls mmap() on a goldfish pipe fd,
// which means that it wants access to the physically
// contiguous region.
//
// The guest can then write normally into the
// pointer returned by mmap(), and these writes
// become very quickly visible on the host.
// This memory is shared between the host and guest,
// and can also be shared between different guest users.
// One controls the sharing through controlling the
// offset into the physically contiguous region, by
// using the lock, or some other synchronization scheme---
// the point of Goldfish DMA is to allow applications
// to each use a synchronization scheme that is best suited
// for particular purposes.
//
// There are two main ioctls(): globally lock and lock the
// DMA region. The guest can also optionally specify
// and query the offset that mmap() will take into
// the DMA region using ioctl().
//
// The pipe user can now both do conventional pipe
// operations and perform DMA transfers on the same fd.
// The mmaped() region can handle very high bandwidth
// transfers, and pipe operations can be used at the same
// time to handle synchronization and command communication.
//
// Interface (host side):
// On startup:
// android_goldfish_dma_get_bufsize() is used to obtain
// the desired size of the buffer. This is communicated
// to the driver.
// android_goldfish_dma_init() initializes the pointer after
// the buffer size is known. This is communicated from
// the driver.
// All that happens during initialization of the pipe device.
struct GoldfishDmaRegion;
uint32_t android_goldfish_dma_get_bufsize(void);
void android_goldfish_dma_init(GoldfishDmaRegion* x);
// android_goldfish_dma_read() returns a pointer to the |offset|
// (in bytes) into the physically contiguous guest region.
void* android_goldfish_dma_read(uint32_t offset); // WIP TODO: remove

// the real host interface:
//
// android_goldfish_dma_add_buffer():
// Tell us that there is a physically-contiguous buffer in the guest
// starting at |guest_paddr|. This should be used upon allocation
// of the DMA buffer in the ugest.
void android_goldfish_dma_add_buffer(uint64_t guest_paddr, uint64_t sz);
// android_goldfish_dma_remove_buffer():
// Tell us that we don't care about tracking this buffer anymore.
// This should usually be used when the DMA buffer has been freed
// in the ugest.
void android_goldfish_dma_remove_buffer(uint64_t guest_paddr);
// android_goldfish_dma_get_host_addr():
// Obtain a pointer to guest physical memory that is usable as
// as host void*.
//
// TODO: Change return type to void*, which would require a slight
// adjustment to the design of ApiGen *.in files and how they are
// handled.
uint64_t android_goldfish_dma_get_host_addr(uint64_t guest_paddr);
// android_goldfish_dma_invalidate_host_mappings():
// Sometimes (say, on snapshot save/load) we may need to re-map
// the guest DMA buffers so to regain access to them.
// This function tells our map of DMA buffers to remap the buffers
// next time they are used.
void android_goldfish_dma_invalidate_host_mappings(void);
