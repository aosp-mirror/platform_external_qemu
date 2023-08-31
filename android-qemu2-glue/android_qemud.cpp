// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android-qemu2-glue/android_qemud.h"

#include "android/emulation/android_qemud.h"
#include "android/hw-qemud.h"
#include "android-qemu2-glue/utils/stream.h"

#include <glib.h>
extern "C" {
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qom/object.h"
#include "migration/vmstate.h"
#include "migration/register.h"
}

/* Version number of snapshots code. Increment whenever the data saved
 * or the layout in which it is saved is changed.
 */
#define QEMUD_SAVE_VERSION 5

static void qemud_save(QEMUFile* f, void* opaque) {
    Stream* stream = stream_from_qemufile(f);
    QemudMultiplexer* m = static_cast<QemudMultiplexer*>(opaque);
    qemud_multiplexer_save(m, stream);
    stream_free(stream);
}

static int qemud_load(QEMUFile* f, void* opaque, int version) {
    Stream* stream = stream_from_qemufile(f);
    QemudMultiplexer* m = static_cast<QemudMultiplexer*>(opaque);
    int ret = qemud_multiplexer_load(m, stream, version);
    stream_free(stream);
    return ret;
}

static SaveVMHandlers qemud_vmhandlers;

void android_qemu2_qemud_init(void) {
    (void) android_qemud_get_serial_line();

    // Workaround gcc 4.8 bug: 55606
    qemud_vmhandlers.save_state = qemud_save;
    qemud_vmhandlers.load_state = qemud_load;

    register_savevm_live(nullptr,
                         "qemud",
                         0,
                         QEMUD_SAVE_VERSION,
                         &qemud_vmhandlers,
                         qemud_multiplexer);
}
