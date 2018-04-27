// Copyright (C) 2016 The Android Open Source Project
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

// Interface between android-emu non-base libraries and emugl

#include "android/emulation/GoldfishDma.h"
#include "android/emulation/goldfish_sync.h"
#include "android/featurecontrol/FeatureControl.h"

// Crash reporter
using emugl_crash_reporter_t = void (*)(const char*, ...);

// Feature control
using emugl_feature_is_enabled_t = bool (*)(android::featurecontrol::Feature);

// Goldfish sync device
using emugl_sync_create_timeline_t = uint64_t (*)();
using emugl_sync_create_fence_t = int (*)(uint64_t, uint32_t);
using emugl_sync_timeline_inc_t = void (*)(uint64_t, uint32_t);
using emugl_sync_destroy_timeline_t = void (*)(uint64_t);

using emugl_sync_trigger_wait_t = void (*)(uint64_t, uint64_t, uint64_t);
using emugl_sync_register_trigger_wait_t = void (*)(emugl_sync_trigger_wait_t);

using emugl_sync_device_exists_t = bool (*)();

// OpenGL timestamped logger
using emugl_logger_t = void (*)(const char*, ...);
typedef struct {
    emugl_logger_t coarse;
    emugl_logger_t fine;
} emugl_logger_struct;

// Function type that describes functions for
// reading from the Goldfish DMA region
// at a specified offset.
using emugl_dma_add_buffer_t = void (*)(void*, uint64_t, uint64_t);
using emugl_dma_remove_buffer_t = void (*)(uint64_t);
using emugl_dma_get_host_addr_t = void* (*)(uint64_t);
using emugl_dma_invalidate_host_mappings_t = void (*)();
using emugl_dma_unlock_t = void (*)(uint64_t);

typedef struct {
    emugl_dma_add_buffer_t add_buffer;
    emugl_dma_remove_buffer_t remove_buffer;
    emugl_dma_get_host_addr_t get_host_addr;
    emugl_dma_invalidate_host_mappings_t invalidate_host_mappings;
    emugl_dma_unlock_t unlock;
} emugl_dma_ops;
