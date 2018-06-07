// Copyright 2017 The Android Open Source Project
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
// limitations under the License

#include "android-qemu2-glue/snapshot_compression.h"

#include <type_traits>

extern "C" {
#include "qemu/osdep.h"
#include "migration/compression.h"
}

#include "lz4.h"

#include <cassert>

static ssize_t max_compressed_size(ssize_t size) {
    return LZ4_compressBound(size);
}

static ssize_t compress(uint8_t *dest, ssize_t dest_size,
                        const uint8_t *data, ssize_t size, int level) {
    return LZ4_compress_fast((const char*)data, (char*)dest, size, dest_size, 1);
}

static ssize_t uncompress(uint8_t *dest, ssize_t dest_size,
                          const uint8_t *data, ssize_t size) {
    int res = LZ4_decompress_safe((const char*)data, (char*)dest, size, dest_size);
    assert(res == dest_size);
    return res;
}

void qemu_snapshot_compression_setup() {
    MigrationCompressionOps ops = {
        max_compressed_size,
        compress,
        uncompress
    };
    migrate_set_compression_ops(&ops);
}
