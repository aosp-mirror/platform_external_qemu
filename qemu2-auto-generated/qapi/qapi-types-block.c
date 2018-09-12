/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * Schema-defined QAPI types
 *
 * Copyright IBM, Corp. 2011
 * Copyright (c) 2013-2018 Red Hat Inc.
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qapi/dealloc-visitor.h"
#include "qapi-types-block.h"
#include "qapi-visit-block.h"

const QEnumLookup BiosAtaTranslation_lookup = {
    .array = (const char *const[]) {
        [BIOS_ATA_TRANSLATION_AUTO] = "auto",
        [BIOS_ATA_TRANSLATION_NONE] = "none",
        [BIOS_ATA_TRANSLATION_LBA] = "lba",
        [BIOS_ATA_TRANSLATION_LARGE] = "large",
        [BIOS_ATA_TRANSLATION_RECHS] = "rechs",
    },
    .size = BIOS_ATA_TRANSLATION__MAX
};

const QEnumLookup FloppyDriveType_lookup = {
    .array = (const char *const[]) {
        [FLOPPY_DRIVE_TYPE_144] = "144",
        [FLOPPY_DRIVE_TYPE_288] = "288",
        [FLOPPY_DRIVE_TYPE_120] = "120",
        [FLOPPY_DRIVE_TYPE_NONE] = "none",
        [FLOPPY_DRIVE_TYPE_AUTO] = "auto",
    },
    .size = FLOPPY_DRIVE_TYPE__MAX
};

void qapi_free_BlockdevSnapshotInternal(BlockdevSnapshotInternal *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevSnapshotInternal(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup NbdServerRemoveMode_lookup = {
    .array = (const char *const[]) {
        [NBD_SERVER_REMOVE_MODE_SAFE] = "safe",
        [NBD_SERVER_REMOVE_MODE_HARD] = "hard",
    },
    .size = NBD_SERVER_REMOVE_MODE__MAX
};

const QEnumLookup QuorumOpType_lookup = {
    .array = (const char *const[]) {
        [QUORUM_OP_TYPE_READ] = "read",
        [QUORUM_OP_TYPE_WRITE] = "write",
        [QUORUM_OP_TYPE_FLUSH] = "flush",
    },
    .size = QUORUM_OP_TYPE__MAX
};
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_types_block_c;
