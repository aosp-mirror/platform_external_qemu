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
#include <string.h>
#include <sys/time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sched.h>
#include <unistd.h>
#endif

#define RING_BUFFER_MASK (RING_BUFFER_SIZE - 1)

void ring_buffer_init(struct ring_buffer* r) {
    r->write_pos = 0;
    r->read_pos = 0;
}

static uint32_t get_ring_pos(uint32_t index) {
    return index & RING_BUFFER_MASK;
}

static bool ring_buffer_can_write(const struct ring_buffer* r, uint32_t bytes) {
    uint32_t read_view;
    __atomic_load(&r->read_pos, &read_view, __ATOMIC_SEQ_CST);
    return get_ring_pos(read_view - r->write_pos - 1) >= bytes;
}

static bool ring_buffer_can_read(const struct ring_buffer* r, uint32_t bytes) {
    uint32_t write_view;
    __atomic_load(&r->write_pos, &write_view, __ATOMIC_SEQ_CST);
    return get_ring_pos(write_view - r->read_pos) >= bytes;
}

long ring_buffer_write(
    struct ring_buffer* r, const void* data, uint32_t step_size, uint32_t steps) {
    uint8_t* data_bytes = (uint8_t*)data;
    uint32_t i;

    for (i = 0; i < steps; ++i) {
        if (!ring_buffer_can_write(r, step_size)) {
            errno = -EAGAIN;
            return (long)i;
        }

        memcpy(
            &r->buf[get_ring_pos(r->write_pos)],
            data_bytes + i * step_size,
            step_size);

        __atomic_add_fetch(&r->write_pos, step_size, __ATOMIC_SEQ_CST);
    }

    errno = 0;
    return (long)steps;
}

long ring_buffer_read(
    struct ring_buffer* r, void* data, uint32_t step_size, uint32_t steps) {
    uint8_t* data_bytes = (uint8_t*)data;
    uint32_t i;

    for (i = 0; i < steps; ++i) {
        if (!ring_buffer_can_read(r, step_size)) {
            errno = -EAGAIN;
            return (long)i;
        }

        memcpy(
            data_bytes + i * step_size,
            &r->buf[get_ring_pos(r->read_pos)],
            step_size);

        __atomic_add_fetch(&r->read_pos, step_size, __ATOMIC_SEQ_CST);
    }

    errno = 0;
    return (long)steps;
}

void ring_buffer_view_init(
    struct ring_buffer* r,
    struct ring_buffer_view* v,
    uint8_t* buf,
    uint32_t size) {
    ring_buffer_init(r);

    uint32_t shift = 0;
    while ((1 << shift) < size) {
        ++shift;
    }

    // if size is not a power of 2,
    if ((1 << shift) > size) {
        --shift;
    }

    v->buf = buf;
    v->size = size;
    v->mask = (1 << shift) - 1;
}

static uint32_t ring_buffer_view_get_ring_pos(
    const struct ring_buffer_view* v,
    uint32_t index) {
    return index & v->mask;
}

static bool ring_buffer_view_can_write(
    const struct ring_buffer* r,
    const struct ring_buffer_view* v,
    uint32_t bytes) {
    uint32_t read_view;
    __atomic_load(&r->read_pos, &read_view, __ATOMIC_SEQ_CST);
    return ring_buffer_view_get_ring_pos(
            v, read_view - r->write_pos - 1) >= bytes;
}

static bool ring_buffer_view_can_read(
    const struct ring_buffer* r,
    const struct ring_buffer_view* v,
    uint32_t bytes) {
    uint32_t write_view;
    __atomic_load(&r->write_pos, &write_view, __ATOMIC_SEQ_CST);
    return ring_buffer_view_get_ring_pos(
            v, write_view - r->read_pos) >= bytes;
}

long ring_buffer_view_write(
    struct ring_buffer* r,
    struct ring_buffer_view* v,
    const void* data, uint32_t step_size, uint32_t steps) {

    uint8_t* data_bytes = (uint8_t*)data;
    uint32_t i;

    for (i = 0; i < steps; ++i) {
        if (!ring_buffer_view_can_write(r, v, step_size)) {
            errno = -EAGAIN;
            return (long)i;
        }

        memcpy(
            &v->buf[ring_buffer_view_get_ring_pos(v, r->write_pos)],
            data_bytes + i * step_size,
            step_size);

        __atomic_add_fetch(&r->write_pos, step_size, __ATOMIC_SEQ_CST);
    }

    errno = 0;
    return (long)steps;

}

long ring_buffer_view_read(
    struct ring_buffer* r,
    struct ring_buffer_view* v,
    void* data, uint32_t step_size, uint32_t steps) {
    uint8_t* data_bytes = (uint8_t*)data;
    uint32_t i;

    for (i = 0; i < steps; ++i) {
        if (!ring_buffer_view_can_read(r, v, step_size)) {
            errno = -EAGAIN;
            return (long)i;
        }

        memcpy(
            data_bytes + i * step_size,
            &v->buf[ring_buffer_view_get_ring_pos(v, r->read_pos)],
            step_size);

        __atomic_add_fetch(&r->read_pos, step_size, __ATOMIC_SEQ_CST);
    }

    errno = 0;
    return (long)steps;
}

static void ring_buffer_yield() {
#ifdef _WIN32
    if (!::SwitchToThread()) {
        ::Sleep(0);
    }
#else
    sched_yield();
#endif
}

static void ring_buffer_sleep() {
#ifdef _WIN32
    ::Sleep(1);
#else
    usleep(1000);
#endif
}

static uint64_t ring_buffer_curr_us() {
    uint64_t res;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    res = tv.tv_sec * 1000000ULL + tv.tv_usec;
    return res;
}

static const uint32_t yield_backoff_us = 1000;
static const uint32_t sleep_backoff_us = 2000;

void ring_buffer_wait_write(
    const struct ring_buffer* r,
    const struct ring_buffer_view* v,
    uint32_t bytes) {

    uint64_t start_us = ring_buffer_curr_us();
    uint64_t curr_wait_us;

    bool can_write =
        v ? ring_buffer_view_can_write(r, v, bytes) :
            ring_buffer_can_write(r, bytes);

    while (!can_write) {
        curr_wait_us = ring_buffer_curr_us();

        if (curr_wait_us > yield_backoff_us) {
            ring_buffer_yield();
        }

        if (curr_wait_us > sleep_backoff_us) {
            ring_buffer_sleep();
        }

        can_write =
            v ? ring_buffer_view_can_write(r, v, bytes) :
                ring_buffer_can_write(r, bytes);
    }
}

void ring_buffer_wait_read(
    const struct ring_buffer* r,
    const struct ring_buffer_view* v,
    uint32_t bytes) {

    uint64_t start_us = ring_buffer_curr_us();
    uint64_t curr_wait_us;

    bool can_read =
        v ? ring_buffer_view_can_read(r, v, bytes) :
            ring_buffer_can_read(r, bytes);

    while (!can_read) {
        curr_wait_us = ring_buffer_curr_us();

        if (curr_wait_us > yield_backoff_us) {
            ring_buffer_yield();
        }

        if (curr_wait_us > sleep_backoff_us) {
            ring_buffer_sleep();
        }

        can_read =
            v ? ring_buffer_view_can_read(r, v, bytes) :
                ring_buffer_can_read(r, bytes);
    }
}

void ring_buffer_write_fully(
    const struct ring_buffer* r,
    const struct ring_buffer_view* v,
    const void* data,
    uint32_t bytes) {
    // TODO
}

void ring_buffer_read_fully(
    const struct ring_buffer* r,
    const struct ring_buffer_view* v,
    void* data,
    uint32_t bytes) {
    // TODO
}

