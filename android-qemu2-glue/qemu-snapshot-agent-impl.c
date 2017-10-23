/* Copyright (C) 2017 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#include "android-qemu2-glue/qemu-control-impl.h"

#include "android/emulation/control/snapshot_agent.h"
//#include "android/hw-fingerprint.h"

#include <stdio.h> // ?? for printf

static void snapshot_take()
{
    printf("qemu-snapshot-agent-impl.c: In snapshot_take(): Stub does nothing.\n");
}

static const QAndroidSnapshotAgent sQAndroidSnapshotAgent = {
    .takeSnapshot = snapshot_take
};

const QAndroidSnapshotAgent* const gQAndroidSnapshotAgent = &sQAndroidSnapshotAgent;
