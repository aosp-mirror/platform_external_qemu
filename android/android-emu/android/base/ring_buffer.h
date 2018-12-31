// Copyright 2018 The Android Open Source Project
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

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

#include <stdbool.h>
#include <stdint.h>

#define RING_BUFFER_SHIFT 11
#define RING_BUFFER_SIZE (1 << RING_BUFFER_SHIFT)

struct ring_buffer {
    uint32_t write_pos;
    uint32_t unused0[15];
    uint32_t read_pos;
    uint32_t unused1[15];
    // last element is reserved
    uint8_t buf[RING_BUFFER_SIZE];
};

void ring_buffer_init(struct ring_buffer* r);
bool ring_buffer_empty(const struct ring_buffer* r);
bool ring_buffer_full(const struct ring_buffer* r);

// sets errno if full or empty. Returns the number of bytes read.
long ring_buffer_write(struct ring_buffer* r, const void* data, uint32_t bytes);
long ring_buffer_read(struct ring_buffer* r, void* data, uint32_t bytes);

// Writes or reads step_size at a time. Precondition: the buffer must never have been
// used for any other size.
// sets errno if full or empty. Returns the number of step_size steps read.
bool ring_buffer_full_general(const struct ring_buffer* r, uint32_t bytes);
long ring_buffer_write_general(
    struct ring_buffer* r, const void* data, uint32_t step_size, uint32_t steps);
long ring_buffer_read_general(
    struct ring_buffer* r, void* data, uint32_t step_size, uint32_t steps);

ANDROID_END_HEADER
