// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#ifndef QEMU_MIGRATION_COMPRESSION_H
#define QEMU_MIGRATION_COMPRESSION_H

/* Compression interface for migration */
struct MigrationCompressionOps {
    /* Returns the max buffer size needed to hold |data_size| after compression */
    ssize_t (*max_compressed_size)(ssize_t data_size);

    /* Compresses |size| bytes of |data| into |dest|. Returns compressed size */
    ssize_t (*compress)(uint8_t *dest, ssize_t dest_size,
                        const uint8_t *data, ssize_t size, int level);

    /* Decompresses data from the |data| buffer into |dest| buffer. Returns >0
     * on success */
    ssize_t (*uncompress)(uint8_t *dest, ssize_t dest_size,
                          const uint8_t *data, ssize_t size);
};

void migrate_set_compression_ops(const MigrationCompressionOps *ops);
#endif
