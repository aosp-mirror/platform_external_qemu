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

#include "android/cros.h"

#include "android/emulation/android_qemud.h"
#include "android/hw-qemud.h"
#include "android/utils/debug.h"

#include <stdlib.h>
#include <string.h>

static QemudService* service;
static QemudClient*  client;

#define  D(...) VERBOSE_PRINT(events,__VA_ARGS__)

void
cros_send_message(const char*  message) {
    if (message == NULL)
        return;

    D("sending '%s'", message);

    if (client == NULL) {
        D("no client, ignored");
        return;
    }

    qemud_client_send(client, (const uint8_t*) message, strlen(message));
}

static void cros_close(void* opaque) {
    client = NULL;
}

static QemudClient* cros_connect(void*  opaque,
                                 QemudService* service,
                                 int channel,
                                 const char* client_param) {
    client = qemud_client_new(service, channel, client_param,
                              opaque, NULL, cros_close, NULL, NULL);
    qemud_client_set_framing(client, 1);
    return client;
}

int cros_pipe_init() {
    service = qemud_service_register(ANDROID_QEMUD_CROS, 1, NULL, cros_connect,
                                     NULL, NULL);
    if (!service)
        return -1;
    return 0;
}
