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
#include "android/base/ring_buffer.h"

#include <errno.h>

#define RING_BUFFER_MASK (RING_BUFFER_SIZE - 1)

static uint32_t get_ring_pos(uint32_t index) {
    return index & RING_BUFFER_MASK;
}

void ring_buffer_init(struct ring_buffer* r) {
    r->write_pos = 0;
    r->read_pos = 0;
}

bool ring_buffer_empty(const struct ring_buffer* r) {
    uint32_t write_view;
    __atomic_load(&r->write_pos, &write_view, __ATOMIC_SEQ_CST);
    return r->read_pos == write_view;
}

bool ring_buffer_full(const struct ring_buffer* r) {
    uint32_t read_view;
    __atomic_load(&r->read_pos, &read_view, __ATOMIC_SEQ_CST);
    return get_ring_pos(r->write_pos) == get_ring_pos(read_view - 1);
}

long ring_buffer_write(struct ring_buffer* r, const void* data, uint32_t bytes) {
    uint8_t* data_bytes = (uint8_t*)data;

    for (uint32_t i = 0; i < bytes; ++i) {
        if (ring_buffer_full(r)) { errno = -EAGAIN; return (long)i; }
        r->buf[get_ring_pos(r->write_pos)] = data_bytes[i];
        __atomic_add_fetch(&r->write_pos, 1, __ATOMIC_SEQ_CST);
    }

    errno = 0;
    return (long)bytes;
}

long ring_buffer_read(struct ring_buffer* r, void* data, uint32_t bytes) {
    uint8_t* data_bytes = (uint8_t*)data;

    for (uint32_t i = 0; i < bytes; ++i) {
        if (ring_buffer_empty(r)) { errno = -EAGAIN; return (long)i; }
        data_bytes[i] = r->buf[get_ring_pos(r->read_pos)];
        __atomic_add_fetch(&r->read_pos, 1, __ATOMIC_SEQ_CST);
    }

    errno = 0;
    return (long)bytes;
}
