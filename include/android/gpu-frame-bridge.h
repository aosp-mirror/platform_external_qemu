/* Copyright (C) 2015 The Android Open Source Project
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
#ifndef ANDROID_GPU_FRAME_BRIDGE_H
#define ANDROID_GPU_FRAME_BRIDGE_H

#include "android/opengles.h"

// The Android GPU emulation (EmuGL) libraries provide a way to send GPU
// frame data to its client, but will do so by calling a user-provided
// callback in an EmuGL-created thread. The corresponding data cannot be
// easily sent to the UI, which expects to run on the process' main loop.
//
// This provides helper functions to pass frame data from one thread to
// the other. Usage is the following:
//
//   1) Call android_gpu_frame_bridge_init() from the main loop thread,
//      and pass the address of a callback to retrieve frame data.
//
//   2) The callback will be called from the main loop whenever a new GPU
//      frame is available. Format is 32-bit ARGB pixel values.
//

typedef void (GpuFrameBridgeCallback)(void* opaque,
                                      int width,
                                      int height,
                                      const void* pixels);

void android_gpu_frame_bridge_init(GpuFrameBridgeCallback* callback,
                                   void* callback_opaque);

#endif  // ANDROID_GPU_FRAME_BRIDGE_H
