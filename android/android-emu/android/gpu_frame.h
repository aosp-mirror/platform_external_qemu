// Copyright (C) 2015 The Android Open Source Project
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

#include <stdint.h>                  // for uint32_t

#include "android/utils/compiler.h"  // for ANDROID_BEGIN_HEADER, ANDROID_EN...

ANDROID_BEGIN_HEADER

// Recording mode can only be enabled in host gpu mode. Any other configuration
// will not work. Turning record mode on will initialize the gpu frame state for
// recording, and turning off will detach and deallocate resources that were
// being used, if any.
void gpu_frame_set_record_mode(bool on, uint32_t displayId = 0);

// Use in recording mode only. Make sure to turn on recording mode first with
// gpu_frame_set_record_mode() before using this. May return NULL if no data is
// available.
void* gpu_frame_get_record_frame(uint32_t displayId = 0);

typedef void (*FrameAvailableCallback)(void* opaque);

// Used by the VideoFrameSharer to obtain a new frame when one is available.
// Do not do any expensive calculations on this callback, it should be as fast
// as possible.
void gpu_register_shared_memory_callback(FrameAvailableCallback frameAvailable,
                                         void* opaque);
void gpu_unregister_shared_memory_callback(void* opaque);

// Used to inject a looper that will be used to create timers for the gpu bridge.
// should be called early during startup with a thread that hulooper that has support for timers.
void gpu_initialize_recorders();

// Used to inform the recorders that the emulator is terminating.
void gpu_emulator_shutdown();

ANDROID_END_HEADER
