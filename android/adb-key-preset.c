// Copyright (C) 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

#include "adb-key-preset.h"

#include "android/emulation/android_qemud.h"
#include "android/utils/debug.h"
#include "android/utils/stream.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define  D(...)  VERBOSE_PRINT(init,__VA_ARGS__)

static bool s_inited = false;

#define SERVICE_NAME  "adb-key-preset"
static char * s_adbKeyPreset = NULL;

static void adb_key_preset_client_recv(void *opaque, uint8_t *msg, int msglen,
                                   QemudClient*  client ) {
    if (msglen == 6 && !memcmp(msg, "adbKey", 6)) {
        if (s_adbKeyPreset) {
            qemud_client_send(client, (const uint8_t*)s_adbKeyPreset,
                          strlen(s_adbKeyPreset));
        }
    } else {
        D("%s: ignoring unknown command: %.*s", __FUNCTION__, msglen, msg);
    }
}

static QemudClient* adb_key_preset_service_connect(void* opaque,
        QemudService* serv, int channel, const char* client_param ) {
    QemudClient*  client;
    client = qemud_client_new( serv, channel, client_param, NULL,
                               adb_key_preset_client_recv,
                               NULL, NULL, NULL );
    qemud_client_set_framing(client, 1);
    return client;
}

void adb_key_preset_init_service() {
    if (!s_inited) {
        QemudService*  serv = qemud_service_register(SERVICE_NAME, 1, NULL,
                                              adb_key_preset_service_connect,
                                              NULL,
                                              NULL);
        if (serv == NULL) {
            derror("could not register '%s' service", SERVICE_NAME);
            return;
        }
        D("registered '%s' qemu service", SERVICE_NAME);

        s_inited = true;
    }
}

void adb_key_preset(const char* adbKey) {
    adb_key_preset_init_service();
    size_t keySize = strlen(adbKey) + 1;
    s_adbKeyPreset = realloc(s_adbKeyPreset, keySize);
    strcpy(s_adbKeyPreset, adbKey);
}
