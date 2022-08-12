// Copyright 2021 The Android Open Source Project
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

#ifndef AEMU_GFXSTREAM_BACKEND
#error "Don't include migration stubs if AEMU_GFXSTREAM_BACKEND not defined"
#endif

#include <inttypes.h>

typedef uint64_t hwaddr;

static inline uint32_t qemu_get_be32(QEMUFile*) { return 0; }
static inline uint64_t qemu_get_be64(QEMUFile*) { return 0; }

static inline void qemu_put_be64(QEMUFile*, uint64_t) { }
static inline void qemu_put_be32(QEMUFile*, uint32_t) { }

static inline void qemu_put_buffer(QEMUFile*, const uint8_t*, int) { }
static inline size_t qemu_get_buffer(QEMUFile*, uint8_t*, int) {
    return 0;
}
static inline size_t qemu_peek_buffer(QEMUFile* f,
                                      uint8_t** buf,
                                      size_t size,
                                      size_t offset) {
    return 0;
}

static inline void* cpu_physical_memory_map(hwaddr, hwaddr*, int) { return 0; }
