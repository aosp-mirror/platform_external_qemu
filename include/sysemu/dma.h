/*
 * DMA helper functions
 *
 * Copyright (c) 2009 Red Hat
 *
 * This work is licensed under the terms of the GNU General Public License
 * (GNU GPL), version 2 or later.
 */

#ifndef DMA_H
#define DMA_H

#include <stdio.h>
//#include "cpu.h"
#include "hw/hw.h"
#include "block/block.h"

typedef struct {
    hwaddr base;
    hwaddr len;
} ScatterGatherEntry;

struct QEMUSGList {
    ScatterGatherEntry *sg;
    int nsg;
    int nalloc;
    hwaddr size;
};

void qemu_sglist_init(QEMUSGList *qsg, int alloc_hint);
void qemu_sglist_add(QEMUSGList *qsg, hwaddr base,
                     hwaddr len);
void qemu_sglist_destroy(QEMUSGList *qsg);

BlockDriverAIOCB *dma_bdrv_read(BlockDriverState *bs,
                                QEMUSGList *sg, uint64_t sector,
                                BlockDriverCompletionFunc *cb, void *opaque);
BlockDriverAIOCB *dma_bdrv_write(BlockDriverState *bs,
                                 QEMUSGList *sg, uint64_t sector,
                                 BlockDriverCompletionFunc *cb, void *opaque);
#endif
