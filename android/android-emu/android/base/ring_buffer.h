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

// Single producer/consumer ring buffer struct that can be shared
// between host and guest as-is.
struct ring_buffer {
    uint32_t write_pos; // Atomically updated for the consumer
    uint32_t unused0[15]; // Separate cache line
    uint32_t read_pos; // Atomically updated for the producer
    uint32_t unused1[15]; // Separate cache line
    uint8_t buf[RING_BUFFER_SIZE];
    uint32_t state; // An atomically updated variable from both
                    // producer and consumer for other forms of
                    // coordination.
};

void ring_buffer_init(struct ring_buffer* r);

// Writes or reads step_size at a time. Sets errno=EAGAIN if full or empty.
// Returns the number of step_size steps read.
long ring_buffer_write(
    struct ring_buffer* r, const void* data, uint32_t step_size, uint32_t steps);
long ring_buffer_read(
    struct ring_buffer* r, void* data, uint32_t step_size, uint32_t steps);

// If we want to work with dynamically allocated buffers, a separate struct is
// needed; the host and guest are in different address spaces and thus have
// different views of the same memory, with the host and guest having different
// copies of this struct.
struct ring_buffer_view {
    uint8_t* buf;
    uint32_t size;
    uint32_t mask;
};

// Initializes ring buffer with view using |buf|. If |size| is not a power of
// two, then the buffer will assume a size equal to the greater power of two
// less than |size|.
void ring_buffer_view_init(
    struct ring_buffer* r,
    struct ring_buffer_view* v,
    uint8_t* buf,
    uint32_t size);

// Read/write functions with the view.
long ring_buffer_view_write(
    struct ring_buffer* r,
    struct ring_buffer_view* v,
    const void* data, uint32_t step_size, uint32_t steps);
long ring_buffer_view_read(
    struct ring_buffer* r,
    struct ring_buffer_view* v,
    void* data, uint32_t step_size, uint32_t steps);

// Usage of ring_buffer as a waitable object.
// These functions will back off if spinning too long.
//
// if |v| is null, it is assumed that the statically allocated ring buffer is
// used.
//
// Returns true if ring buffer became available, false if timed out.
bool ring_buffer_wait_write(
    const struct ring_buffer* r,
    const struct ring_buffer_view* v,
    uint32_t bytes,
    uint64_t timeout_us);
bool ring_buffer_wait_read(
    const struct ring_buffer* r,
    const struct ring_buffer_view* v,
    uint32_t bytes,
    uint64_t timeout_us);

// read/write fully, blocking if there is nothing to read/write.
void ring_buffer_write_fully(
    struct ring_buffer* r,
    struct ring_buffer_view* v,
    const void* data,
    uint32_t bytes);
void ring_buffer_read_fully(
    struct ring_buffer* r,
    struct ring_buffer_view* v,
    void* data,
    uint32_t bytes);

ANDROID_END_HEADER
