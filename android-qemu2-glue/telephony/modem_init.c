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

#include "android-qemu2-glue/telephony/modem_init.h"

#include "android/console.h"
#include "android/telephony/modem_driver.h"
#include "android_modem_v2.h"
#include "android-qemu2-glue/utils/stream.h"

#include "qemu/osdep.h"
#include "hw/hw.h"

#include <assert.h>
#include "migration/register.h"

extern int sim_is_present();

static void modem_state_save(QEMUFile* file, void* opaque)
{
    Stream* const s = stream_from_qemufile(file);
    amodem_state_save_vx((AModem)opaque, (SysFile*)s);
    stream_free(s);
    if (getConsoleAgents()->settings->android_snapshot_update_timer()) {
        android_modem_driver_send_nitz_now();
    }
}

static int modem_state_load(QEMUFile* file, void* opaque, int version_id)
{
    Stream* const s = stream_from_qemufile(file);
    const int res =
            amodem_state_load_vx((AModem)opaque, (SysFile*)s, version_id);
    stream_free(s);
      if (getConsoleAgents()->settings->android_snapshot_update_timer()) {
        android_modem_driver_send_nitz_now();
    }
    return res;
}

static SaveVMHandlers modem_vmhandlers = {
    .save_state = modem_state_save,
    .load_state = modem_state_load,
};

void qemu_android_modem_init(int base_port) {
    android_modem_init(base_port, sim_is_present());

    assert(android_modem_serial_line != NULL);

    register_savevm_live(NULL,
                    "android_modem",
                    0,
                    MODEM_DEV_STATE_SAVE_VERSION,
                    &modem_vmhandlers,
                    android_modem);
}
